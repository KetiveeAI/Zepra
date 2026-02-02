/**
 * @file WasmMemoryContract.h
 * @brief Memory Ownership Contracts for Host-WASM Interop
 * 
 * Implements:
 * - Ownership semantics (exclusive, shared, borrowed)
 * - Memory region tagging
 * - Transfer protocol for host ↔ WASM memory
 * - Zero-copy optimizations
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Zepra::Wasm {

// =============================================================================
// Ownership Semantics
// =============================================================================

enum class Ownership {
    Exclusive,  // Single owner, can mutate
    Shared,     // Multiple readers, no mutation
    Borrowed,   // Temporary access, must return
    Moved       // Ownership transferred
};

/**
 * @brief Access mode for memory regions
 */
enum class AccessMode {
    ReadOnly,
    WriteOnly,
    ReadWrite
};

// =============================================================================
// Memory Region
// =============================================================================

/**
 * @brief Tagged memory region
 */
struct MemoryRegion {
    uint32_t offset;
    uint32_t size;
    Ownership ownership = Ownership::Exclusive;
    AccessMode access = AccessMode::ReadWrite;
    bool pinned = false;  // Cannot be moved by GC
    
    uint32_t end() const { return offset + size; }
    
    bool overlaps(const MemoryRegion& other) const {
        return !(end() <= other.offset || other.end() <= offset);
    }
    
    bool contains(uint32_t addr) const {
        return addr >= offset && addr < end();
    }
};

/**
 * @brief Memory view (borrowed region)
 */
class MemoryView {
public:
    MemoryView(uint8_t* data, size_t size, AccessMode access)
        : data_(data), size_(size), access_(access) {}
    
    uint8_t* data() { return access_ != AccessMode::ReadOnly ? data_ : nullptr; }
    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }
    AccessMode access() const { return access_; }
    
    // Read operations
    uint8_t readU8(size_t offset) const {
        return offset < size_ ? data_[offset] : 0;
    }
    
    uint32_t readU32(size_t offset) const {
        if (offset + 4 > size_) return 0;
        uint32_t val;
        std::memcpy(&val, data_ + offset, 4);
        return val;
    }
    
    // Write operations
    void writeU8(size_t offset, uint8_t value) {
        if (access_ != AccessMode::ReadOnly && offset < size_) {
            data_[offset] = value;
        }
    }
    
    void writeU32(size_t offset, uint32_t value) {
        if (access_ != AccessMode::ReadOnly && offset + 4 <= size_) {
            std::memcpy(data_ + offset, &value, 4);
        }
    }
    
private:
    uint8_t* data_;
    size_t size_;
    AccessMode access_;
};

// =============================================================================
// Memory Contract
// =============================================================================

/**
 * @brief Contract for memory transfer between host and WASM
 */
class MemoryContract {
public:
    explicit MemoryContract(uint8_t* memory, size_t memorySize)
        : memory_(memory), memorySize_(memorySize) {}
    
    // -------------------------------------------------------------------------
    // Region Management
    // -------------------------------------------------------------------------
    
    // Allocate a new region with specified ownership
    uint32_t allocate(uint32_t size, Ownership ownership = Ownership::Exclusive) {
        // Find free region (simple bump allocator)
        uint32_t offset = nextAlloc_;
        if (offset + size > memorySize_) {
            return UINT32_MAX;  // OOM
        }
        
        nextAlloc_ = offset + size;
        
        MemoryRegion region;
        region.offset = offset;
        region.size = size;
        region.ownership = ownership;
        regions_[offset] = region;
        
        return offset;
    }
    
    // Free a region
    void free(uint32_t offset) {
        regions_.erase(offset);
    }
    
    // Get region info
    const MemoryRegion* getRegion(uint32_t offset) const {
        auto it = regions_.find(offset);
        return it != regions_.end() ? &it->second : nullptr;
    }
    
    // -------------------------------------------------------------------------
    // Ownership Transfer
    // -------------------------------------------------------------------------
    
    // Transfer ownership from host to WASM
    bool transferToWasm(uint32_t offset, Ownership newOwnership) {
        auto it = regions_.find(offset);
        if (it == regions_.end()) return false;
        
        it->second.ownership = newOwnership;
        return true;
    }
    
    // Transfer ownership from WASM to host
    bool transferToHost(uint32_t offset) {
        auto it = regions_.find(offset);
        if (it == regions_.end()) return false;
        
        it->second.ownership = Ownership::Moved;
        return true;
    }
    
    // -------------------------------------------------------------------------
    // Borrowing
    // -------------------------------------------------------------------------
    
    // Borrow a region for read access
    MemoryView borrowRead(uint32_t offset, uint32_t size) {
        if (offset + size > memorySize_) {
            return MemoryView(nullptr, 0, AccessMode::ReadOnly);
        }
        
        // Check if region allows read
        auto region = getRegion(offset);
        if (region && region->ownership == Ownership::Moved) {
            return MemoryView(nullptr, 0, AccessMode::ReadOnly);  // Invalid
        }
        
        return MemoryView(memory_ + offset, size, AccessMode::ReadOnly);
    }
    
