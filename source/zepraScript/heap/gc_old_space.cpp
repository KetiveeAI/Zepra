/**
 * @file gc_old_space.cpp
 * @brief Old generation space management
 *
 * The old space holds objects promoted from the nursery.
 * Unlike the nursery, old space uses free-list allocation
 * and is collected by major/full GC (mark-sweep or mark-compact).
 *
 * Free-list structure:
 * - segregated free lists by size class
 * - coalescing of adjacent free blocks
 * - first-fit within size class
 *
 * Fragmentation management:
 * - track per-page fragmentation
 * - trigger compaction when fragmentation > threshold
 * - evacuate fragmented pages to fresh pages
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
#include <unordered_map>
#include <array>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Zepra::Heap {

// =============================================================================
// Free Block
// =============================================================================

struct FreeBlock {
    size_t size;
    FreeBlock* next;

    FreeBlock(size_t s) : size(s), next(nullptr) {}
};

// =============================================================================
// Size-Segregated Free List
// =============================================================================

class SegregatedFreeList {
public:
    // Size classes: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, large
    static constexpr size_t NUM_CLASSES = 10;
    static constexpr size_t MAX_SMALL_SIZE = 4096;

    SegregatedFreeList() {
        for (size_t i = 0; i < NUM_CLASSES; i++) {
            heads_[i] = nullptr;
            counts_[i] = 0;
            totalBytes_[i] = 0;
        }
    }

    /**
     * @brief Add a free block to the appropriate list
     */
    void addBlock(uintptr_t addr, size_t size) {
        auto* block = reinterpret_cast<FreeBlock*>(addr);
        block->size = size;

        size_t classIdx = sizeClass(size);
        block->next = heads_[classIdx];
        heads_[classIdx] = block;
        counts_[classIdx]++;
        totalBytes_[classIdx] += size;
        totalFree_ += size;
    }

    /**
     * @brief Allocate from free list (first-fit in class)
     */
    uintptr_t allocate(size_t size) {
        size = (size + 7) & ~size_t(7);
        size_t classIdx = sizeClass(size);

        // Try exact class first, then larger classes
        for (size_t c = classIdx; c < NUM_CLASSES; c++) {
            FreeBlock** prev = &heads_[c];
            FreeBlock* current = heads_[c];

            while (current) {
                if (current->size >= size) {
                    uintptr_t addr = reinterpret_cast<uintptr_t>(current);
                    size_t blockSize = current->size;

                    // Remove from list
                    *prev = current->next;
                    counts_[c]--;
                    totalBytes_[c] -= blockSize;
                    totalFree_ -= blockSize;

                    // Split if remainder >= minimum
                    if (blockSize - size >= 16) {
                        addBlock(addr + size, blockSize - size);
                        totalFree_ += (blockSize - size);
                    }

                    return addr;
                }
                prev = &current->next;
                current = current->next;
            }
        }

        return 0;  // No suitable block
    }

    /**
     * @brief Coalesce adjacent free blocks (reduces fragmentation)
     */
    size_t coalesce() {
        // Collect all free blocks, sort by address, merge adjacent
        std::vector<std::pair<uintptr_t, size_t>> blocks;

        for (size_t c = 0; c < NUM_CLASSES; c++) {
            FreeBlock* current = heads_[c];
            while (current) {
                blocks.push_back({
                    reinterpret_cast<uintptr_t>(current),
                    current->size});
                current = current->next;
            }
            heads_[c] = nullptr;
            counts_[c] = 0;
            totalBytes_[c] = 0;
        }
        totalFree_ = 0;

        if (blocks.empty()) return 0;

        std::sort(blocks.begin(), blocks.end());

        // Merge adjacent
        size_t merged = 0;
        std::vector<std::pair<uintptr_t, size_t>> merged_blocks;
        merged_blocks.push_back(blocks[0]);

        for (size_t i = 1; i < blocks.size(); i++) {
            auto& last = merged_blocks.back();
            if (last.first + last.second == blocks[i].first) {
                last.second += blocks[i].second;
                merged++;
            } else {
                merged_blocks.push_back(blocks[i]);
            }
        }

        // Re-add to free lists
        for (auto& [addr, size] : merged_blocks) {
            addBlock(addr, size);
        }

        return merged;
    }

    size_t totalFree() const { return totalFree_; }
    size_t blockCount(size_t classIdx) const {
        return classIdx < NUM_CLASSES ? counts_[classIdx] : 0;
    }

    size_t totalBlockCount() const {
        size_t total = 0;
        for (size_t i = 0; i < NUM_CLASSES; i++) total += counts_[i];
        return total;
    }

private:
    static size_t sizeClass(size_t size) {
        if (size <= 16) return 0;
        if (size <= 32) return 1;
        if (size <= 64) return 2;
        if (size <= 128) return 3;
        if (size <= 256) return 4;
        if (size <= 512) return 5;
        if (size <= 1024) return 6;
        if (size <= 2048) return 7;
        if (size <= 4096) return 8;
        return 9;  // Large
    }

    FreeBlock* heads_[NUM_CLASSES];
    size_t counts_[NUM_CLASSES];
    size_t totalBytes_[NUM_CLASSES];
    size_t totalFree_ = 0;
};

