/**
 * @file WasmMultiMemory.h
 * @brief WebAssembly Multi-Memory Proposal
 * 
 * Implements multiple independent memories:
 * - Multiple memory declarations
 * - Memory index in load/store opcodes
 */

#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace Zepra::Wasm {

// =============================================================================
// Memory Instance
// =============================================================================

/**
 * @brief Single memory instance
 */
class MemoryInstance {
public:
    MemoryInstance(uint32_t initial, uint32_t maximum, bool isShared = false, bool is64 = false)
        : pageCount_(initial)
        , maxPages_(maximum)
        , isShared_(isShared)
        , is64_(is64) {
        data_.resize(initial * PAGE_SIZE, 0);
    }
    
    static constexpr uint32_t PAGE_SIZE = 65536;  // 64KB
    
    uint8_t* data() { return data_.data(); }
    const uint8_t* data() const { return data_.data(); }
    size_t size() const { return data_.size(); }
    uint32_t pageCount() const { return pageCount_; }
    uint32_t maxPages() const { return maxPages_; }
    bool isShared() const { return isShared_; }
    bool is64() const { return is64_; }
    
    int32_t grow(uint32_t deltaPages) {
        uint32_t oldPages = pageCount_;
        uint32_t newPages = oldPages + deltaPages;
        
        if (newPages > maxPages_) return -1;
        
        try {
            data_.resize(newPages * PAGE_SIZE, 0);
            pageCount_ = newPages;
            return static_cast<int32_t>(oldPages);
        } catch (...) {
            return -1;
        }
    }
    
    // Bounds check
    bool inBounds(uint64_t offset, uint32_t size) const {
        return offset + size <= data_.size();
    }
    
    // Load operations
    template<typename T>
    T load(uint64_t offset) const {
        T value;
        std::memcpy(&value, data_.data() + offset, sizeof(T));
        return value;
    }
    
    // Store operations
    template<typename T>
    void store(uint64_t offset, T value) {
        std::memcpy(data_.data() + offset, &value, sizeof(T));
    }
    
private:
    std::vector<uint8_t> data_;
    uint32_t pageCount_;
    uint32_t maxPages_;
    bool isShared_;
    bool is64_;
};

// =============================================================================
// Multi-Memory Manager
// =============================================================================

/**
 * @brief Manages multiple memory instances
 */
class MultiMemoryManager {
public:
    // Add a new memory
    uint32_t addMemory(uint32_t initial, uint32_t maximum, bool isShared = false, bool is64 = false) {
        uint32_t idx = static_cast<uint32_t>(memories_.size());
        memories_.push_back(std::make_unique<MemoryInstance>(initial, maximum, isShared, is64));
        return idx;
    }
    
    // Get memory by index
    MemoryInstance* getMemory(uint32_t idx) {
        return idx < memories_.size() ? memories_[idx].get() : nullptr;
    }
    
    const MemoryInstance* getMemory(uint32_t idx) const {
        return idx < memories_.size() ? memories_[idx].get() : nullptr;
    }
    
    // Default memory (index 0)
    MemoryInstance* defaultMemory() { return getMemory(0); }
    const MemoryInstance* defaultMemory() const { return getMemory(0); }
    
    size_t memoryCount() const { return memories_.size(); }
    
    // Cross-memory copy
    void memoryCopy(uint32_t dstIdx, uint64_t dstOffset,
                   uint32_t srcIdx, uint64_t srcOffset,
                   uint64_t length) {
        auto* dst = getMemory(dstIdx);
        auto* src = getMemory(srcIdx);
        
        if (!dst || !src) return;
        if (!dst->inBounds(dstOffset, length)) return;
        if (!src->inBounds(srcOffset, length)) return;
        
        std::memmove(dst->data() + dstOffset, src->data() + srcOffset, length);
    }
    
    // Memory fill
    void memoryFill(uint32_t memIdx, uint64_t offset, uint8_t value, uint64_t length) {
        auto* mem = getMemory(memIdx);
        if (!mem || !mem->inBounds(offset, length)) return;
        std::memset(mem->data() + offset, value, length);
    }
    
private:
    std::vector<std::unique_ptr<MemoryInstance>> memories_;
};

// =============================================================================
// Multi-Memory Opcodes
// =============================================================================

namespace MultiMemoryOp {
    // Memory operations with index (0xFC prefix)
    constexpr uint8_t memory_init = 0x08;   // memory.init memidx dataidx
    constexpr uint8_t data_drop = 0x09;     // data.drop dataidx
    constexpr uint8_t memory_copy = 0x0A;   // memory.copy dstmem srcmem
    constexpr uint8_t memory_fill = 0x0B;   // memory.fill memidx
    
    // Load/store with memory index (extended encoding)
    // Regular load/store: memarg encodes memidx in upper bits
}

} // namespace Zepra::Wasm
