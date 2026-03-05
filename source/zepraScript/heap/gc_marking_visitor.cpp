// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_marking_visitor.cpp — Object graph traversal for GC marking

#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <vector>

namespace Zepra::Heap {

// Visits all reference slots in a heap object. The marker calls
// visitObject() for each grey object; the visitor pushes children
// onto the mark worklist.

enum class SlotType : uint8_t {
    Strong,      // Normal reference — must be traced
    Weak,        // Weak reference — traced but doesn't keep alive
    Untraced     // Raw data — skip
};

struct ObjectLayout {
    uint8_t typeTag;
    size_t totalSize;
    size_t fixedSlotCount;
    size_t dynamicSlotOffset;
    size_t dynamicSlotCount;
};

class MarkingVisitor {
public:
    using MarkCallback = std::function<void(uintptr_t childAddr)>;
    using WeakCallback = std::function<void(uintptr_t* slotAddr)>;

    struct Backend {
        // Read a pointer-sized slot at given offset from object base.
        std::function<uintptr_t(uintptr_t objAddr, size_t offset)> readSlot;
        // Get layout description for object type.
        std::function<ObjectLayout(uintptr_t objAddr)> getLayout;
        // Check if address is a valid heap pointer.
        std::function<bool(uintptr_t)> isHeapPointer;
    };

    void setBackend(Backend b) { backend_ = std::move(b); }

    // Visit all slots in an object, calling markCb for strong refs.
    void visitObject(uintptr_t objAddr, MarkCallback markCb,
                      WeakCallback weakCb = nullptr) {
        if (!backend_.getLayout || !backend_.readSlot) return;

        auto layout = backend_.getLayout(objAddr);
        stats_.objectsVisited++;

        // Visit fixed slots (e.g., prototype, constructor, __proto__).
        for (size_t i = 0; i < layout.fixedSlotCount; i++) {
            size_t offset = 8 + i * 8;  // Skip 8-byte header
            uintptr_t child = backend_.readSlot(objAddr, offset);
            if (child != 0 && backend_.isHeapPointer &&
                backend_.isHeapPointer(child)) {
                markCb(child);
                stats_.slotsScanned++;
            }
        }

        // Visit dynamic slots (property storage, array elements).
        for (size_t i = 0; i < layout.dynamicSlotCount; i++) {
            size_t offset = layout.dynamicSlotOffset + i * 8;
            uintptr_t child = backend_.readSlot(objAddr, offset);
            if (child != 0 && backend_.isHeapPointer &&
                backend_.isHeapPointer(child)) {
                markCb(child);
                stats_.slotsScanned++;
            }
        }
    }

    // Visit a range of objects (e.g., array elements).
    void visitRange(uintptr_t base, size_t count, size_t stride,
                     MarkCallback markCb) {
        for (size_t i = 0; i < count; i++) {
            uintptr_t addr = base + i * stride;
            if (addr != 0 && backend_.isHeapPointer &&
                backend_.isHeapPointer(addr)) {
                markCb(addr);
                stats_.slotsScanned++;
            }
        }
    }

    struct Stats {
        uint64_t objectsVisited;
        uint64_t slotsScanned;
    };

    const Stats& stats() const { return stats_; }
    void resetStats() { stats_ = {}; }

private:
    Backend backend_;
    Stats stats_{};
};

} // namespace Zepra::Heap
