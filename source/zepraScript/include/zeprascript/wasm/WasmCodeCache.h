/**
 * @file WasmCodeCache.h
 * @brief Disk-based Code Cache for Fast Cold Starts
 * 
 * Implements:
 * - Disk-based code cache
 * - Hash-based module lookup
 * - Lazy deserialization
 * - Cache eviction policies
 */

#pragma once

#include "WasmAOT.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <chrono>

namespace Zepra::Wasm {

// =============================================================================
// Cache Entry
// =============================================================================

/**
 * @brief Metadata for a cached module
 */
struct CacheEntry {
    std::string path;                  // Path to cached file
    uint8_t moduleHash[32];            // Hash of original WASM module
    uint64_t timestamp;                // When cached
    uint64_t lastAccess;               // Last access time
    uint32_t size;                     // Size in bytes
    Platform platform;                 // Target platform
    
    // In-memory cached module (lazy loaded)
    std::shared_ptr<AOTModule> module;
};

// =============================================================================
// Eviction Policies
// =============================================================================

enum class EvictionPolicy {
    LRU,        // Least Recently Used
    LFU,        // Least Frequently Used
    FIFO,       // First In First Out
    Size        // Largest First
};

/**
 * @brief Cache eviction configuration
 */
struct CacheConfig {
    std::string cacheDir = ".zepra_cache/wasm";
    size_t maxCacheSize = 256 * 1024 * 1024;  // 256MB
    size_t maxEntries = 1000;
    EvictionPolicy evictionPolicy = EvictionPolicy::LRU;
    bool compressEntries = false;
    uint64_t maxAge = 7 * 24 * 60 * 60;  // 7 days in seconds
};

// =============================================================================
// Code Cache
// =============================================================================

/**
 * @brief Disk-based code cache manager
 */
class CodeCache {
public:
    explicit CodeCache(CacheConfig config = CacheConfig())
        : config_(std::move(config)) {
        initialize();
    }
    
    // Check if module is cached
    bool hasEntry(const uint8_t moduleHash[32]) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = hashToString(moduleHash);
        return entries_.count(key) > 0;
    }
    
    // Get cached module (loads from disk if needed)
    std::shared_ptr<AOTModule> get(const uint8_t moduleHash[32]) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = hashToString(moduleHash);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            return nullptr;
        }
        
        // Update access time
        it->second.lastAccess = currentTime();
        
        // Lazy load if not in memory
        if (!it->second.module) {
            it->second.module = loadFromDisk(it->second.path);
        }
        
        return it->second.module;
    }
    
    // Store compiled module in cache
    bool put(const uint8_t moduleHash[32], std::shared_ptr<AOTModule> module) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Evict if necessary
        evictIfNeeded(module->serialize().size());
        
        // Generate cache path
        std::string key = hashToString(moduleHash);
        std::string path = config_.cacheDir + "/" + key + ".waot";
        
        // Write to disk
        if (!saveToDisk(path, *module)) {
            return false;
        }
        
        // Create entry
        CacheEntry entry;
        entry.path = path;
        std::memcpy(entry.moduleHash, moduleHash, 32);
        entry.timestamp = currentTime();
        entry.lastAccess = entry.timestamp;
        entry.size = static_cast<uint32_t>(module->serialize().size());
        entry.platform = detectPlatform();
        entry.module = module;
        
        entries_[key] = std::move(entry);
        currentSize_ += entry.size;
        
        return true;
    }
    
    // Remove entry from cache
    void remove(const uint8_t moduleHash[32]) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = hashToString(moduleHash);
        auto it = entries_.find(key);
        if (it != entries_.end()) {
            // Delete file
            std::filesystem::remove(it->second.path);
            currentSize_ -= it->second.size;
            entries_.erase(it);
        }
    }
    
    // Clear entire cache
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& [key, entry] : entries_) {
            std::filesystem::remove(entry.path);
        }
        entries_.clear();
        currentSize_ = 0;
    }
    
    // Get cache statistics
    struct Stats {
        size_t entryCount;
        size_t totalSize;
        size_t maxSize;
        uint64_t hits;
        uint64_t misses;
    };
    
    Stats stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return {
            entries_.size(),
            currentSize_,
            config_.maxCacheSize,
            hits_,
            misses_
        };
    }
    
    // Prune old entries
    void pruneOld() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        uint64_t now = currentTime();
        std::vector<std::string> toRemove;
        
        for (const auto& [key, entry] : entries_) {
            if (now - entry.timestamp > config_.maxAge) {
                toRemove.push_back(key);
            }
        }
        
        for (const auto& key : toRemove) {
            auto it = entries_.find(key);
            if (it != entries_.end()) {
                std::filesystem::remove(it->second.path);
                currentSize_ -= it->second.size;
                entries_.erase(it);
            }
        }
    }
    
