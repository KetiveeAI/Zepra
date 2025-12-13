#pragma once

/**
 * @file promise.hpp
 * @brief Promise implementation for async/await support
 */

#include "../config.hpp"
#include "value.hpp"
#include "object.hpp"
#include <functional>
#include <vector>
#include <queue>

namespace Zepra::Runtime {

class VM;

/**
 * @brief Promise state
 */
enum class PromiseState : uint8 {
    Pending,
    Fulfilled,
    Rejected
};

// Forward declaration
class Promise;

/**
 * @brief Promise reaction (then/catch handler)
 */
struct PromiseReaction {
    Value onFulfilled;
    Value onRejected;
    Promise* resultPromise = nullptr;
};

/**
 * @brief JavaScript Promise object
 */
class Promise : public Object {
public:
    Promise();
    
    /**
     * @brief Get promise state
     */
    PromiseState state() const { return state_; }
    
    /**
     * @brief Get result value (if fulfilled/rejected)
     */
    Value result() const { return result_; }
    
    /**
     * @brief Resolve the promise with a value
     */
    void resolve(Value value);
    
    /**
     * @brief Reject the promise with a reason
     */
    void reject(Value reason);
    
    /**
     * @brief Add then handler
     */
    Promise* then(Value onFulfilled, Value onRejected = Value::undefined());
    
    /**
     * @brief Add catch handler
     */
    Promise* catchError(Value onRejected);
    
    /**
     * @brief Add finally handler
     */
    Promise* finally(Value onFinally);
    
    /**
     * @brief Static Promise.resolve()
     */
    static Promise* resolved(Value value);
    
    /**
     * @brief Static Promise.reject()
     */
    static Promise* rejected(Value reason);
    
private:
    void triggerReactions();
    
    PromiseState state_ = PromiseState::Pending;
    Value result_;
    std::vector<PromiseReaction> reactions_;
};

/**
 * @brief Microtask queue for promise resolution
 */
class MicrotaskQueue {
public:
    using Microtask = std::function<void()>;
    
    /**
     * @brief Add a microtask
     */
    void enqueue(Microtask task);
    
    /**
     * @brief Run all pending microtasks
     */
    void drain();
    
    /**
     * @brief Check if queue is empty
     */
    bool empty() const { return tasks_.empty(); }
    
    /**
     * @brief Get singleton instance
     */
    static MicrotaskQueue& instance();
    
private:
    std::queue<Microtask> tasks_;
};

} // namespace Zepra::Runtime
