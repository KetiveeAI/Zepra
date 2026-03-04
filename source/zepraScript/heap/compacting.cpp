/**
 * @file compacting.cpp
 * @brief Compacting GC phase — defragments the heap
 *
 * After mark-sweep, live objects may be scattered across pages.
 * Compaction slides live objects together, eliminating fragmentation.
 *
 * Algorithm:
 *   1. Identify movable pages (fragmented beyond threshold)
 *   2. Compute forwarding addresses (new locations for live objects)
 *   3. Update all references (roots, stack, other objects)
 *   4. Move objects to new locations
 *   5. Release empty pages
 *
 * Ref: V8 Mark-Compact, Shenandoah GC (OpenJDK), LuaJIT GC compaction
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unordered_map>

namespace Zepra::GC {

// =============================================================================
// Object Header (for GC purposes)
// =============================================================================

struct GCObjectHeader {
    uint32_t size;          // Object size in bytes (including header)
    uint32_t gcFlags;       // Mark bits, forwarding flag, etc.
    uintptr_t forwardPtr;   // Forwarding address (set during compaction)

    static constexpr uint32_t MARK_BIT     = 1 << 0;
    static constexpr uint32_t PINNED_BIT   = 1 << 1;  // Cannot be moved
    static constexpr uint32_t FORWARDED    = 1 << 2;

    bool isMarked() const { return gcFlags & MARK_BIT; }
    bool isPinned() const { return gcFlags & PINNED_BIT; }
    bool isForwarded() const { return gcFlags & FORWARDED; }

    void setForwarded(uintptr_t newAddr) {
        gcFlags |= FORWARDED;
        forwardPtr = newAddr;
    }

    void clearMark() { gcFlags &= ~MARK_BIT; }
};

// =============================================================================
// Heap Page
// =============================================================================

static constexpr size_t PAGE_SIZE = 256 * 1024; // 256 KB pages

struct HeapPage {
    uint8_t* base;
    size_t size;
    size_t used;
    size_t liveBytes;     // Bytes occupied by live objects (after marking)
    uint32_t pageId;
    bool evacuating;      // True if this page is being compacted

    double fragmentationRatio() const {
        if (used == 0) return 0;
        return 1.0 - static_cast<double>(liveBytes) / used;
    }

    bool shouldEvacuate(double threshold = 0.5) const {
        return fragmentationRatio() > threshold && !evacuating;
    }
};

// =============================================================================
// Compactor
// =============================================================================

class Compactor {
public:
    struct Stats {
        size_t pagesEvacuated = 0;
        size_t objectsMoved = 0;
        size_t bytesFreed = 0;
        size_t referencesUpdated = 0;
        double durationMs = 0;
    };

    Compactor() : fragmentationThreshold_(0.5) {}

    /**
     * Run compaction on the given set of pages.
     */
    Stats compact(std::vector<HeapPage>& pages) {
        Stats stats;

        // Step 1: Select evacuation candidates
        std::vector<HeapPage*> candidates;
        for (auto& page : pages) {
            if (page.shouldEvacuate(fragmentationThreshold_)) {
                page.evacuating = true;
                candidates.push_back(&page);
            }
        }

        if (candidates.empty()) return stats;

        stats.pagesEvacuated = candidates.size();

        // Step 2: Compute forwarding addresses
        // Find target pages (non-evacuating pages with free space)
        std::vector<HeapPage*> targets;
        for (auto& page : pages) {
            if (!page.evacuating && page.used < page.size * 0.9) {
                targets.push_back(&page);
            }
        }

        size_t targetIdx = 0;
        size_t targetOffset = 0;

        for (auto* page : candidates) {
            if (targetIdx >= targets.size()) break;

            // Walk live objects on this page
            size_t offset = 0;
            while (offset < page->used) {
                auto* header = reinterpret_cast<GCObjectHeader*>(
                    page->base + offset);

                if (!header->isMarked() || header->size == 0) {
                    offset += std::max<size_t>(header->size, sizeof(GCObjectHeader));
                    continue;
                }

                if (header->isPinned()) {
                    offset += header->size;
                    continue;
                }

                // Find space in target page
                while (targetIdx < targets.size()) {
                    HeapPage* target = targets[targetIdx];
                    if (targetOffset + header->size <= target->size) {
                        break; // Found space
                    }
                    targetIdx++;
                    targetOffset = targets[targetIdx]->used;
                }

                if (targetIdx >= targets.size()) break;

                // Set forwarding address
                HeapPage* target = targets[targetIdx];
                uintptr_t newAddr = reinterpret_cast<uintptr_t>(
                    target->base + targetOffset);
                header->setForwarded(newAddr);

                forwardingMap_[reinterpret_cast<uintptr_t>(page->base + offset)] = newAddr;

                targetOffset += header->size;
                stats.objectsMoved++;
                offset += header->size;
            }
        }

        // Step 3: Update references in all live objects
        for (auto& page : pages) {
            size_t offset = 0;
            while (offset < page.used) {
                auto* header = reinterpret_cast<GCObjectHeader*>(
                    page.base + offset);

                if (!header->isMarked() || header->size == 0) {
                    offset += std::max<size_t>(header->size, sizeof(GCObjectHeader));
                    continue;
                }

                stats.referencesUpdated += updateReferences(
                    page.base + offset + sizeof(GCObjectHeader),
                    header->size - sizeof(GCObjectHeader));

                offset += header->size;
            }
        }

        // Step 4: Move objects
        for (auto& [oldAddr, newAddr] : forwardingMap_) {
            auto* oldHeader = reinterpret_cast<GCObjectHeader*>(oldAddr);
            std::memmove(reinterpret_cast<void*>(newAddr),
                         reinterpret_cast<void*>(oldAddr),
                         oldHeader->size);

            // Clear forwarding bit at new location
            auto* newHeader = reinterpret_cast<GCObjectHeader*>(newAddr);
            newHeader->gcFlags &= ~GCObjectHeader::FORWARDED;
        }

        // Step 5: Release evacuated pages
        for (auto* page : candidates) {
            stats.bytesFreed += page->used;
            page->used = 0;
            page->liveBytes = 0;
            page->evacuating = false;
        }

        forwardingMap_.clear();
        return stats;
    }

    void setFragmentationThreshold(double t) { fragmentationThreshold_ = t; }

