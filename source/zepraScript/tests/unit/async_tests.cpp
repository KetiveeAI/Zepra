/**
 * @file async_tests.cpp
 * @brief Unit tests for async/await functionality
 */

#include <gtest/gtest.h>
#include "runtime/async/async_function.hpp"
#include "runtime/async/promise.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/objects/object.hpp"

using namespace Zepra::Runtime;

// =============================================================================
// Promise Tests
// =============================================================================

TEST(AsyncTests, PromiseCreation) {
    Promise* p = new Promise();
    EXPECT_EQ(p->state(), PromiseState::Pending);
    EXPECT_TRUE(p->result().isUndefined());
    delete p;
}

TEST(AsyncTests, PromiseResolve) {
    Promise* p = new Promise();
    p->resolve(Value::number(42));
    
    EXPECT_EQ(p->state(), PromiseState::Fulfilled);
    EXPECT_TRUE(p->result().isNumber());
    EXPECT_EQ(p->result().asNumber(), 42.0);
    delete p;
}

TEST(AsyncTests, PromiseReject) {
    Promise* p = new Promise();
    p->reject(Value::number(-1));  // Use number for simpler test
    
    EXPECT_EQ(p->state(), PromiseState::Rejected);
    EXPECT_TRUE(p->result().isNumber());
    EXPECT_EQ(p->result().asNumber(), -1.0);
    delete p;
}

TEST(AsyncTests, PromiseResolved) {
    Promise* p = Promise::resolved(Value::number(100));
    
    EXPECT_EQ(p->state(), PromiseState::Fulfilled);
    EXPECT_EQ(p->result().asNumber(), 100.0);
    delete p;
}

TEST(AsyncTests, PromiseRejected) {
    Promise* p = Promise::rejected(Value::number(-1));
    
    EXPECT_EQ(p->state(), PromiseState::Rejected);
    EXPECT_EQ(p->result().asNumber(), -1.0);
    delete p;
}

TEST(AsyncTests, PromiseIgnoresSecondResolve) {
    Promise* p = new Promise();
    p->resolve(Value::number(1));
    p->resolve(Value::number(2));  // Should be ignored
    
    EXPECT_EQ(p->result().asNumber(), 1.0);
    delete p;
}

TEST(AsyncTests, PromiseIgnoresSecondReject) {
    Promise* p = new Promise();
    p->reject(Value::number(1));
    p->reject(Value::number(2));  // Should be ignored
    
    EXPECT_EQ(p->result().asNumber(), 1.0);
    delete p;
}

// =============================================================================
// Promise.all Tests
// =============================================================================

TEST(AsyncTests, PromiseAllEmpty) {
    std::vector<Promise*> empty;
    Promise* result = Promise::all(empty);
    
    EXPECT_EQ(result->state(), PromiseState::Fulfilled);
    EXPECT_TRUE(result->result().isObject());
    
    Array* arr = dynamic_cast<Array*>(result->result().asObject());
    EXPECT_NE(arr, nullptr);
    EXPECT_EQ(arr->length(), 0);
    delete result;
}

TEST(AsyncTests, PromiseAllResolved) {
    std::vector<Promise*> promises = {
        Promise::resolved(Value::number(1)),
        Promise::resolved(Value::number(2)),
        Promise::resolved(Value::number(3))
    };
    
    Promise* result = Promise::all(promises);
    MicrotaskQueue::instance().process();
    
    EXPECT_EQ(result->state(), PromiseState::Fulfilled);
    
    for (auto* p : promises) delete p;
    delete result;
}

TEST(AsyncTests, PromiseRaceEmpty) {
    std::vector<Promise*> empty;
    Promise* result = Promise::race(empty);
    
    // Empty race never settles
    EXPECT_EQ(result->state(), PromiseState::Pending);
    delete result;
}

// =============================================================================
// Promise.allSettled Tests
// =============================================================================

TEST(AsyncTests, PromiseAllSettledEmpty) {
    std::vector<Promise*> empty;
    Promise* result = Promise::allSettled(empty);
    
    EXPECT_EQ(result->state(), PromiseState::Fulfilled);
    delete result;
}

TEST(AsyncTests, PromiseAllSettledMixed) {
    std::vector<Promise*> promises = {
        Promise::resolved(Value::number(1)),
        Promise::rejected(Value::string(new String("error")))
    };
    
    Promise* result = Promise::allSettled(promises);
    MicrotaskQueue::instance().process();
    
    EXPECT_EQ(result->state(), PromiseState::Fulfilled);
    
    for (auto* p : promises) delete p;
    delete result;
}

// =============================================================================
// Promise.any Tests
// =============================================================================

TEST(AsyncTests, PromiseAnyEmpty) {
    std::vector<Promise*> empty;
    Promise* result = Promise::any(empty);
    
    // Empty any rejects with AggregateError
    EXPECT_EQ(result->state(), PromiseState::Rejected);
    delete result;
}

