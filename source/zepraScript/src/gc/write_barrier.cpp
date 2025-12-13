/**
 * @file write_barrier.cpp
 * @brief Write barrier for generational and incremental GC
 * 
 * Implements write barriers that maintain invariants for:
 * - Generational GC: Track cross-generation pointers
 * - Incremental GC: Maintain tri-color invariant
 */

#include "zeprascript/gc/incremental_gc.hpp"
#include "zeprascript/runtime/object.hpp"
#include <atomic>
#include <vector>
#include <bitset>

namespace Zepra::GC {

// =============================================================================
// Card Table - For generational GC cross-generation pointer tracking
// =============================================================================

/**
 * @brief Card table for tracking dirty regions
 * 
 * Each card represents a fixed-size region of memory.
 * When an old-gen object is written, its card is marked dirty.
 * During minor GC, only dirty cards need scanning for cross-gen refs.
 */
class CardTable {
public:
    static constexpr size_t CARD_SIZE = 512;        // Bytes per card
    static constexpr size_t CARDS_PER_PAGE = 128;   // Cards per tracking page
    
    CardTable(uintptr_t heapStart, size_t heapSize)
        : heapStart_(heapStart)
        , heapSize_(heapSize) {
        size_t numCards = (heapSize + CARD_SIZE - 1) / CARD_SIZE;
        cards_.resize(numCards, false);
    }
    
    /**
     * @brief Mark a memory address as dirty
     */
    void markDirty(void* ptr) {
        size_t cardIndex = ptrToCardIndex(ptr);
        if (cardIndex < cards_.size()) {
            cards_[cardIndex] = true;
            dirtyCount_++;
        }
    }
    
    /**
     * @brief Check if an address is in a dirty card
     */
    bool isDirty(void* ptr) const {
        size_t cardIndex = ptrToCardIndex(ptr);
        return cardIndex < cards_.size() && cards_[cardIndex];
    }
    
    /**
     * @brief Clear all dirty marks
     */
    void clearAll() {
        std::fill(cards_.begin(), cards_.end(), false);
        dirtyCount_ = 0;
    }
    
    /**
     * @brief Iterate over dirty cards
     */
    template<typename Callback>
    void forEachDirtyCard(Callback&& callback) const {
        for (size_t i = 0; i < cards_.size(); ++i) {
            if (cards_[i]) {
                uintptr_t cardStart = heapStart_ + (i * CARD_SIZE);
                uintptr_t cardEnd = cardStart + CARD_SIZE;
                callback(reinterpret_cast<void*>(cardStart),
                        reinterpret_cast<void*>(cardEnd));
            }
        }
    }
    
    /**
     * @brief Get number of dirty cards
     */
    size_t dirtyCount() const { return dirtyCount_; }
    
    /**
     * @brief Get total number of cards
     */
    size_t totalCards() const { return cards_.size(); }
    
private:
    size_t ptrToCardIndex(void* ptr) const {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        if (addr < heapStart_) return SIZE_MAX;
        return (addr - heapStart_) / CARD_SIZE;
    }
    
    uintptr_t heapStart_;
    size_t heapSize_;
    std::vector<bool> cards_;
    size_t dirtyCount_ = 0;
};

// =============================================================================
// RememberedSet - Alternative to card table for precise tracking
// =============================================================================

/**
 * @brief Remembered set for tracking specific cross-generation pointers
 * 
 * More precise than card table but uses more memory.
 * Stores actual (holder, slot) pairs that contain cross-gen references.
 */
class RememberedSet {
public:
    struct Entry {
        Runtime::Object* holder;    // Object containing the reference
        Runtime::Object** slot;     // Pointer to the slot holding the reference
    };
    
    /**
     * @brief Add an entry to the remembered set
     */
    void add(Runtime::Object* holder, Runtime::Object** slot) {
        entries_.push_back({holder, slot});
    }
    
    /**
     * @brief Clear all entries
     */
    void clear() {
        entries_.clear();
    }
    
    /**
     * @brief Iterate over entries
     */
    template<typename Callback>
    void forEach(Callback&& callback) const {
        for (const auto& entry : entries_) {
            callback(entry);
        }
    }
    
    /**
     * @brief Get number of entries
     */
    size_t size() const { return entries_.size(); }
    
