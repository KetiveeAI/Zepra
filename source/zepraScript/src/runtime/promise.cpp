/**
 * @file promise.cpp
 * @brief Promise implementation for async/await
 */

#include "zeprascript/runtime/promise.hpp"
#include "zeprascript/runtime/function.hpp"

namespace Zepra::Runtime {

// =============================================================================
// Promise Implementation
// =============================================================================

Promise::Promise() : Object(ObjectType::Promise) {}

void Promise::resolve(Value value) {
    if (state_ != PromiseState::Pending) return; // Already settled
    
    state_ = PromiseState::Fulfilled;
    result_ = value;
    triggerReactions();
}

void Promise::reject(Value reason) {
    if (state_ != PromiseState::Pending) return; // Already settled
    
    state_ = PromiseState::Rejected;
    result_ = reason;
    triggerReactions();
}

Promise* Promise::then(Value onFulfilled, Value onRejected) {
    Promise* resultPromise = new Promise();
    
    PromiseReaction reaction;
    reaction.onFulfilled = onFulfilled;
    reaction.onRejected = onRejected;
    reaction.resultPromise = resultPromise;
    
    if (state_ == PromiseState::Pending) {
        // Still pending - queue reaction
        reactions_.push_back(reaction);
    } else {
        // Already settled - schedule microtask
        MicrotaskQueue::instance().enqueue([this, reaction]() {
            Value handler = (state_ == PromiseState::Fulfilled) 
                ? reaction.onFulfilled 
                : reaction.onRejected;
                
            if (handler.isObject() && handler.asObject()->isFunction()) {
                // Call handler with result
                // TODO: Actually call the function through VM
                reaction.resultPromise->resolve(result_);
            } else {
                // No handler - propagate result
                if (state_ == PromiseState::Fulfilled) {
                    reaction.resultPromise->resolve(result_);
                } else {
                    reaction.resultPromise->reject(result_);
                }
            }
        });
    }
    
    return resultPromise;
}

Promise* Promise::catchError(Value onRejected) {
    return then(Value::undefined(), onRejected);
}

Promise* Promise::finally(Value onFinally) {
    return then(onFinally, onFinally);
}

void Promise::triggerReactions() {
    for (const auto& reaction : reactions_) {
        MicrotaskQueue::instance().enqueue([this, reaction]() {
            Value handler = (state_ == PromiseState::Fulfilled) 
                ? reaction.onFulfilled 
                : reaction.onRejected;
                
            if (handler.isObject() && handler.asObject()->isFunction()) {
                // TODO: Call handler through VM
                reaction.resultPromise->resolve(result_);
            } else {
                if (state_ == PromiseState::Fulfilled) {
                    reaction.resultPromise->resolve(result_);
                } else {
                    reaction.resultPromise->reject(result_);
                }
            }
        });
    }
    reactions_.clear();
}

Promise* Promise::resolved(Value value) {
    Promise* p = new Promise();
    p->resolve(value);
    return p;
}

Promise* Promise::rejected(Value reason) {
    Promise* p = new Promise();
    p->reject(reason);
    return p;
}

// =============================================================================
// Microtask Queue Implementation
// =============================================================================

void MicrotaskQueue::enqueue(Microtask task) {
    tasks_.push(std::move(task));
}

void MicrotaskQueue::drain() {
    while (!tasks_.empty()) {
        Microtask task = std::move(tasks_.front());
        tasks_.pop();
        task();
    }
}

MicrotaskQueue& MicrotaskQueue::instance() {
    static MicrotaskQueue queue;
    return queue;
}

} // namespace Zepra::Runtime
