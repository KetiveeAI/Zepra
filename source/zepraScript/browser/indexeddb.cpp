/**
 * @file indexeddb.cpp
 * @brief IndexedDB API implementation
 * 
 * In-memory implementation for initial version.
 * Future: SQLite or LevelDB backend for persistence.
 */

#include "browser/IndexedDBAPI.h"
#include "browser/StructuredClone.h"
#include "runtime/objects/function.hpp"
#include "runtime/objects/object.hpp"
#include <algorithm>
#include <stdexcept>

namespace Zepra::Browser {

// Array wrapper for IndexedDB results
namespace {
class ArrayObject : public Runtime::Object {
public:
    ArrayObject() : Object(Runtime::ObjectType::Array) {}
    
    void push(const Value& val) {
        elements_.push_back(val);
    }
    
    size_t getLength() const {
        return elements_.size();
    }
    
    Value getAt(size_t index) const {
        if (index < elements_.size()) {
            return elements_[index];
        }
        return Value::undefined();
    }
};
}

// Static members
std::unordered_map<std::string, IDBDatabase*> IDBFactory::databases_;
std::mutex IDBFactory::mutex_;

// =============================================================================
// Key Comparison
// =============================================================================

static int compareKeys(const Value& a, const Value& b) {
    // Type ordering: undefined < null < number < string < date < binary < array
    if (a.isUndefined() && b.isUndefined()) return 0;
    if (a.isUndefined()) return -1;
    if (b.isUndefined()) return 1;
    
    if (a.isNull() && b.isNull()) return 0;
    if (a.isNull()) return -1;
    if (b.isNull()) return 1;
    
    if (a.isNumber() && b.isNumber()) {
        double da = a.asNumber(), db = b.asNumber();
        if (da < db) return -1;
        if (da > db) return 1;
        return 0;
    }
    if (a.isNumber()) return -1;
    if (b.isNumber()) return 1;
    
    if (a.isString() && b.isString()) {
        return a.asString()->value().compare(b.asString()->value());
    }
    if (a.isString()) return -1;
    if (b.isString()) return 1;
    
    // For objects, compare by string representation
    return 0;
}

// =============================================================================
// IDBKeyRange Implementation
// =============================================================================

IDBKeyRange::IDBKeyRange() 
    : Object(Runtime::ObjectType::Ordinary) {}

bool IDBKeyRange::includes(const Value& key) const {
    if (hasLower_) {
        int cmp = compareKeys(key, lower_);
        if (lowerOpen_ ? cmp <= 0 : cmp < 0) return false;
    }
    if (hasUpper_) {
        int cmp = compareKeys(key, upper_);
        if (upperOpen_ ? cmp >= 0 : cmp > 0) return false;
    }
    return true;
}

IDBKeyRange* IDBKeyRange::only(const Value& value) {
    IDBKeyRange* range = new IDBKeyRange();
    range->lower_ = value;
    range->upper_ = value;
    range->hasLower_ = true;
    range->hasUpper_ = true;
    range->lowerOpen_ = false;
    range->upperOpen_ = false;
    return range;
}

IDBKeyRange* IDBKeyRange::bound(const Value& lower, const Value& upper,
                                 bool lowerOpen, bool upperOpen) {
    IDBKeyRange* range = new IDBKeyRange();
    range->lower_ = lower;
    range->upper_ = upper;
    range->hasLower_ = true;
    range->hasUpper_ = true;
    range->lowerOpen_ = lowerOpen;
    range->upperOpen_ = upperOpen;
    return range;
}

IDBKeyRange* IDBKeyRange::lowerBound(const Value& lower, bool open) {
    IDBKeyRange* range = new IDBKeyRange();
    range->lower_ = lower;
    range->hasLower_ = true;
    range->hasUpper_ = false;
    range->lowerOpen_ = open;
    return range;
}

IDBKeyRange* IDBKeyRange::upperBound(const Value& upper, bool open) {
    IDBKeyRange* range = new IDBKeyRange();
    range->upper_ = upper;
    range->hasLower_ = false;
    range->hasUpper_ = true;
    range->upperOpen_ = open;
    return range;
}

// =============================================================================
// IDBRequest Implementation
// =============================================================================

IDBRequest::IDBRequest() 
    : Object(Runtime::ObjectType::Ordinary) {}

void IDBRequest::setResult(const Value& result) {
    result_ = result;
    readyState_ = ReadyState::Done;
    if (onsuccess) {
        onsuccess(this);
    }
}

void IDBRequest::setError(const Value& error) {
    error_ = error;
    readyState_ = ReadyState::Done;
    if (onerror) {
        onerror(this);
    }
}

// =============================================================================
// IDBOpenDBRequest Implementation
// =============================================================================

IDBOpenDBRequest::IDBOpenDBRequest() 
    : IDBRequest() {}

// =============================================================================
// IDBCursor Implementation
// =============================================================================

IDBCursor::IDBCursor(IDBObjectStore* store, IDBKeyRange* range, Direction dir)
    : Object(Runtime::ObjectType::Ordinary)
    , store_(store)
    , range_(range)
    , direction_(dir) {}

Object* IDBCursor::source() const {
    return store_;
}

void IDBCursor::advance(uint32_t count) {
    for (uint32_t i = 0; i < count && !done_; i++) {
        position_++;
    }
    // Update key/value from store at new position
}

void IDBCursor::continueKey(const Value& key) {
    if (key.isUndefined()) {
        position_++;
    } else {
        // Find position of key
        auto records = store_->getAllInternal();
        for (size_t i = position_ + 1; i < records.size(); i++) {
            if (compareKeys(records[i].first, key) >= 0) {
                position_ = i;
                key_ = records[i].first;
                value_ = records[i].second;
                return;
            }
        }
        done_ = true;
    }
}

void IDBCursor::continuePrimaryKey(const Value& key, const Value& primaryKey) {
    continueKey(key);
}

IDBRequest* IDBCursor::update(const Value& value) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(store_->transaction());
    
