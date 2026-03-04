/**
 * @file IndexedDBAPI.h
 * @brief IndexedDB API for persistent client-side storage
 * 
 * Implements:
 * - IDBFactory: open, deleteDatabase, databases
 * - IDBDatabase: createObjectStore, deleteObjectStore, transaction
 * - IDBObjectStore: add, put, get, delete, clear, openCursor
 * - IDBTransaction: commit, abort, objectStore
 * - IDBCursor: continue, advance, update, delete
 * - IDBKeyRange: only, bound, lowerBound, upperBound
 * - IDBIndex: get, getKey, getAll, openCursor
 * 
 * Reference: https://www.w3.org/TR/IndexedDB-3/
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/async/promise.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;
using Runtime::Promise;

// Forward declarations
class IDBDatabase;
class IDBObjectStore;
class IDBTransaction;
class IDBCursor;
class IDBIndex;
class IDBRequest;
class IDBOpenDBRequest;
class IDBKeyRange;

// =============================================================================
// IDBKeyRange
// =============================================================================

/**
 * @brief Key range for querying object stores
 */
class IDBKeyRange : public Object {
public:
    IDBKeyRange();
    
    Value lower() const { return lower_; }
    Value upper() const { return upper_; }
    bool lowerOpen() const { return lowerOpen_; }
    bool upperOpen() const { return upperOpen_; }
    
    /**
     * @brief Check if a key is within this range
     */
    bool includes(const Value& key) const;
    
    // Static factory methods
    static IDBKeyRange* only(const Value& value);
    static IDBKeyRange* bound(const Value& lower, const Value& upper,
                              bool lowerOpen = false, bool upperOpen = false);
    static IDBKeyRange* lowerBound(const Value& lower, bool open = false);
    static IDBKeyRange* upperBound(const Value& upper, bool open = false);
    
private:
    Value lower_;
    Value upper_;
    bool lowerOpen_ = false;
    bool upperOpen_ = false;
    bool hasLower_ = false;
    bool hasUpper_ = false;
};

// =============================================================================
// IDBRequest
// =============================================================================

/**
 * @brief Represents an async request to IndexedDB
 */
class IDBRequest : public Object {
public:
    enum class ReadyState {
        Pending,
        Done
    };
    
    IDBRequest();
    
    Value result() const { return result_; }
    Value error() const { return error_; }
    ReadyState readyState() const { return readyState_; }
    IDBTransaction* transaction() const { return transaction_; }
    Object* source() const { return source_; }
    
    void setResult(const Value& result);
    void setError(const Value& error);
    void setTransaction(IDBTransaction* tx) { transaction_ = tx; }
    void setSource(Object* src) { source_ = src; }
    
    // Event handlers
    std::function<void(IDBRequest*)> onsuccess;
    std::function<void(IDBRequest*)> onerror;
    
private:
    Value result_;
    Value error_;
    ReadyState readyState_ = ReadyState::Pending;
    IDBTransaction* transaction_ = nullptr;
    Object* source_ = nullptr;
};

// =============================================================================
// IDBOpenDBRequest
// =============================================================================

/**
 * @brief Request for opening/creating a database
 */
class IDBOpenDBRequest : public IDBRequest {
public:
    IDBOpenDBRequest();
    
    // Additional event handlers
    std::function<void(IDBOpenDBRequest*, uint64_t oldVersion, uint64_t newVersion)> onupgradeneeded;
    std::function<void(IDBOpenDBRequest*)> onblocked;
};

// =============================================================================
// IDBCursor
// =============================================================================

/**
 * @brief Cursor for iterating over records
 */
class IDBCursor : public Object {
public:
    enum class Direction {
        Next,
        NextUnique,
        Prev,
        PrevUnique
    };
    
    IDBCursor(IDBObjectStore* store, IDBKeyRange* range, Direction dir);
    
    Value key() const { return key_; }
    Value primaryKey() const { return primaryKey_; }
    Value value() const { return value_; }
    Direction direction() const { return direction_; }
    Object* source() const;
    IDBRequest* request() const { return request_; }
    
    /**
     * @brief Advance cursor by count
     */
    void advance(uint32_t count);
    
