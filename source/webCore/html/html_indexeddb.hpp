/**
 * @file html_indexeddb.hpp
 * @brief IndexedDB API (simplified interface)
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <any>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief IDB request ready state
 */
enum class IDBRequestReadyState {
    Pending,
    Done
};

/**
 * @brief IDB transaction mode
 */
enum class IDBTransactionMode {
    ReadOnly,
    ReadWrite,
    VersionChange
};

/**
 * @brief IDB cursor direction
 */
enum class IDBCursorDirection {
    Next,
    NextUnique,
    Prev,
    PrevUnique
};

/**
 * @brief IDB key range
 */
class IDBKeyRange {
public:
    static IDBKeyRange only(const std::any& value);
    static IDBKeyRange lowerBound(const std::any& lower, bool open = false);
    static IDBKeyRange upperBound(const std::any& upper, bool open = false);
    static IDBKeyRange bound(const std::any& lower, const std::any& upper,
                              bool lowerOpen = false, bool upperOpen = false);
    
    std::any lower() const { return lower_; }
    std::any upper() const { return upper_; }
    bool lowerOpen() const { return lowerOpen_; }
    bool upperOpen() const { return upperOpen_; }
    
    bool includes(const std::any& key) const;
    
private:
    std::any lower_;
    std::any upper_;
    bool lowerOpen_ = false;
    bool upperOpen_ = false;
};

/**
 * @brief IDB request
 */
class IDBRequest {
public:
    virtual ~IDBRequest() = default;
    
    std::any result() const { return result_; }
    std::string error() const { return error_; }
    IDBRequestReadyState readyState() const { return readyState_; }
    
    std::function<void()> onSuccess;
    std::function<void()> onError;
    
protected:
    std::any result_;
    std::string error_;
    IDBRequestReadyState readyState_ = IDBRequestReadyState::Pending;
};

/**
 * @brief IDB object store
 */
class IDBObjectStore {
public:
    std::string name() const { return name_; }
    std::string keyPath() const { return keyPath_; }
    bool autoIncrement() const { return autoIncrement_; }
    std::vector<std::string> indexNames() const { return indexNames_; }
    
    // CRUD operations
    std::unique_ptr<IDBRequest> add(const std::any& value, const std::any& key = {});
    std::unique_ptr<IDBRequest> put(const std::any& value, const std::any& key = {});
    std::unique_ptr<IDBRequest> get(const std::any& key);
    std::unique_ptr<IDBRequest> getAll(const IDBKeyRange& range = {}, int count = -1);
    std::unique_ptr<IDBRequest> delete_(const std::any& key);
    std::unique_ptr<IDBRequest> clear();
    std::unique_ptr<IDBRequest> count(const IDBKeyRange& range = {});
    
    // Index management
    class IDBIndex* createIndex(const std::string& name, const std::string& keyPath,
                                 bool unique = false, bool multiEntry = false);
    void deleteIndex(const std::string& name);
    class IDBIndex* index(const std::string& name);
    
private:
    std::string name_;
    std::string keyPath_;
    bool autoIncrement_ = false;
    std::vector<std::string> indexNames_;
};

/**
 * @brief IDB index
 */
class IDBIndex {
public:
    std::string name() const { return name_; }
    std::string keyPath() const { return keyPath_; }
    bool unique() const { return unique_; }
    bool multiEntry() const { return multiEntry_; }
    
    std::unique_ptr<IDBRequest> get(const std::any& key);
    std::unique_ptr<IDBRequest> getKey(const std::any& key);
    std::unique_ptr<IDBRequest> getAll(const IDBKeyRange& range = {}, int count = -1);
    std::unique_ptr<IDBRequest> count(const IDBKeyRange& range = {});
    
private:
    std::string name_;
    std::string keyPath_;
    bool unique_ = false;
    bool multiEntry_ = false;
};

/**
 * @brief IDB transaction
 */
class IDBTransaction {
public:
    std::vector<std::string> objectStoreNames() const { return storeNames_; }
    IDBTransactionMode mode() const { return mode_; }
    
    IDBObjectStore* objectStore(const std::string& name);
    void abort();
    void commit();
    
    std::function<void()> onComplete;
    std::function<void()> onError;
    std::function<void()> onAbort;
    
private:
    std::vector<std::string> storeNames_;
    IDBTransactionMode mode_;
};

/**
 * @brief IDB database
 */
class IDBDatabase {
public:
    std::string name() const { return name_; }
    unsigned long long version() const { return version_; }
    std::vector<std::string> objectStoreNames() const { return storeNames_; }
    
    IDBObjectStore* createObjectStore(const std::string& name,
                                       const std::string& keyPath = "",
                                       bool autoIncrement = false);
    void deleteObjectStore(const std::string& name);
    
    std::unique_ptr<IDBTransaction> transaction(
        const std::vector<std::string>& storeNames,
        IDBTransactionMode mode = IDBTransactionMode::ReadOnly);
    
    void close();
    
    std::function<void()> onClose;
    std::function<void()> onVersionChange;
    std::function<void()> onError;
    std::function<void()> onAbort;
    
private:
    std::string name_;
    unsigned long long version_ = 1;
    std::vector<std::string> storeNames_;
};

/**
 * @brief IDB factory
 */
class IDBFactory {
public:
    std::unique_ptr<IDBRequest> open(const std::string& name,
                                      unsigned long long version = 1);
    std::unique_ptr<IDBRequest> deleteDatabase(const std::string& name);
    
    int cmp(const std::any& first, const std::any& second);
    std::vector<std::string> databases();
};

} // namespace Zepra::WebCore
