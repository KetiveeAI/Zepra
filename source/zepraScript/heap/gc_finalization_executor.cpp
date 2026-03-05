// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_finalization_executor.cpp — Run weak ref/FinalizationRegistry callbacks

#include <mutex>
#include <deque>
#include <functional>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// After GC marking, objects with registered cleanup callbacks
// need their callbacks invoked. This runs outside the GC pause
// on the mutator thread via microtask queue.

struct FinalizationEntry {
    uintptr_t heldValue;
    std::function<void(uintptr_t)> callback;
    uint64_t registryId;
};

class FinalizationExecutor {
public:
    void schedule(uintptr_t heldValue,
                   std::function<void(uintptr_t)> callback,
                   uint64_t registryId) {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back({heldValue, std::move(callback), registryId});
    }

    // Called on the mutator thread after GC completes.
    size_t drainPending() {
        std::deque<FinalizationEntry> batch;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            batch.swap(pending_);
        }

        size_t count = 0;
        for (auto& entry : batch) {
            try {
                if (entry.callback) {
                    entry.callback(entry.heldValue);
                    count++;
                }
            } catch (...) {
                stats_.errors++;
            }
        }

        stats_.executed += count;
        return count;
    }

    // Unregister all callbacks for a specific registry.
    void unregister(uint64_t registryId) {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.erase(
            std::remove_if(pending_.begin(), pending_.end(),
                [registryId](const FinalizationEntry& e) {
                    return e.registryId == registryId;
                }),
            pending_.end());
    }

    size_t pendingCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_.size();
    }

    struct Stats { uint64_t executed; uint64_t errors; };
    Stats stats() const { return stats_; }

private:
    mutable std::mutex mutex_;
    std::deque<FinalizationEntry> pending_;
    Stats stats_{};
};

} // namespace Zepra::Heap