    // Borrow a region for write access
    MemoryView borrowWrite(uint32_t offset, uint32_t size) {
        if (offset + size > memorySize_) {
            return MemoryView(nullptr, 0, AccessMode::ReadOnly);
        }
        
        auto region = getRegion(offset);
        if (region) {
            // Must have exclusive ownership to write
            if (region->ownership != Ownership::Exclusive) {
                return MemoryView(nullptr, 0, AccessMode::ReadOnly);
            }
        }
        
        return MemoryView(memory_ + offset, size, AccessMode::ReadWrite);
    }
    
    // -------------------------------------------------------------------------
    // Zero-Copy Transfers
    // -------------------------------------------------------------------------
    
    // Pin a region (prevent GC from moving)
    bool pin(uint32_t offset) {
        auto it = regions_.find(offset);
        if (it == regions_.end()) return false;
        
        it->second.pinned = true;
        return true;
    }
    
    // Unpin a region
    bool unpin(uint32_t offset) {
        auto it = regions_.find(offset);
        if (it == regions_.end()) return false;
        
        it->second.pinned = false;
        return true;
    }
    
    // Get raw pointer (only for pinned regions)
    uint8_t* getRawPointer(uint32_t offset) {
        auto region = getRegion(offset);
        if (!region || !region->pinned) {
            return nullptr;
        }
        return memory_ + offset;
    }
    
    // -------------------------------------------------------------------------
    // Shared Memory
    // -------------------------------------------------------------------------
    
    // Mark region as shared (multiple readers allowed)
    bool share(uint32_t offset) {
        auto it = regions_.find(offset);
        if (it == regions_.end()) return false;
        
        if (it->second.ownership == Ownership::Exclusive) {
            it->second.ownership = Ownership::Shared;
            return true;
        }
        return false;
    }
    
    // Attempt to get exclusive access (fails if shared)
    bool makeExclusive(uint32_t offset) {
        auto it = regions_.find(offset);
        if (it == regions_.end()) return false;
        
        // Only works if not currently shared
        if (it->second.ownership == Ownership::Shared) {
            return false;  // Cannot downgrade
        }
        
        it->second.ownership = Ownership::Exclusive;
        return true;
    }
    
    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------
    
    // Check if access is valid
    bool validateAccess(uint32_t offset, uint32_t size, AccessMode mode) const {
        if (offset + size > memorySize_) return false;
        
        auto region = getRegion(offset);
        if (!region) return true;  // Untracked region, allow
        
        if (region->ownership == Ownership::Moved) return false;
        
        if (mode == AccessMode::WriteOnly || mode == AccessMode::ReadWrite) {
            if (region->ownership == Ownership::Shared) return false;
        }
        
        return true;
    }
    
    // Check for overlapping regions
    bool hasOverlap(uint32_t offset, uint32_t size) const {
        MemoryRegion query{offset, size, Ownership::Exclusive, AccessMode::ReadWrite, false};
        for (const auto& [_, region] : regions_) {
            if (region.overlaps(query)) {
                return true;
            }
        }
        return false;
    }
    
private:
    uint8_t* memory_;
    size_t memorySize_;
    uint32_t nextAlloc_ = 0;
    std::unordered_map<uint32_t, MemoryRegion> regions_;
};

// =============================================================================
// Smart Pointers for WASM Memory
// =============================================================================

/**
 * @brief Unique pointer to WASM memory (exclusive ownership)
 */
class WasmPtr {
public:
    WasmPtr() = default;
    WasmPtr(MemoryContract* contract, uint32_t offset, uint32_t size)
        : contract_(contract), offset_(offset), size_(size) {}
    
    ~WasmPtr() {
        if (contract_ && offset_ != UINT32_MAX) {
            contract_->free(offset_);
        }
    }
    
    // Move only
    WasmPtr(WasmPtr&& other) noexcept
        : contract_(other.contract_), offset_(other.offset_), size_(other.size_) {
        other.contract_ = nullptr;
        other.offset_ = UINT32_MAX;
    }
    
    WasmPtr& operator=(WasmPtr&& other) noexcept {
        if (this != &other) {
            if (contract_ && offset_ != UINT32_MAX) {
                contract_->free(offset_);
            }
            contract_ = other.contract_;
            offset_ = other.offset_;
            size_ = other.size_;
            other.contract_ = nullptr;
            other.offset_ = UINT32_MAX;
        }
        return *this;
    }
    
    WasmPtr(const WasmPtr&) = delete;
    WasmPtr& operator=(const WasmPtr&) = delete;
    
    uint32_t offset() const { return offset_; }
    uint32_t size() const { return size_; }
    bool valid() const { return contract_ != nullptr && offset_ != UINT32_MAX; }
    
    MemoryView borrow() {
        return contract_ ? contract_->borrowWrite(offset_, size_) 
                        : MemoryView(nullptr, 0, AccessMode::ReadOnly);
    }
    
    // Release ownership (returns offset, caller responsible for cleanup)
    uint32_t release() {
        uint32_t off = offset_;
        contract_ = nullptr;
        offset_ = UINT32_MAX;
        return off;
    }
    
private:
    MemoryContract* contract_ = nullptr;
    uint32_t offset_ = UINT32_MAX;
    uint32_t size_ = 0;
};

} // namespace Zepra::Wasm
