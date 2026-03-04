#pragma once

/**
 * @file promise.hpp
 * @brief Promise implementation (ES6)
 */

#include "config.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include <vector>
#include <functional>

namespace Zepra::Runtime {

/**
 * @brief Promise states
 */
enum class PromiseState {
    Pending,
    Fulfilled,
    Rejected
};

/**
 * @brief Promise callback
 */
using PromiseCallback = std::function<Value(const Value&)>;

/**
 * @brief Promise object
 * 
 * Implements ES6 Promise with:
 * - then(onFulfilled, onRejected)
 * - catch(onRejected)
 * - finally(onFinally)
 */
class Promise : public Object {
public:
    Promise();
    
    // State management
    PromiseState state() const { return state_; }
    Value result() const { return result_; }
    
    // Resolve/Reject
    void resolve(const Value& value);
    void reject(const Value& reason);
    
    // Then/Catch/Finally
    Promise* then(PromiseCallback onFulfilled, PromiseCallback onRejected = nullptr);
    Promise* catchError(PromiseCallback onRejected);
    Promise* finally(PromiseCallback onFinally);
    
    // Static methods
    static Promise* resolved(const Value& value);
    static Promise* rejected(const Value& reason);
    static Promise* all(const std::vector<Promise*>& promises);
    static Promise* race(const std::vector<Promise*>& promises);
    
    // ES2020+
    static Promise* allSettled(const std::vector<Promise*>& promises);
    static Promise* any(const std::vector<Promise*>& promises);
    
    // ES2024
    static Object* withResolvers();
    
private:
    struct Reaction {
        PromiseCallback onFulfilled;
        PromiseCallback onRejected;
        Promise* promise;
    };
    
    void fulfill(const Value& value);
    void rejectInternal(const Value& reason);
    void processReactions();
    
    PromiseState state_;
    Value result_;
    std::vector<Reaction> reactions_;
};

/**
 * @brief Microtask queue (for promises)
 */
class MicrotaskQueue {
public:
    static MicrotaskQueue& instance();
    
    void enqueue(std::function<void()> task);
    void process();
    void drain(); // Defined in stubs/microtask_drain_stub.cpp
    bool isEmpty() const;
    
private:
    std::vector<std::function<void()>> queue_;
};

} // namespace Zepra::Runtime
