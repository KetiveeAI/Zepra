/**
 * @file PromiseAPI.h
 * @brief Promise Implementation
 * 
 * ECMAScript Promise:
 * - Promise state machine (pending/fulfilled/rejected)
 * - PromiseReaction for then/catch
 * - Promise combinators (all, race, allSettled, any)
 */

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <variant>
#include <mutex>
#include <optional>

namespace Zepra::Runtime {

// Forward declarations
class Promise;
class PromiseReaction;

// =============================================================================
// Promise State
// =============================================================================

enum class PromiseState {
    Pending,
    Fulfilled,
    Rejected
};

// =============================================================================
// Promise Value
// =============================================================================

using PromiseValue = std::variant<
    std::monostate,                    // undefined
    bool,
    double,
    std::string,
    std::shared_ptr<void>,             // Object reference
    std::shared_ptr<Promise>           // Nested promise
>;

// =============================================================================
// Promise Reaction
// =============================================================================

/**
 * @brief Handler for promise resolution/rejection
 */
class PromiseReaction {
public:
    using Handler = std::function<PromiseValue(const PromiseValue&)>;
    
    PromiseReaction(std::shared_ptr<Promise> promise,
                    Handler onFulfilled = nullptr,
                    Handler onRejected = nullptr)
        : promise_(promise)
        , onFulfilled_(std::move(onFulfilled))
        , onRejected_(std::move(onRejected)) {}
    
    std::shared_ptr<Promise> promise() const { return promise_; }
    
    void trigger(PromiseState state, const PromiseValue& value) {
        Handler handler = (state == PromiseState::Fulfilled) 
            ? onFulfilled_ : onRejected_;
        
        if (handler) {
            try {
                PromiseValue result = handler(value);
                promise_->resolve(result);
            } catch (const std::exception& e) {
                promise_->reject(std::string(e.what()));
            }
        } else {
            if (state == PromiseState::Fulfilled) {
                promise_->resolve(value);
            } else {
                promise_->reject(value);
            }
        }
    }
    
private:
    std::shared_ptr<Promise> promise_;
    Handler onFulfilled_;
    Handler onRejected_;
};

// =============================================================================
// Promise
// =============================================================================

/**
 * @brief ECMAScript Promise implementation
 */
class Promise : public std::enable_shared_from_this<Promise> {
public:
    using Executor = std::function<void(
        std::function<void(PromiseValue)>,   // resolve
        std::function<void(PromiseValue)>    // reject
    )>;
    
    using Callback = std::function<PromiseValue(const PromiseValue&)>;
    
    // Create with executor
    static std::shared_ptr<Promise> create(Executor executor) {
        auto promise = std::make_shared<Promise>();
        
        try {
            executor(
                [promise](PromiseValue value) { promise->resolve(value); },
                [promise](PromiseValue reason) { promise->reject(reason); }
            );
        } catch (const std::exception& e) {
            promise->reject(std::string(e.what()));
        }
        
        return promise;
    }
    
    // Create resolved promise
    static std::shared_ptr<Promise> resolve(PromiseValue value) {
        auto promise = std::make_shared<Promise>();
        promise->resolve(value);
        return promise;
    }
    
    // Create rejected promise
    static std::shared_ptr<Promise> reject(PromiseValue reason) {
        auto promise = std::make_shared<Promise>();
        promise->reject(reason);
        return promise;
    }
    
    // State
    PromiseState state() const {
        std::lock_guard lock(mutex_);
        return state_;
    }
    
    const PromiseValue& result() const {
        std::lock_guard lock(mutex_);
        return result_;
    }
    
    // then()
    std::shared_ptr<Promise> then(Callback onFulfilled,
                                   Callback onRejected = nullptr) {
        auto nextPromise = std::make_shared<Promise>();
        
        {
            std::lock_guard lock(mutex_);
            
            if (state_ == PromiseState::Pending) {
                reactions_.push_back(std::make_unique<PromiseReaction>(
                    nextPromise, onFulfilled, onRejected));
            } else {
                // Already settled - schedule microtask
                PromiseReaction reaction(nextPromise, onFulfilled, onRejected);
                reaction.trigger(state_, result_);
            }
        }
        
        return nextPromise;
    }
    
    // catch()
    std::shared_ptr<Promise> catch_(Callback onRejected) {
        return then(nullptr, onRejected);
    }
    
    // finally()
    std::shared_ptr<Promise> finally_(std::function<void()> onFinally) {
        return then(
            [onFinally](const PromiseValue& value) -> PromiseValue {
                if (onFinally) onFinally();
                return value;
            },
            [onFinally](const PromiseValue& reason) -> PromiseValue {
                if (onFinally) onFinally();
                throw std::runtime_error("rethrow");
            }
        );
    }
    
    // Resolve
    void resolve(PromiseValue value) {
        std::vector<std::unique_ptr<PromiseReaction>> reactions;
        
        {
            std::lock_guard lock(mutex_);
            if (state_ != PromiseState::Pending) return;
            
            // Check for promise
            if (auto* nested = std::get_if<std::shared_ptr<Promise>>(&value)) {
                (*nested)->then(
                    [self = shared_from_this()](const PromiseValue& v) -> PromiseValue {
                        self->resolve(v);
                        return v;
                    },
                    [self = shared_from_this()](const PromiseValue& r) -> PromiseValue {
                        self->reject(r);
                        return r;
                    }
                );
                return;
            }
            
            state_ = PromiseState::Fulfilled;
            result_ = value;
            reactions = std::move(reactions_);
        }
        
        for (auto& reaction : reactions) {
            reaction->trigger(PromiseState::Fulfilled, value);
        }
    }
    
