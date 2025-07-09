#pragma once

#include <common/utils/asyncSharedMutex.h>
#include <utility>

namespace common {

/**
 * @class Guarded
 * @brief A template class that bundles a data object with an `AsyncSharedMutex`.
 * @tparam T The type of the data to protect.
 *
 * @details This class provides a convenient and safe abstraction for managing a shared
 * resource in an asynchronous context. Instead of managing a mutex and data
 * separately, this class encapsulates them, significantly reducing the risk of
 * misuse.
 *
 * Access to the underlying data is exclusively provided through RAII-style proxy
 * objects. These proxies are obtained by `co_await`ing the `lock_shared()` or
 * `lock_unique()` methods. The lock is held for the lifetime of the proxy object,
 * and access to the data is provided via overloaded `operator->` and `operator*`.
 * This design ensures that the data can never be accessed without first acquiring
 * the appropriate lock.
 *
 */
template <typename T>
class Guarded {
public:
    /**
     * @class SharedProxy
     * @brief A proxy object providing temporary, read-only access to the guarded data.
     * @details This object is returned by `co_await guarded_obj.lock_shared()`. The
     * underlying shared lock is held for the lifetime of this proxy. It provides
     * `const` access to the data via `operator->` and `operator*`.
     */
    class SharedProxy {
    public:
        const T* operator->() const noexcept { return data_; }
        const T& operator*() const noexcept { return *data_; }

    private:
        friend class Guarded<T>;
        explicit SharedProxy(AsyncSharedMutex::SharedLockGuard&& guard, const T* data) noexcept
            : guard_(std::move(guard)), data_(data) {}
            
        AsyncSharedMutex::SharedLockGuard guard_;
        const T* data_;
    };

    /**
     * @class UniqueProxy
     * @brief A proxy object providing temporary, read-write access to the guarded data.
     * @details This object is returned by `co_await guarded_obj.lock_unique()`. The
     * underlying unique lock is held for the lifetime of this proxy. It provides
     * `non-const` access to the data via `operator->` and `operator*`.
     */
    class UniqueProxy {
    public:
        T* operator->() noexcept { return data_; }
        T& operator*() noexcept { return *data_; }

    private:
        friend class Guarded<T>;
        explicit UniqueProxy(AsyncSharedMutex::UniqueLockGuard&& guard, T* data) noexcept
            : guard_(std::move(guard)), data_(data) {}
            
        AsyncSharedMutex::UniqueLockGuard guard_;
        T* data_;
    };

    /**
     * @brief Constructs the Guarded object, perfectly forwarding arguments
     *        to the constructor of the underlying data type T.
     * @param args Arguments to be forwarded to T's constructor.
     */
    template <typename... Args>
    explicit Guarded(Args&&... args)
        : mutex_(AsyncSharedMutex::create()),
          data_(std::forward<Args>(args)...) {}

    // A guarded object is a unique resource: non-copyable, but movable.
    Guarded(const Guarded&) = delete;
    Guarded& operator=(const Guarded&) = delete;
    Guarded(Guarded&&) noexcept = default;
    Guarded& operator=(Guarded&&) noexcept = default;

private:
    /// @brief Internal awaitable that combines the mutex awaitable with proxy creation.
    struct SharedAwaitable {
        Guarded<T>* guarded_;
        AsyncSharedMutex::SharedLockAwaitable inner_awaitable_;
        
        bool await_ready() noexcept { return inner_awaitable_.await_ready(); }
        void await_suspend(std::coroutine_handle<> h) noexcept { inner_awaitable_.await_suspend(h); }
        SharedProxy await_resume() noexcept {
            return SharedProxy(inner_awaitable_.await_resume(), &guarded_->data_);
        }
    };
    
    /// @brief Internal awaitable that combines the mutex awaitable with proxy creation.
    struct UniqueAwaitable {
        Guarded<T>* guarded_;
        AsyncSharedMutex::UniqueLockAwaitable inner_awaitable_;

        bool await_ready() noexcept { return inner_awaitable_.await_ready(); }
        void await_suspend(std::coroutine_handle<> h) noexcept { inner_awaitable_.await_suspend(h); }
        UniqueProxy await_resume() noexcept {
            return UniqueProxy(inner_awaitable_.await_resume(), &guarded_->data_);
        }
    };

public:
    /**
     * @brief Asynchronously acquires a shared (reader) lock on the data.
     * @return An awaitable that, upon success, resolves to a `SharedProxy` object.
     */
    SharedAwaitable lock_shared() {
        return {this, mutex_->lock_shared()};
    }

    /**
     * @brief Asynchronously acquires a unique (writer) lock on the data.
     * @return An awaitable that, upon success, resolves to a `UniqueProxy` object.
     */
    UniqueAwaitable lock_unique() {
        return {this, mutex_->lock_unique()};
    }

    /**
     * @brief Verifies if this Guarded object is the container for the given data reference.
     *
     * This method enables safe re-entrant patterns by allowing functions to check
     * if a provided data reference corresponds to a specific guarded object,
     * thus avoiding attempts to re-lock a mutex already held by the caller.
     *
     * @param data A const reference to a raw data object to check.
     * @return `true` if this instance holds the provided data object, `false` otherwise.
     */
    bool isHolding(const T& data) const noexcept {
        // Compare memory addresses to check if the provided data is the one we are guarding.
        return &this->data_ == &data;
    }

private:
    std::shared_ptr<AsyncSharedMutex> mutex_;
    T data_;
};

} // namespace common
