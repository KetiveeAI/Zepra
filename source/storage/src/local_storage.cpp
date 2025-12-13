/**
 * @file local_storage.cpp
 * @brief LocalStorage implementation
 */

#include "storage/local_storage.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace Zepra::Storage {

std::string LocalStorage::storageDir_ = "";

// =============================================================================
// LocalStorage Implementation
// =============================================================================

LocalStorage::LocalStorage(const std::string& origin) : origin_(origin) {
    load();
}

LocalStorage::~LocalStorage() {
    save();
}

std::optional<std::string> LocalStorage::getItem(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void LocalStorage::setItem(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string oldValue;
    auto it = data_.find(key);
    if (it != data_.end()) {
        oldValue = it->second;
        currentSize_ -= it->first.size() + it->second.size();
    }
    
    size_t newEntrySize = key.size() + value.size();
    if (currentSize_ + newEntrySize > quota_) {
        throw std::runtime_error("QuotaExceededError: LocalStorage quota exceeded");
    }
    
    data_[key] = value;
    currentSize_ += newEntrySize;
    
    notifyChange(key, oldValue, value);
    save();
}

void LocalStorage::removeItem(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = data_.find(key);
    if (it != data_.end()) {
        std::string oldValue = it->second;
        currentSize_ -= it->first.size() + it->second.size();
        data_.erase(it);
        
        notifyChange(key, oldValue, "");
        save();
    }
}

void LocalStorage::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    data_.clear();
    currentSize_ = 0;
    
    notifyChange("", "", "");
    save();
}

std::optional<std::string> LocalStorage::key(size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (index >= data_.size()) {
        return std::nullopt;
    }
    
    auto it = data_.begin();
    std::advance(it, index);
    return it->first;
}

size_t LocalStorage::length() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

size_t LocalStorage::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentSize_;
}

bool LocalStorage::load() {
    std::string path = getStoragePath();
    
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
    currentSize_ = 0;
    
    std::string line;
    while (std::getline(file, line)) {
        size_t sep = line.find('\t');
        if (sep != std::string::npos) {
            std::string key = line.substr(0, sep);
            std::string value = line.substr(sep + 1);
            
            // Unescape newlines
            size_t pos = 0;
            while ((pos = value.find("\\n", pos)) != std::string::npos) {
                value.replace(pos, 2, "\n");
                pos++;
            }
            
            data_[key] = value;
            currentSize_ += key.size() + value.size();
        }
    }
    
    return true;
}

bool LocalStorage::save() {
    std::string path = getStoragePath();
    
    // Create directory if needed
    fs::path dir = fs::path(path).parent_path();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
    
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& [key, value] : data_) {
        // Escape newlines
        std::string escaped = value;
        size_t pos = 0;
        while ((pos = escaped.find('\n', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\\n");
            pos += 2;
        }
        
        file << key << '\t' << escaped << '\n';
    }
    
    return true;
}

void LocalStorage::setStorageDirectory(const std::string& path) {
    storageDir_ = path;
}

std::string LocalStorage::getStoragePath() const {
    std::string dir = storageDir_.empty() ? "/tmp/zepra_storage" : storageDir_;
    
    // Hash origin for filename
    std::hash<std::string> hasher;
    std::string filename = std::to_string(hasher(origin_)) + ".localStorage";
    
    return dir + "/" + filename;
}

void LocalStorage::notifyChange(const std::string& key, 
                                 const std::string& oldVal,
                                 const std::string& newVal) {
    if (onChange_) {
        StorageEvent event;
        event.key = key;
        event.oldValue = oldVal;
        event.newValue = newVal;
        event.url = origin_;
        event.storageArea = "localStorage";
        onChange_(event);
    }
}

// =============================================================================
// Global Functions
// =============================================================================

static std::mutex storagesMutex;
static std::unordered_map<std::string, std::unique_ptr<LocalStorage>> storages;

LocalStorage& getLocalStorage(const std::string& origin) {
    std::lock_guard<std::mutex> lock(storagesMutex);
    
    auto it = storages.find(origin);
    if (it != storages.end()) {
        return *it->second;
    }
    
    auto storage = std::make_unique<LocalStorage>(origin);
    LocalStorage& ref = *storage;
    storages[origin] = std::move(storage);
    return ref;
}

void clearAllLocalStorage() {
    std::lock_guard<std::mutex> lock(storagesMutex);
    
    for (auto& [origin, storage] : storages) {
        storage->clear();
    }
    storages.clear();
}

} // namespace Zepra::Storage
