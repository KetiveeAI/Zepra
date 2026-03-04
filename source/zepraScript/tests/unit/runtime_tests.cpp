/**
 * @file runtime_tests.cpp
 * @brief Runtime API Tests for ZepraScript
 */

#include <gtest/gtest.h>

// Runtime APIs
#include "runtime/objects/MapSetAPI.h"
#include "runtime/builtins_api/TypedArrayAPI.h"
#include "runtime/builtins_api/BufferAPI.h"
#include "runtime/builtins_api/Base64API.h"
#include "runtime/objects/CoercionAPI.h"
#include "runtime/intl/TemporalAPI.h"

using namespace Zepra::Runtime;

// =============================================================================
// Map Tests
// =============================================================================

TEST(MapAPI, SetAndGet) {
    Map<std::string, int> map;
    map.set("one", 1);
    map.set("two", 2);
    
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(*map.get("one"), 1);
    EXPECT_EQ(*map.get("two"), 2);
}

TEST(MapAPI, Has) {
    Map<std::string, int> map;
    map.set("exists", 42);
    
    EXPECT_TRUE(map.has("exists"));
    EXPECT_FALSE(map.has("missing"));
}

TEST(MapAPI, Delete) {
    Map<std::string, int> map;
    map.set("key", 100);
    
    EXPECT_TRUE(map.has("key"));
    map.delete_("key");
    EXPECT_FALSE(map.has("key"));
}

// =============================================================================
// Set Tests
// =============================================================================

TEST(SetAPI, Add) {
    Set<int> set;
    set.add(1);
    set.add(2);
    set.add(1);  // Duplicate
    
    EXPECT_EQ(set.size(), 2);
    EXPECT_TRUE(set.has(1));
    EXPECT_TRUE(set.has(2));
}

TEST(SetAPI, Delete) {
    Set<int> set;
    set.add(10);
    set.add(20);
    
    EXPECT_TRUE(set.has(10));
    set.delete_(10);
    EXPECT_FALSE(set.has(10));
    EXPECT_EQ(set.size(), 1);
}

// =============================================================================
// TypedArray Tests
// =============================================================================

TEST(TypedArrayAPI, CreateAndFill) {
    Int32Array arr(5);
    arr.fill(42);
    
    EXPECT_EQ(arr.length(), 5);
    EXPECT_EQ(arr.at(0), 42);
    EXPECT_EQ(arr.at(4), 42);
}

TEST(TypedArrayAPI, Subarray) {
    Int32Array arr(5);
    for (size_t i = 0; i < 5; ++i) {
        arr.data()[i] = i * 10;
    }
    
    auto sub = arr.subarray(1, 4);
    
    EXPECT_EQ(sub.length(), 3);
    EXPECT_EQ(sub.at(0), 10);
    EXPECT_EQ(sub.at(1), 20);
    EXPECT_EQ(sub.at(2), 30);
}

// =============================================================================
// Base64 Tests
// =============================================================================

TEST(Base64API, Encode) {
    EXPECT_EQ(Base64::encode("Hello"), "SGVsbG8=");
    EXPECT_EQ(Base64::encode("World"), "V29ybGQ=");
    EXPECT_EQ(Base64::encode(""), "");
}

TEST(Base64API, Decode) {
    EXPECT_EQ(Base64::decodeToString("SGVsbG8="), "Hello");
    EXPECT_EQ(Base64::decodeToString("V29ybGQ="), "World");
}

TEST(Base64API, BtoaAtob) {
    std::string original = "Test String 123!";
    std::string encoded = btoa(original);
    std::string decoded = atob(encoded);
    
    EXPECT_EQ(original, decoded);
}

// =============================================================================
// Coercion Tests
// =============================================================================

TEST(CoercionAPI, ToBoolean) {
    EXPECT_FALSE(toBoolean(JSValue{}));  // undefined
    EXPECT_FALSE(toBoolean(JSValue{nullptr}));  // null
    EXPECT_FALSE(toBoolean(JSValue{false}));
    EXPECT_TRUE(toBoolean(JSValue{true}));
    EXPECT_FALSE(toBoolean(JSValue{0.0}));
    EXPECT_TRUE(toBoolean(JSValue{1.0}));
    EXPECT_FALSE(toBoolean(JSValue{std::string{}}));  // empty string
    EXPECT_TRUE(toBoolean(JSValue{std::string{"hello"}}));
}

TEST(CoercionAPI, ToNumber) {
    EXPECT_EQ(toNumber(JSValue{nullptr}), 0.0);
    EXPECT_EQ(toNumber(JSValue{true}), 1.0);
    EXPECT_EQ(toNumber(JSValue{false}), 0.0);
    EXPECT_EQ(toNumber(JSValue{42.5}), 42.5);
    EXPECT_EQ(toNumber(JSValue{std::string{"123"}}), 123.0);
}

TEST(CoercionAPI, ToString) {
    EXPECT_EQ(toString(JSValue{}), "undefined");
    EXPECT_EQ(toString(JSValue{nullptr}), "null");
    EXPECT_EQ(toString(JSValue{true}), "true");
    EXPECT_EQ(toString(JSValue{false}), "false");
}

// =============================================================================
// Temporal Tests
// =============================================================================

TEST(TemporalAPI, PlainDate) {
    PlainDate date(2024, 6, 15);
    
    EXPECT_EQ(date.year(), 2024);
    EXPECT_EQ(date.month(), 6);
    EXPECT_EQ(date.day(), 15);
    EXPECT_EQ(date.toString(), "2024-06-15");
}

TEST(TemporalAPI, PlainDateDayOfWeek) {
    PlainDate saturday(2024, 6, 15);  // June 15, 2024 is a Saturday
    EXPECT_EQ(saturday.dayOfWeek(), 6);  // Saturday = 6
}

TEST(TemporalAPI, Duration) {
    Duration d(1, 2, 0, 5);
    
    EXPECT_EQ(d.years(), 1);
    EXPECT_EQ(d.months(), 2);
    EXPECT_EQ(d.days(), 5);
}

TEST(TemporalAPI, DurationToString) {
    Duration d(1, 2, 0, 5, 3, 30, 0);
    std::string str = d.toString();
    
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("1Y"), std::string::npos);
}

// =============================================================================
// Buffer Tests
// =============================================================================

TEST(BufferAPI, ArrayBuffer) {
    ArrayBuffer buf(16);
    
    EXPECT_EQ(buf.byteLength(), 16);
    EXPECT_FALSE(buf.detached());
}

TEST(BufferAPI, ArrayBufferSlice) {
    ArrayBuffer buf(16);
    for (size_t i = 0; i < 16; ++i) buf.data()[i] = i;
    
    auto slice = buf.slice(4, 8);
    EXPECT_EQ(slice->byteLength(), 4);
    EXPECT_EQ(slice->data()[0], 4);
}

TEST(BufferAPI, DataView) {
    auto buf = std::make_shared<ArrayBuffer>(8);
    DataView view(buf);
    
    view.setInt32(0, 12345);
    EXPECT_EQ(view.getInt32(0), 12345);
    
    view.setFloat64(0, 3.14159);
    EXPECT_NEAR(view.getFloat64(0), 3.14159, 0.00001);
}
