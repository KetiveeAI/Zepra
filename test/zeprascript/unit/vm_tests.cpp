// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

/**
 * @file vm_tests.cpp
 * @brief Comprehensive VM opcode and execution tests
 */

#include <gtest/gtest.h>
#include "runtime/execution/vm.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/function.hpp"
#include "bytecode/opcode.hpp"
#include <vector>
#include <string>

using namespace Zepra::Runtime;
using Zepra::Bytecode::Opcode;

// =============================================================================
// Stack Operations
// =============================================================================

TEST(VMTests, Stack) {
    VM vm(nullptr);
    vm.push(Value::number(42));
    EXPECT_EQ(vm.stackSize(), 1);
    EXPECT_EQ(vm.peek().asNumber(), 42.0);
    Value v = vm.pop();
    EXPECT_EQ(v.asNumber(), 42.0);
    EXPECT_EQ(vm.stackSize(), 0);
}

TEST(VMTests, StackPushPopStress) {
    VM vm(nullptr);
    constexpr size_t N = 10000;
    for (size_t i = 0; i < N; ++i) {
        vm.push(Value::number(static_cast<double>(i)));
    }
    EXPECT_EQ(vm.stackSize(), N);
    for (size_t i = N; i > 0; --i) {
        Value v = vm.pop();
        EXPECT_EQ(v.asNumber(), static_cast<double>(i - 1));
    }
    EXPECT_EQ(vm.stackSize(), 0);
}

TEST(VMTests, StackMixedTypes) {
    VM vm(nullptr);
    vm.push(Value::number(3.14));
    vm.push(Value::boolean(true));
    vm.push(Value::null());
    vm.push(Value::undefined());
    vm.push(Value::string(new String("hello")));

    EXPECT_TRUE(vm.pop().isString());
    EXPECT_TRUE(vm.pop().isUndefined());
    EXPECT_TRUE(vm.pop().isNull());
    EXPECT_TRUE(vm.pop().asBoolean());
    EXPECT_EQ(vm.pop().asNumber(), 3.14);
}

// =============================================================================
// Value Type Tests
// =============================================================================

TEST(VMTests, ValueNanBoxingEdgeCases) {
    // NaN itself
    Value nan = Value::number(std::numeric_limits<double>::quiet_NaN());
    EXPECT_TRUE(nan.isNumber());
    EXPECT_TRUE(std::isnan(nan.asNumber()));

    // Infinity
    Value inf = Value::number(std::numeric_limits<double>::infinity());
    EXPECT_TRUE(inf.isNumber());
    EXPECT_TRUE(std::isinf(inf.asNumber()));

    // Negative zero
    Value negZero = Value::number(-0.0);
    EXPECT_TRUE(negZero.isNumber());
    EXPECT_EQ(1.0 / negZero.asNumber(), -std::numeric_limits<double>::infinity());
}

TEST(VMTests, ValueFalsiness) {
    EXPECT_TRUE(Value::undefined().isFalsy());
    EXPECT_TRUE(Value::null().isFalsy());
    EXPECT_TRUE(Value::boolean(false).isFalsy());
    EXPECT_TRUE(Value::number(0).isFalsy());
    EXPECT_TRUE(Value::number(-0.0).isFalsy());
    EXPECT_TRUE(Value::number(std::numeric_limits<double>::quiet_NaN()).isFalsy());
    EXPECT_TRUE(Value::string(new String("")).isFalsy());

    EXPECT_FALSE(Value::boolean(true).isFalsy());
    EXPECT_FALSE(Value::number(1).isFalsy());
    EXPECT_FALSE(Value::number(-1).isFalsy());
    EXPECT_FALSE(Value::string(new String("x")).isFalsy());
    EXPECT_FALSE(Value::object(new Object()).isFalsy());
}

// =============================================================================
// Arithmetic Operations (direct stack manipulation)
// =============================================================================

TEST(VMTests, ArithmeticAdd) {
    VM vm(nullptr);
    vm.push(Value::number(10));
    vm.push(Value::number(32));
    Value b = vm.pop();
    Value a = vm.pop();
    vm.push(Value::number(a.asNumber() + b.asNumber()));
    EXPECT_EQ(vm.pop().asNumber(), 42.0);
}

TEST(VMTests, ArithmeticSubtract) {
    VM vm(nullptr);
    vm.push(Value::number(100));
    vm.push(Value::number(58));
    Value b = vm.pop();
    Value a = vm.pop();
    vm.push(Value::number(a.asNumber() - b.asNumber()));
    EXPECT_EQ(vm.pop().asNumber(), 42.0);
}