    store_->putInternal(key_, value);
    value_ = value;
    
    req->setResult(key_);
    return req;
}

IDBRequest* IDBCursor::deleteRecord() {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(store_->transaction());
    
    store_->deleteInternal(key_);
    
    req->setResult(Value::undefined());
    return req;
}

// =============================================================================
// IDBIndex Implementation
// =============================================================================

IDBIndex::IDBIndex(IDBObjectStore* store, const std::string& name,
                   const std::string& keyPath, bool unique, bool multiEntry)
    : Object(Runtime::ObjectType::Ordinary)
    , store_(store)
    , name_(name)
    , keyPath_(keyPath)
    , unique_(unique)
    , multiEntry_(multiEntry) {}

IDBRequest* IDBIndex::get(const Value& key) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    
    // Search through store records for matching index key
    auto records = store_->getAllInternal();
    for (const auto& [k, v] : records) {
        if (v.isObject()) {
            Value indexValue = v.asObject()->get(keyPath_);
            if (compareKeys(indexValue, key) == 0) {
                req->setResult(v);
                return req;
            }
        }
    }
    
    req->setResult(Value::undefined());
    return req;
}

IDBRequest* IDBIndex::getKey(const Value& key) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    
    auto records = store_->getAllInternal();
    for (const auto& [k, v] : records) {
        if (v.isObject()) {
            Value indexValue = v.asObject()->get(keyPath_);
            if (compareKeys(indexValue, key) == 0) {
                req->setResult(k);
                return req;
            }
        }
    }
    
    req->setResult(Value::undefined());
    return req;
}

IDBRequest* IDBIndex::getAll(const Value& query, uint32_t count) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    
    Runtime::Array* results = new Runtime::Array();
    auto records = store_->getAllInternal();
    uint32_t added = 0;
    
    for (const auto& [k, v] : records) {
        if (count > 0 && added >= count) break;
        
        bool matches = true;
        if (!query.isUndefined() && v.isObject()) {
            Value indexValue = v.asObject()->get(keyPath_);
            matches = compareKeys(indexValue, query) == 0;
        }
        
        if (matches) {
            results->push(v);
            added++;
        }
    }
    
    req->setResult(Value::object(results));
    return req;
}

IDBRequest* IDBIndex::getAllKeys(const Value& query, uint32_t count) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    
    Runtime::Array* results = new Runtime::Array();
    auto records = store_->getAllInternal();
    uint32_t added = 0;
    
    for (const auto& [k, v] : records) {
        if (count > 0 && added >= count) break;
        
        bool matches = true;
        if (!query.isUndefined() && v.isObject()) {
            Value indexValue = v.asObject()->get(keyPath_);
            matches = compareKeys(indexValue, query) == 0;
        }
        
        if (matches) {
            results->push(k);
            added++;
        }
    }
    
    req->setResult(Value::object(results));
    return req;
}

IDBRequest* IDBIndex::count(const Value& query) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    
    uint32_t cnt = 0;
    auto records = store_->getAllInternal();
    
    for (const auto& [k, v] : records) {
        bool matches = true;
        if (!query.isUndefined() && v.isObject()) {
            Value indexValue = v.asObject()->get(keyPath_);
            matches = compareKeys(indexValue, query) == 0;
        }
        if (matches) cnt++;
    }
    
    req->setResult(Value::number(static_cast<double>(cnt)));
    return req;
}

