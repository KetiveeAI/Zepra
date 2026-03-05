/**
 * @file gc_sweeper.cpp
 * @brief Sweep phase implementation — reclaims unmarked objects
 *
 * After marking, the sweeper walks the heap and:
 * - Reclaims unmarked objects (adds to free lists)
 * - Coalesces adjacent free blocks
 * - Updates region live-byte counts
 * - Optionally runs concurrently on worker threads
 *
 * Sweep modes:
 * 1. Eager sweep: sweep all pages immediately
 * 2. Lazy sweep: sweep pages on demand (when allocation needs space)
 * 3. Concurrent sweep: sweep in background thread
 */

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>

namespace Zepra::Heap {

// =============================================================================
// Sweep Page Descriptor
// =============================================================================

struct SweepPageInfo {
    uintptr_t start;
    uintptr_t end;
    uint32_t regionId;
    bool swept;
    size_t liveBytes;
    size_t freeBytes;
    size_t objectCount;
    size_t freeBlockCount;

    SweepPageInfo()
        : start(0), end(0), regionId(0), swept(false)
        , liveBytes(0), freeBytes(0), objectCount(0), freeBlockCount(0) {}
};

// =============================================================================
// Sweeper
// =============================================================================

class GCSweeper {
public:
    struct Config {
        bool concurrent;
        bool lazy;
        size_t minSweepBatchBytes;  // Min bytes to sweep per batch

        Config()
            : concurrent(false)
            , lazy(false)
            , minSweepBatchBytes(64 * 1024) {}
    };

    struct Callbacks {
        // Check if object at addr is marked
        std::function<bool(uintptr_t addr)> isMarked;

        // Get object size at addr
        std::function<size_t(uintptr_t addr)> objectSize;

        // Add free block to allocator
        std::function<void(uintptr_t addr, size_t size)> addFreeBlock;

        // Run finalizer for object being swept
        std::function<void(uintptr_t addr)> runFinalizer;

        // Update region live bytes
        std::function<void(uint32_t regionId, size_t liveBytes)>
            updateLiveBytes;
    };

    struct Stats {
        uint64_t pagesSwept;
        uint64_t objectsReclaimed;
        uint64_t bytesReclaimed;
        uint64_t objectsSurvived;
        uint64_t bytesSurvived;
        uint64_t freeBlocksCreated;
        uint64_t finalizersRun;
        double sweepMs;
    };

    explicit GCSweeper(const Config& config = Config{})
        : config_(config) {}

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    // -------------------------------------------------------------------------
    // Sweep methods
    // -------------------------------------------------------------------------

    /**
     * @brief Add pages to sweep (called after marking completes)
     */
    void addPages(const std::vector<SweepPageInfo>& pages) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& p : pages) {
            pendingPages_.push_back(p);
        }
    }

    /**
     * @brief Sweep all pending pages (eager mode)
     */
    Stats sweepAll() {
        stats_ = {};
        auto start = std::chrono::steady_clock::now();

        std::vector<SweepPageInfo> pages;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pages = std::move(pendingPages_);
            pendingPages_.clear();
        }

        for (auto& page : pages) {
            sweepPage(page);
        }

        stats_.sweepMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        return stats_;
    }

    /**
     * @brief Sweep one page lazily (called when allocator needs space)
     * @return bytes freed, or 0 if no pages to sweep
     */
    size_t sweepOnePage() {
        SweepPageInfo page;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (pendingPages_.empty()) return 0;
            page = pendingPages_.back();
            pendingPages_.pop_back();
        }

        size_t before = stats_.bytesReclaimed;
        sweepPage(page);
        return stats_.bytesReclaimed - before;
    }

    /**
     * @brief Start concurrent sweep in background thread
     */
    void startConcurrentSweep() {
        if (sweepThread_.joinable()) sweepThread_.join();

        sweepThread_ = std::thread([this]() {
            sweepAll();
        });
    }

    void waitForSweep() {
        if (sweepThread_.joinable()) sweepThread_.join();
    }

    bool hasPendingPages() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !pendingPages_.empty();
    }

    const Stats& lastStats() const { return stats_; }

private:
    /**
     * @brief Sweep a single page: walk objects, reclaim unmarked
     */
    void sweepPage(SweepPageInfo& page) {
        if (!cb_.isMarked || !cb_.objectSize) return;

        uintptr_t cursor = page.start;
        uintptr_t freeStart = 0;
        size_t freeSize = 0;
        size_t liveBytes = 0;

        while (cursor < page.end) {
            size_t objSize = cb_.objectSize(cursor);
            if (objSize == 0) break;

            objSize = (objSize + 7) & ~size_t(7);

            if (cb_.isMarked(cursor)) {
                // Live object — flush any pending free block
                if (freeSize > 0 && cb_.addFreeBlock) {
                    cb_.addFreeBlock(freeStart, freeSize);
                    stats_.freeBlocksCreated++;
                    freeSize = 0;
                }

                liveBytes += objSize;
                page.objectCount++;
                stats_.objectsSurvived++;
                stats_.bytesSurvived += objSize;
            } else {
                // Dead object
                if (cb_.runFinalizer) {
                    cb_.runFinalizer(cursor);
                    stats_.finalizersRun++;
                }

                if (freeSize == 0) {
                    freeStart = cursor;
                }
                freeSize += objSize;

                stats_.objectsReclaimed++;
                stats_.bytesReclaimed += objSize;
            }

            cursor += objSize;
        }

        // Flush trailing free block
        if (freeSize > 0 && cb_.addFreeBlock) {
            cb_.addFreeBlock(freeStart, freeSize);
            stats_.freeBlocksCreated++;
        }

        page.liveBytes = liveBytes;
        page.freeBytes = (page.end - page.start) - liveBytes;
        page.swept = true;
        stats_.pagesSwept++;

        // Update region metadata
        if (cb_.updateLiveBytes) {
            cb_.updateLiveBytes(page.regionId, liveBytes);
        }
    }

    Config config_;
    Callbacks cb_;
    Stats stats_{};

    mutable std::mutex mutex_;
    std::vector<SweepPageInfo> pendingPages_;
    std::thread sweepThread_;
};

} // namespace Zepra::Heap
