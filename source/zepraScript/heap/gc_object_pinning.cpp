// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_object_pinning.cpp — Pin objects to prevent evacuation/compaction

#include <atomic>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Pinned objects cannot be moved during compaction/evacuation.
// Use cases:
// - JNI/FFI: native code holds raw pointers
// - WASM: linear memory anchors
// - JIT: code objects referenced by compiled code
// - DevTools: objects being inspected

enum class PinReason : uint8_t {
    FFI,
    WASM,
    JIT,
    DevTools,
    Internal
};

struct PinRecord {
    uintptr_t addr;
    PinReason reason;
    uint32_t refCount;  // Multiple pins share same record

    PinRecord() : addr(0), reason(PinReason::Internal), refCount(0) {}
    PinRecord(uintptr_t a, PinReason r) : addr(a), reason(r), refCount(1) {}
};

class ObjectPinSet {
public:
    void pin(uintptr_t addr, PinReason reason) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pins_.find(addr);
        if (it != pins_.end()) {
            it->second.refCount++;
        } else {
            pins_[addr] = PinRecord(addr, reason);
        }
    }

    bool unpin(uintptr_t addr) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pins_.find(addr);
        if (it == pins_.end()) return false;
        it->second.refCount--;
        if (it->second.refCount == 0) {
            pins_.erase(it);
        }
        return true;
    }

    bool isPinned(uintptr_t addr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pins_.count(addr) > 0;
    }

    // Check if any object in a page range is pinned.
    bool hasPin(uintptr_t pageBase, size_t pageSize) const {
        std::lock_guard<std::mutex> lock(mutex_);
        uintptr_t end = pageBase + pageSize;
        for (auto& [a, _] : pins_) {
            if (a >= pageBase && a < end) return true;
        }
        return false;
    }

    size_t pinnedCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pins_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        pins_.clear();
    }

    void forEach(std::function<void(uintptr_t, PinReason)> fn) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [addr, rec] : pins_) {
            fn(addr, rec.reason);
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<uintptr_t, PinRecord> pins_;
};

} // namespace Zepra::Heap
