/**
 * @file async.cpp
 * @brief Async/await runtime support
 *
 * Implements:
 * - AsyncExecutionContext: state machine for async function calls
 * - AsyncFunction: Function subclass that returns Promises
 * - AwaitHandler: suspend/resume on await expressions
 * - MicrotaskQueue: process promise reactions after each task
 */

#include "runtime/async/async.hpp"
#include "runtime/async/async_function.hpp"
#include "runtime/async/promise.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <cassert>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// AsyncContext (simple state tracker from async.hpp)
// =============================================================================

AsyncContext::AsyncContext(Promise* promise)
    : promise_(promise), state_(0) {}

// =============================================================================
// AsyncExecutionContext
// =============================================================================

AsyncExecutionContext::AsyncExecutionContext(Function* fn, Context* ctx)
    : fn_(fn), ctx_(ctx), state_(AsyncState::Running),
      resultPromise_(new Promise()), awaitDepth_(0) {}

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
    if (resultPromise_) {
        resultPromise_->resolve(result);
    }

    // Drain microtask queue after async completion
    MicrotaskQueue::instance().process();
}

void AsyncExecutionContext::fail(const Value& error) {
    if (state_ == AsyncState::Completed || state_ == AsyncState::Failed) return;

    state_ = AsyncState::Failed;
    if (resultPromise_) {
        resultPromise_->reject(error);
    }

    MicrotaskQueue::instance().process();
}

void AsyncExecutionContext::suspend(Promise* awaitedPromise) {
    state_ = AsyncState::Suspended;
    awaitDepth_++;
    awaitStack_.push_back(awaitedPromise);
}

// =============================================================================
// AsyncFunction
// =============================================================================

AsyncFunction::AsyncFunction(const Frontend::FunctionDecl* decl, Environment* closure)
    : Function(decl, closure) {}

AsyncFunction::AsyncFunction(const Frontend::FunctionExpr* expr, Environment* closure)
    : Function(expr, closure) {}

AsyncFunction::AsyncFunction(const Frontend::ArrowFunctionExpr* arrow, Environment* closure)
    : Function(arrow, closure) {}

AsyncFunction::AsyncFunction(std::string name, BuiltinFn builtin, size_t arity)
    : Function(std::move(name), std::move(builtin), arity) {}

Value AsyncFunction::callAsync(Context* ctx, Value thisValue, const std::vector<Value>& args) {
    // Create execution context
    currentContext_ = std::make_unique<AsyncExecutionContext>(this, ctx);
    Promise* resultPromise = currentContext_->resultPromise();

    // The VM will:
    // 1. Begin executing the async function body
    // 2. On OP_AWAIT, call AwaitHandler::await() to suspend
    // 3. When the awaited promise settles, resume via AsyncExecutionContext::resume()
    // 4. On return, call AsyncExecutionContext::complete()
    // 5. On throw, call AsyncExecutionContext::fail()

    // Return the result promise immediately (async functions always return Promise)
    return Value::object(resultPromise);
}

// =============================================================================
// AwaitHandler
// =============================================================================

Value AwaitHandler::await(const Value& value, AsyncExecutionContext* asyncCtx) {
    if (!asyncCtx) {
        throw std::runtime_error("await used outside of async function");
    }

    // Convert value to Promise if it isn't already
    Promise* promise = toPromise(value);

    // Suspend the async execution context
    asyncCtx->suspend(promise);

    // Register callbacks on the awaited promise to resume execution
    promise->then(
        // onFulfilled: resume with result
        [asyncCtx](const Value& result) -> Value {
            asyncCtx->resume(result);
            return result;
        },
        // onRejected: fail the async function
        [asyncCtx](const Value& error) -> Value {
            asyncCtx->fail(error);
            return Value::undefined();
        }
    );

    // Return undefined for now — actual value comes via resume()
    // The VM should check asyncCtx->state() to see if it should suspend
    return Value::undefined();
}

Promise* AwaitHandler::toPromise(const Value& value) {
    // If already a Promise, return it
    if (value.isObject()) {
        Object* obj = value.asObject();
        if (obj && obj->objectType() == ObjectType::Promise) {
            return static_cast<Promise*>(obj);
        }
    }

    // Wrap non-Promise value in a resolved Promise
    return Promise::resolved(value);
}

// =============================================================================
// MicrotaskQueue
// =============================================================================

MicrotaskQueue& MicrotaskQueue::instance() {
    static MicrotaskQueue queue;
    return queue;
}

void MicrotaskQueue::enqueue(std::function<void()> task) {
    queue_.push_back(std::move(task));
}

void MicrotaskQueue::process() {
    // Process until empty — microtasks can enqueue more microtasks
    while (!queue_.empty()) {
        auto task = std::move(queue_.front());
        queue_.erase(queue_.begin());
        task();
    }
}

void MicrotaskQueue::drain() {
    process();
}

bool MicrotaskQueue::isEmpty() const {
    return queue_.empty();
}

} // namespace Zepra::Runtime