IDBRequest* IDBIndex::openCursor(const Value& query, IDBCursor::Direction dir) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    
    IDBKeyRange* range = nullptr;
    if (!query.isUndefined()) {
        if (auto* kr = dynamic_cast<IDBKeyRange*>(query.asObject())) {
            range = kr;
        } else {
            range = IDBKeyRange::only(query);
        }
    }
    
    IDBCursor* cursor = new IDBCursor(store_, range, dir);
    req->setResult(Value::object(cursor));
    return req;
}

IDBRequest* IDBIndex::openKeyCursor(const Value& query, IDBCursor::Direction dir) {
    return openCursor(query, dir);
}

// =============================================================================
// IDBObjectStore Implementation
// =============================================================================

IDBObjectStore::IDBObjectStore(IDBDatabase* db, const std::string& name,
                               const std::string& keyPath, bool autoIncrement)
    : Object(Runtime::ObjectType::Ordinary)
    , db_(db)
    , name_(name)
    , keyPath_(keyPath)
    , autoIncrement_(autoIncrement) {}

std::vector<std::string> IDBObjectStore::indexNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : indexes_) {
        names.push_back(name);
    }
    return names;
}

std::string IDBObjectStore::keyToString(const Value& key) const {
    if (key.isNumber()) {
        return std::to_string(static_cast<int64_t>(key.asNumber()));
    }
    if (key.isString()) {
        return key.asString()->value();
    }
    return std::to_string(reinterpret_cast<uintptr_t>(&key));
}

Value IDBObjectStore::extractKey(const Value& value, const Value& explicitKey) {
    if (!explicitKey.isUndefined()) {
        return explicitKey;
    }
    
    if (!keyPath_.empty() && value.isObject()) {
        return value.asObject()->get(keyPath_);
    }
    
    if (autoIncrement_) {
        return Value::number(static_cast<double>(keyGenerator_++));
    }
    
    throw std::runtime_error("No key provided and no key path/auto-increment");
}

IDBRequest* IDBObjectStore::add(const Value& value, const Value& key) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    Value actualKey = extractKey(value, key);
    std::string keyStr = keyToString(actualKey);
    
    if (records_.find(keyStr) != records_.end()) {
        req->setError(Value::string(new Runtime::String("Key already exists")));
        return req;
    }
    
    // Clone value for storage
    Value cloned = StructuredClone::clone(value);
    records_[keyStr] = cloned;
    
    req->setResult(actualKey);
    return req;
}

IDBRequest* IDBObjectStore::put(const Value& value, const Value& key) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    Value actualKey = extractKey(value, key);
    std::string keyStr = keyToString(actualKey);
    
    Value cloned = StructuredClone::clone(value);
    records_[keyStr] = cloned;
    
    req->setResult(actualKey);
    return req;
}

IDBRequest* IDBObjectStore::get(const Value& key) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    std::string keyStr = keyToString(key);
    auto it = records_.find(keyStr);
    
    if (it != records_.end()) {
        req->setResult(it->second);
    } else {
        req->setResult(Value::undefined());
    }
    
    return req;
}

IDBRequest* IDBObjectStore::getKey(const Value& key) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    std::string keyStr = keyToString(key);
    if (records_.find(keyStr) != records_.end()) {
        req->setResult(key);
    } else {
        req->setResult(Value::undefined());
    }
    
    return req;
}

IDBRequest* IDBObjectStore::getAll(const Value& query, uint32_t count) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    Runtime::Array* results = new Runtime::Array();
    uint32_t added = 0;
    
    for (const auto& [k, v] : records_) {
        if (count > 0 && added >= count) break;
        results->push(v);
        added++;
    }
    
    req->setResult(Value::object(results));
    return req;
}

IDBRequest* IDBObjectStore::getAllKeys(const Value& query, uint32_t count) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    Runtime::Array* results = new Runtime::Array();
    uint32_t added = 0;
    
    for (const auto& [k, v] : records_) {
        if (count > 0 && added >= count) break;
        // Parse key back to value (simplified)
        results->push(Value::string(new Runtime::String(k)));
        added++;
    }
    
    req->setResult(Value::object(results));
    return req;
}

IDBRequest* IDBObjectStore::count(const Value& query) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    req->setResult(Value::number(static_cast<double>(records_.size())));
    return req;
}