private:
    void initialize() {
        // Create cache directory
        std::filesystem::create_directories(config_.cacheDir);
        
        // Scan existing entries
        for (const auto& file : std::filesystem::directory_iterator(config_.cacheDir)) {
            if (file.path().extension() == ".waot") {
                loadEntryMetadata(file.path().string());
            }
        }
    }
    
    void loadEntryMetadata(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return;
        
        AOTHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        if (header.magic != AOTHeader::MAGIC ||
            header.platform != detectPlatform()) {
            // Invalid or wrong platform, remove
            std::filesystem::remove(path);
            return;
        }
        
        CacheEntry entry;
        entry.path = path;
        std::memcpy(entry.moduleHash, header.moduleHash, 32);
        entry.timestamp = header.timestamp;
        entry.lastAccess = currentTime();
        entry.size = static_cast<uint32_t>(std::filesystem::file_size(path));
        entry.platform = header.platform;
        
        std::string key = hashToString(entry.moduleHash);
        entries_[key] = std::move(entry);
        currentSize_ += entry.size;
    }
    
    std::shared_ptr<AOTModule> loadFromDisk(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return nullptr;
        
        // Read entire file
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> buffer(size);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        
        hits_++;
        return AOTModule::deserialize(buffer);
    }
    
    bool saveToDisk(const std::string& path, const AOTModule& module) {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        
        auto data = module.serialize();
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return true;
    }
    
    void evictIfNeeded(size_t newEntrySize) {
        while (currentSize_ + newEntrySize > config_.maxCacheSize ||
               entries_.size() >= config_.maxEntries) {
            if (entries_.empty()) break;
            
            std::string victim;
            
            switch (config_.evictionPolicy) {
                case EvictionPolicy::LRU:
                    victim = findLRU();
                    break;
                case EvictionPolicy::FIFO:
                    victim = findFIFO();
                    break;
                case EvictionPolicy::Size:
                    victim = findLargest();
                    break;
                default:
                    victim = findLRU();
            }
            
            if (!victim.empty()) {
                auto it = entries_.find(victim);
                if (it != entries_.end()) {
                    std::filesystem::remove(it->second.path);
                    currentSize_ -= it->second.size;
                    entries_.erase(it);
                }
            } else {
                break;
            }
        }
    }
    
    std::string findLRU() {
        std::string result;
        uint64_t oldest = UINT64_MAX;
        
        for (const auto& [key, entry] : entries_) {
            if (entry.lastAccess < oldest) {
                oldest = entry.lastAccess;
                result = key;
            }
        }
        return result;
    }
    
    std::string findFIFO() {
        std::string result;
        uint64_t oldest = UINT64_MAX;
        
        for (const auto& [key, entry] : entries_) {
            if (entry.timestamp < oldest) {
                oldest = entry.timestamp;
                result = key;
            }
        }
        return result;
    }
    
    std::string findLargest() {
        std::string result;
        uint32_t largest = 0;
        
        for (const auto& [key, entry] : entries_) {
            if (entry.size > largest) {
                largest = entry.size;
                result = key;
            }
        }
        return result;
    }
    
    static std::string hashToString(const uint8_t hash[32]) {
        static const char* hex = "0123456789abcdef";
        std::string result;
        result.reserve(64);
        for (int i = 0; i < 32; i++) {
            result += hex[(hash[i] >> 4) & 0xf];
            result += hex[hash[i] & 0xf];
        }
        return result;
    }
    
    static uint64_t currentTime() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    CacheConfig config_;
    std::unordered_map<std::string, CacheEntry> entries_;
    size_t currentSize_ = 0;
    mutable std::mutex mutex_;
    mutable uint64_t hits_ = 0;
    mutable uint64_t misses_ = 0;
};

// =============================================================================
// Cache Manager (Singleton)
// =============================================================================

/**
 * @brief Global cache manager
 */
class CacheManager {
public:
    static CacheManager& instance() {
        static CacheManager instance;
        return instance;
    }
    
    CodeCache& codeCache() { return codeCache_; }
    
    // Background compilation support
    void scheduleCompile(const uint8_t* wasmBytes, size_t size, 
                        std::function<void(std::shared_ptr<AOTModule>)> callback) {
        // Would spawn background thread to compile
        (void)wasmBytes; (void)size; (void)callback;
    }
    
private:
    CacheManager() = default;
    CodeCache codeCache_;
};

} // namespace Zepra::Wasm