private:
    /**
     * Scan a region of memory for pointers that need updating.
     * Returns number of references updated.
     */
    size_t updateReferences(uint8_t* data, size_t size) {
        size_t count = 0;
        // Scan for 8-byte aligned values that might be pointers
        for (size_t i = 0; i + 8 <= size; i += 8) {
            uintptr_t val;
            std::memcpy(&val, data + i, sizeof(val));

            auto it = forwardingMap_.find(val);
            if (it != forwardingMap_.end()) {
                std::memcpy(data + i, &it->second, sizeof(uintptr_t));
                count++;
            }
        }
        return count;
    }

    double fragmentationThreshold_;
    std::unordered_map<uintptr_t, uintptr_t> forwardingMap_;
};

// =============================================================================
// Generational GC Support
// =============================================================================

enum class Generation : uint8_t {
    Young,    // Nursery — collected frequently with copying collector
    Old,      // Tenured — collected with mark-sweep-compact
    Large     // Large object space — never moved, only swept
};

struct GenerationConfig {
    size_t youngSize = 2 * 1024 * 1024;     // 2 MB nursery
    size_t oldSize = 64 * 1024 * 1024;       // 64 MB tenured space
    size_t largeThreshold = 8192;            // Objects > 8KB go to large space
    size_t promotionAge = 2;                 // Survive 2 young GCs to promote
    double oldGrowthFactor = 1.5;            // Grow old space by 50%
};

/**
 * Nursery (young generation) — semi-space copying collector.
 *
 * Two equal-sized semi-spaces: from-space and to-space.
 * Allocation bumps a pointer in from-space.
 * Minor GC copies live objects to to-space, then swaps.
 */
