// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_heap_iterator.cpp — Safe heap walking for DevTools/snapshots

#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <unordered_map>

namespace Zepra::Heap {

// Iterates all live objects in the heap. Used by:
// - Heap snapshot (DevTools memory panel)
// - Heap verification (debug builds)
// - Object statistics collection

enum class ObjectType : uint8_t {
    JSObject, JSArray, JSFunction, JSString,
    JSSymbol, JSBigInt, JSRegExp, JSPromise,
    JSMap, JSSet, JSWeakMap, JSWeakSet,
    JSArrayBuffer, JSTypedArray, JSDataView,
    JSProxy, JSDate, JSError, JSIterator,
    InternalHidden, NativeCode, FreeSpace
};

struct HeapObjectInfo {
    uintptr_t addr;
    size_t size;
    ObjectType type;
    bool isMarked;
};

class HeapIterator {
public:
    struct Callbacks {
        std::function<size_t(uintptr_t)> objectSize;
        std::function<ObjectType(uintptr_t)> objectType;
        std::function<bool(uintptr_t)> isMarked;
        std::function<bool(uintptr_t)> isFreeSpace;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    // Iterate all objects in a contiguous region.
    size_t iterateRegion(uintptr_t start, uintptr_t end,
                          std::function<bool(const HeapObjectInfo&)> visitor) {
        if (!cb_.objectSize) return 0;
        size_t count = 0;
        uintptr_t cursor = start;

        while (cursor < end) {
            size_t size = cb_.objectSize(cursor);
            if (size == 0) break;

            if (cb_.isFreeSpace && cb_.isFreeSpace(cursor)) {
                cursor += size;
                continue;
            }

            HeapObjectInfo info;
            info.addr = cursor;
            info.size = size;
            info.type = cb_.objectType ? cb_.objectType(cursor) : ObjectType::JSObject;
            info.isMarked = cb_.isMarked ? cb_.isMarked(cursor) : false;

            if (!visitor(info)) break;  // Visitor returns false to stop.

            count++;
            cursor += size;
        }
        return count;
    }

    // Iterate all regions.
    struct RegionInfo {
        uintptr_t base;
        size_t size;
    };

    size_t iterateAll(const std::vector<RegionInfo>& regions,
                       std::function<bool(const HeapObjectInfo&)> visitor) {
        size_t total = 0;
        for (auto& r : regions) {
            total += iterateRegion(r.base, r.base + r.size, visitor);
        }
        return total;
    }

    // Collect type statistics.
    struct TypeStats {
        size_t count;
        size_t totalBytes;
    };

    std::unordered_map<uint8_t, TypeStats> collectTypeStats(
            const std::vector<RegionInfo>& regions) {
        std::unordered_map<uint8_t, TypeStats> stats;
        iterateAll(regions, [&](const HeapObjectInfo& info) -> bool {
            auto& s = stats[static_cast<uint8_t>(info.type)];
            s.count++;
            s.totalBytes += info.size;
            return true;
        });
        return stats;
    }

private:
    Callbacks cb_;
};

} // namespace Zepra::Heap
