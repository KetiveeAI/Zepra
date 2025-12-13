// Unit test stubs for ZepraScript
#include <gtest/gtest.h>
#include "zeprascript/runtime/value.hpp"

using namespace Zepra::Runtime;

TEST(ValueTests, Undefined) {
    Value v = Value::undefined();
    EXPECT_TRUE(v.isUndefined());
    EXPECT_FALSE(v.isNull());
    EXPECT_TRUE(v.isFalsy());
}

TEST(ValueTests, Null) {
    Value v = Value::null();
    EXPECT_TRUE(v.isNull());
    EXPECT_FALSE(v.isUndefined());
    EXPECT_TRUE(v.isFalsy());
}

TEST(ValueTests, Boolean) {
    Value t = Value::boolean(true);
    Value f = Value::boolean(false);
    
    EXPECT_TRUE(t.isBoolean());
    EXPECT_TRUE(t.asBoolean());
    EXPECT_TRUE(t.isTruthy());
    
    EXPECT_TRUE(f.isBoolean());
    EXPECT_FALSE(f.asBoolean());
    EXPECT_TRUE(f.isFalsy());
}

TEST(ValueTests, Number) {
    Value v = Value::number(42.0);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asNumber(), 42.0);
}

TEST(ValueTests, NumberArithmetic) {
    Value a = Value::number(10.0);
    Value b = Value::number(5.0);
    
    EXPECT_EQ(Value::add(a, b).asNumber(), 15.0);
    EXPECT_EQ(Value::subtract(a, b).asNumber(), 5.0);
    EXPECT_EQ(Value::multiply(a, b).asNumber(), 50.0);
    EXPECT_EQ(Value::divide(a, b).asNumber(), 2.0);
}

TEST(ValueTests, StrictEquals) {
    EXPECT_TRUE(Value::number(42).strictEquals(Value::number(42)));
    EXPECT_FALSE(Value::number(42).strictEquals(Value::number(43)));
    EXPECT_TRUE(Value::boolean(true).strictEquals(Value::boolean(true)));
    EXPECT_TRUE(Value::null().strictEquals(Value::null()));
    EXPECT_TRUE(Value::undefined().strictEquals(Value::undefined()));
    EXPECT_FALSE(Value::null().strictEquals(Value::undefined()));
}
