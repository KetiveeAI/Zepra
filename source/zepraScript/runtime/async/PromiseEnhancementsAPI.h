#pragma once

#include <functional>
#include <future>
#include <exception>
#include <optional>
#include <tuple>

namespace Zepra::Runtime {

template<typename T>
struct PromiseWithResolvers {
    std::promise<T> promise;
    std::future<T> future;
    std::function<void(T)> resolve;
    std::function<void(std::exception_ptr)> reject;

    PromiseWithResolvers() {
        future = promise.get_future();
        
        auto promisePtr = std::make_shared<std::promise<T>>(std::move(promise));
        
        resolve = [promisePtr](T value) {
            promisePtr->set_value(std::move(value));
        };
        
        reject = [promisePtr](std::exception_ptr ex) {
            promisePtr->set_exception(ex);
        };
    }
};

template<>
struct PromiseWithResolvers<void> {
    std::promise<void> promise;
    std::future<void> future;
    std::function<void()> resolve;
    std::function<void(std::exception_ptr)> reject;

    PromiseWithResolvers() {
        future = promise.get_future();
        
        auto promisePtr = std::make_shared<std::promise<void>>(std::move(promise));
        
        resolve = [promisePtr]() {
            promisePtr->set_value();
        };
        
        reject = [promisePtr](std::exception_ptr ex) {
            promisePtr->set_exception(ex);
        };
    }
};

class PromiseEnhancements {
public:
    template<typename T>
    static PromiseWithResolvers<T> withResolvers() {
        return PromiseWithResolvers<T>();
    }

    template<typename T, typename Fn>
    static std::future<T> trySync(Fn&& fn) {
        std::promise<T> p;
        auto future = p.get_future();
        
        try {
            if constexpr (std::is_void_v<T>) {
                fn();
                p.set_value();
            } else {
                p.set_value(fn());
            }
        } catch (...) {
            p.set_exception(std::current_exception());
        }
        
        return future;
    }

    template<typename T, typename Fn>
    static std::future<T> tryAsync(Fn&& fn) {
        return std::async(std::launch::async, [fn = std::forward<Fn>(fn)]() {
            return fn();
        });
    }

    template<typename T>
    static std::future<T> resolve(T value) {
        std::promise<T> p;
        p.set_value(std::move(value));
        return p.get_future();
    }

    static std::future<void> resolve() {
        std::promise<void> p;
        p.set_value();
        return p.get_future();
    }

    template<typename T>
    static std::future<T> reject(std::exception_ptr ex) {
        std::promise<T> p;
        p.set_exception(ex);
        return p.get_future();
    }

    template<typename T, typename E>
    static std::future<T> reject(const E& error) {
        std::promise<T> p;
        p.set_exception(std::make_exception_ptr(error));
        return p.get_future();
    }

    template<typename T, typename OnFulfilled, typename OnRejected>
    static std::future<T> then(std::future<T>& fut, OnFulfilled onFulfilled, OnRejected onRejected) {
        return std::async(std::launch::async, [&fut, onFulfilled, onRejected]() {
            try {
                if constexpr (std::is_void_v<T>) {
                    fut.get();
                    return onFulfilled();
                } else {
                    return onFulfilled(fut.get());
                }
            } catch (...) {
                return onRejected(std::current_exception());
            }
        });
    }

    template<typename T, typename Handler>
    static std::future<T> catchError(std::future<T>& fut, Handler handler) {
        return std::async(std::launch::async, [&fut, handler]() {
            try {
                return fut.get();
            } catch (...) {
                return handler(std::current_exception());
            }
        });
    }

    template<typename T, typename Finally>
    static std::future<T> finally(std::future<T>& fut, Finally fn) {
        return std::async(std::launch::async, [&fut, fn]() {
            std::exception_ptr ex;
            std::optional<T> result;
            
            try {
                if constexpr (!std::is_void_v<T>) {
                    result = fut.get();
                } else {
                    fut.get();
                }
            } catch (...) {
                ex = std::current_exception();
            }
            
            fn();
            
            if (ex) std::rethrow_exception(ex);
            
            if constexpr (!std::is_void_v<T>) {
                return *result;
            }
        });
    }
};

template<typename T>
class DeferredPromise {
private:
    std::shared_ptr<std::promise<T>> promise_;
    std::shared_future<T> future_;
    bool settled_ = false;

public:
    DeferredPromise() 
        : promise_(std::make_shared<std::promise<T>>())
        , future_(promise_->get_future().share()) {}

    void resolve(T value) {
        if (!settled_) {
            promise_->set_value(std::move(value));
            settled_ = true;
        }
    }

    void reject(std::exception_ptr ex) {
        if (!settled_) {
            promise_->set_exception(ex);
            settled_ = true;
        }
    }

    template<typename E>
    void reject(const E& error) {
        reject(std::make_exception_ptr(error));
    }

    std::shared_future<T> getFuture() const { return future_; }
    bool isSettled() const { return settled_; }
};

template<>
class DeferredPromise<void> {
private:
    std::shared_ptr<std::promise<void>> promise_;
    std::shared_future<void> future_;
    bool settled_ = false;

public:
    DeferredPromise()
        : promise_(std::make_shared<std::promise<void>>())
        , future_(promise_->get_future().share()) {}

    void resolve() {
        if (!settled_) {
            promise_->set_value();
            settled_ = true;
        }
    }

    void reject(std::exception_ptr ex) {
        if (!settled_) {
            promise_->set_exception(ex);
            settled_ = true;
        }
    }

    std::shared_future<void> getFuture() const { return future_; }
    bool isSettled() const { return settled_; }
};

}
