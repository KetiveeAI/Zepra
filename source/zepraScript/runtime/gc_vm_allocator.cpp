// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_vm_allocator.cpp — VM-facing allocation interface

#include <atomic>
#include <mutex>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Runtime {

// This is the allocation entry point used by the VM. All object creation
// (OP_NEW_OBJECT, OP_CREATE_ARRAY, OP_CLOSURE, etc.) calls through here.
// Routes to TLAB → nursery → old-gen → large-object based on size.

enum class AllocSpace : uint8_t {
    Nursery,
    OldGen,
    LargeObject,
    Code
};

struct AllocationRequest {
    size_t bytes;
    uint8_t typeTag;       // ObjectType enum
    uint32_t siteId;       // Allocation site for pretenuring
    bool pinned;           // Pin immediately (JIT code, FFI)
    AllocSpace preferred;  // Hint — may be overridden by pretenuring

    AllocationRequest()
        : bytes(0), typeTag(0), siteId(0)
        , pinned(false), preferred(AllocSpace::Nursery) {}
};

struct AllocationResult {
    uintptr_t addr;
    AllocSpace actualSpace;
    bool triggeredGC;

    AllocationResult() : addr(0), actualSpace(AllocSpace::Nursery), triggeredGC(false) {}
};

class VMAllocator {
public:
    struct Backend {
        // TLAB fast path — no sync.
        std::function<uintptr_t(size_t)> tlabAllocate;
        // Nursery slow path — may trigger minor GC.
        std::function<uintptr_t(size_t)> nurseryAllocate;
        // Old-gen allocation.
        std::function<uintptr_t(size_t)> oldGenAllocate;
        // Large object allocation.
        std::function<uintptr_t(size_t)> largeObjectAllocate;
        // Pretenure decision.
        std::function<bool(uint32_t siteId)> shouldPretenure;
        // Write header at allocated address.
        std::function<void(uintptr_t, uint8_t typeTag, size_t bytes)> writeHeader;
        // Record allocation for telemetry.
        std::function<void(size_t bytes, AllocSpace space)> recordAllocation;
        // Force GC.
        std::function<void()> triggerGC;
    };

    void setBackend(Backend b) { backend_ = std::move(b); }

    AllocationResult allocate(const AllocationRequest& req) {
        AllocationResult result;

        // Decide space.
        AllocSpace space = decideSpace(req);
        result.actualSpace = space;

        // Try allocation.
        uintptr_t addr = tryAllocate(space, req.bytes);

        // If nursery fails, try GC then retry.
        if (addr == 0 && space == AllocSpace::Nursery) {
            if (backend_.triggerGC) {
                backend_.triggerGC();
                result.triggeredGC = true;
            }
            addr = tryAllocate(space, req.bytes);

            // Still fails — promote to old gen.
            if (addr == 0) {
                space = AllocSpace::OldGen;
                addr = tryAllocate(space, req.bytes);
                result.actualSpace = space;
            }
        }

        if (addr == 0) return result;

        // Write object header.
        if (backend_.writeHeader) {
            backend_.writeHeader(addr, req.typeTag, req.bytes);
        }

        // Record allocation.
        if (backend_.recordAllocation) {
            backend_.recordAllocation(req.bytes, space);
        }

        result.addr = addr;
        stats_.totalAllocated += req.bytes;
        stats_.allocCount++;
        if (result.triggeredGC) stats_.gcTriggers++;

        return result;
    }

    struct Stats {
        uint64_t allocCount;
        uint64_t totalAllocated;
        uint64_t gcTriggers;
        uint64_t promotions;
    };

    const Stats& stats() const { return stats_; }

private:
    AllocSpace decideSpace(const AllocationRequest& req) {
        // Large objects bypass nursery.
        if (req.bytes > 8192) return AllocSpace::LargeObject;

        // Code always goes to code space.
        if (req.preferred == AllocSpace::Code) return AllocSpace::Code;

        // Pinned objects go to old gen (can't move).
        if (req.pinned) return AllocSpace::OldGen;

        // Pretenure check.
        if (req.siteId != 0 && backend_.shouldPretenure &&
            backend_.shouldPretenure(req.siteId)) {
            stats_.promotions++;
            return AllocSpace::OldGen;
        }

        return AllocSpace::Nursery;
    }

    uintptr_t tryAllocate(AllocSpace space, size_t bytes) {
        switch (space) {
            case AllocSpace::Nursery:
                // Try TLAB first.
                if (backend_.tlabAllocate) {
                    uintptr_t addr = backend_.tlabAllocate(bytes);
                    if (addr != 0) return addr;
                }
                if (backend_.nurseryAllocate) return backend_.nurseryAllocate(bytes);
                return 0;
            case AllocSpace::OldGen:
                if (backend_.oldGenAllocate) return backend_.oldGenAllocate(bytes);
                return 0;
            case AllocSpace::LargeObject:
                if (backend_.largeObjectAllocate) return backend_.largeObjectAllocate(bytes);
                return 0;
            case AllocSpace::Code:
                if (backend_.oldGenAllocate) return backend_.oldGenAllocate(bytes);
                return 0;
        }
        return 0;
    }

    Backend backend_;
    mutable Stats stats_{};
};

} // namespace Zepra::Runtime
