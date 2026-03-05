/**
 * @file remembered_set.cpp
 * @brief Remembered set for tracking cross-region/cross-generation references
 *
 * When an old-generation object references a young-generation object,
 * the GC must know about it to avoid scanning the entire old gen
 * during minor GC. The remembered set tracks these cross-generation
 * references.
 *
 * Implementation:
 *
 * 1. Card table (coarse grain):
 *    - Heap divided into 512-byte "cards"
 *    - Each card has a 1-byte dirty flag
 *    - Write barrier dirties the card containing the source
 *    - Minor GC scans dirty cards as additional roots
 *
 * 2. Remembered set (fine grain):
 *    - Per-region hash set of slot addresses
 *    - Populated by refining dirty cards after marking
 *    - More precise than cards (avoids re-scanning entire cards)
 *
 * 3. Cross-region reference set:
 *    - For region-based collectors: tracks refs between regions
 *    - Used during selective region collection
 *
 * Write barrier flow:
 *   obj.x = value  (runtime write)
 *   → postWriteBarrier(obj_addr, value_addr)
 *   → if (region(obj) != region(value))
 *       cardTable.dirty(obj_addr)
 *       crossRegionSet.add(obj_region, value_region)
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

namespace Zepra::Heap {

// =============================================================================
// Card Table
// =============================================================================

class CardTable {
public:
    static constexpr size_t CARD_SHIFT = 9;  // 512 bytes per card
    static constexpr size_t CARD_SIZE = size_t(1) << CARD_SHIFT;

    static constexpr uint8_t CLEAN = 0;
    static constexpr uint8_t DIRTY = 1;
    static constexpr uint8_t DIRTY_FROM_GC = 2;  // Dirty but already scanned

    CardTable() : base_(0), cards_(nullptr), numCards_(0) {}

    ~CardTable() { destroy(); }

    CardTable(const CardTable&) = delete;
    CardTable& operator=(const CardTable&) = delete;

    bool initialize(uintptr_t heapBase, size_t heapSize) {
        base_ = heapBase;
        numCards_ = (heapSize + CARD_SIZE - 1) / CARD_SIZE;
        cards_ = new (std::nothrow) uint8_t[numCards_];
        if (!cards_) return false;
        std::memset(cards_, CLEAN, numCards_);
        return true;
    }

    void destroy() {
        delete[] cards_;
        cards_ = nullptr;
        numCards_ = 0;
    }

    /**
     * @brief Mark the card containing addr as dirty
     *
     * Called by the write barrier. Must be very fast (hot path).
     */
    void dirty(uintptr_t addr) {
        size_t index = cardIndex(addr);
        if (index < numCards_) {
            cards_[index] = DIRTY;
        }
    }

    /**
     * @brief Atomic dirty (for concurrent write barrier)
     */
    void dirtyAtomic(uintptr_t addr) {
        size_t index = cardIndex(addr);
        if (index < numCards_) {
            auto* p = reinterpret_cast<std::atomic<uint8_t>*>(&cards_[index]);
            p->store(DIRTY, std::memory_order_release);
        }
    }

    bool isDirty(uintptr_t addr) const {
        size_t index = cardIndex(addr);
        return index < numCards_ && cards_[index] == DIRTY;
    }

    void clean(uintptr_t addr) {
        size_t index = cardIndex(addr);
        if (index < numCards_) {
            cards_[index] = CLEAN;
        }
    }

    /**
     * @brief Clear all dirty flags
     */
    void clearAll() {
        if (cards_) {
            std::memset(cards_, CLEAN, numCards_);
        }
    }

    /**
     * @brief Iterate all dirty cards
     *
     * For each dirty card, calls visitor with the card's base address
     * and the card's end address. Used during minor GC to find
     * cross-generation references.
     */
    void forEachDirtyCard(
        std::function<void(uintptr_t cardBase, uintptr_t cardEnd)> visitor
    ) const {
        for (size_t i = 0; i < numCards_; i++) {
            if (cards_[i] == DIRTY) {
                uintptr_t cardBase = base_ + i * CARD_SIZE;
                uintptr_t cardEnd = cardBase + CARD_SIZE;
                visitor(cardBase, cardEnd);
            }
        }
    }

    /**
     * @brief Count dirty cards (for stats)
     */
    size_t dirtyCount() const {
        size_t count = 0;
        for (size_t i = 0; i < numCards_; i++) {
            if (cards_[i] == DIRTY) count++;
        }
        return count;
    }

    /**
     * @brief Iterate dirty cards in a range (for region-scoped scans)
     */
    void forEachDirtyCardInRange(uintptr_t start, uintptr_t end,
        std::function<void(uintptr_t cardBase, uintptr_t cardEnd)> visitor
    ) const {
        size_t startIdx = cardIndex(start);
        size_t endIdx = cardIndex(end);
        if (endIdx > numCards_) endIdx = numCards_;

        for (size_t i = startIdx; i < endIdx; i++) {
            if (cards_[i] == DIRTY) {
                uintptr_t cardBase = base_ + i * CARD_SIZE;
                uintptr_t cardEnd = cardBase + CARD_SIZE;
                visitor(cardBase, cardEnd);
            }
        }
    }

    size_t cardCount() const { return numCards_; }

    /**
     * @brief Get card base address for a given heap address
     */
    uintptr_t cardBase(uintptr_t addr) const {
        return addr & ~(CARD_SIZE - 1);
    }

