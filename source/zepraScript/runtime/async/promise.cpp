#include "runtime/async/promise.hpp"
#include "runtime/objects/function.hpp"

namespace Zepra::Runtime {

// ============================================================================
// Promise
// ============================================================================

Promise::Promise()
    : state_(PromiseState::Pending), result_(Value::undefined()) {}

void Promise::resolve(const Value& value) {
    if (state_ != PromiseState::Pending) return;
    fulfill(value);
}

void Promise::reject(const Value& reason) {
    if (state_ != PromiseState::Pending) return;
    rejectInternal(reason);
}

void Promise::fulfill(const Value& value) {
    state_ = PromiseState::Fulfilled;
    result_ = value;
    processReactions();
}

void Promise::rejectInternal(const Value& reason) {
    state_ = PromiseState::Rejected;
    result_ = reason;
    processReactions();
}

void Promise::processReactions() {
    for (auto& reaction : reactions_) {
        MicrotaskQueue::instance().enqueue([this, reaction]() {
            try {
                if (state_ == PromiseState::Fulfilled && reaction.onFulfilled) {
                    Value result = reaction.onFulfilled(result_);
                    reaction.promise->resolve(result);
                } else if (state_ == PromiseState::Rejected && reaction.onRejected) {
                    Value result = reaction.onRejected(result_);
                    reaction.promise->resolve(result);
                } else if (state_ == PromiseState::Rejected) {
                    reaction.promise->reject(result_);
                } else {
                    reaction.promise->resolve(result_);
                }
            } catch (...) {
                reaction.promise->reject(Value::string(new String("Promise callback error")));
            }
        });
    }
    reactions_.clear();
}

Promise* Promise::then(PromiseCallback onFulfilled, PromiseCallback onRejected) {
    Promise* promise = new Promise();
    
    if (state_ == PromiseState::Pending) {
        reactions_.push_back({onFulfilled, onRejected, promise});
    } else {
        Reaction reaction{onFulfilled, onRejected, promise};
        MicrotaskQueue::instance().enqueue([this, reaction]() {
            if (state_ == PromiseState::Fulfilled && reaction.onFulfilled) {
                Value result = reaction.onFulfilled(result_);
                reaction.promise->resolve(result);
            } else if (state_ == PromiseState::Rejected && reaction.onRejected) {
                Value result = reaction.onRejected(result_);
                reaction.promise->resolve(result);
            }
        });
    }
    
    return promise;
}

Promise* Promise::catchError(PromiseCallback onRejected) {
    return then(nullptr, onRejected);
}

Promise* Promise::finally(PromiseCallback onFinally) {
    return then([onFinally](const Value& value) {
        if (onFinally) onFinally(Value::undefined());
        return value;
    }, [onFinally](const Value& reason) {
        if (onFinally) onFinally(Value::undefined());
        return reason;
    });
}

Promise* Promise::resolved(const Value& value) {
    Promise* promise = new Promise();
    promise->fulfill(value);
    return promise;
}

Promise* Promise::rejected(const Value& reason) {
    Promise* promise = new Promise();
    promise->rejectInternal(reason);
    return promise;
}

Promise* Promise::all(const std::vector<Promise*>& promises) {
    Promise* result = new Promise();
    
    if (promises.empty()) {
        // Empty array resolves immediately with empty array
        result->fulfill(Value::object(new Array({})));
        return result;
    }
    
    // Track completion state
    struct AllState {
        std::vector<Value> values;
        size_t remaining;
        bool rejected;
    };
    
    auto* state = new AllState();
    state->values.resize(promises.size());
    state->remaining = promises.size();
    state->rejected = false;
    
    for (size_t i = 0; i < promises.size(); i++) {
        Promise* p = promises[i];
        size_t index = i;
        
        p->then([result, state, index](const Value& value) -> Value {
            if (state->rejected) return Value::undefined();
            
            state->values[index] = value;
            state->remaining--;
            
            if (state->remaining == 0) {
                std::vector<Value> resultValues(state->values);
                delete state;
                result->fulfill(Value::object(new Array(std::move(resultValues))));
            }
            return Value::undefined();
        }, [result, state](const Value& reason) -> Value {
            if (!state->rejected) {
                state->rejected = true;
                result->reject(reason);
            }
            return Value::undefined();
        });
    }
    
    return result;
}

Promise* Promise::race(const std::vector<Promise*>& promises) {
    Promise* result = new Promise();
    
    if (promises.empty()) {
        // Empty array never settles (per spec)
        return result;
    }
    
    auto* settled = new bool(false);
    
    for (Promise* p : promises) {
        p->then([result, settled](const Value& value) -> Value {
            if (!*settled) {
                *settled = true;
                result->fulfill(value);
            }
            return Value::undefined();
        }, [result, settled](const Value& reason) -> Value {
            if (!*settled) {
                *settled = true;
                result->reject(reason);
            }
            return Value::undefined();
        });
    }
    
    return result;
}

// Promise.allSettled(promises) - ES2020
// Resolves when all promises settle (fulfilled or rejected)
Promise* Promise::allSettled(const std::vector<Promise*>& promises) {
    Promise* result = new Promise();
    
    if (promises.empty()) {
        result->fulfill(Value::object(new Array({})));
        return result;
    }
    
    struct AllSettledState {
        std::vector<Value> results;
        size_t remaining;
    };
    
    auto* state = new AllSettledState();
    state->results.resize(promises.size());
    state->remaining = promises.size();
    
    for (size_t i = 0; i < promises.size(); i++) {
        Promise* p = promises[i];
        size_t index = i;
        
        p->then([result, state, index](const Value& value) -> Value {
            // Create {status: "fulfilled", value: value}
            Object* obj = new Object();
            obj->set("status", Value::string(new String("fulfilled")));
            obj->set("value", value);
            state->results[index] = Value::object(obj);
            state->remaining--;
            
            if (state->remaining == 0) {
                std::vector<Value> resultValues(state->results);
                delete state;
                result->fulfill(Value::object(new Array(std::move(resultValues))));
            }
            return Value::undefined();
        }, [result, state, index](const Value& reason) -> Value {
            // Create {status: "rejected", reason: reason}
            Object* obj = new Object();
            obj->set("status", Value::string(new String("rejected")));
            obj->set("reason", reason);
            state->results[index] = Value::object(obj);
            state->remaining--;
            
            if (state->remaining == 0) {
                std::vector<Value> resultValues(state->results);
                delete state;
                result->fulfill(Value::object(new Array(std::move(resultValues))));
            }
            return Value::undefined();
        });
    }
    
    return result;
}

// Promise.any(promises) - ES2021
// Resolves with first fulfilled, rejects with AggregateError if all reject
Promise* Promise::any(const std::vector<Promise*>& promises) {
    Promise* result = new Promise();
    
    if (promises.empty()) {
        // Empty array rejects with AggregateError
        Object* error = new Object();
        error->set("name", Value::string(new String("AggregateError")));
        error->set("message", Value::string(new String("All promises were rejected")));
        error->set("errors", Value::object(new Array({})));
        result->reject(Value::object(error));
        return result;
    }
    
    struct AnyState {
        std::vector<Value> errors;
        size_t remaining;
        bool fulfilled;
    };
    
    auto* state = new AnyState();
    state->errors.resize(promises.size());
    state->remaining = promises.size();
    state->fulfilled = false;
    
    for (size_t i = 0; i < promises.size(); i++) {
        Promise* p = promises[i];
        size_t index = i;
        
        p->then([result, state](const Value& value) -> Value {
            if (!state->fulfilled) {
                state->fulfilled = true;
                result->fulfill(value);
            }
            return Value::undefined();
        }, [result, state, index](const Value& reason) -> Value {
            if (state->fulfilled) return Value::undefined();
            
            state->errors[index] = reason;
            state->remaining--;
            
            if (state->remaining == 0) {
                // All rejected - create AggregateError
                Object* error = new Object();
                error->set("name", Value::string(new String("AggregateError")));
                error->set("message", Value::string(new String("All promises were rejected")));
                std::vector<Value> errorsCopy(state->errors);
                error->set("errors", Value::object(new Array(std::move(errorsCopy))));
                delete state;
                result->reject(Value::object(error));
            }
            return Value::undefined();
        });
    }
    
    return result;
}

// Promise.withResolvers() - ES2024
// Returns {promise, resolve, reject} object
Object* Promise::withResolvers() {
    Promise* promise = new Promise();
    
    // Create resolve function
    Function* resolveFn = new Function("resolve", [promise](const FunctionCallInfo& info) -> Value {
        Value value = info.argumentCount() > 0 ? info.argument(0) : Value::undefined();
        promise->resolve(value);
        return Value::undefined();
    }, 1);
    
    // Create reject function  
    Function* rejectFn = new Function("reject", [promise](const FunctionCallInfo& info) -> Value {
        Value reason = info.argumentCount() > 0 ? info.argument(0) : Value::undefined();
        promise->reject(reason);
        return Value::undefined();
    }, 1);
    
    // Return {promise, resolve, reject}
    Object* result = new Object();
    result->set("promise", Value::object(promise));
    result->set("resolve", Value::object(resolveFn));
    result->set("reject", Value::object(rejectFn));
    
    return result;
}

// ============================================================================
// MicrotaskQueue
// ============================================================================

MicrotaskQueue& MicrotaskQueue::instance() {
    static MicrotaskQueue instance;
    return instance;
}

void MicrotaskQueue::enqueue(std::function<void()> task) {
    queue_.push_back(task);
}

void MicrotaskQueue::process() {
    while (!queue_.empty()) {
        auto task = queue_.front();
        queue_.erase(queue_.begin());
        task();
    }
}

bool MicrotaskQueue::isEmpty() const {
    return queue_.empty();
}

} // namespace Zepra::Runtime