    /**
     * @brief Continue to next record (optionally matching key)
     */
    void continueKey(const Value& key = Value::undefined());
    
    /**
     * @brief Continue to primary key
     */
    void continuePrimaryKey(const Value& key, const Value& primaryKey);
    
    /**
     * @brief Update current record
     */
    IDBRequest* update(const Value& value);
    
    /**
     * @brief Delete current record
     */
    IDBRequest* deleteRecord();
    
private:
    IDBObjectStore* store_;
    IDBKeyRange* range_;
    Direction direction_;
    Value key_;
    Value primaryKey_;
    Value value_;
    IDBRequest* request_ = nullptr;
    size_t position_ = 0;
    bool done_ = false;
};

// =============================================================================
// IDBIndex
// =============================================================================

/**
 * @brief Index on an object store
 */
class IDBIndex : public Object {
public:
    IDBIndex(IDBObjectStore* store, const std::string& name, 
             const std::string& keyPath, bool unique, bool multiEntry);
    
    const std::string& name() const { return name_; }
    IDBObjectStore* objectStore() const { return store_; }
    const std::string& keyPath() const { return keyPath_; }
    bool unique() const { return unique_; }
    bool multiEntry() const { return multiEntry_; }
    
    IDBRequest* get(const Value& key);
    IDBRequest* getKey(const Value& key);
    IDBRequest* getAll(const Value& query = Value::undefined(), uint32_t count = 0);
    IDBRequest* getAllKeys(const Value& query = Value::undefined(), uint32_t count = 0);
    IDBRequest* count(const Value& query = Value::undefined());
    IDBRequest* openCursor(const Value& query = Value::undefined(), 
                           IDBCursor::Direction dir = IDBCursor::Direction::Next);
    IDBRequest* openKeyCursor(const Value& query = Value::undefined(),
                              IDBCursor::Direction dir = IDBCursor::Direction::Next);
    
private:
    IDBObjectStore* store_;
    std::string name_;
    std::string keyPath_;
    bool unique_;
    bool multiEntry_;
};

// =============================================================================
// IDBObjectStore
// =============================================================================

/**
 * @brief Object store (like a table)
 */
class IDBObjectStore : public Object {
public:
    IDBObjectStore(IDBDatabase* db, const std::string& name,
                   const std::string& keyPath = "", bool autoIncrement = false);
    
    const std::string& name() const { return name_; }
    const std::string& keyPath() const { return keyPath_; }
    bool autoIncrement() const { return autoIncrement_; }
    IDBTransaction* transaction() const { return transaction_; }
    std::vector<std::string> indexNames() const;
    
    void setTransaction(IDBTransaction* tx) { transaction_ = tx; }
    
    // CRUD operations
    IDBRequest* add(const Value& value, const Value& key = Value::undefined());
    IDBRequest* put(const Value& value, const Value& key = Value::undefined());
    IDBRequest* get(const Value& key);
    IDBRequest* getKey(const Value& key);
    IDBRequest* getAll(const Value& query = Value::undefined(), uint32_t count = 0);
    IDBRequest* getAllKeys(const Value& query = Value::undefined(), uint32_t count = 0);
    IDBRequest* count(const Value& query = Value::undefined());
    IDBRequest* deleteRecord(const Value& key);
    IDBRequest* clear();
    
    // Cursor
    IDBRequest* openCursor(const Value& query = Value::undefined(),
                           IDBCursor::Direction dir = IDBCursor::Direction::Next);
    IDBRequest* openKeyCursor(const Value& query = Value::undefined(),
                              IDBCursor::Direction dir = IDBCursor::Direction::Next);
    
    // Index management
    IDBIndex* createIndex(const std::string& name, const std::string& keyPath,
                          bool unique = false, bool multiEntry = false);
    void deleteIndex(const std::string& name);
    IDBIndex* index(const std::string& name);
    
    // Internal storage
    void putInternal(const Value& key, const Value& value);
    Value getInternal(const Value& key) const;
    bool hasInternal(const Value& key) const;
    void deleteInternal(const Value& key);
    std::vector<std::pair<Value, Value>> getAllInternal() const;
    
private:
    Value extractKey(const Value& value, const Value& explicitKey);
    std::string keyToString(const Value& key) const;
    