IDBRequest* IDBObjectStore::deleteRecord(const Value& key) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    std::string keyStr = keyToString(key);
    records_.erase(keyStr);
    
    req->setResult(Value::undefined());
    return req;
}

IDBRequest* IDBObjectStore::clear() {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    records_.clear();
    
    req->setResult(Value::undefined());
    return req;
}

IDBRequest* IDBObjectStore::openCursor(const Value& query, IDBCursor::Direction dir) {
    IDBRequest* req = new IDBRequest();
    req->setSource(this);
    req->setTransaction(transaction_);
    
    IDBKeyRange* range = nullptr;
    if (!query.isUndefined()) {
        if (auto* kr = dynamic_cast<IDBKeyRange*>(query.asObject())) {
            range = kr;
        } else {
            range = IDBKeyRange::only(query);
        }
    }
    
    IDBCursor* cursor = new IDBCursor(this, range, dir);
    
    // Initialize cursor with first record
    if (!records_.empty()) {
        auto it = records_.begin();
        // TODO: Apply range filter and direction
        req->setResult(Value::object(cursor));
    } else {
        req->setResult(Value::null());
    }
    
    return req;
}

IDBRequest* IDBObjectStore::openKeyCursor(const Value& query, IDBCursor::Direction dir) {
    return openCursor(query, dir);
}

IDBIndex* IDBObjectStore::createIndex(const std::string& name, 
                                       const std::string& keyPath,
                                       bool unique, bool multiEntry) {
    if (indexes_.find(name) != indexes_.end()) {
        throw std::runtime_error("Index already exists: " + name);
    }
    
    IDBIndex* index = new IDBIndex(this, name, keyPath, unique, multiEntry);
    indexes_[name] = index;
    return index;
}

void IDBObjectStore::deleteIndex(const std::string& name) {
    indexes_.erase(name);
}

IDBIndex* IDBObjectStore::index(const std::string& name) {
    auto it = indexes_.find(name);
    if (it == indexes_.end()) {
        throw std::runtime_error("Index not found: " + name);
    }
    return it->second;
}

void IDBObjectStore::putInternal(const Value& key, const Value& value) {
    std::string keyStr = keyToString(key);
    records_[keyStr] = value;
}

Value IDBObjectStore::getInternal(const Value& key) const {
    std::string keyStr = keyToString(key);
    auto it = records_.find(keyStr);
    return it != records_.end() ? it->second : Value::undefined();
}

bool IDBObjectStore::hasInternal(const Value& key) const {
    std::string keyStr = keyToString(key);
    return records_.find(keyStr) != records_.end();
}

void IDBObjectStore::deleteInternal(const Value& key) {
    std::string keyStr = keyToString(key);
    records_.erase(keyStr);
}

std::vector<std::pair<Value, Value>> IDBObjectStore::getAllInternal() const {
    std::vector<std::pair<Value, Value>> result;
    for (const auto& [k, v] : records_) {
        result.emplace_back(Value::string(new Runtime::String(k)), v);
    }
    return result;
}

// =============================================================================
// IDBTransaction Implementation
// =============================================================================

IDBTransaction::IDBTransaction(IDBDatabase* db, 
                               const std::vector<std::string>& storeNames,
                               Mode mode, Durability durability)
    : Object(Runtime::ObjectType::Ordinary)
    , db_(db)
    , storeNames_(storeNames)
    , mode_(mode)
    , durability_(durability) {}

IDBObjectStore* IDBTransaction::objectStore(const std::string& name) {
    if (!active_) {
        throw std::runtime_error("Transaction is not active");
    }
    
    // Check if store is in scope
    if (std::find(storeNames_.begin(), storeNames_.end(), name) == storeNames_.end()) {
        throw std::runtime_error("Object store not in transaction scope: " + name);
    }
    
    // Get or create store wrapper
    auto it = stores_.find(name);
    if (it != stores_.end()) {
        return it->second;
    }
    
    IDBObjectStore* store = db_->getObjectStore(name);
    if (!store) {
        throw std::runtime_error("Object store not found: " + name);
    }
    
    store->setTransaction(this);
    stores_[name] = store;
    return store;
}

void IDBTransaction::commit() {
    if (finished_) return;
    
    active_ = false;
    finished_ = true;
    
    if (oncomplete) {
        oncomplete(this);
    }
}

void IDBTransaction::abort() {
    if (finished_) return;
    
    active_ = false;
    finished_ = true;
    
    // TODO: Rollback changes
    
    if (onabort) {
        onabort(this);
    }
}

// =============================================================================
// IDBDatabase Implementation
// =============================================================================