private:
    size_t cardIndex(uintptr_t addr) const {
        if (addr < base_) return SIZE_MAX;
        return (addr - base_) >> CARD_SHIFT;
    }

    uintptr_t base_;
    uint8_t* cards_;
    size_t numCards_;
};

// =============================================================================
// Slot Remembered Set (per-region fine-grained set)
// =============================================================================

/**
 * @brief Fine-grained remembered set for a single region
 *
 * Stores the exact slot addresses that hold cross-region references.
 * Populated by refining dirty cards: scan objects in dirty cards,
 * find which specific slots point across regions.
 */
class SlotRememberedSet {
public:
    void addSlot(uintptr_t slotAddr) {
        std::lock_guard<std::mutex> lock(mutex_);
        slots_.insert(slotAddr);
    }

    void removeSlot(uintptr_t slotAddr) {
        std::lock_guard<std::mutex> lock(mutex_);
        slots_.erase(slotAddr);
    }

    bool contains(uintptr_t slotAddr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return slots_.find(slotAddr) != slots_.end();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        slots_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return slots_.size();
    }

    /**
     * @brief Enumerate slots (for GC root scanning)
     */
    void enumerate(std::function<void(uintptr_t slotAddr)> visitor) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto addr : slots_) {
            visitor(addr);
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_set<uintptr_t> slots_;
};

// =============================================================================
// Cross-Region Reference Set
// =============================================================================

/**
 * @brief Tracks which regions reference which other regions
 *
 * When region A has a reference to an object in region B,
 * we record (A, B) in this set. During selective collection
 * of region B, we know to scan region A.
 */
class CrossRegionRefSet {
public:
    struct RegionPair {
        uint32_t sourceRegion;
        uint32_t targetRegion;

        bool operator==(const RegionPair& o) const {
            return sourceRegion == o.sourceRegion &&
                   targetRegion == o.targetRegion;
        }
    };

    struct PairHash {
        size_t operator()(const RegionPair& p) const {
            return std::hash<uint64_t>()(
                (static_cast<uint64_t>(p.sourceRegion) << 32) |
                p.targetRegion);
        }
    };

    void addReference(uint32_t srcRegion, uint32_t dstRegion) {
        if (srcRegion == dstRegion) return;  // Same region, not interesting
        std::lock_guard<std::mutex> lock(mutex_);
        pairs_.insert({srcRegion, dstRegion});
    }

    /**
     * @brief Get all regions that reference a given target region
     *
     * Used during selective collection: which regions could have
     * references into the region being collected?
     */
    std::vector<uint32_t> getReferencingRegions(uint32_t targetRegion) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<uint32_t> result;
        for (const auto& pair : pairs_) {
            if (pair.targetRegion == targetRegion) {
                result.push_back(pair.sourceRegion);
            }
        }
        return result;
    }

    /**
     * @brief Get all regions referenced by a given source region
     */
    std::vector<uint32_t> getReferencedRegions(uint32_t sourceRegion) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<uint32_t> result;
        for (const auto& pair : pairs_) {
            if (pair.sourceRegion == sourceRegion) {
                result.push_back(pair.targetRegion);
            }
        }
        return result;
    }

    void removeRegion(uint32_t regionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::unordered_set<RegionPair, PairHash> filtered;
        for (const auto& p : pairs_) {
            if (p.sourceRegion != regionId && p.targetRegion != regionId) {
                filtered.insert(p);
            }
        }
        pairs_ = std::move(filtered);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        pairs_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pairs_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_set<RegionPair, PairHash> pairs_;
};

// =============================================================================
// Remembered Set Manager
// =============================================================================

