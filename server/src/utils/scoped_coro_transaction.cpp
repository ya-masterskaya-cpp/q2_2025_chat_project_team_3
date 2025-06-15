#include <server/utils/scoped_coro_transaction.h>

drogon::Task<ScopedTransactionResult> WithTransaction(ScopedTransactionFunc userLambda) {
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