IDBDatabase::IDBDatabase(const std::string& name, uint64_t version)
    : Object(Runtime::ObjectType::Ordinary)
    , name_(name)
    , version_(version) {}

std::vector<std::string> IDBDatabase::objectStoreNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : stores_) {
        names.push_back(name);
    }
    return names;
}

IDBObjectStore* IDBDatabase::createObjectStore(const std::string& name,
                                                const std::string& keyPath,
                                                bool autoIncrement) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (stores_.find(name) != stores_.end()) {
        throw std::runtime_error("Object store already exists: " + name);
    }
    
    IDBObjectStore* store = new IDBObjectStore(this, name, keyPath, autoIncrement);
    stores_[name] = store;
    return store;
}

void IDBDatabase::deleteObjectStore(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    stores_.erase(name);
}

IDBTransaction* IDBDatabase::transaction(const std::vector<std::string>& storeNames,
                                          IDBTransaction::Mode mode,
                                          IDBTransaction::Durability durability) {
    if (closed_) {
        throw std::runtime_error("Database is closed");
    }
    
    // Validate store names
    for (const auto& name : storeNames) {
        if (!hasObjectStore(name)) {
            throw std::runtime_error("Object store not found: " + name);
        }
    }
    
    return new IDBTransaction(this, storeNames, mode, durability);
}

void IDBDatabase::close() {
    closed_ = true;
    if (onclose) {
        onclose(this);
    }
}

IDBObjectStore* IDBDatabase::getObjectStore(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = stores_.find(name);
    return it != stores_.end() ? it->second : nullptr;
}

bool IDBDatabase::hasObjectStore(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stores_.find(name) != stores_.end();
}

// =============================================================================
// IDBFactory Implementation
// =============================================================================

IDBFactory::IDBFactory() 
    : Object(Runtime::ObjectType::Ordinary) {}

IDBFactory* IDBFactory::instance() {
    static IDBFactory factory;
    return &factory;
}

IDBOpenDBRequest* IDBFactory::open(const std::string& name, uint64_t version) {
    IDBOpenDBRequest* req = new IDBOpenDBRequest();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = databases_.find(name);
    if (it != databases_.end()) {
        IDBDatabase* db = it->second;
        
        if (version > db->version()) {
            // Trigger upgrade
            uint64_t oldVersion = db->version();
            
            // Create new database with higher version
            IDBDatabase* newDb = new IDBDatabase(name, version);
            
            // Copy stores
            for (const auto& storeName : db->objectStoreNames()) {
                IDBObjectStore* oldStore = db->getObjectStore(storeName);
                IDBObjectStore* newStore = newDb->createObjectStore(
                    storeName, oldStore->keyPath(), oldStore->autoIncrement());
                
                // Copy records
                auto records = oldStore->getAllInternal();
                for (const auto& [k, v] : records) {
                    newStore->putInternal(k, v);
                }
            }
            
            databases_[name] = newDb;
            
            if (req->onupgradeneeded) {
                req->onupgradeneeded(req, oldVersion, version);
            }
            
            req->setResult(Value::object(newDb));
        } else {
            req->setResult(Value::object(db));
        }
    } else {
        // Create new database
        IDBDatabase* db = new IDBDatabase(name, version);
        databases_[name] = db;
        
        if (req->onupgradeneeded) {
            req->onupgradeneeded(req, 0, version);
        }
        
        req->setResult(Value::object(db));
    }
    
    return req;
}

IDBOpenDBRequest* IDBFactory::deleteDatabase(const std::string& name) {
    IDBOpenDBRequest* req = new IDBOpenDBRequest();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = databases_.find(name);
    if (it != databases_.end()) {
        delete it->second;
        databases_.erase(it);
    }
    
    req->setResult(Value::undefined());
    return req;
}

Promise* IDBFactory::databases() {
    Promise* p = new Promise();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    Runtime::Array* result = new Runtime::Array();
    for (const auto& [name, db] : databases_) {
        Runtime::Object* info = new Runtime::Object();
        info->set("name", Value::string(new Runtime::String(name)));
        info->set("version", Value::number(static_cast<double>(db->version())));
        result->push(Value::object(info));
    }
    
    p->resolve(Value::object(result));
    return p;
}

int IDBFactory::cmp(const Value& first, const Value& second) {
    return compareKeys(first, second);
}

// =============================================================================
// Builtin Functions
// =============================================================================

Value indexedDBBuiltin(Runtime::Context*, const std::vector<Value>&) {
    return Value::object(IDBFactory::instance());
}

void initIndexedDB() {
    // Register indexedDB on global object
}

} // namespace Zepra::Browser
