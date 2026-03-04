/**
 * @file WasmBulkMemory.h
 * @brief WebAssembly Bulk Memory, Multi-Memory, and Reference Types
 * 
 * Implements:
 * - Bulk Memory Operations (memory.copy, memory.fill, table ops)
 * - Multi-Memory (multiple memory instances)
 * - Reference Types (externref, funcref)
 */

#pragma once

#include "wasm.hpp"
#include <vector>
#include <cstring>
#include <functional>

namespace Zepra::Wasm {

// =============================================================================
// Bulk Memory Opcodes (0xFC prefix)
// =============================================================================

namespace BulkOp {
    constexpr uint8_t MEMORY_INIT = 0x08;
    constexpr uint8_t DATA_DROP = 0x09;
    constexpr uint8_t MEMORY_COPY = 0x0A;
    constexpr uint8_t MEMORY_FILL = 0x0B;
    
    constexpr uint8_t TABLE_INIT = 0x0C;
    constexpr uint8_t ELEM_DROP = 0x0D;
    constexpr uint8_t TABLE_COPY = 0x0E;
    
    constexpr uint8_t TABLE_GROW = 0x0F;
    constexpr uint8_t TABLE_SIZE = 0x10;
    constexpr uint8_t TABLE_FILL = 0x11;
}

// =============================================================================
// Multi-Memory Support
// =============================================================================

/**
 * @brief A single linear memory instance
 */
class WasmMemory {
public:
    static constexpr size_t PAGE_SIZE = 65536;  // 64KB
    
    WasmMemory(uint32_t initialPages, uint32_t maxPages = UINT32_MAX, bool shared = false)
        : currentPages_(initialPages)
        , maxPages_(maxPages)
        , shared_(shared) {
        buffer_.resize(initialPages * PAGE_SIZE, 0);
    }
    
    uint8_t* data() { return buffer_.data(); }
    const uint8_t* data() const { return buffer_.data(); }
    size_t size() const { return buffer_.size(); }
    uint32_t pages() const { return currentPages_; }
    uint32_t maxPages() const { return maxPages_; }
    bool isShared() const { return shared_; }
    
    // memory.grow - returns old page count or -1 on failure
    int32_t grow(uint32_t delta) {
        uint32_t oldPages = currentPages_;
        uint64_t newPages = static_cast<uint64_t>(currentPages_) + delta;
        
        if (newPages > maxPages_ || newPages > 65536) {
            return -1;
        }
        
        buffer_.resize(newPages * PAGE_SIZE, 0);
        currentPages_ = static_cast<uint32_t>(newPages);
        return static_cast<int32_t>(oldPages);
    }
    
    // memory.fill
    void fill(uint32_t dest, uint8_t value, uint32_t count) {
        if (dest + count <= buffer_.size()) {
            std::memset(buffer_.data() + dest, value, count);
        }
    }
    
    // memory.copy (within same memory)
    void copy(uint32_t dest, uint32_t src, uint32_t count) {
        if (dest + count <= buffer_.size() && src + count <= buffer_.size()) {
            std::memmove(buffer_.data() + dest, buffer_.data() + src, count);
        }
    }
    
    // memory.init from data segment
    void init(uint32_t dest, const uint8_t* src, uint32_t srcOffset, uint32_t count) {
        if (dest + count <= buffer_.size()) {
            std::memcpy(buffer_.data() + dest, src + srcOffset, count);
        }
    }
    
private:
    std::vector<uint8_t> buffer_;
    uint32_t currentPages_;
    uint32_t maxPages_;
    bool shared_;
};

/**
 * @brief Multi-memory manager
 */
class MemoryManager {
public:
    uint32_t addMemory(uint32_t initialPages, uint32_t maxPages = UINT32_MAX, bool shared = false) {
        uint32_t index = static_cast<uint32_t>(memories_.size());
        memories_.emplace_back(initialPages, maxPages, shared);
        return index;
    }
    
    WasmMemory* getMemory(uint32_t index) {
        return index < memories_.size() ? &memories_[index] : nullptr;
    }
    
    const WasmMemory* getMemory(uint32_t index) const {
        return index < memories_.size() ? &memories_[index] : nullptr;
    }
    
    size_t count() const { return memories_.size(); }
    
    // Cross-memory copy
    void copyBetween(uint32_t destMemIdx, uint32_t destOffset,
                     uint32_t srcMemIdx, uint32_t srcOffset, uint32_t count) {
        if (destMemIdx < memories_.size() && srcMemIdx < memories_.size()) {
            auto* dest = &memories_[destMemIdx];
            auto* src = &memories_[srcMemIdx];
            if (destOffset + count <= dest->size() && srcOffset + count <= src->size()) {
                std::memmove(dest->data() + destOffset, src->data() + srcOffset, count);
            }
        }
    }
    
private:
    std::vector<WasmMemory> memories_;
};

// =============================================================================
// Reference Types
// =============================================================================

/**
 * @brief Reference value kinds
 */
enum class RefKind : uint8_t {
    Null = 0,
    Func = 1,
    Extern = 2,
    Any = 3
};

/**
 * @brief A reference value (externref or funcref)
 */
struct WasmRef {
    RefKind kind = RefKind::Null;
    uintptr_t value = 0;  // Function index or external pointer
    
