/**
 * @file WasmGCBarriers.h
 * @brief Write and Read Barriers for WebAssembly GC
 * 
 * Implements generational and concurrent GC barriers for managed references.
 */

#pragma once

#include <cstdint>
#include <atomic>

namespace Zepra::Wasm {

// =============================================================================
// Barrier Types
// =============================================================================

enum class BarrierType : uint8_t {
    None,           // No barriers needed
    PostWrite,      // Generational (post-write) barrier only
    PreWrite,       // SATB (pre-write) barrier for concurrent GC
    FullWrite,      // Both pre and post barriers
    Read            // Read barrier (for concurrent compaction)
};

// =============================================================================
// Card Table (Generational GC)
// =============================================================================

class CardTable {
public:
    static constexpr size_t CardShift = 9;      // 512 bytes per card
    static constexpr size_t CardSize = 1 << CardShift;
    static constexpr uint8_t CleanCard = 0;
    static constexpr uint8_t DirtyCard = 1;
    
    CardTable(void* heapBase, size_t heapSize);
    ~CardTable();
    
    // Mark a card as dirty (called by post-write barrier)
    void markCard(void* addr) {
        size_t cardIndex = (reinterpret_cast<uintptr_t>(addr) - heapBase_) >> CardShift;
        cards_[cardIndex] = DirtyCard;
    }
    
    // Check if card is dirty
    bool isDirty(void* addr) const {
        size_t cardIndex = (reinterpret_cast<uintptr_t>(addr) - heapBase_) >> CardShift;
        return cards_[cardIndex] == DirtyCard;
    }
    
    // Clear all cards
    void clear();
    
    // Iterate dirty cards
    template<typename Callback>
    void forEachDirtyCard(Callback&& cb) {
        for (size_t i = 0; i < numCards_; ++i) {
            if (cards_[i] == DirtyCard) {
                void* cardAddr = reinterpret_cast<void*>(heapBase_ + (i << CardShift));
                cb(cardAddr, CardSize);
            }
        }
    }
    
private:
    uintptr_t heapBase_;
    uint8_t* cards_;
    size_t numCards_;
};

// =============================================================================
// SATB Buffer (Snapshot-at-the-Beginning for Concurrent GC)
// =============================================================================

class SATBBuffer {
public:
    static constexpr size_t BufferSize = 1024;
    
    SATBBuffer();
    ~SATBBuffer();
    
    // Record a reference being overwritten (pre-write barrier)
    void record(void* oldValue) {
        if (index_ < BufferSize) {
            buffer_[index_++] = oldValue;
        } else {
            flush();
            buffer_[index_++] = oldValue;
        }
    }
    
    // Flush buffer to GC
    void flush();
    
    // Check if buffer is empty
    bool isEmpty() const { return index_ == 0; }
    
private:
    void* buffer_[BufferSize];
    size_t index_ = 0;
};

// Thread-local SATB buffer
extern thread_local SATBBuffer* threadSATBBuffer;

// =============================================================================
// Write Barrier Implementation
// =============================================================================

class WriteBarrier {
public:
    // Post-write barrier (generational)
    // Called AFTER writing a reference to a slot
    static void postWrite(void* container, void* slot, void* newValue) {
        if (!barrierEnabled_) return;
        
        // Mark card for remembered set
        if (cardTable_) {
            cardTable_->markCard(slot);
        }
    }
    
    // Pre-write barrier (SATB)
    // Called BEFORE overwriting a reference
    static void preWrite(void* slot, void* oldValue) {
        if (!barrierEnabled_) return;
        if (!oldValue) return;  // Don't record null
        
        // Record in SATB buffer
        if (threadSATBBuffer) {
            threadSATBBuffer->record(oldValue);
        }
    }
    
    // Combined barrier
    static void write(void* container, void* slot, void* oldValue, void* newValue) {
        preWrite(slot, oldValue);
        postWrite(container, slot, newValue);
    }
    
    // Enable/disable barriers
    static void enable() { barrierEnabled_ = true; }
    static void disable() { barrierEnabled_ = false; }
    static bool isEnabled() { return barrierEnabled_; }
    
    // Set card table
    static void setCardTable(CardTable* ct) { cardTable_ = ct; }
    
private:
    static inline bool barrierEnabled_ = false;
    static inline CardTable* cardTable_ = nullptr;
};

// =============================================================================
// Read Barrier (for Concurrent Compaction)
// =============================================================================

class ReadBarrier {
public:
    // Called when reading a reference (for concurrent compaction)
    static void* read(void* ptr) {
        if (!barrierEnabled_) return ptr;
        if (!ptr) return nullptr;
        
        // Check forwarding pointer (in object header)
        // If object has been moved, update the pointer
        return resolveForwarding(ptr);
    }
    
    static void enable() { barrierEnabled_ = true; }
    static void disable() { barrierEnabled_ = false; }
    
private:
    static void* resolveForwarding(void* ptr);
    static inline bool barrierEnabled_ = false;
};

// =============================================================================
// Barrier Helpers for JIT Code
// =============================================================================

// Slow path for write barrier (called from JIT when inlining not possible)
extern "C" void wasmWriteBarrierSlow(void* container, void* slot, void* oldValue, void* newValue);

// Check if barrier is needed (for JIT fast path)
extern "C" bool wasmNeedsWriteBarrier(void* container);

// =============================================================================
// Reference Pinning (for Host ↔ WASM Transitions)
// =============================================================================

class RefPin {
public:
    explicit RefPin(void* ref);
    ~RefPin();
    
    RefPin(const RefPin&) = delete;
    RefPin& operator=(const RefPin&) = delete;
    
    void* get() const { return ref_; }
    
private:
    void* ref_;
};

// Pin a reference during host function execution
class PinnedRefSet {
public:
    void pin(void* ref);
    void unpin(void* ref);
    void unpinAll();
    
    // Iterate all pinned refs (for GC roots)
    template<typename Callback>
    void forEach(Callback&& cb) const {
        for (void* ref : pinnedRefs_) {
            cb(ref);
        }
    }
    
private:
    std::vector<void*> pinnedRefs_;
};

// Thread-local pinned refs
extern thread_local PinnedRefSet* threadPinnedRefs;

// =============================================================================
// Externref Conversion
// =============================================================================

class ExternRefConvert {
public:
    // Wrap a host object as externref
    static void* toExternRef(void* hostObj);
    
    // Unwrap externref to host object
    static void* fromExternRef(void* externRef);
    
    // Register host object type for GC tracing
    using TraceCallback = void(*)(void* obj, void* visitor);
    static void registerHostType(uint32_t typeId, TraceCallback traceFn);
};

} // namespace Zepra::Wasm