/**
 * @brief Coordinates card table, slot sets, and cross-region tracking
 */
class RememberedSetManager {
public:
    struct Config {
        uintptr_t heapBase;
        size_t heapSize;
        size_t maxRegions;

        Config()
            : heapBase(0), heapSize(0), maxRegions(4096) {}
    };

    struct Stats {
        uint64_t barrierInvocations;
        uint64_t dirtyCards;
        size_t totalSlotEntries;
        size_t crossRegionPairs;
    };

    explicit RememberedSetManager(const Config& config)
        : config_(config)
        , barrierCount_(0) {}

    bool initialize() {
        return cardTable_.initialize(config_.heapBase, config_.heapSize);
    }

    // -------------------------------------------------------------------------
    // Write barrier entry point
    // -------------------------------------------------------------------------

    /**
     * @brief Post-write barrier: called after a reference store
     *
     * @param srcAddr Address of the source object
     * @param srcRegion Region ID of the source
     * @param dstAddr Address of the referenced object
     * @param dstRegion Region ID of the target
     */
    void recordReference(uintptr_t srcAddr, uint32_t srcRegion,
                          uintptr_t dstAddr, uint32_t dstRegion) {
        barrierCount_++;

        // Dirty the card
        cardTable_.dirty(srcAddr);

        // Track cross-region
        if (srcRegion != dstRegion) {
            crossRegionSet_.addReference(srcRegion, dstRegion);
        }
    }

    // -------------------------------------------------------------------------
    // GC scanning
    // -------------------------------------------------------------------------

    /**
     * @brief Scan dirty cards as roots for minor GC
     *
     * Iterates dirty cards, for each card scans objects within
     * and traces references that point into the nursery.
     */
    void scanDirtyCards(
        std::function<void(uintptr_t cardBase, uintptr_t cardEnd)> scanner
    ) {
        cardTable_.forEachDirtyCard(scanner);
    }

    /**
     * @brief Refine dirty cards into per-region slot sets
     *
     * After marking, convert coarse card table entries into
     * precise slot addresses for more efficient future scans.
     */
    void refineDirtyCards(
        uint32_t regionId,
        uintptr_t regionStart, uintptr_t regionEnd,
        std::function<void(uintptr_t objAddr,
            std::function<void(uintptr_t slotAddr, uintptr_t refAddr)>)>
            objectScanner
    ) {
        auto& slotSet = getOrCreateSlotSet(regionId);

        cardTable_.forEachDirtyCardInRange(regionStart, regionEnd,
            [&](uintptr_t cardBase, uintptr_t cardEnd) {
                objectScanner(cardBase,
                    [&](uintptr_t slotAddr, uintptr_t refAddr) {
                        // Check if reference crosses region boundary
                        if (refAddr < regionStart || refAddr >= regionEnd) {
                            slotSet.addSlot(slotAddr);
                        }
                    });
            });
    }

    /**
     * @brief Clear all dirty cards and slot sets (after full GC)
     */
    void clearAll() {
        cardTable_.clearAll();
        crossRegionSet_.clear();
        std::lock_guard<std::mutex> lock(slotSetMutex_);
        slotSets_.clear();
    }

    // -------------------------------------------------------------------------
    // Stats
    // -------------------------------------------------------------------------

    Stats computeStats() const {
        Stats s{};
        s.barrierInvocations = barrierCount_;
        s.dirtyCards = cardTable_.dirtyCount();
        s.crossRegionPairs = crossRegionSet_.size();

        std::lock_guard<std::mutex> lock(slotSetMutex_);
        for (const auto& [id, set] : slotSets_) {
            s.totalSlotEntries += set->size();
        }

        return s;
    }

    CardTable& cardTable() { return cardTable_; }
    CrossRegionRefSet& crossRegionSet() { return crossRegionSet_; }

private:
    SlotRememberedSet& getOrCreateSlotSet(uint32_t regionId) {
        std::lock_guard<std::mutex> lock(slotSetMutex_);
        auto it = slotSets_.find(regionId);
        if (it != slotSets_.end()) return *it->second;
        auto set = std::make_unique<SlotRememberedSet>();
        auto& ref = *set;
        slotSets_[regionId] = std::move(set);
        return ref;
    }

    Config config_;
    CardTable cardTable_;
    CrossRegionRefSet crossRegionSet_;

    mutable std::mutex slotSetMutex_;
    std::unordered_map<uint32_t, std::unique_ptr<SlotRememberedSet>> slotSets_;

    uint64_t barrierCount_;
};

} // namespace Zepra::Heap