TEST(VMTests, ArithmeticMultiply) {
    VM vm(nullptr);
    vm.push(Value::number(6));
    vm.push(Value::number(7));
    Value b = vm.pop();
    Value a = vm.pop();
    vm.push(Value::number(a.asNumber() * b.asNumber()));
    EXPECT_EQ(vm.pop().asNumber(), 42.0);
}

TEST(VMTests, ArithmeticDivide) {
    VM vm(nullptr);
    vm.push(Value::number(84));
    vm.push(Value::number(2));
    Value b = vm.pop();
    Value a = vm.pop();
    vm.push(Value::number(a.asNumber() / b.asNumber()));
    EXPECT_EQ(vm.pop().asNumber(), 42.0);
}

TEST(VMTests, ArithmeticModulo) {
    VM vm(nullptr);
    vm.push(Value::number(47));
    vm.push(Value::number(5));
    Value b = vm.pop();
    Value a = vm.pop();
    vm.push(Value::number(std::fmod(a.asNumber(), b.asNumber())));
    EXPECT_EQ(vm.pop().asNumber(), 2.0);
}

TEST(VMTests, ArithmeticPower) {
    VM vm(nullptr);
    vm.push(Value::number(2));
    vm.push(Value::number(10));
    Value b = vm.pop();
    Value a = vm.pop();
    vm.push(Value::number(std::pow(a.asNumber(), b.asNumber())));
    EXPECT_EQ(vm.pop().asNumber(), 1024.0);
}

// =============================================================================
// Bitwise Operations
// =============================================================================

TEST(VMTests, BitwiseAnd) {
    int32_t a = 0xFF00, b = 0x0FF0;
    Value result = Value::number(static_cast<double>(a & b));
    EXPECT_EQ(result.asNumber(), static_cast<double>(0x0F00));
}

TEST(VMTests, BitwiseOr) {
    int32_t a = 0xFF00, b = 0x00FF;
    Value result = Value::number(static_cast<double>(a | b));
    EXPECT_EQ(result.asNumber(), static_cast<double>(0xFFFF));
}

TEST(VMTests, BitwiseXor) {
    int32_t a = 0xAAAA, b = 0x5555;
    Value result = Value::number(static_cast<double>(a ^ b));
    EXPECT_EQ(result.asNumber(), static_cast<double>(0xFFFF));
}

TEST(VMTests, BitwiseShifts) {
    // Left shift
    int32_t val = 1;
    EXPECT_EQ(Value::number(static_cast<double>(val << 10)).asNumber(), 1024.0);

    // Right shift
    val = 1024;
    EXPECT_EQ(Value::number(static_cast<double>(val >> 5)).asNumber(), 32.0);

    // Unsigned right shift
    uint32_t uval = 0xFFFFFFFF;
    EXPECT_EQ(Value::number(static_cast<double>(uval >> 1)).asNumber(), 2147483647.0);
}

// =============================================================================
// Comparison Operations
// =============================================================================

TEST(VMTests, ComparisonEqual) {
    EXPECT_TRUE(Value::number(42).equals(Value::number(42)));
    EXPECT_FALSE(Value::number(42).equals(Value::number(43)));
    EXPECT_TRUE(Value::boolean(true).equals(Value::boolean(true)));
    EXPECT_TRUE(Value::null().equals(Value::null()));
    EXPECT_TRUE(Value::undefined().equals(Value::undefined()));
}

TEST(VMTests, ComparisonStrictEqual) {
    EXPECT_TRUE(Value::number(42).strictEquals(Value::number(42)));
    EXPECT_FALSE(Value::number(42).strictEquals(Value::string(new String("42"))));
    EXPECT_FALSE(Value::null().strictEquals(Value::undefined()));
}

TEST(VMTests, ComparisonLessGreater) {
    EXPECT_TRUE(Value::number(1).asNumber() < Value::number(2).asNumber());
    EXPECT_TRUE(Value::number(5).asNumber() > Value::number(3).asNumber());
    EXPECT_TRUE(Value::number(3).asNumber() <= Value::number(3).asNumber());
    EXPECT_TRUE(Value::number(3).asNumber() >= Value::number(3).asNumber());
}

// =============================================================================
// Object Operations
// =============================================================================

TEST(VMTests, ObjectPropertyAccess) {
    Object* obj = new Object();
    obj->set("x", Value::number(10));
    obj->set("y", Value::number(20));

    EXPECT_EQ(obj->get("x").asNumber(), 10.0);
    EXPECT_EQ(obj->get("y").asNumber(), 20.0);
    EXPECT_TRUE(obj->get("z").isUndefined());
    EXPECT_TRUE(obj->has("x"));
    EXPECT_FALSE(obj->has("z"));
    delete obj;
}

