/**
 * @file zeprascript_comprehensive_tests.cpp
 * @brief Comprehensive Value Test Suite for ZebraScript
 * 
 * Following test262 patterns for testing Value operations.
 * Tests the runtime primitives and arithmetic operations.
 * 
 * References:
 * - test262: https://github.com/tc39/test262
 */

#include <gtest/gtest.h>
#include "runtime/objects/value.hpp"

#include <cmath>
#include <limits>

using namespace Zepra::Runtime;

// =============================================================================
// VALUE TYPE TESTS (ECMAScript 6.1)
// =============================================================================

class ValueTypeTests : public ::testing::Test {};

TEST_F(ValueTypeTests, UndefinedType) {
    Value v = Value::undefined();
    EXPECT_TRUE(v.isUndefined());
    EXPECT_FALSE(v.isNull());
    EXPECT_FALSE(v.isNumber());
    EXPECT_FALSE(v.isString());
    EXPECT_FALSE(v.isBoolean());
    EXPECT_FALSE(v.isObject());
    EXPECT_TRUE(v.isFalsy());
}

TEST_F(ValueTypeTests, NullType) {
    Value v = Value::null();
    EXPECT_TRUE(v.isNull());
    EXPECT_FALSE(v.isUndefined());
    EXPECT_TRUE(v.isFalsy());
}

TEST_F(ValueTypeTests, BooleanTrue) {
    Value v = Value::boolean(true);
    EXPECT_TRUE(v.isBoolean());
    EXPECT_TRUE(v.asBoolean());
    EXPECT_TRUE(v.isTruthy());
    EXPECT_FALSE(v.isFalsy());
}

TEST_F(ValueTypeTests, BooleanFalse) {
    Value v = Value::boolean(false);
    EXPECT_TRUE(v.isBoolean());
    EXPECT_FALSE(v.asBoolean());
    EXPECT_FALSE(v.isTruthy());
    EXPECT_TRUE(v.isFalsy());
}

TEST_F(ValueTypeTests, NumberInteger) {
    Value v = Value::number(42.0);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asNumber(), 42.0);
    EXPECT_TRUE(v.isTruthy());
}

TEST_F(ValueTypeTests, NumberZero) {
    Value v = Value::number(0.0);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asNumber(), 0.0);
    EXPECT_TRUE(v.isFalsy());  // 0 is falsy
}

TEST_F(ValueTypeTests, NumberNaN) {
    Value v = Value::number(std::nan(""));
    EXPECT_TRUE(v.isNumber());
    EXPECT_TRUE(std::isnan(v.asNumber()));
    EXPECT_TRUE(v.isFalsy());  // NaN is falsy
}

TEST_F(ValueTypeTests, NumberInfinity) {
    Value v = Value::number(std::numeric_limits<double>::infinity());
    EXPECT_TRUE(v.isNumber());
    EXPECT_TRUE(std::isinf(v.asNumber()));
    EXPECT_TRUE(v.isTruthy());  // Infinity is truthy
}

TEST_F(ValueTypeTests, NumberNegativeInfinity) {
    Value v = Value::number(-std::numeric_limits<double>::infinity());
    EXPECT_TRUE(v.isNumber());
    EXPECT_TRUE(std::isinf(v.asNumber()));
    EXPECT_LT(v.asNumber(), 0);
}

TEST_F(ValueTypeTests, NumberNegativeZero) {
    Value v = Value::number(-0.0);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asNumber(), 0.0);
}

// =============================================================================
// ARITHMETIC OPERATIONS (ECMAScript 13.15)
// =============================================================================

class ArithmeticTests : public ::testing::Test {};

TEST_F(ArithmeticTests, Addition) {
    Value a = Value::number(10.0);
    Value b = Value::number(5.0);
    Value result = Value::add(a, b);
    EXPECT_EQ(result.asNumber(), 15.0);
}

TEST_F(ArithmeticTests, Subtraction) {
    Value a = Value::number(10.0);
    Value b = Value::number(5.0);
    Value result = Value::subtract(a, b);
    EXPECT_EQ(result.asNumber(), 5.0);
}

TEST_F(ArithmeticTests, Multiplication) {
    Value a = Value::number(10.0);
    Value b = Value::number(5.0);
    Value result = Value::multiply(a, b);
    EXPECT_EQ(result.asNumber(), 50.0);
}