    WasmRef() = default;
    WasmRef(RefKind k, uintptr_t v) : kind(k), value(v) {}
    
    static WasmRef null() { return WasmRef(); }
    static WasmRef func(uint32_t funcIndex) { return WasmRef(RefKind::Func, funcIndex); }
    static WasmRef externRef(void* ptr) { return WasmRef(RefKind::Extern, reinterpret_cast<uintptr_t>(ptr)); }
    
    bool isNull() const { return kind == RefKind::Null; }
    bool isFunc() const { return kind == RefKind::Func; }
    bool isExtern() const { return kind == RefKind::Extern; }
    
    uint32_t funcIndex() const { return static_cast<uint32_t>(value); }
    void* externPtr() const { return reinterpret_cast<void*>(value); }
};

/**
 * @brief A WASM table (array of references)
 */
class WasmTable {
public:
    WasmTable(RefKind elemType, uint32_t initialSize, uint32_t maxSize = UINT32_MAX)
        : elemType_(elemType)
        , maxSize_(maxSize) {
        elements_.resize(initialSize);
    }
    
    RefKind elemType() const { return elemType_; }
    uint32_t size() const { return static_cast<uint32_t>(elements_.size()); }
    uint32_t maxSize() const { return maxSize_; }
    
    WasmRef get(uint32_t index) const {
        return index < elements_.size() ? elements_[index] : WasmRef::null();
    }
    
    void set(uint32_t index, WasmRef ref) {
        if (index < elements_.size()) {
            elements_[index] = ref;
        }
    }
    
    // table.grow - returns old size or -1 on failure
    int32_t grow(uint32_t delta, WasmRef initValue = WasmRef::null()) {
        uint32_t oldSize = size();
        uint64_t newSize = static_cast<uint64_t>(oldSize) + delta;
        
        if (newSize > maxSize_) {
            return -1;
        }
        
        elements_.resize(static_cast<size_t>(newSize), initValue);
        return static_cast<int32_t>(oldSize);
    }
    
    // table.fill
    void fill(uint32_t dest, WasmRef value, uint32_t count) {
        for (uint32_t i = 0; i < count && dest + i < elements_.size(); i++) {
            elements_[dest + i] = value;
        }
    }
    
    // table.copy (within same table)
    void copy(uint32_t dest, uint32_t src, uint32_t count) {
        if (dest < src) {
            for (uint32_t i = 0; i < count; i++) {
                if (dest + i < elements_.size() && src + i < elements_.size()) {
                    elements_[dest + i] = elements_[src + i];
                }
            }
        } else {
            for (uint32_t i = count; i > 0; i--) {
                if (dest + i - 1 < elements_.size() && src + i - 1 < elements_.size()) {
                    elements_[dest + i - 1] = elements_[src + i - 1];
                }
            }
        }
    }
    
    // table.init from element segment
    void init(uint32_t dest, const std::vector<WasmRef>& segment, uint32_t srcOffset, uint32_t count) {
        for (uint32_t i = 0; i < count; i++) {
            if (dest + i < elements_.size() && srcOffset + i < segment.size()) {
                elements_[dest + i] = segment[srcOffset + i];
            }
        }
    }
    
private:
    RefKind elemType_;
    uint32_t maxSize_;
    std::vector<WasmRef> elements_;
};

/**
 * @brief Multi-table manager
 */
class TableManager {
public:
    uint32_t addTable(RefKind elemType, uint32_t initialSize, uint32_t maxSize = UINT32_MAX) {
        uint32_t index = static_cast<uint32_t>(tables_.size());
        tables_.emplace_back(elemType, initialSize, maxSize);
        return index;
    }
    
    WasmTable* getTable(uint32_t index) {
        return index < tables_.size() ? &tables_[index] : nullptr;
    }
    
    size_t count() const { return tables_.size(); }
    
    // Cross-table copy
    void copyBetween(uint32_t destTableIdx, uint32_t destOffset,
                     uint32_t srcTableIdx, uint32_t srcOffset, uint32_t count) {
        if (destTableIdx < tables_.size() && srcTableIdx < tables_.size()) {
            auto* dest = &tables_[destTableIdx];
            auto* src = &tables_[srcTableIdx];
            for (uint32_t i = 0; i < count; i++) {
                WasmRef val = src->get(srcOffset + i);
                dest->set(destOffset + i, val);
            }
        }
    }
    
private:
    std::vector<WasmTable> tables_;
};

// =============================================================================
// Data and Element Segments
// =============================================================================

/**
 * @brief Passive data segment for memory.init
 */
struct DataSegment {
    std::vector<uint8_t> data;
    bool dropped = false;
    
    void drop() { 
        data.clear();
        dropped = true;
    }
};

/**
 * @brief Passive element segment for table.init  
 */
struct ElemSegment {
    RefKind type = RefKind::Func;
    std::vector<WasmRef> elements;
    bool dropped = false;
    
    void drop() {
        elements.clear();
        dropped = true;
    }
};

} // namespace Zepra::Wasm