// =============================================================================
// Old Space Page
// =============================================================================

class OldSpacePage {
public:
    static constexpr size_t PAGE_SIZE = 256 * 1024;  // 256KB

    OldSpacePage() : base_(nullptr), used_(0), liveBytes_(0),
                      id_(nextId_++) {}

    ~OldSpacePage() { destroy(); }

    bool initialize() {
#ifdef __linux__
        base_ = static_cast<uint8_t*>(
            mmap(nullptr, PAGE_SIZE, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (base_ == MAP_FAILED) {
            base_ = nullptr;
            return false;
        }
#else
        base_ = static_cast<uint8_t*>(std::aligned_alloc(4096, PAGE_SIZE));
        if (!base_) return false;
#endif
        return true;
    }

    void destroy() {
        if (!base_) return;
#ifdef __linux__
        munmap(base_, PAGE_SIZE);
#else
        std::free(base_);
#endif
        base_ = nullptr;
    }

    bool contains(uintptr_t addr) const {
        auto b = reinterpret_cast<uintptr_t>(base_);
        return addr >= b && addr < b + PAGE_SIZE;
    }

    uintptr_t start() const { return reinterpret_cast<uintptr_t>(base_); }
    uintptr_t end() const { return start() + PAGE_SIZE; }
    uint32_t id() const { return id_; }

    size_t used() const { return used_; }
    size_t liveBytes() const { return liveBytes_; }
    void setLiveBytes(size_t lb) { liveBytes_ = lb; }
    void addUsed(size_t bytes) { used_ += bytes; }

    /**
     * @brief Fragmentation ratio (0 = no fragmentation, 1 = all dead)
     */
    double fragmentation() const {
        if (used_ == 0) return 0;
        return 1.0 - static_cast<double>(liveBytes_) / used_;
    }

private:
    uint8_t* base_;
    size_t used_;
    size_t liveBytes_;
    uint32_t id_;
    static inline std::atomic<uint32_t> nextId_{0};
};

// =============================================================================
// Old Space Manager
// =============================================================================

class OldSpaceManager {
public:
    struct Config {
        size_t maxPages;
        double compactionThreshold;  // Fragmentation ratio to trigger

        Config()
            : maxPages(1024)
            , compactionThreshold(0.5) {}
    };

    struct Stats {
        uint64_t allocations;
        uint64_t bytesAllocated;
        uint64_t promotions;
        uint64_t bytesPromoted;
        size_t pageCount;
        size_t totalUsed;
        size_t totalLive;
        size_t totalFree;
        double avgFragmentation;
    };

    explicit OldSpaceManager(const Config& config = Config{})
        : config_(config) {}

    /**
     * @brief Allocate in old space (free-list based)
     */
    uintptr_t allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);

        uintptr_t addr = freeList_.allocate(size);
        if (addr != 0) {
            stats_.allocations++;
            stats_.bytesAllocated += size;
            return addr;
        }

        // Need new page
        if (!allocateNewPage()) return 0;

        // Page is contiguous free space
        auto& page = *pages_.back();
        addr = page.start();
        page.addUsed(size);
        stats_.allocations++;
        stats_.bytesAllocated += size;
        return addr;
    }

    /**
     * @brief Promote an object from nursery
     */
    uintptr_t promote(uintptr_t oldAddr, size_t size) {
        uintptr_t addr = allocate(size);
        if (addr == 0) return 0;
        stats_.promotions++;
        stats_.bytesPromoted += size;
        return addr;
    }

    /**
     * @brief After sweep: add reclaimed memory to free list
     */
    void addFreeBlock(uintptr_t addr, size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        freeList_.addBlock(addr, size);
    }

    /**
     * @brief After GC: coalesce free blocks
     */
    size_t coalesceFreeLists() {
        std::lock_guard<std::mutex> lock(mutex_);
        return freeList_.coalesce();
    }

    /**
     * @brief Find pages needing compaction
     */
    std::vector<uint32_t> findFragmentedPages() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<uint32_t> result;
        for (const auto& page : pages_) {
            if (page->fragmentation() > config_.compactionThreshold) {
                result.push_back(page->id());
            }
        }
        return result;
    }

    bool contains(uintptr_t addr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& page : pages_) {
            if (page->contains(addr)) return true;
        }
        return false;
    }

    Stats computeStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats s = stats_;
        s.pageCount = pages_.size();
        s.totalFree = freeList_.totalFree();

        double fragSum = 0;
        for (const auto& page : pages_) {
            s.totalUsed += page->used();
            s.totalLive += page->liveBytes();
            fragSum += page->fragmentation();
        }
        s.avgFragmentation = pages_.empty() ? 0 :
            fragSum / pages_.size();

        return s;
    }

private:
    bool allocateNewPage() {
        if (pages_.size() >= config_.maxPages) return false;

        auto page = std::make_unique<OldSpacePage>();
        if (!page->initialize()) return false;

        pages_.push_back(std::move(page));
        return true;
    }

    Config config_;
    mutable std::mutex mutex_;
    SegregatedFreeList freeList_;
    std::vector<std::unique_ptr<OldSpacePage>> pages_;
    Stats stats_{};
};

} // namespace Zepra::Heap
