/**
 * @file indexeddb_tests.cpp
 * @brief Integration tests for IndexedDB API
 */

#include <gtest/gtest.h>
#include "browser/IndexedDBAPI.h"
#include "browser/StructuredClone.h"
#include "builtins/string.hpp"
#include "builtins/array.hpp"

using namespace Zepra::Browser;
using namespace Zepra::Runtime;

// =============================================================================
// IDBKeyRange Tests
// =============================================================================

TEST(IDBKeyRangeTests, OnlyRange) {
    auto* range = IDBKeyRange::only(Value::number(5));
    
    EXPECT_TRUE(range->includes(Value::number(5)));
    EXPECT_FALSE(range->includes(Value::number(4)));
    EXPECT_FALSE(range->includes(Value::number(6)));
}

TEST(IDBKeyRangeTests, BoundRange) {
    auto* range = IDBKeyRange::bound(Value::number(5), Value::number(10), false, false);
    
    EXPECT_TRUE(range->includes(Value::number(5)));
    EXPECT_TRUE(range->includes(Value::number(7)));
    EXPECT_TRUE(range->includes(Value::number(10)));
    EXPECT_FALSE(range->includes(Value::number(4)));
    EXPECT_FALSE(range->includes(Value::number(11)));
}

TEST(IDBKeyRangeTests, OpenBoundRange) {
    auto* range = IDBKeyRange::bound(Value::number(5), Value::number(10), true, true);
    
    EXPECT_FALSE(range->includes(Value::number(5)));
    EXPECT_TRUE(range->includes(Value::number(6)));
    EXPECT_TRUE(range->includes(Value::number(9)));
    EXPECT_FALSE(range->includes(Value::number(10)));
}

TEST(IDBKeyRangeTests, LowerBound) {
    auto* range = IDBKeyRange::lowerBound(Value::number(5), false);
    
    EXPECT_TRUE(range->includes(Value::number(5)));
    EXPECT_TRUE(range->includes(Value::number(100)));
    EXPECT_FALSE(range->includes(Value::number(4)));
}

TEST(IDBKeyRangeTests, UpperBound) {
    auto* range = IDBKeyRange::upperBound(Value::number(10), false);
    
    EXPECT_TRUE(range->includes(Value::number(10)));
    EXPECT_TRUE(range->includes(Value::number(1)));
    EXPECT_FALSE(range->includes(Value::number(11)));
}

// =============================================================================
// IDBFactory Tests
// =============================================================================

TEST(IDBFactoryTests, OpenNewDatabase) {
    auto* factory = IDBFactory::instance();
    
    auto* request = factory->open("testdb1", 1);
    EXPECT_NE(request, nullptr);
    EXPECT_EQ(request->readyState(), IDBRequest::ReadyState::Done);
    
    auto result = request->result();
    EXPECT_TRUE(result.isObject());
    
    auto* db = dynamic_cast<IDBDatabase*>(result.asObject());
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(db->name(), "testdb1");
    EXPECT_EQ(db->version(), 1);
    
    // Cleanup
    factory->deleteDatabase("testdb1");
}

TEST(IDBFactoryTests, DeleteDatabase) {
    auto* factory = IDBFactory::instance();
    
    // Create database
    factory->open("testdb2", 1);
    
    // Delete it
    auto* delRequest = factory->deleteDatabase("testdb2");
    EXPECT_NE(delRequest, nullptr);
}

// =============================================================================
// IDBDatabase Tests
// =============================================================================

TEST(IDBDatabaseTests, CreateObjectStore) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb3", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    
    auto* store = db->createObjectStore("users", "id", true);
    EXPECT_NE(store, nullptr);
    EXPECT_EQ(store->name(), "users");
    EXPECT_EQ(store->keyPath(), "id");
    EXPECT_TRUE(store->autoIncrement());
    
    auto names = db->objectStoreNames();
    EXPECT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], "users");
    
    factory->deleteDatabase("testdb3");
}

