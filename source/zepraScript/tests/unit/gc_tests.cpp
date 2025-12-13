/**
 * @file gc_tests.cpp
 * @brief Unit tests for GC subsystem - Object and Array focused
 */

#include <gtest/gtest.h>
#include "zeprascript/runtime/object.hpp"

using namespace Zepra::Runtime;

// =============================================================================
// Object Tests
// =============================================================================

TEST(ObjectTests, Creation) {
    Object* obj = new Object();
    EXPECT_NE(obj, nullptr);
    EXPECT_EQ(obj->objectType(), ObjectType::Ordinary);
    delete obj;
}

TEST(ObjectTests, PropertySetAndGet) {
    Object* obj = new Object();
    
    obj->set("name", Value::number(42));
    Value value = obj->get("name");
    
    EXPECT_TRUE(value.isNumber());
    EXPECT_EQ(value.asNumber(), 42);
    
    delete obj;
}

TEST(ObjectTests, PropertyHas) {
    Object* obj = new Object();
    
    EXPECT_FALSE(obj->has("missing"));
    
    obj->set("exists", Value::boolean(true));
    EXPECT_TRUE(obj->has("exists"));
    
    delete obj;
}

TEST(ObjectTests, PropertyDelete) {
    Object* obj = new Object();
    
    obj->set("temp", Value::number(1));
    EXPECT_TRUE(obj->has("temp"));
    
    obj->deleteProperty("temp");
    EXPECT_FALSE(obj->has("temp"));
    
    delete obj;
}

TEST(ObjectTests, Marking) {
    Object* obj = new Object();
    
    EXPECT_FALSE(obj->isMarked());
    obj->markGC();
    EXPECT_TRUE(obj->isMarked());
    obj->clearMark();
    EXPECT_FALSE(obj->isMarked());
    
    delete obj;
}

TEST(ObjectTests, Prototype) {
    Object* proto = new Object();
    proto->set("inherited", Value::number(100));
    
    Object* obj = new Object();
    obj->setPrototype(proto);
    
    EXPECT_EQ(obj->prototype(), proto);
    
    delete obj;
    delete proto;
}

TEST(ObjectTests, Keys) {
    Object* obj = new Object();
    obj->set("a", Value::number(1));
    obj->set("b", Value::number(2));
    obj->set("c", Value::number(3));
    
    auto keys = obj->keys();
    EXPECT_EQ(keys.size(), 3);
    
    delete obj;
}

TEST(ObjectTests, Extensibility) {
    Object* obj = new Object();
    
    EXPECT_TRUE(obj->isExtensible());
    obj->preventExtensions();
    EXPECT_FALSE(obj->isExtensible());
    
    delete obj;
}

// =============================================================================
// Array Tests
// =============================================================================

TEST(ArrayTests, Creation) {
    Array* arr = new Array();
    EXPECT_EQ(arr->length(), 0);
    EXPECT_TRUE(arr->isArray());
    delete arr;
}

TEST(ArrayTests, CreationWithSize) {
    Array* arr = new Array(10);
    EXPECT_EQ(arr->length(), 10);
    delete arr;
}

TEST(ArrayTests, Push) {
    Array* arr = new Array();
    arr->push(Value::number(1));
    arr->push(Value::number(2));
    arr->push(Value::number(3));
    
    EXPECT_EQ(arr->length(), 3);
    delete arr;
}

TEST(ArrayTests, At) {
    Array* arr = new Array();
    arr->push(Value::number(10));
    arr->push(Value::number(20));
    
    Value first = arr->at(0);
    Value second = arr->at(1);
    
    EXPECT_EQ(first.asNumber(), 10);
    EXPECT_EQ(second.asNumber(), 20);
    
    delete arr;
}

TEST(ArrayTests, Pop) {
    Array* arr = new Array();
    arr->push(Value::number(1));
    arr->push(Value::number(2));
    
    Value popped = arr->pop();
    
    EXPECT_EQ(popped.asNumber(), 2);
    EXPECT_EQ(arr->length(), 1);
    
    delete arr;
}

TEST(ArrayTests, Shift) {
    Array* arr = new Array();
    arr->push(Value::number(1));
    arr->push(Value::number(2));
    arr->push(Value::number(3));
    
    Value shifted = arr->shift();
    
    EXPECT_EQ(shifted.asNumber(), 1);
    EXPECT_EQ(arr->length(), 2);
    
    delete arr;
}

TEST(ArrayTests, Unshift) {
    Array* arr = new Array();
    arr->push(Value::number(2));
    arr->push(Value::number(3));
    
    arr->unshift(Value::number(1));
    
    EXPECT_EQ(arr->length(), 3);
    EXPECT_EQ(arr->at(0).asNumber(), 1);
    
    delete arr;
}

TEST(ArrayTests, IndexOf) {
    Array* arr = new Array();
    arr->push(Value::number(10));
    arr->push(Value::number(20));
    arr->push(Value::number(30));
    
    int idx = arr->indexOf(Value::number(20));
    EXPECT_EQ(idx, 1);
    
    delete arr;
}

TEST(ArrayTests, Includes) {
    Array* arr = new Array();
    arr->push(Value::number(5));
    arr->push(Value::number(10));
    
    EXPECT_TRUE(arr->includes(Value::number(5)));
    EXPECT_FALSE(arr->includes(Value::number(99)));
    
    delete arr;
}

TEST(ArrayTests, SetLength) {
    Array* arr = new Array();
    arr->push(Value::number(1));
    arr->push(Value::number(2));
    arr->push(Value::number(3));
    
    arr->setLength(1);
    EXPECT_EQ(arr->length(), 1);
    
    delete arr;
}