TEST_F(ArithmeticTests, Division) {
    Value a = Value::number(10.0);
    Value b = Value::number(5.0);
    Value result = Value::divide(a, b);
    EXPECT_EQ(result.asNumber(), 2.0);
}

TEST_F(ArithmeticTests, DivisionByZero) {
    Value a = Value::number(10.0);
    Value b = Value::number(0.0);
    Value result = Value::divide(a, b);
    EXPECT_TRUE(std::isinf(result.asNumber()));
}

TEST_F(ArithmeticTests, NegativeDivisionByZero) {
    Value a = Value::number(-10.0);
    Value b = Value::number(0.0);
    Value result = Value::divide(a, b);
    EXPECT_TRUE(std::isinf(result.asNumber()));
    EXPECT_LT(result.asNumber(), 0);
}

TEST_F(ArithmeticTests, ZeroByZero) {
    Value a = Value::number(0.0);
    Value b = Value::number(0.0);
    Value result = Value::divide(a, b);
    EXPECT_TRUE(std::isnan(result.asNumber()));
}

TEST_F(ArithmeticTests, FloatingPointPrecision) {
    Value a = Value::number(0.1);
    Value b = Value::number(0.2);
    Value result = Value::add(a, b);
    EXPECT_NEAR(result.asNumber(), 0.3, 0.0000001);
}

TEST_F(ArithmeticTests, LargeNumbers) {
    Value a = Value::number(1e100);
    Value b = Value::number(1e100);
    Value result = Value::add(a, b);
    EXPECT_EQ(result.asNumber(), 2e100);
}

TEST_F(ArithmeticTests, VerySmallNumbers) {
    Value a = Value::number(1e-300);
    Value b = Value::number(1e-300);
    Value result = Value::add(a, b);
    EXPECT_EQ(result.asNumber(), 2e-300);
}

TEST_F(ArithmeticTests, NegativeNumbers) {
    Value a = Value::number(-10.0);
    Value b = Value::number(-5.0);
    Value result = Value::add(a, b);
    EXPECT_EQ(result.asNumber(), -15.0);
}

TEST_F(ArithmeticTests, MixedSigns) {
    Value a = Value::number(10.0);
    Value b = Value::number(-3.0);
    Value result = Value::add(a, b);
    EXPECT_EQ(result.asNumber(), 7.0);
}

// =============================================================================
// COMPARISON & EQUALITY (ECMAScript 13.10-13.11)
// =============================================================================

class EqualityTests : public ::testing::Test {};

TEST_F(EqualityTests, StrictEqualsNumbers) {
    EXPECT_TRUE(Value::number(42).strictEquals(Value::number(42)));
    EXPECT_FALSE(Value::number(42).strictEquals(Value::number(43)));
}

TEST_F(EqualityTests, StrictEqualsBooleans) {
    EXPECT_TRUE(Value::boolean(true).strictEquals(Value::boolean(true)));
    EXPECT_TRUE(Value::boolean(false).strictEquals(Value::boolean(false)));
    EXPECT_FALSE(Value::boolean(true).strictEquals(Value::boolean(false)));
}

TEST_F(EqualityTests, StrictEqualsNull) {
    EXPECT_TRUE(Value::null().strictEquals(Value::null()));
    EXPECT_FALSE(Value::null().strictEquals(Value::undefined()));
}

TEST_F(EqualityTests, StrictEqualsUndefined) {
    EXPECT_TRUE(Value::undefined().strictEquals(Value::undefined()));
    EXPECT_FALSE(Value::undefined().strictEquals(Value::null()));
}

TEST_F(EqualityTests, StrictEqualsDifferentTypes) {
    EXPECT_FALSE(Value::number(42).strictEquals(Value::boolean(true)));
    EXPECT_FALSE(Value::number(0).strictEquals(Value::boolean(false)));
    EXPECT_FALSE(Value::number(0).strictEquals(Value::null()));
}

TEST_F(EqualityTests, NaNNotEqualToItself) {
    Value nan1 = Value::number(std::nan(""));
    Value nan2 = Value::number(std::nan(""));
    EXPECT_FALSE(nan1.strictEquals(nan2));
}

TEST_F(EqualityTests, NegativeZeroEqualsPositiveZero) {
    Value negZero = Value::number(-0.0);
    Value posZero = Value::number(0.0);
    EXPECT_TRUE(negZero.strictEquals(posZero));
}

TEST_F(EqualityTests, SmallDifference) {
    Value a = Value::number(1.0000000001);
    Value b = Value::number(1.0000000002);
    EXPECT_FALSE(a.strictEquals(b));
}