TEST(IDBDatabaseTests, DeleteObjectStore) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb4", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    
    db->createObjectStore("temp", "", false);
    EXPECT_EQ(db->objectStoreNames().size(), 1);
    
    db->deleteObjectStore("temp");
    EXPECT_EQ(db->objectStoreNames().size(), 0);
    
    factory->deleteDatabase("testdb4");
}

// =============================================================================
// IDBObjectStore Tests
// =============================================================================

TEST(IDBObjectStoreTests, AddAndGet) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb5", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    auto* store = db->createObjectStore("items", "", true);
    
    // Create transaction
    auto* tx = db->transaction({"items"}, IDBTransaction::Mode::ReadWrite);
    auto* txStore = tx->objectStore("items");
    
    // Add item
    Object* item = new Object();
    item->set("name", Value::string(new String("Test Item")));
    item->set("price", Value::number(9.99));
    
    auto* addReq = txStore->add(Value::object(item));
    EXPECT_NE(addReq, nullptr);
    EXPECT_EQ(addReq->readyState(), IDBRequest::ReadyState::Done);
    
    Value key = addReq->result();
    EXPECT_TRUE(key.isNumber());
    
    // Get item
    auto* getReq = txStore->get(key);
    EXPECT_NE(getReq, nullptr);
    EXPECT_TRUE(getReq->result().isObject());
    
    tx->commit();
    factory->deleteDatabase("testdb5");
}

TEST(IDBObjectStoreTests, PutReplaces) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb6", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    auto* store = db->createObjectStore("data", "", false);
    
    auto* tx = db->transaction({"data"}, IDBTransaction::Mode::ReadWrite);
    auto* txStore = tx->objectStore("data");
    
    // Put first value
    txStore->put(Value::string(new String("value1")), Value::number(1));
    
    // Put replacement
    txStore->put(Value::string(new String("value2")), Value::number(1));
    
    // Verify replacement
    auto* getReq = txStore->get(Value::number(1));
    EXPECT_EQ(getReq->result().asString()->value(), "value2");
    
    tx->commit();
    factory->deleteDatabase("testdb6");
}

TEST(IDBObjectStoreTests, Delete) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb7", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    auto* store = db->createObjectStore("data", "", false);
    
    auto* tx = db->transaction({"data"}, IDBTransaction::Mode::ReadWrite);
    auto* txStore = tx->objectStore("data");
    
    // Add item
    txStore->put(Value::string(new String("test")), Value::number(1));
    
    // Verify exists
    auto* getReq1 = txStore->get(Value::number(1));
    EXPECT_TRUE(getReq1->result().isString());
    
    // Delete
    txStore->deleteRecord(Value::number(1));
    
    // Verify gone
    auto* getReq2 = txStore->get(Value::number(1));
    EXPECT_TRUE(getReq2->result().isUndefined());
    
    tx->commit();
    factory->deleteDatabase("testdb7");
}

TEST(IDBObjectStoreTests, Clear) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb8", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    auto* store = db->createObjectStore("data", "", false);
    
    auto* tx = db->transaction({"data"}, IDBTransaction::Mode::ReadWrite);
    auto* txStore = tx->objectStore("data");
    
    // Add multiple items
    txStore->put(Value::number(1), Value::number(1));
    txStore->put(Value::number(2), Value::number(2));
    txStore->put(Value::number(3), Value::number(3));
    
    auto* countReq1 = txStore->count();
    EXPECT_EQ(countReq1->result().asNumber(), 3.0);
    
    // Clear
    txStore->clear();
    
    auto* countReq2 = txStore->count();
    EXPECT_EQ(countReq2->result().asNumber(), 0.0);
    
    tx->commit();
    factory->deleteDatabase("testdb8");
}