TEST(AsyncTests, PromiseAnyFirstFulfilled) {
    std::vector<Promise*> promises = {
        Promise::rejected(Value::number(-1)),
        Promise::resolved(Value::number(42)),
        Promise::rejected(Value::number(-2))
    };
    
    Promise* result = Promise::any(promises);
    MicrotaskQueue::instance().process();
    
    EXPECT_EQ(result->state(), PromiseState::Fulfilled);
    EXPECT_EQ(result->result().asNumber(), 42.0);
    
    for (auto* p : promises) delete p;
    delete result;
}

// =============================================================================
// Promise.withResolvers Tests (ES2024)
// =============================================================================

TEST(AsyncTests, PromiseWithResolvers) {
    Object* resolvers = Promise::withResolvers();
    
    EXPECT_TRUE(resolvers->get("promise").isObject());
    EXPECT_TRUE(resolvers->get("resolve").isObject());
    EXPECT_TRUE(resolvers->get("reject").isObject());
    
    Promise* promise = dynamic_cast<Promise*>(resolvers->get("promise").asObject());
    EXPECT_NE(promise, nullptr);
    EXPECT_EQ(promise->state(), PromiseState::Pending);
    
    delete resolvers;
}

// =============================================================================
// MicrotaskQueue Tests
// =============================================================================

TEST(AsyncTests, MicrotaskQueueEnqueue) {
    bool executed = false;
    
    MicrotaskQueue::instance().enqueue([&executed]() {
        executed = true;
    });
    
    EXPECT_FALSE(executed);
    MicrotaskQueue::instance().process();
    EXPECT_TRUE(executed);
}

TEST(AsyncTests, MicrotaskQueueOrder) {
    std::vector<int> order;
    
    MicrotaskQueue::instance().enqueue([&order]() { order.push_back(1); });
    MicrotaskQueue::instance().enqueue([&order]() { order.push_back(2); });
    MicrotaskQueue::instance().enqueue([&order]() { order.push_back(3); });
    
    MicrotaskQueue::instance().process();
    
    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// =============================================================================
// AsyncExecutionContext Tests
// =============================================================================

TEST(AsyncTests, AsyncExecutionContextCreation) {
    Function* fn = new Function("test", [](const FunctionCallInfo&) {
        return Value::number(42);
    }, 0);
    
    AsyncExecutionContext ctx(fn, nullptr);
    
    EXPECT_EQ(ctx.state(), AsyncState::Running);
    EXPECT_NE(ctx.resultPromise(), nullptr);
    EXPECT_EQ(ctx.resultPromise()->state(), PromiseState::Pending);
    
    delete fn;
}

TEST(AsyncTests, AsyncExecutionContextComplete) {
    Function* fn = new Function("test", [](const FunctionCallInfo&) {
        return Value::undefined();
    }, 0);
    
    AsyncExecutionContext ctx(fn, nullptr);
    ctx.complete(Value::number(99));
    
    EXPECT_EQ(ctx.state(), AsyncState::Completed);
    EXPECT_EQ(ctx.resultPromise()->state(), PromiseState::Fulfilled);
    EXPECT_EQ(ctx.resultPromise()->result().asNumber(), 99.0);
    
    delete fn;
}

TEST(AsyncTests, AsyncExecutionContextFail) {
    Function* fn = new Function("test", [](const FunctionCallInfo&) {
        return Value::undefined();
    }, 0);
    
    AsyncExecutionContext ctx(fn, nullptr);
    ctx.fail(Value::string(new String("error")));
    
    EXPECT_EQ(ctx.state(), AsyncState::Failed);
    EXPECT_EQ(ctx.resultPromise()->state(), PromiseState::Rejected);
    
    delete fn;
}

// =============================================================================
// AwaitHandler Tests
// =============================================================================

TEST(AsyncTests, AwaitHandlerToPromiseFromPromise) {
    Promise* original = Promise::resolved(Value::number(42));
    Promise* result = AwaitHandler::toPromise(Value::object(original));
    
    // Should return the same promise
    EXPECT_EQ(result, original);
    
    delete original;
}

TEST(AsyncTests, AwaitHandlerToPromiseFromValue) {
    Promise* result = AwaitHandler::toPromise(Value::number(123));
    
    // Should wrap in resolved promise
    EXPECT_EQ(result->state(), PromiseState::Fulfilled);
    EXPECT_EQ(result->result().asNumber(), 123.0);
    
    delete result;
}

TEST(AsyncTests, AwaitHandlerAwaitResolved) {
    Function* fn = new Function("test", [](const FunctionCallInfo&) {
        return Value::undefined();
    }, 0);
    
    AsyncExecutionContext ctx(fn, nullptr);
    
    // Await an already resolved promise
    Promise* p = Promise::resolved(Value::number(42));
    Value result = AwaitHandler::await(Value::object(p), &ctx);
    
    EXPECT_EQ(result.asNumber(), 42.0);
    
    delete fn;
    delete p;
}