    // Reject
    void reject(PromiseValue reason) {
        std::vector<std::unique_ptr<PromiseReaction>> reactions;
        
        {
            std::lock_guard lock(mutex_);
            if (state_ != PromiseState::Pending) return;
            
            state_ = PromiseState::Rejected;
            result_ = reason;
            reactions = std::move(reactions_);
        }
        
        for (auto& reaction : reactions) {
            reaction->trigger(PromiseState::Rejected, reason);
        }
    }
    
    // =========================================================================
    // Static Combinators
    // =========================================================================
    
    // Promise.all
    static std::shared_ptr<Promise> all(
            std::vector<std::shared_ptr<Promise>> promises) {
        if (promises.empty()) {
            return resolve(PromiseValue{});
        }
        
        struct Context {
            std::vector<PromiseValue> results;
            size_t remaining;
            bool rejected = false;
            std::mutex mutex;
        };
        
        auto ctx = std::make_shared<Context>();
        ctx->results.resize(promises.size());
        ctx->remaining = promises.size();
        
        return create([promises, ctx](auto resolve, auto reject) {
            for (size_t i = 0; i < promises.size(); i++) {
                promises[i]->then(
                    [ctx, i, resolve](const PromiseValue& value) -> PromiseValue {
                        std::lock_guard lock(ctx->mutex);
                        if (ctx->rejected) return value;
                        
                        ctx->results[i] = value;
                        ctx->remaining--;
                        
                        if (ctx->remaining == 0) {
                            // Would convert to array
                            resolve(PromiseValue{});
                        }
                        return value;
                    },
                    [ctx, reject](const PromiseValue& reason) -> PromiseValue {
                        std::lock_guard lock(ctx->mutex);
                        if (!ctx->rejected) {
                            ctx->rejected = true;
                            reject(reason);
                        }
                        return reason;
                    }
                );
            }
        });
    }
    
    // Promise.race
    static std::shared_ptr<Promise> race(
            std::vector<std::shared_ptr<Promise>> promises) {
        struct Context {
            bool settled = false;
            std::mutex mutex;
        };
        
        auto ctx = std::make_shared<Context>();
        
        return create([promises, ctx](auto resolve, auto reject) {
            for (auto& promise : promises) {
                promise->then(
                    [ctx, resolve](const PromiseValue& value) -> PromiseValue {
                        std::lock_guard lock(ctx->mutex);
                        if (!ctx->settled) {
                            ctx->settled = true;
                            resolve(value);
                        }
                        return value;
                    },
                    [ctx, reject](const PromiseValue& reason) -> PromiseValue {
                        std::lock_guard lock(ctx->mutex);
                        if (!ctx->settled) {
                            ctx->settled = true;
                            reject(reason);
                        }
                        return reason;
                    }
                );
            }
        });
    }
    
    // Promise.allSettled
    static std::shared_ptr<Promise> allSettled(
            std::vector<std::shared_ptr<Promise>> promises) {
        if (promises.empty()) {
            return resolve(PromiseValue{});
        }
        
        struct SettledResult {
            std::string status;
            std::optional<PromiseValue> value;
            std::optional<PromiseValue> reason;
        };
        
        struct Context {
            std::vector<SettledResult> results;
            size_t remaining;
            std::mutex mutex;
        };
        
        auto ctx = std::make_shared<Context>();
        ctx->results.resize(promises.size());
        ctx->remaining = promises.size();
        
        return create([promises, ctx](auto resolve, auto reject) {
            for (size_t i = 0; i < promises.size(); i++) {
                promises[i]->then(
                    [ctx, i, resolve](const PromiseValue& value) -> PromiseValue {
                        std::lock_guard lock(ctx->mutex);
                        ctx->results[i] = {"fulfilled", value, std::nullopt};
                        ctx->remaining--;
                        if (ctx->remaining == 0) resolve(PromiseValue{});
                        return value;
                    },
                    [ctx, i, resolve](const PromiseValue& reason) -> PromiseValue {
                        std::lock_guard lock(ctx->mutex);
                        ctx->results[i] = {"rejected", std::nullopt, reason};
                        ctx->remaining--;
                        if (ctx->remaining == 0) resolve(PromiseValue{});
                        return reason;
                    }
                );
            }
        });
    }
    
    // Promise.any
    static std::shared_ptr<Promise> any(
            std::vector<std::shared_ptr<Promise>> promises) {
        struct Context {
            std::vector<PromiseValue> errors;
            size_t remaining;
            bool resolved = false;
            std::mutex mutex;
        };
        
        auto ctx = std::make_shared<Context>();
        ctx->errors.resize(promises.size());
        ctx->remaining = promises.size();
        
        return create([promises, ctx](auto resolve, auto reject) {
            for (size_t i = 0; i < promises.size(); i++) {
                promises[i]->then(
                    [ctx, resolve](const PromiseValue& value) -> PromiseValue {
                        std::lock_guard lock(ctx->mutex);
                        if (!ctx->resolved) {
                            ctx->resolved = true;
                            resolve(value);
                        }
                        return value;
                    },
                    [ctx, i, reject](const PromiseValue& reason) -> PromiseValue {
                        std::lock_guard lock(ctx->mutex);
                        if (ctx->resolved) return reason;
                        
                        ctx->errors[i] = reason;
                        ctx->remaining--;
                        
                        if (ctx->remaining == 0) {
                            reject(std::string("AggregateError"));
                        }
                        return reason;
                    }
                );
            }
        });
    }
    
private:
    Promise() = default;
    
    mutable std::mutex mutex_;
    PromiseState state_ = PromiseState::Pending;
    PromiseValue result_;
    std::vector<std::unique_ptr<PromiseReaction>> reactions_;
};

} // namespace Zepra::Runtime
