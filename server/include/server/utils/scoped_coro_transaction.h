#pragma once

#include <common/utils/utils.h>
#include <server/utils/switch_to_io_loop.h>

/**
 * @file scoped_coro_transaction.h
 * @brief Provides a high-level, coroutine-friendly database transaction wrapper.
 */

namespace server {

/// @brief The result type for a transactional operation. An empty optional indicates success.
/// A filled optional contains a string with the error message.
using ScopedTransactionResult =
    std::optional<std::string>;
/// @brief The function type for a lambda to be executed within a transaction.
/// It receives the transaction object and must return a Task resolving to a ScopedTransactionResult.
using ScopedTransactionFunc =
    std::function<drogon::Task<ScopedTransactionResult>(
        std::shared_ptr<drogon::orm::Transaction>)>;

namespace detail {
    /**
     * @struct CommitAwaiter
     * @brief (Implementation Detail) A C++ coroutine awaiter for Drogon's asynchronous transaction commit.
     *
     * @details Drogon's transaction object commits its changes upon destruction,
     * invoking a callback to signal success or failure. This awaiter provides a
     * clean, `co_await`-able mechanism to bridge this callback-based pattern with
     * coroutines.
     *
     * It works by:
     * 1. Taking ownership of the transaction pointer.
     * 2. Setting the commit callback on the transaction.
     * 3. Releasing its own reference to the transaction, allowing it to be destroyed and the commit to begin.
     * 4. Suspending the coroutine until the commit callback fires.
     * 5. Resuming the coroutine and returning the boolean result of the commit.
     *
     * @note This is an internal helper for WithTransaction and should not be used directly.
     */
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
} // namespace detail

/**
 * @brief A high-level wrapper that executes a lambda within a database transaction.
 *
 * @details This function abstracts away the complexity of manual transaction
 * management in a coroutine-based environment. It handles starting the
 * transaction, executing the user's logic, and correctly committing or rolling
 * back based on the outcome.
 *
 * The user provides a lambda containing their database operations.
 * - If the lambda returns an empty `std::optional`, the transaction is committed.
 * - If the lambda returns an optional with an error string, the transaction is rolled back, and the error is propagated.
 * - If the commit itself fails, an error is returned.
 * - Any exceptions thrown during the process are caught, the transaction is rolled back, and an error is returned.
 *
 * @param userLambda The function to execute. It receives the transaction
 *        object and must return a `drogon::Task<ScopedTransactionResult>`.
 * @return A `drogon::Task` that resolves to `std::nullopt` on success, or a
 *         string error message on failure.
 */
inline drogon::Task<ScopedTransactionResult> WithTransaction(ScopedTransactionFunc userLambda) {
    std::shared_ptr<drogon::orm::Transaction> tx;

    try {
        auto db = drogon::app().getDbClient();
        if(!db) {
            LOG_ERROR << "DB client not available";
            co_return "Internal: DB client unavailable";
        }

        tx = co_await switch_to_io_loop(db->newTransactionCoro());
        LOG_TRACE << "Transaction started";

        if(auto err = co_await userLambda(tx)) {
            LOG_DEBUG << "User lambda returned error, rolling back";
            tx->rollback();
            co_return err;
        }

        if(!co_await switch_to_io_loop(detail::CommitAwaiter{std::move(tx)})) {
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

} // namespace server