class Nursery {
public:
    explicit Nursery(size_t size)
        : size_(size)
        , allocPtr_(0)
        , currentSpace_(0)
        , collections_(0)
        , promoted_(0)
        , survived_(0) {
        spaces_[0].resize(size);
        spaces_[1].resize(size);
    }

    /**
     * Allocate in young generation. Returns nullptr if nursery is full.
     */
    void* allocate(size_t bytes) {
        // Align to 8 bytes
        bytes = (bytes + 7) & ~7;
        if (allocPtr_ + bytes > size_) return nullptr;

        void* ptr = spaces_[currentSpace_].data() + allocPtr_;
        allocPtr_ += bytes;
        return ptr;
    }

    /**
     * Minor GC: copy live objects from from-space to to-space.
     * Objects that have survived enough collections get promoted.
     */
    struct MinorGCResult {
        size_t survived = 0;
        size_t promoted = 0;
        size_t freed = 0;
    };

    MinorGCResult collect(const std::vector<uintptr_t>& roots) {
        MinorGCResult result;
        collections_++;

        size_t toSpace = 1 - currentSpace_;
        size_t toPtr = 0;

        // Process roots
        for (uintptr_t root : roots) {
            if (!isInNursery(root)) continue;

            auto* header = reinterpret_cast<GCObjectHeader*>(root);
            if (header->isForwarded()) continue;

            size_t objSize = header->size;

            // Copy to to-space
            void* dst = spaces_[toSpace].data() + toPtr;
            std::memcpy(dst, reinterpret_cast<void*>(root), objSize);
            toPtr += objSize;

            // Set forwarding pointer in from-space
            header->setForwarded(reinterpret_cast<uintptr_t>(dst));
            result.survived++;
        }

        // Swap spaces
        currentSpace_ = toSpace;
        allocPtr_ = toPtr;

        result.freed = size_ - toPtr;
        survived_ += result.survived;
        promoted_ += result.promoted;

        return result;
    }

    bool isInNursery(uintptr_t addr) const {
        const uint8_t* base = spaces_[currentSpace_].data();
        return addr >= reinterpret_cast<uintptr_t>(base) &&
               addr < reinterpret_cast<uintptr_t>(base + size_);
    }

    size_t used() const { return allocPtr_; }
    size_t available() const { return size_ - allocPtr_; }
    size_t totalCollections() const { return collections_; }

private:
    size_t size_;
    size_t allocPtr_;
    size_t currentSpace_;
    size_t collections_;
    size_t promoted_;
    size_t survived_;
    std::vector<uint8_t> spaces_[2];
};

/**
 * Remembered set — tracks old→young pointers for write barrier.
 * Uses a card table: the heap is divided into 512-byte cards,
 * each tracked by a single byte.
 */
class CardTable {
public:
    explicit CardTable(size_t heapSize)
        : cardSize_(512)
        , numCards_((heapSize + cardSize_ - 1) / cardSize_)
        , cards_(numCards_, 0)
        , heapBase_(0) {}

    void setHeapBase(uintptr_t base) { heapBase_ = base; }

    void markDirty(uintptr_t addr) {
        size_t cardIdx = (addr - heapBase_) / cardSize_;
        if (cardIdx < numCards_) {
            cards_[cardIdx] = 1;
        }
    }

    bool isDirty(size_t cardIdx) const {
        return cardIdx < numCards_ && cards_[cardIdx] != 0;
    }

    void clear() {
        std::fill(cards_.begin(), cards_.end(), 0);
    }

    /**
     * Get all dirty card ranges (for scanning during minor GC).
     */
    std::vector<std::pair<uintptr_t, uintptr_t>> dirtyRanges() const {
        std::vector<std::pair<uintptr_t, uintptr_t>> ranges;
        for (size_t i = 0; i < numCards_; ++i) {
            if (cards_[i]) {
                uintptr_t start = heapBase_ + i * cardSize_;
                uintptr_t end = start + cardSize_;
                ranges.push_back({start, end});
            }
        }
        return ranges;
    }

private:
    size_t cardSize_;
    size_t numCards_;
    std::vector<uint8_t> cards_;
    uintptr_t heapBase_;
};

} // namespace Zepra::GC
