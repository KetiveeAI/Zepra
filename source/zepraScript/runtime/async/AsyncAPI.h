/**
 * @file AsyncAPI.h
 * @brief Async/Await Runtime Implementation
 * 
 * ECMAScript Async Functions:
 * - AsyncFunction wrapper
 * - AsyncGenerator
 * - Await suspension
 */

#pragma once

#include "PromiseAPI.h"
#include "GeneratorAPI.h"
#include <memory>
#include <functional>

namespace Zepra::Runtime {

// =============================================================================
// Async Function Result
// =============================================================================

/**
 * @brief Result of async function execution
 */
struct AsyncResult {
    bool done;
    std::shared_ptr<Promise> promise;
};

// =============================================================================
// Async Context
// =============================================================================

/**
 * @brief Execution context for async functions
 */
class AsyncContext {
public:
    AsyncContext() : resultPromise_(std::make_shared<Promise>()) {}
    
    // Get the result promise
    std::shared_ptr<Promise> resultPromise() const { return resultPromise_; }
    
    // Await a promise
    template<typename T>
    T await(std::shared_ptr<Promise> promise);
    
    // Resolve the async function
    void resolve(PromiseValue value) {
        resultPromise_->resolve(std::move(value));
    }
    
    // Reject the async function
    void reject(PromiseValue reason) {
        resultPromise_->reject(std::move(reason));
    }
    
private:
    std::shared_ptr<Promise> resultPromise_;
};

// =============================================================================
// Async Function
// =============================================================================

/**
 * @brief Async function that returns a Promise
 */
class AsyncFunction {
public:
    using Body = std::function<void(AsyncContext&)>;
    
    explicit AsyncFunction(Body body) : body_(std::move(body)) {}
    
    // Call the async function
    std::shared_ptr<Promise> operator()() const {
        auto ctx = std::make_shared<AsyncContext>();
        
        try {
            if (body_) {
                body_(*ctx);
            }
        } catch (const std::exception& e) {
            ctx->reject(std::string(e.what()));
        }
        
        return ctx->resultPromise();
    }
    
private:
    Body body_;
};

// =============================================================================
// Async Generator State
// =============================================================================

enum class AsyncGeneratorState {
    SuspendedStart,
    SuspendedYield,
    Executing,
    AwaitingReturn,
    Completed
};

// =============================================================================
// Async Generator
// =============================================================================

/**
 * @brief Async generator with yield and await
 */
class AsyncGenerator {
public:
    using Body = std::function<void(class AsyncGeneratorContext&)>;
    
    explicit AsyncGenerator(Body body)
        : body_(std::move(body))
        , state_(AsyncGeneratorState::SuspendedStart) {}
    
    // async iterator protocol
    std::shared_ptr<Promise> next() {
        return next(PromiseValue{});
    }
    
    std::shared_ptr<Promise> next(PromiseValue value) {
        if (state_ == AsyncGeneratorState::Completed) {
            return Promise::resolve(PromiseValue{});  // {value: undefined, done: true}
        }
        
        return Promise::create([this, value](auto resolve, auto reject) {
            try {
                // Would execute and yield
                state_ = AsyncGeneratorState::Executing;
                // Execute body_...
                
                resolve(PromiseValue{});
            } catch (const std::exception& e) {
                reject(std::string(e.what()));
            }
        });
    }
    
    std::shared_ptr<Promise> return_(PromiseValue value) {
        state_ = AsyncGeneratorState::Completed;
        return Promise::resolve(value);
    }
    
    std::shared_ptr<Promise> throw_(PromiseValue exception) {
        state_ = AsyncGeneratorState::Completed;
        return Promise::reject(exception);
    }
    
    AsyncGeneratorState state() const { return state_; }
    
private:
    Body body_;
    AsyncGeneratorState state_;
};

// =============================================================================
// Async Generator Context
// =============================================================================

/**
 * @brief Context for async generator execution
 */
class AsyncGeneratorContext {
public:
    AsyncGeneratorContext() = default;
    
    // Yield a value (returns promise)
    std::shared_ptr<Promise> yield(PromiseValue value) {
        yieldedValue_ = std::move(value);
        return Promise::resolve(PromiseValue{});
    }
    
    // Await a promise
    template<typename T>
    T await(std::shared_ptr<Promise> promise);
    
    // Get yielded value
    const std::optional<PromiseValue>& yieldedValue() const {
        return yieldedValue_;
    }
    
private:
    std::optional<PromiseValue> yieldedValue_;
};

// =============================================================================
// Async Iteration Helpers
// =============================================================================

/**
 * @brief for-await-of helper
 */
template<typename Fn>
std::shared_ptr<Promise> forAwaitOf(AsyncGenerator& gen, Fn callback) {
    return Promise::create([&gen, callback](auto resolve, auto reject) {
        std::function<void()> iterate;
        
        iterate = [&gen, callback, resolve, reject, &iterate]() {
            gen.next()->then(
                [callback, resolve, &iterate](const PromiseValue& result) -> PromiseValue {
                    // Check if done
                    // Would check result.done
                    // callback(result.value);
                    // iterate();
                    resolve(PromiseValue{});
                    return result;
                },
                [reject](const PromiseValue& reason) -> PromiseValue {
                    reject(reason);
                    return reason;
                }
            );
        };
        
        iterate();
    });
}

// =============================================================================
// Async From Sync Iterator
// =============================================================================

/**
 * @brief Wrap sync iterator as async iterator
 */
class AsyncFromSyncIterator {
public:
    explicit AsyncFromSyncIterator(std::unique_ptr<Iterator> syncIterator)
        : syncIterator_(std::move(syncIterator)) {}
    
    std::shared_ptr<Promise> next() {
        auto result = syncIterator_->next();
        
        // Convert to async result
        return Promise::resolve(result.value);
    }
    
private:
    std::unique_ptr<Iterator> syncIterator_;
};

} // namespace Zepra::Runtime