TEST(IDBObjectStoreTests, GetAll) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb9", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    auto* store = db->createObjectStore("data", "", false);
    
    auto* tx = db->transaction({"data"}, IDBTransaction::Mode::ReadWrite);
    auto* txStore = tx->objectStore("data");
    
    // Add items
    txStore->put(Value::string(new String("a")), Value::number(1));
    txStore->put(Value::string(new String("b")), Value::number(2));
    txStore->put(Value::string(new String("c")), Value::number(3));
    
    // Get all
    auto* getAllReq = txStore->getAll();
    EXPECT_TRUE(getAllReq->result().isObject());
    
    auto* arr = dynamic_cast<Array*>(getAllReq->result().asObject());
    EXPECT_NE(arr, nullptr);
    EXPECT_EQ(arr->length(), 3);
    
    tx->commit();
    factory->deleteDatabase("testdb9");
}

// =============================================================================
// IDBTransaction Tests
// =============================================================================

TEST(IDBTransactionTests, CommitCompletes) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb10", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    db->createObjectStore("data", "", false);
    
    auto* tx = db->transaction({"data"}, IDBTransaction::Mode::ReadWrite);
    
    EXPECT_TRUE(tx->isActive());
    EXPECT_FALSE(tx->isFinished());
    
    tx->commit();
    
    EXPECT_FALSE(tx->isActive());
    EXPECT_TRUE(tx->isFinished());
    
    factory->deleteDatabase("testdb10");
}

TEST(IDBTransactionTests, AbortFinishes) {
    auto* factory = IDBFactory::instance();
    auto* request = factory->open("testdb11", 1);
    auto* db = dynamic_cast<IDBDatabase*>(request->result().asObject());
    db->createObjectStore("data", "", false);
    
    auto* tx = db->transaction({"data"}, IDBTransaction::Mode::ReadWrite);
    
    tx->abort();
    
    EXPECT_FALSE(tx->isActive());
    EXPECT_TRUE(tx->isFinished());
    
    factory->deleteDatabase("testdb11");
}

// =============================================================================
// StructuredClone Tests
// =============================================================================

TEST(StructuredCloneTests, ClonePrimitives) {
    EXPECT_TRUE(StructuredClone::clone(Value::undefined()).isUndefined());
    EXPECT_TRUE(StructuredClone::clone(Value::null()).isNull());
    EXPECT_EQ(StructuredClone::clone(Value::number(42)).asNumber(), 42.0);
    EXPECT_EQ(StructuredClone::clone(Value::boolean(true)).asBoolean(), true);
    
    auto str = Value::string(new String("hello"));
    auto cloned = StructuredClone::clone(str);
    EXPECT_EQ(cloned.asString()->value(), "hello");
}

TEST(StructuredCloneTests, CloneObject) {
    Object* obj = new Object();
    obj->set("a", Value::number(1));
    obj->set("b", Value::string(new String("test")));
    
    auto cloned = StructuredClone::clone(Value::object(obj));
    EXPECT_TRUE(cloned.isObject());
    
    Object* clonedObj = cloned.asObject();
    EXPECT_NE(clonedObj, obj);
    EXPECT_EQ(clonedObj->get("a").asNumber(), 1.0);
    EXPECT_EQ(clonedObj->get("b").asString()->value(), "test");
}

TEST(StructuredCloneTests, CloneArray) {
    Array* arr = new Array();
    arr->push(Value::number(1));
    arr->push(Value::number(2));
    arr->push(Value::number(3));
    
    auto cloned = StructuredClone::clone(Value::object(arr));
    EXPECT_TRUE(cloned.isObject());
    
    Array* clonedArr = dynamic_cast<Array*>(cloned.asObject());
    EXPECT_NE(clonedArr, nullptr);
    EXPECT_EQ(clonedArr->length(), 3);
}

TEST(StructuredCloneTests, SerializeDeserialize) {
    Object* obj = new Object();
    obj->set("x", Value::number(100));
    obj->set("y", Value::string(new String("hello")));
    
    auto serialized = StructuredClone::serialize(Value::object(obj));
    EXPECT_FALSE(serialized.buffer.empty());
    
    auto deserialized = StructuredClone::deserialize(serialized);
    EXPECT_TRUE(deserialized.isObject());
    
    Object* result = deserialized.asObject();
    EXPECT_EQ(result->get("x").asNumber(), 100.0);
    EXPECT_EQ(result->get("y").asString()->value(), "hello");
}
