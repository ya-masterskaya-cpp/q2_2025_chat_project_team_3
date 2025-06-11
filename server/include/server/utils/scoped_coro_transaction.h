#pragma once

#include <server/utils/utils.h>

using ScopedTransactionResult =
    std::optional<std::string>;
using ScopedTransactionFunc =
    std::function<drogon::Task<ScopedTransactionResult>(
        std::shared_ptr<drogon::orm::Transaction>)>;

struct CommitAwaiter : public drogon::CallbackAwaiter<bool> {
    std::shared_ptr<drogon::orm::Transaction> m_tx;
    std::coroutine_handle<>                   m_h;

    explicit CommitAwaiter(std::shared_ptr<drogon::orm::Transaction> tx)
        : m_tx(std::move(tx))
    {}

    void await_suspend(std::coroutine_handle<> handle) noexcept {
        m_h = handle;
        LOG_TRACE << "Installing commit callback";
        m_tx->setCommitCallback([this](bool ok) {
            LOG_TRACE << "Commit callback fired: " << (ok ? "success" : "failure");
            setValue(ok);
            m_h.resume();
        });
        LOG_TRACE << "Releasing transaction reference to allow destructor/commit";
        m_tx.reset();
    }
};

//wraps lambda in a transaction
//if lambda returns anything - rollbacks and passes value up
//if lambda returned empty optional - tries to commit
//if commit failed - returns error message
//otherwise - returns empty optional
//NOTE: Drogon has no explicit commit for transactions, that's wh we need this
inline drogon::Task<ScopedTransactionResult> WithTransaction(ScopedTransactionFunc userLambda) {
    std::shared_ptr<drogon::orm::Transaction> tx;

    try {
        auto db = drogon::app().getDbClient();
        if(!db) {
            LOG_ERROR << "DB client not available";
            co_return "Internal: DB client unavailable";
        }

        tx = co_await db->newTransactionCoro();
        LOG_TRACE << "Transaction started";

        if(auto err = co_await userLambda(tx)) {
            LOG_DEBUG << "User lambda returned error, rolling back";
            tx->rollback();
            co_return err;
        }

        CommitAwaiter waiter{std::move(tx)};
        if(!co_await waiter) {
            LOG_ERROR << "Commit failed via callback";
            co_return "Transaction commit failed";
        }

        LOG_TRACE << "Transaction committed successfully";
    } catch(const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DrogonDbException: " << e.base().what();
        if(tx) {
            tx->rollback();
        }
        co_return std::string("DB exception: ") + e.base().what();
    } catch(const std::exception &e) {
        LOG_ERROR << "std::exception during transaction: " << e.what();
        if(tx) {
            tx->rollback();
        }
        co_return std::string("Unexpected error: ") + e.what();
    }

    co_return std::nullopt;
}
