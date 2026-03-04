/**
 * @file async_function.cpp
 * @brief Async function implementation
 */

#include "runtime/async/async_function.hpp"
#include "runtime/async/promise.hpp"
#include "runtime/objects/function.hpp"

namespace Zepra::Runtime {

// ============================================================================
// AsyncExecutionContext
// ============================================================================

AsyncExecutionContext::AsyncExecutionContext(Function* fn, Context* ctx)
    : fn_(fn)
    , ctx_(ctx)
    , state_(AsyncState::Running)
    , resultPromise_(new Promise())
{}

void AsyncExecutionContext::resume(const Value& awaitResult) {
    if (state_ != AsyncState::Suspended) return;
    
    state_ = AsyncState::Running;
    awaitDepth_--;
    
    if (!awaitStack_.empty()) {
        awaitStack_.pop_back();
    }
}

void AsyncExecutionContext::complete(const Value& result) {
    if (state_ == AsyncState::Completed || state_ == AsyncState::Failed) return;
    
    state_ = AsyncState::Completed;
    resultPromise_->resolve(result);
}

void AsyncExecutionContext::fail(const Value& error) {
    if (state_ == AsyncState::Completed || state_ == AsyncState::Failed) return;
    
    state_ = AsyncState::Failed;
    resultPromise_->reject(error);
}

void AsyncExecutionContext::suspend(Promise* awaitedPromise) {
    state_ = AsyncState::Suspended;
    awaitDepth_++;
    awaitStack_.push_back(awaitedPromise);
    
    // Set up continuation when awaited promise settles
    awaitedPromise->then(
        [this](const Value& value) -> Value {
            resume(value);
            return value;
        },
        [this](const Value& error) -> Value {
            fail(error);
            return error;
        }
    );
}

// ============================================================================
// AsyncFunction
// ============================================================================

AsyncFunction::AsyncFunction(const Frontend::FunctionDecl* decl, Environment* closure)
    : Function(decl, closure)
{
    // Mark as async (inherited from Function)
}

AsyncFunction::AsyncFunction(const Frontend::FunctionExpr* expr, Environment* closure)
    : Function(expr, closure)
{
}

AsyncFunction::AsyncFunction(const Frontend::ArrowFunctionExpr* arrow, Environment* closure)
    : Function(arrow, closure)
{
}

AsyncFunction::AsyncFunction(std::string name, BuiltinFn builtin, size_t arity)
    : Function(std::move(name), std::move(builtin), arity)
{
}

Value AsyncFunction::callAsync(Context* ctx, Value thisValue, const std::vector<Value>& args) {
    // Create execution context
    currentContext_ = std::make_unique<AsyncExecutionContext>(this, ctx);
    
    // Execute the function body
    try {
        // If this is a native/builtin async function, call it directly
        if (isBuiltin()) {
            FunctionCallInfo info(ctx, thisValue, args);
            Value result = builtinFunction()(info);
            
            // If result is a Promise, chain it to our result promise
            if (result.isObject()) {
                if (auto* promise = dynamic_cast<Promise*>(result.asObject())) {
                    promise->then(
                        [this](const Value& v) -> Value {
                            currentContext_->complete(v);
                            return v;
                        },
                        [this](const Value& e) -> Value {
                            currentContext_->fail(e);
                            return e;
                        }
                    );
                } else {
                    currentContext_->complete(result);
                }
            } else {
                currentContext_->complete(result);
            }
        } else if (isNative()) {
            Value result = nativeFunction()(ctx, args);
            currentContext_->complete(result);
        } else {
            // For JS async functions, the VM handles execution with await points
            // The result promise will be resolved when execution completes
            Value result = call(ctx, thisValue, args);
            
            // If still running (suspended on await), the promise stays pending
            if (currentContext_->state() == AsyncState::Running) {
                currentContext_->complete(result);
            }
        }
    } catch (...) {
        currentContext_->fail(Value::string(new String("Async function error")));
    }
    
    return Value::object(currentContext_->resultPromise());
}

// ============================================================================
// AwaitHandler
// ============================================================================

Value AwaitHandler::await(const Value& value, AsyncExecutionContext* asyncCtx) {
    if (!asyncCtx) {
        // await outside async function - return value immediately
        return value;
    }
    
    Promise* promise = toPromise(value);
    
    if (promise->state() == PromiseState::Fulfilled) {
        // Already resolved - return immediately
        return promise->result();
    }
    
    if (promise->state() == PromiseState::Rejected) {
        // Already rejected - propagate error
        asyncCtx->fail(promise->result());
        return Value::undefined();
    }
    
    // Pending - suspend execution
    asyncCtx->suspend(promise);
    
    // Return undefined for now - actual value comes via resume()
    return Value::undefined();
}

Promise* AwaitHandler::toPromise(const Value& value) {
    // If already a Promise, return it
    if (value.isObject()) {
        if (auto* promise = dynamic_cast<Promise*>(value.asObject())) {
            return promise;
        }
        
        // Check for thenable (has .then method)
        Object* obj = value.asObject();
        Value thenMethod = obj->get("then");
        if (thenMethod.isObject() && thenMethod.asObject()->isCallable()) {
            // Create a Promise that follows the thenable
            Promise* promise = new Promise();
            
            Function* thenFn = dynamic_cast<Function*>(thenMethod.asObject());
            if (thenFn) {
                // Create resolve/reject callbacks
                Function* resolveFn = new Function("resolve", 
                    [promise](const FunctionCallInfo& info) -> Value {
                        Value v = info.argumentCount() > 0 ? info.argument(0) : Value::undefined();
                        promise->resolve(v);
                        return Value::undefined();
                    }, 1);
                
                Function* rejectFn = new Function("reject",
                    [promise](const FunctionCallInfo& info) -> Value {
                        Value e = info.argumentCount() > 0 ? info.argument(0) : Value::undefined();
                        promise->reject(e);
                        return Value::undefined();
                    }, 1);
                
                // Call thenable.then(resolve, reject)
                std::vector<Value> callArgs = {
                    Value::object(resolveFn),
                    Value::object(rejectFn)
                };
                FunctionCallInfo callInfo(nullptr, value, callArgs);
                thenFn->builtinFunction()(callInfo);
            }
            
            return promise;
        }
    }
    
    // Not a Promise or thenable - wrap in resolved Promise
    return Promise::resolved(value);
}

} // namespace Zepra::Runtime
