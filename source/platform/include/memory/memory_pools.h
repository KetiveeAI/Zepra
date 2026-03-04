#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>

namespace Zepra::Memory {

/**
 * @brief Fixed-size memory pool for fast allocation
 * 
 * Pre-allocates blocks of memory to avoid frequent malloc/free.
 * Reduces fragmentation and improves cache locality.
 */
template<typename T, size_t BlockSize = 64>
class Pool {
public:
    Pool() {
        allocateBlock();
    }
    
    ~Pool() {
        for (Block* block : blocks_) {
            delete[] reinterpret_cast<uint8_t*>(block);
        }
    }
    
    // Allocate object from pool
    T* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Try to get from free list
        if (!free_list_.empty()) {
            T* ptr = free_list_.back();
            free_list_.pop_back();
            allocated_count_++;
            return ptr;
        }
        
        // Need new block
        if (current_offset_ >= BlockSize) {
            allocateBlock();
        }
        
        T* ptr = &current_block_->objects[current_offset_++];
        allocated_count_++;
        return ptr;
    }
    
    // Return object to pool
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Call destructor
        ptr->~T();
        
        // Add to free list
        free_list_.push_back(ptr);
        allocated_count_--;
    }
    
    // Get statistics
    size_t allocatedCount() const { return allocated_count_; }
    size_t blockCount() const { return blocks_.size(); }
    size_t totalCapacity() const { return blocks_.size() * BlockSize; }
    size_t freeCount() const { return free_list_.size(); }
    
    // Memory usage in bytes
    size_t memoryUsage() const {
        return blocks_.size() * sizeof(Block);
    }
    
private:
    struct Block {
        T objects[BlockSize];
    };
    
    void allocateBlock() {
        Block* block = reinterpret_cast<Block*>(new uint8_t[sizeof(Block)]);
        blocks_.push_back(block);
        current_block_ = block;
        current_offset_ = 0;
    }
    
    std::vector<Block*> blocks_;
    std::vector<T*> free_list_;
    Block* current_block_ = nullptr;
    size_t current_offset_ = 0;
    size_t allocated_count_ = 0;
    std::mutex mutex_;
};

/**
 * @brief String pool for small strings
 * 
 * Most HTML attribute names and CSS properties are < 32 chars.
 * Pool them for faster allocation.
 */
template<size_t MaxLength>
class SmallString {
public:
    SmallString() : length_(0) {
        data_[0] = '\0';
    }
    
    SmallString(const char* str) {
        set(str);
    }
    
    void set(const char* str) {
        length_ = 0;
        while (str[length_] && length_ < MaxLength - 1) {
            data_[length_] = str[length_];
            length_++;
        }
        data_[length_] = '\0';
    }
    
    const char* c_str() const { return data_; }
    size_t length() const { return length_; }
    
private:
    char data_[MaxLength];
    size_t length_;
};

/**
 * @brief Global memory pools instance
 */
class MemoryPools {
public:
    static MemoryPools& instance() {
        static MemoryPools pools;
        return pools;
    }
    
    // DOM node pool (will define DOMNode type later)
    // Pool for 64-byte blocks (typical DOM node)
    Pool<uint8_t, 1024> dom_node_pool;
    
    // Small string pool (tag names, attributes)
    Pool<SmallString<32>, 512> small_string_pool;
    
    // Medium string pool (text content)
    Pool<SmallString<128>, 256> medium_string_pool;
    
    // Generic 64-byte blocks
    Pool<uint8_t[64], 512> block_64_pool;
    
    // Generic 256-byte blocks
    Pool<uint8_t[256], 128> block_256_pool;
    
    // Get total memory usage
    size_t totalMemoryUsage() const {
        return dom_node_pool.memoryUsage() +
               small_string_pool.memoryUsage() +
               medium_string_pool.memoryUsage() +
               block_64_pool.memoryUsage() +
               block_256_pool.memoryUsage();
    }
    
    // Get statistics
    struct Stats {
        size_t total_allocated;
        size_t total_capacity;
        size_t total_free;
        size_t memory_usage;
    };
    
    Stats getStats() const {
        Stats stats;
        stats.total_allocated = 
            dom_node_pool.allocatedCount() +
            small_string_pool.allocatedCount() +
            medium_string_pool.allocatedCount() +
            block_64_pool.allocatedCount() +
            block_256_pool.allocatedCount();
            
        stats.total_capacity = 
            dom_node_pool.totalCapacity() +
            small_string_pool.totalCapacity() +
            medium_string_pool.totalCapacity() +
            block_64_pool.totalCapacity() +
            block_256_pool.totalCapacity();
            
        stats.total_free = 
            dom_node_pool.freeCount() +
            small_string_pool.freeCount() +
            medium_string_pool.freeCount() +
            block_64_pool.freeCount() +
            block_256_pool.freeCount();
            
        stats.memory_usage = totalMemoryUsage();
        return stats;
    }
    
private:
    MemoryPools() = default;
    ~MemoryPools() = default;
    MemoryPools(const MemoryPools&) = delete;
    MemoryPools& operator=(const MemoryPools&) = delete;
};

/**
 * @brief RAII wrapper for pool-allocated objects
 */
template<typename T>
class PoolPtr {
public:
    PoolPtr(T* ptr, Pool<T>& pool) : ptr_(ptr), pool_(&pool) {}
    
    ~PoolPtr() {
        if (ptr_) {
            pool_->deallocate(ptr_);
        }
    }
    
    // Move semantics
    PoolPtr(PoolPtr&& other) : ptr_(other.ptr_), pool_(other.pool_) {
        other.ptr_ = nullptr;
    }
    
    PoolPtr& operator=(PoolPtr&& other) {
        if (this != &other) {
            if (ptr_) pool_->deallocate(ptr_);
            ptr_ = other.ptr_;
            pool_ = other.pool_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    // No copy
    PoolPtr(const PoolPtr&) = delete;
    PoolPtr& operator=(const PoolPtr&) = delete;
    
    T* get() { return ptr_; }
    T* operator->() { return ptr_; }
    T& operator*() { return *ptr_; }
    
private:
    T* ptr_;
    Pool<T>* pool_;
};

} // namespace Zepra::Memory
