/**
 * @file html_storage.hpp
 * @brief Web Storage API (localStorage, sessionStorage)
 */

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief Storage interface
 */
class Storage {
public:
    virtual ~Storage() = default;
    
    virtual size_t length() const = 0;
    virtual std::string key(size_t index) const = 0;
    virtual std::string getItem(const std::string& key) const = 0;
    virtual void setItem(const std::string& key, const std::string& value) = 0;
    virtual void removeItem(const std::string& key) = 0;
    virtual void clear() = 0;
};

/**
 * @brief In-memory storage implementation
 */
class MemoryStorage : public Storage {
public:
    MemoryStorage() = default;
    ~MemoryStorage() override = default;
    
    size_t length() const override { return data_.size(); }
    std::string key(size_t index) const override;
    std::string getItem(const std::string& key) const override;
    void setItem(const std::string& key, const std::string& value) override;
    void removeItem(const std::string& key) override;
    void clear() override { data_.clear(); keys_.clear(); }
    
private:
    std::unordered_map<std::string, std::string> data_;
    std::vector<std::string> keys_;
};

/**
 * @brief Storage event
 */
struct StorageEvent {
    std::string key;
    std::string oldValue;
    std::string newValue;
    std::string url;
    Storage* storageArea = nullptr;
};

/**
 * @brief Local storage (persistent)
 */
class LocalStorage : public MemoryStorage {
public:
    LocalStorage(const std::string& origin);
    ~LocalStorage() override;
    
    void setItem(const std::string& key, const std::string& value) override;
    void removeItem(const std::string& key) override;
    void clear() override;
    
    void load();
    void save();
    
private:
    std::string origin_;
    std::string storagePath_;
};

/**
 * @brief Session storage (per-session)
 */
class SessionStorage : public MemoryStorage {
public:
    SessionStorage() = default;
    ~SessionStorage() override = default;
};

} // namespace Zepra::WebCore
