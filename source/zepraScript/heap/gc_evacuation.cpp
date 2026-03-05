// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_evacuation.cpp — Object relocation during compaction

#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <functional>
#include <algorithm>
#include <unordered_map>

namespace Zepra::Heap {

// Evacuates live objects from fragmented regions into fresh pages.
// After evacuation, old pages are freed and all references are updated.
//
// Steps:
// 1. Select evacuation candidates (fragmented pages)
// 2. Allocate destination pages
// 3. Copy live objects, install forwarding pointers
// 4. Update all references (roots + heap)
// 5. Free source pages

struct ForwardingEntry {
    uintptr_t oldAddr;
    uintptr_t newAddr;
    size_t size;
};

class ForwardingTable {
public:
    void record(uintptr_t old, uintptr_t neu, size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        table_[old] = {old, neu, size};
    }

    uintptr_t lookup(uintptr_t old) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = table_.find(old);
        return it != table_.end() ? it->second.newAddr : old;
    }

    bool hasForwarding(uintptr_t old) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return table_.count(old) > 0;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        table_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return table_.size();
    }

    void forEach(std::function<void(uintptr_t old, uintptr_t neu)> fn) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [k, v] : table_) {
            fn(v.oldAddr, v.newAddr);
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<uintptr_t, ForwardingEntry> table_;
};

struct EvacuationPage {
    uintptr_t base;
    size_t size;
    size_t cursor;

    EvacuationPage() : base(0), size(0), cursor(0) {}
    EvacuationPage(uintptr_t b, size_t s) : base(b), size(s), cursor(0) {}

    uintptr_t allocate(size_t bytes) {
        bytes = (bytes + 7) & ~size_t(7);
        if (cursor + bytes > size) return 0;
        uintptr_t addr = base + cursor;
        cursor += bytes;
        return addr;
    }

    size_t remaining() const { return size - cursor; }
};

class Evacuator {
public:
    struct Callbacks {
        // Get object size at address.
        std::function<size_t(uintptr_t)> objectSize;
        // Check if object is live (marked).
        std::function<bool(uintptr_t)> isLive;
        // Iterate all slots in an object that hold heap references.
        std::function<void(uintptr_t, std::function<void(uintptr_t*)>)> iterateSlots;
        // Iterate all root slots.
        std::function<void(std::function<void(uintptr_t*)>)> iterateRoots;
        // Allocate a fresh destination page.
        std::function<uintptr_t(size_t)> allocatePage;
        // Free a source page.
        std::function<void(uintptr_t, size_t)> freePage;
    };

    struct Stats {
        uint64_t objectsMoved;
        uint64_t bytesMoved;
        uint64_t refsUpdated;
        uint64_t pagesEvacuated;
        uint64_t pagesAllocated;
        double evacuationMs;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    // Phase 1: Copy live objects from source pages to destination pages.
    void evacuatePages(const std::vector<std::pair<uintptr_t, size_t>>& sourcePages) {
        auto start = std::chrono::steady_clock::now();

        for (auto& [pageBase, pageSize] : sourcePages) {
            evacuateSinglePage(pageBase, pageSize);
            stats_.pagesEvacuated++;
        }

        stats_.evacuationMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
    }

    // Phase 2: Update all references using forwarding table.
    void updateReferences() {
        // Update roots.
        if (cb_.iterateRoots) {
            cb_.iterateRoots([&](uintptr_t* slot) {
                updateSlot(slot);
            });
        }

        // Update heap references in all live objects.
        forwarding_.forEach([&](uintptr_t, uintptr_t newAddr) {
            if (cb_.iterateSlots) {
                cb_.iterateSlots(newAddr, [&](uintptr_t* slot) {
                    updateSlot(slot);
                });
            }
        });
    }

    // Phase 3: Free source pages.
    void freeSourcePages(const std::vector<std::pair<uintptr_t, size_t>>& pages) {
        if (!cb_.freePage) return;
        for (auto& [base, size] : pages) {
            cb_.freePage(base, size);
        }
    }

    const ForwardingTable& forwardingTable() const { return forwarding_; }
    const Stats& stats() const { return stats_; }

private:
    void evacuateSinglePage(uintptr_t pageBase, size_t pageSize) {
        if (!cb_.objectSize || !cb_.isLive) return;

        uintptr_t scan = pageBase;
        uintptr_t pageEnd = pageBase + pageSize;

        while (scan < pageEnd) {
            size_t objSize = cb_.objectSize(scan);
            if (objSize == 0) break;

            if (cb_.isLive(scan)) {
                uintptr_t dest = allocateInDest(objSize);
                if (dest != 0) {
                    std::memcpy(reinterpret_cast<void*>(dest),
                                reinterpret_cast<void*>(scan), objSize);
                    forwarding_.record(scan, dest, objSize);
                    stats_.objectsMoved++;
                    stats_.bytesMoved += objSize;
                }
            }

            scan += objSize;
        }
    }

    uintptr_t allocateInDest(size_t bytes) {
        // Try current destination page.
        if (!destPages_.empty()) {
            uintptr_t addr = destPages_.back().allocate(bytes);
            if (addr != 0) return addr;
        }

        // Allocate new destination page.
        if (!cb_.allocatePage) return 0;
        static constexpr size_t DEST_PAGE_SIZE = 256 * 1024;
        uintptr_t pageBase = cb_.allocatePage(DEST_PAGE_SIZE);
        if (pageBase == 0) return 0;

        destPages_.emplace_back(pageBase, DEST_PAGE_SIZE);
        stats_.pagesAllocated++;
        return destPages_.back().allocate(bytes);
    }

    void updateSlot(uintptr_t* slot) {
        if (!slot || *slot == 0) return;
        uintptr_t forwarded = forwarding_.lookup(*slot);
        if (forwarded != *slot) {
            *slot = forwarded;
            stats_.refsUpdated++;
        }
    }

    Callbacks cb_;
    ForwardingTable forwarding_;
    std::vector<EvacuationPage> destPages_;
    Stats stats_{};
};

} // namespace Zepra::Heap