    /**
     * @brief Remove entries where holder is no longer alive
     */
    void compact(std::function<bool(Runtime::Object*)> isAlive) {
        auto newEnd = std::remove_if(entries_.begin(), entries_.end(),
            [&](const Entry& e) { return !isAlive(e.holder); });
        entries_.erase(newEnd, entries_.end());
    }
    
private:
    std::vector<Entry> entries_;
};

// =============================================================================
// WriteBarrierConfig - Configuration for write barrier behavior
// =============================================================================

struct WriteBarrierConfig {
    bool useCardTable = true;       // Use card table for generational GC
    bool useRememberedSet = false;  // Use remembered set (mutually exclusive with card table)
    bool incrementalGCEnabled = true;
    bool generationalGCEnabled = false;  // Not yet implemented
};

// =============================================================================
// WriteBarrierState - Global state for write barriers
// =============================================================================

class WriteBarrierState {
public:
    static WriteBarrierState& instance() {
        static WriteBarrierState inst;
        return inst;
    }
    
    void configure(const WriteBarrierConfig& config) {
        config_ = config;
    }
    
    const WriteBarrierConfig& config() const { return config_; }
    
    /**
     * @brief Initialize card table for heap region
     */
    void initCardTable(uintptr_t heapStart, size_t heapSize) {
        cardTable_ = std::make_unique<CardTable>(heapStart, heapSize);
    }
    
    CardTable* cardTable() { return cardTable_.get(); }
    RememberedSet& rememberedSet() { return rememberedSet_; }
    
    /**
     * @brief Record a write for incremental GC
     * Called when an old (marked) object gains reference to new (unmarked) object
     */
    void recordIncrementalWrite(Runtime::Object* holder, Runtime::Object* newValue) {
        if (!config_.incrementalGCEnabled) return;
        if (!holder || !newValue) return;
        if (!holder->isMarked() || newValue->isMarked()) return;
        
        // Re-mark to maintain tri-color invariant
        // The actual work is handled by IncrementalGC via callback
        if (incrementalCallback_) {
            incrementalCallback_(holder, newValue);
        } else {
            // Fallback: directly mark the new value
            newValue->markGC();
        }
        
        incrementalWrites_++;
    }
    
    /**
     * @brief Record a write for generational GC
     */
    void recordGenerationalWrite(Runtime::Object* holder, Runtime::Object* newValue) {
        if (!config_.generationalGCEnabled) return;
        if (!holder || !newValue) return;
        
        // Mark card dirty if using card table
        if (config_.useCardTable && cardTable_) {
            cardTable_->markDirty(holder);
        }
        
        generationalWrites_++;
    }
    
    /**
     * @brief Set callback for incremental GC integration
     */
    using IncrementalCallback = std::function<void(Runtime::Object*, Runtime::Object*)>;
    void setIncrementalCallback(IncrementalCallback cb) {
        incrementalCallback_ = std::move(cb);
    }
    
    /**
     * @brief Get write barrier statistics
     */
    struct Stats {
        size_t incrementalWrites = 0;
        size_t generationalWrites = 0;
        size_t cardTableDirtyCount = 0;
        size_t rememberedSetSize = 0;
    };
    
    Stats getStats() const {
        Stats s;
        s.incrementalWrites = incrementalWrites_;
        s.generationalWrites = generationalWrites_;
        if (cardTable_) {
            s.cardTableDirtyCount = cardTable_->dirtyCount();
        }
        s.rememberedSetSize = rememberedSet_.size();
        return s;
    }
    
    void resetStats() {
        incrementalWrites_ = 0;
        generationalWrites_ = 0;
    }
    
private:
    WriteBarrierState() = default;
    
    WriteBarrierConfig config_;
    std::unique_ptr<CardTable> cardTable_;
    RememberedSet rememberedSet_;
    IncrementalCallback incrementalCallback_;
    
    std::atomic<size_t> incrementalWrites_{0};
    std::atomic<size_t> generationalWrites_{0};
};

// =============================================================================
// Public Write Barrier Functions
// =============================================================================

void initWriteBarrier(const WriteBarrierConfig& config) {
    WriteBarrierState::instance().configure(config);
}

void onFieldWrite(Runtime::Object* holder, Runtime::Object* newValue) {
    auto& state = WriteBarrierState::instance();
    state.recordIncrementalWrite(holder, newValue);
    state.recordGenerationalWrite(holder, newValue);
}

void onArrayWrite(Runtime::Object* array, size_t index, Runtime::Object* newValue) {
    // Array writes use the same barrier as field writes
    (void)index;  // Index not needed for barrier logic
    onFieldWrite(array, newValue);
}

void setIncrementalWriteBarrierCallback(
    std::function<void(Runtime::Object*, Runtime::Object*)> callback) {
    WriteBarrierState::instance().setIncrementalCallback(std::move(callback));
}

} // namespace Zepra::GC