    IDBDatabase* db_;
    std::string name_;
    std::string keyPath_;
    bool autoIncrement_;
    IDBTransaction* transaction_ = nullptr;
    uint64_t keyGenerator_ = 1;
    
    std::unordered_map<std::string, Value> records_;
    std::unordered_map<std::string, IDBIndex*> indexes_;
};

// =============================================================================
// IDBTransaction
// =============================================================================

/**
 * @brief Transaction for atomic operations
 */
class IDBTransaction : public Object {
public:
    enum class Mode {
        ReadOnly,
        ReadWrite,
        VersionChange
    };
    
    enum class Durability {
        Default,
        Strict,
        Relaxed
    };
    
    IDBTransaction(IDBDatabase* db, const std::vector<std::string>& storeNames,
                   Mode mode, Durability durability = Durability::Default);
    
    IDBDatabase* db() const { return db_; }
    Mode mode() const { return mode_; }
    Durability durability() const { return durability_; }
    Value error() const { return error_; }
    std::vector<std::string> objectStoreNames() const { return storeNames_; }
    
    IDBObjectStore* objectStore(const std::string& name);
    
    void commit();
    void abort();
    
    // Event handlers
    std::function<void(IDBTransaction*)> oncomplete;
    std::function<void(IDBTransaction*)> onerror;
    std::function<void(IDBTransaction*)> onabort;
    
    bool isActive() const { return active_; }
    bool isFinished() const { return finished_; }
    
private:
    IDBDatabase* db_;
    std::vector<std::string> storeNames_;
    Mode mode_;
    Durability durability_;
    Value error_;
    bool active_ = true;
    bool finished_ = false;
    std::unordered_map<std::string, IDBObjectStore*> stores_;
};

// =============================================================================
// IDBDatabase
// =============================================================================

/**
 * @brief Database connection
 */
class IDBDatabase : public Object {
public:
    IDBDatabase(const std::string& name, uint64_t version);
    
    const std::string& name() const { return name_; }
    uint64_t version() const { return version_; }
    std::vector<std::string> objectStoreNames() const;
    
    IDBObjectStore* createObjectStore(const std::string& name,
                                       const std::string& keyPath = "",
                                       bool autoIncrement = false);
    void deleteObjectStore(const std::string& name);
    
    IDBTransaction* transaction(const std::vector<std::string>& storeNames,
                                 IDBTransaction::Mode mode = IDBTransaction::Mode::ReadOnly,
                                 IDBTransaction::Durability durability = IDBTransaction::Durability::Default);
    
    void close();
    
    // Event handlers
    std::function<void(IDBDatabase*)> onclose;
    std::function<void(IDBDatabase*, uint64_t oldVersion, uint64_t newVersion)> onversionchange;
    std::function<void(IDBDatabase*)> onerror;
    std::function<void(IDBDatabase*)> onabort;
    
    // Internal
    IDBObjectStore* getObjectStore(const std::string& name);
    bool hasObjectStore(const std::string& name) const;
    
private:
    std::string name_;
    uint64_t version_;
    bool closed_ = false;
    std::unordered_map<std::string, IDBObjectStore*> stores_;
    mutable std::mutex mutex_;
};

// =============================================================================
// IDBFactory
// =============================================================================

/**
 * @brief Factory for opening databases (window.indexedDB)
 */
class IDBFactory : public Object {
public:
    IDBFactory();
    
    /**
     * @brief Open a database
     */
    IDBOpenDBRequest* open(const std::string& name, uint64_t version = 1);
    
    /**
     * @brief Delete a database
     */
    IDBOpenDBRequest* deleteDatabase(const std::string& name);
    
    /**
     * @brief List all databases
     */
    Promise* databases();
    
    /**
     * @brief Compare two keys
     */
    int cmp(const Value& first, const Value& second);
    
    // Singleton access
    static IDBFactory* instance();
    
private:
    static std::unordered_map<std::string, IDBDatabase*> databases_;
    static std::mutex mutex_;
};

// =============================================================================
// Builtin Functions
// =============================================================================

Value indexedDBBuiltin(void* ctx, const std::vector<Value>& args);
void initIndexedDB();

} // namespace Zepra::Browser
