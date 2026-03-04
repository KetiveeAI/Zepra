/**
 * @file WriteBarrier.h
 * @brief Write barrier implementations for generational GC
 * 
 * Implements:
 * - Card table for tracking old→young references
 * - Store buffers for concurrent write tracking
 * - Inline barrier fast paths
 */

#pragma once

#include "gc_heap.hpp"
#include <atomic>
#include <cstring>

namespace Zepra::GC {

// Card table constants
constexpr size_t CARD_SIZE = 512;           // 512 bytes per card
constexpr size_t CARD_SHIFT = 9;            // log2(512)
constexpr uint8_t CARD_CLEAN = 0;
constexpr uint8_t CARD_DIRTY = 1;
constexpr uint8_t CARD_DIRTY_FROM_MUTATOR = 2;
constexpr uint8_t CARD_DIRTY_FROM_GC = 3;

/**
 * @brief Card table for remembered set
 * 
 * Each card covers CARD_SIZE bytes of heap.
 * Dirty cards are scanned during minor GC.
 */
class CardTable {
public:
    CardTable() = default;
    ~CardTable() { free(cards_); }
    
    // Initialize for heap range
    void init(void* heapStart, size_t heapSize) {
        heapStart_ = reinterpret_cast<uintptr_t>(heapStart);
        heapSize_ = heapSize;
        cardCount_ = (heapSize + CARD_SIZE - 1) / CARD_SIZE;
        cards_ = static_cast<uint8_t*>(calloc(cardCount_, 1));
    }
    
    // Mark card containing address as dirty
    void markDirty(void* addr) {
        size_t idx = cardIndex(addr);
        if (idx < cardCount_) {
            cards_[idx] = CARD_DIRTY_FROM_MUTATOR;
        }
    }
    
    // Mark card for GC-discovered reference
    void markDirtyFromGC(void* addr) {
        size_t idx = cardIndex(addr);
        if (idx < cardCount_) {
            cards_[idx] = CARD_DIRTY_FROM_GC;
        }
    }
    
    // Check if card is dirty
    bool isDirty(void* addr) const {
        size_t idx = cardIndex(addr);
        return idx < cardCount_ && cards_[idx] != CARD_CLEAN;
    }
    
    // Clear all cards
    void clearAll() {
        if (cards_) std::memset(cards_, CARD_CLEAN, cardCount_);
    }
    
    // Iterate dirty cards
    template<typename Fn>
    void forEachDirtyCard(Fn&& fn) {
        for (size_t i = 0; i < cardCount_; ++i) {
            if (cards_[i] != CARD_CLEAN) {
                void* start = cardStart(i);
                void* end = cardEnd(i);
                fn(start, end, cards_[i]);
            }
        }
    }
    
    // Clear card after processing
    void clearCard(void* addr) {
        size_t idx = cardIndex(addr);
        if (idx < cardCount_) {
            cards_[idx] = CARD_CLEAN;
        }
    }
    
    // Get card byte pointer (for inline barrier)
    uint8_t* cardAddress(void* addr) {
        return &cards_[cardIndex(addr)];
    }
    
private:
    size_t cardIndex(void* addr) const {
        uintptr_t offset = reinterpret_cast<uintptr_t>(addr) - heapStart_;
        return offset >> CARD_SHIFT;
    }
    
    void* cardStart(size_t idx) const {
        return reinterpret_cast<void*>(heapStart_ + (idx << CARD_SHIFT));
    }
    
    void* cardEnd(size_t idx) const {
        return reinterpret_cast<void*>(heapStart_ + ((idx + 1) << CARD_SHIFT));
    }
    
    uintptr_t heapStart_ = 0;
    size_t heapSize_ = 0;
    size_t cardCount_ = 0;
    uint8_t* cards_ = nullptr;
};

/**
 * @brief Store buffer for batched barrier processing
 * 
 * Stores (source, slot, value) tuples for later processing
 * during minor GC. Reduces barrier overhead.
 */
class StoreBuffer {
public:
    struct Entry {
        Runtime::Object* source;
        Runtime::Object** slot;
        Runtime::Object* value;
    };
    
