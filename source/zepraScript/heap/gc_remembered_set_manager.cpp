// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_remembered_set_manager.cpp — Coordinated remembered set + card table

#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Coordinates the remembered set (fine-grained: exact slot addresses)
// with the card table (coarse-grained: dirty/clean per 512B card).
//
// Write barrier → dirty card → add to remembered set.
// Minor GC → iterate remembered set → scan dirty cards → find old→young refs.

struct RemSetEntry {
    uintptr_t containerAddr;   // Object holding the reference
    uintptr_t slotAddr;        // Address of the slot within the object
};

class RememberedSetManager {
public:
    struct Backend {
        std::function<void(uintptr_t)> dirtyCard;
        std::function<bool(uintptr_t)> isCardDirty;
        std::function<bool(uintptr_t)> isInNursery;
    };

    void setBackend(Backend b) { backend_ = std::move(b); }

    // Called by write barrier when storing a reference.
    void onStore(uintptr_t containerAddr, uintptr_t newRef) {
        // Only record old→young references.
        if (backend_.isInNursery && backend_.isInNursery(containerAddr)) return;
        if (backend_.isInNursery && !backend_.isInNursery(newRef)) return;

        // Dirty the card.
        if (backend_.dirtyCard) backend_.dirtyCard(containerAddr);

        // Add to remembered set.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            entries_.push_back({containerAddr, 0});
            stats_.entriesAdded++;
        }
    }

    // Called during minor GC: iterate all remembered entries.
    void iterateEntries(std::function<void(uintptr_t containerAddr)> visitor) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& entry : entries_) {
            visitor(entry.containerAddr);
        }
    }

    // After scavenge: clear entries for objects that were updated or died.
    void filterEntries(std::function<bool(uintptr_t)> shouldKeep) {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [&](const RemSetEntry& e) { return !shouldKeep(e.containerAddr); }),
            entries_.end());
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
    }

    size_t entryCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }

    struct Stats { uint64_t entriesAdded; };
    Stats stats() const { return stats_; }

private:
    Backend backend_;
    mutable std::mutex mutex_;
    std::vector<RemSetEntry> entries_;
    Stats stats_{};
};

} // namespace Zepra::Heap