TEST(VMTests, ObjectPrototypeChain) {
    Object* proto = new Object();
    proto->set("inherited", Value::number(99));

    Object* obj = new Object();
    obj->setPrototype(proto);
    obj->set("own", Value::number(42));

    EXPECT_EQ(obj->get("own").asNumber(), 42.0);
    EXPECT_EQ(obj->get("inherited").asNumber(), 99.0);
    EXPECT_TRUE(obj->get("missing").isUndefined());

    delete obj;
    delete proto;
}

TEST(VMTests, ObjectDeleteProperty) {
    Object* obj = new Object();
    obj->set("key", Value::number(1));
    EXPECT_TRUE(obj->has("key"));
    obj->deleteProperty("key");
    EXPECT_FALSE(obj->has("key"));
    delete obj;
}

// =============================================================================
// Array Operations
// =============================================================================

TEST(VMTests, ArrayPushPop) {
    Array* arr = new Array();
    arr->push(Value::number(1));
    arr->push(Value::number(2));
    arr->push(Value::number(3));

    EXPECT_EQ(arr->length(), 3);
    EXPECT_EQ(arr->pop().asNumber(), 3.0);
    EXPECT_EQ(arr->length(), 2);
    EXPECT_EQ(arr->at(0).asNumber(), 1.0);
    EXPECT_EQ(arr->at(1).asNumber(), 2.0);
    delete arr;
}

TEST(VMTests, ArraySlice) {
    std::vector<Value> vals = {Value::number(1), Value::number(2), Value::number(3), Value::number(4), Value::number(5)};
    Array* arr = new Array(vals);
    Array* sliced = arr->slice(1, 4);

    EXPECT_EQ(sliced->length(), 3);
    EXPECT_EQ(sliced->at(0).asNumber(), 2.0);
    EXPECT_EQ(sliced->at(1).asNumber(), 3.0);
    EXPECT_EQ(sliced->at(2).asNumber(), 4.0);

    delete sliced;
    delete arr;
}

TEST(VMTests, ArrayStress) {
    Array* arr = new Array();
    constexpr size_t N = 10000;
    for (size_t i = 0; i < N; ++i) {
        arr->push(Value::number(static_cast<double>(i)));
    }
    EXPECT_EQ(arr->length(), N);

    for (size_t i = 0; i < N; ++i) {
        EXPECT_EQ(arr->at(i).asNumber(), static_cast<double>(i));
    }
    delete arr;
}

// =============================================================================
// String Operations
// =============================================================================

TEST(VMTests, StringConcat) {
    String* a = new String("hello");
    String* b = new String(" world");
    String* c = a->concat(b);
    EXPECT_EQ(c->value(), "hello world");
    delete c;
    delete b;
    delete a;
}

TEST(VMTests, StringMethods) {
    String* s = new String("Hello World");
    EXPECT_EQ(s->indexOf("World"), 6);
    EXPECT_EQ(s->indexOf("Missing"), -1);
    EXPECT_TRUE(s->includes("World"));
    EXPECT_TRUE(s->startsWith("Hello"));
    EXPECT_TRUE(s->endsWith("World"));

    String* lower = s->toLowerCase();
    EXPECT_EQ(lower->value(), "hello world");
    String* upper = s->toUpperCase();
    EXPECT_EQ(upper->value(), "HELLO WORLD");

    delete upper;
    delete lower;
    delete s;
}

// =============================================================================
// Error Handling
// =============================================================================

TEST(VMTests, ErrorTypes) {
    Error* te = Error::typeError("not a function");
    EXPECT_EQ(te->name(), "TypeError");
    EXPECT_EQ(te->message(), "not a function");

    Error* re = Error::rangeError("out of bounds");
    EXPECT_EQ(re->name(), "RangeError");

    Error* rfe = Error::referenceError("x is not defined");
    EXPECT_EQ(rfe->name(), "ReferenceError");

    Error* se = Error::syntaxError("unexpected token");
    EXPECT_EQ(se->name(), "SyntaxError");

    delete se;
    delete rfe;
    delete re;
    delete te;
}

TEST(VMTests, ErrorWithCause) {
    Error* cause = Error::typeError("inner");
    Error* outer = Error::withCause("Error", "outer", Value::object(cause));

    EXPECT_EQ(outer->message(), "outer");
    EXPECT_TRUE(outer->cause().isObject());

    delete outer;
    delete cause;
}