    static constexpr size_t BUFFER_SIZE = 1024;
    
    StoreBuffer() : entries_(new Entry[BUFFER_SIZE]) {}
    ~StoreBuffer() { delete[] entries_; }
    
    // Add entry (fast path)
    bool add(Runtime::Object* src, Runtime::Object** slot, Runtime::Object* val) {
        size_t idx = count_.fetch_add(1, std::memory_order_relaxed);
        if (idx >= BUFFER_SIZE) {
            count_.store(BUFFER_SIZE, std::memory_order_relaxed);
            return false;  // Buffer full
        }
        entries_[idx] = {src, slot, val};
        return true;
    }
    
    // Process all entries
    template<typename Fn>
    void flush(Fn&& fn) {
        size_t count = std::min(count_.load(), BUFFER_SIZE);
        for (size_t i = 0; i < count; ++i) {
            fn(entries_[i]);
        }
        count_.store(0, std::memory_order_relaxed);
    }
    
    bool isFull() const { return count_.load() >= BUFFER_SIZE; }
    size_t size() const { return std::min(count_.load(), BUFFER_SIZE); }
    
private:
    Entry* entries_;
    std::atomic<size_t> count_{0};
};

/**
 * @brief Write barrier manager
 * 
 * Coordinates card table and store buffer for write barriers
 */
class WriteBarrierManager {
public:
    WriteBarrierManager() = default;
    
    void init(void* heapStart, size_t heapSize) {
        cardTable_.init(heapStart, heapSize);
    }
    
    // Generational write barrier (inline version)
    void barrierSlow(Runtime::Object* source, Runtime::Object** slot, Runtime::Object* target) {
        // Record in store buffer
        if (!storeBuffer_.add(source, slot, target)) {
            // Buffer full, flush to card table
            flushStoreBuffer();
            storeBuffer_.add(source, slot, target);
        }
        
        // Also mark card
        cardTable_.markDirty(source);
    }
    
    // Flush store buffer to card table
    void flushStoreBuffer() {
        storeBuffer_.flush([this](const StoreBuffer::Entry& e) {
            cardTable_.markDirty(e.source);
        });
    }
    
    // Process dirty cards during minor GC
    template<typename Scanner>
    void processDirtyCards(Scanner&& scanner) {
        flushStoreBuffer();
        cardTable_.forEachDirtyCard([&](void* start, void* end, uint8_t type) {
            scanner(start, end);
        });
    }
    
    // Clear after GC
    void clearAfterGC() {
        cardTable_.clearAll();
    }
    
    CardTable& cardTable() { return cardTable_; }
    StoreBuffer& storeBuffer() { return storeBuffer_; }
    
private:
    CardTable cardTable_;
    StoreBuffer storeBuffer_;
};

// =============================================================================
// Inline Write Barrier Macros
// =============================================================================

// Fast path: Check if barrier needed
#define ZEPRA_NEEDS_BARRIER(source, target) \
    (Runtime::ObjectHeader::fromObject(source)->generation == Runtime::Generation::Old && \
     Runtime::ObjectHeader::fromObject(target)->generation == Runtime::Generation::Young)

// Full barrier with check
#define ZEPRA_WRITE_BARRIER(gc, source, slot, value) \
    do { \
        Runtime::Object* _target = (value); \
        if (_target && ZEPRA_NEEDS_BARRIER(source, _target)) { \
            (gc).writeBarrier(source, _target); \
        } \
        *(slot) = _target; \
    } while(0)

// Unconditional barrier (for concurrent GC)
#define ZEPRA_WRITE_BARRIER_UNCOND(gc, source, slot, value) \
    do { \
        Runtime::Object* _old = *(slot); \
        *(slot) = (value); \
        if (_old) (gc).satbWriteBarrier(_old); \
    } while(0)

} // namespace Zepra::GC
