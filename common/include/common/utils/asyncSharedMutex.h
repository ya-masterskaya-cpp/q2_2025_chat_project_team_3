#pragma once

#include <drogon/drogon.h>
#include <atomic>
#include <coroutine>
#include <memory>

/**
 * @file asyncSharedMutex.h
 * @brief Defines the AsyncSharedMutex class, a low-level asynchronous synchronization primitive.
 */

/**
 * @class AsyncSharedMutex
 * @brief An asynchronous, lifetime-safe, coroutine-based reader-writer mutex.
 *
 * @details This class provides a foundational mechanism to protect a shared resource in an
 * asynchronous environment, such as a Drogon application, without blocking the
 * event loop thread. It allows multiple concurrent readers (shared locks) or a
 * single exclusive writer (unique lock).
 *
 * The implementation uses an "asynchronous spin-lock" strategy. When a
 * coroutine `co_await`s a lock, it immediately suspends. A small worker lambda
 * is posted to the event loop. This lambda attempts to acquire the lock. If it
 * fails, it re-posts itself to the event loop's queue to try again later. If it
 * succeeds, it resumes the original coroutine.
 *
 * This implementation ensures that a coroutine always resumes on the same IO
 * event loop thread it was suspended on, preserving thread affinity.
 *
 * @note This mutex must be created via the static `create()` factory method.
 */
class AsyncSharedMutex : public std::enable_shared_from_this<AsyncSharedMutex> {
private:
    /// @brief The atomic state of the mutex. (0=free, -1=unique, >0=shared count)
    std::atomic<int> state_{0};

    /// @brief Private constructor to enforce creation via the `create()` factory method.
    AsyncSharedMutex() = default;

public:
    /**
     * @brief Factory method to create a new lifetime-safe AsyncSharedMutex instance.
     * @return A `std::shared_ptr<AsyncSharedMutex>` managing the new instance.
     */
    static std::shared_ptr<AsyncSharedMutex> create() {
        return std::shared_ptr<AsyncSharedMutex>(new AsyncSharedMutex());
    }

    /**
     * @class SharedLockGuard
     * @brief A movable, RAII-style scope guard for a shared (reader) lock.
     */
    class SharedLockGuard {
        friend class AsyncSharedMutex;
        explicit SharedLockGuard(std::shared_ptr<AsyncSharedMutex> mutex) noexcept : mutex_(std::move(mutex)) {}
    public:
        ~SharedLockGuard() {
            if (mutex_) {
                mutex_->state_.fetch_sub(1, std::memory_order_release);
            }
        }
        SharedLockGuard(const SharedLockGuard&) = delete;
        SharedLockGuard& operator=(const SharedLockGuard&) = delete;
        SharedLockGuard(SharedLockGuard&&) noexcept = default;
        SharedLockGuard& operator=(SharedLockGuard&&) = delete;
    private:
        std::shared_ptr<AsyncSharedMutex> mutex_;
    };
    
    /**
     * @class UniqueLockGuard
     * @brief A movable, RAII-style scope guard for a unique (writer) lock.
     */
    class UniqueLockGuard {
        friend class AsyncSharedMutex;
        explicit UniqueLockGuard(std::shared_ptr<AsyncSharedMutex> mutex) noexcept : mutex_(std::move(mutex)) {}
    public:
        ~UniqueLockGuard() {
            if (mutex_) {
                mutex_->state_.store(0, std::memory_order_release);
            }
        }
        UniqueLockGuard(const UniqueLockGuard&) = delete;
        UniqueLockGuard& operator=(const UniqueLockGuard&) = delete;
        UniqueLockGuard(UniqueLockGuard&&) noexcept = default;
        UniqueLockGuard& operator=(UniqueLockGuard&&) = delete;
    private:
        std::shared_ptr<AsyncSharedMutex> mutex_;
    };

    /**
     * @brief An awaitable object for acquiring a shared lock.
     */
    struct SharedLockAwaitable {
        std::shared_ptr<AsyncSharedMutex> mutex;
        std::coroutine_handle<> handle_ = nullptr;

        bool await_ready() noexcept {
            return try_lock();
        }

        void await_suspend(std::coroutine_handle<> h) noexcept {
            handle_ = h;
            drogon::app().getIOLoop(drogon::app().getCurrentThreadIndex())->queueInLoop([this] { spin_lock(); });
        }

        SharedLockGuard await_resume() noexcept {
            return SharedLockGuard{std::move(mutex)};
        }

    private:
        bool try_lock() noexcept {
            int current = mutex->state_.load(std::memory_order_acquire);
            if (current >= 0) {
                return mutex->state_.compare_exchange_strong(current, current + 1, std::memory_order_acq_rel);
            }
            return false;
        }

        void spin_lock() {
            if (try_lock()) {
                // Post resumption to the event loop to keep the call stack shallow.
                drogon::app().getIOLoop(drogon::app().getCurrentThreadIndex())->queueInLoop([h = handle_] { h.resume(); });
            } else {
                // Re-queue this spin_lock task to try again.
                drogon::app().getIOLoop(drogon::app().getCurrentThreadIndex())->queueInLoop([this] { spin_lock(); });
            }
        }
    };
    
    /**
     * @brief An awaitable object for acquiring a unique lock.
     */
    struct UniqueLockAwaitable {
        std::shared_ptr<AsyncSharedMutex> mutex;
        std::coroutine_handle<> handle_ = nullptr;

        bool await_ready() noexcept {
            return try_lock();
        }

        void await_suspend(std::coroutine_handle<> h) noexcept {
            handle_ = h;
            drogon::app().getIOLoop(drogon::app().getCurrentThreadIndex())->queueInLoop([this] { spin_lock(); });
        }

        UniqueLockGuard await_resume() noexcept {
            return UniqueLockGuard{std::move(mutex)};
        }

    private:
        bool try_lock() noexcept {
            int expected = 0;
            return mutex->state_.compare_exchange_strong(expected, -1, std::memory_order_acq_rel);
        }

        void spin_lock() {
            if (try_lock()) {
                // Post resumption to the event loop to keep the call stack shallow.
                drogon::app().getIOLoop(drogon::app().getCurrentThreadIndex())->queueInLoop([h = handle_] { h.resume(); });
            } else {
                // Re-queue this spin_lock task to try again.
                drogon::app().getIOLoop(drogon::app().getCurrentThreadIndex())->queueInLoop([this] { spin_lock(); });
            }
        }
    };

    /**
     * @brief Creates an awaitable to acquire a shared (reader) lock.
     * @return A `SharedLockAwaitable` object to be used with `co_await`.
     */
    [[nodiscard]] SharedLockAwaitable lock_shared() {
        return {shared_from_this()};
    }

    /**
     * @brief Creates an awaitable to acquire a unique (writer) lock.
     * @return A `UniqueLockAwaitable` object to be used with `co_await`.
     */
    [[nodiscard]] UniqueLockAwaitable lock_unique() {
        return {shared_from_this()};
    }
};