// =============================================================================
// TYPE COERCION (ECMAScript 7.1)
// =============================================================================

class TypeCoercionTests : public ::testing::Test {};

TEST_F(TypeCoercionTests, TruthyPositiveNumbers) {
    EXPECT_TRUE(Value::number(1).isTruthy());
    EXPECT_TRUE(Value::number(0.1).isTruthy());
    EXPECT_TRUE(Value::number(100).isTruthy());
}

TEST_F(TypeCoercionTests, TruthyNegativeNumbers) {
    EXPECT_TRUE(Value::number(-1).isTruthy());
    EXPECT_TRUE(Value::number(-0.1).isTruthy());
}

TEST_F(TypeCoercionTests, TruthyInfinity) {
    EXPECT_TRUE(Value::number(std::numeric_limits<double>::infinity()).isTruthy());
    EXPECT_TRUE(Value::number(-std::numeric_limits<double>::infinity()).isTruthy());
}

TEST_F(TypeCoercionTests, FalsyZero) {
    EXPECT_TRUE(Value::number(0).isFalsy());
    EXPECT_TRUE(Value::number(-0.0).isFalsy());
}

TEST_F(TypeCoercionTests, FalsyNaN) {
    EXPECT_TRUE(Value::number(std::nan("")).isFalsy());
}

TEST_F(TypeCoercionTests, FalsyBoolean) {
    EXPECT_TRUE(Value::boolean(false).isFalsy());
    EXPECT_FALSE(Value::boolean(true).isFalsy());
}

TEST_F(TypeCoercionTests, FalsyNullUndefined) {
    EXPECT_TRUE(Value::null().isFalsy());
    EXPECT_TRUE(Value::undefined().isFalsy());
}

// =============================================================================
// NUMBER EDGE CASES (ECMAScript Number Type)
// =============================================================================

class NumberEdgeCases : public ::testing::Test {};

TEST_F(NumberEdgeCases, MaxValue) {
    Value v = Value::number(std::numeric_limits<double>::max());
    EXPECT_TRUE(v.isNumber());
    EXPECT_FALSE(std::isinf(v.asNumber()));
}

TEST_F(NumberEdgeCases, MinValue) {
    Value v = Value::number(std::numeric_limits<double>::min());
    EXPECT_TRUE(v.isNumber());
    EXPECT_GT(v.asNumber(), 0);
}

TEST_F(NumberEdgeCases, Epsilon) {
    Value v = Value::number(std::numeric_limits<double>::epsilon());
    EXPECT_TRUE(v.isNumber());
    EXPECT_GT(v.asNumber(), 0);
}

TEST_F(NumberEdgeCases, SafeInteger) {
    double maxSafeInt = 9007199254740991.0;
    Value v = Value::number(maxSafeInt);
    EXPECT_EQ(v.asNumber(), maxSafeInt);
}

TEST_F(NumberEdgeCases, AdditionOverflow) {
    Value a = Value::number(std::numeric_limits<double>::max());
    Value b = Value::number(std::numeric_limits<double>::max());
    Value result = Value::add(a, b);
    EXPECT_TRUE(std::isinf(result.asNumber()));
}

TEST_F(NumberEdgeCases, MultiplicationOverflow) {
    Value a = Value::number(1e200);
    Value b = Value::number(1e200);
    Value result = Value::multiply(a, b);
    EXPECT_TRUE(std::isinf(result.asNumber()));
}

TEST_F(NumberEdgeCases, InfinityArithmetic) {
    Value inf = Value::number(std::numeric_limits<double>::infinity());
    Value num = Value::number(42);
    
    EXPECT_TRUE(std::isinf(Value::add(inf, num).asNumber()));
    EXPECT_TRUE(std::isinf(Value::multiply(inf, num).asNumber()));
    EXPECT_EQ(Value::divide(num, inf).asNumber(), 0.0);
}

TEST_F(NumberEdgeCases, NaNPropagation) {
    Value nan = Value::number(std::nan(""));
    Value num = Value::number(42);
    
    EXPECT_TRUE(std::isnan(Value::add(nan, num).asNumber()));
    EXPECT_TRUE(std::isnan(Value::subtract(nan, num).asNumber()));
    EXPECT_TRUE(std::isnan(Value::multiply(nan, num).asNumber()));
    EXPECT_TRUE(std::isnan(Value::divide(nan, num).asNumber()));
}

// =============================================================================
// MAIN
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
