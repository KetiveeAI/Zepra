// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_vm_hooks.cpp — VM opcode hooks for GC integration

#include <atomic>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Runtime {

// Hooks injected into the VM dispatch loop for GC integration.
// The VM calls these at specific points so the GC can maintain
// invariants without modifying the VM dispatch directly.

class GCVMHooks {
public:
    struct HookSet {
        // Called before each allocation opcode (OP_NEW_OBJECT etc).
        std::function<void(size_t bytes, uint8_t typeTag)> preAllocate;
        // Called after successful allocation.
        std::function<void(uintptr_t addr, size_t bytes)> postAllocate;
        // Called on OP_SET_PROPERTY / OP_SET_ELEMENT (write barrier).
        std::function<void(uintptr_t obj, uintptr_t oldRef, uintptr_t newRef)> onStore;
        // Called at loop back-edges for safepoint polling.
        std::function<bool()> safepointPoll;
        // Called on OP_CALL (track stack depth for root scanning).
        std::function<void(uintptr_t framePtr)> onCall;
        // Called on OP_RETURN (stack unwinding).
        std::function<void()> onReturn;
        // Called every N instructions for incremental GC budget.
        std::function<void(uint32_t instructionCount)> instructionTick;
        // Called on exception throw (may need deopt + GC).
        std::function<void()> onException;
    };

    void setHooks(HookSet h) { hooks_ = std::move(h); }

    // Inline-friendly polling — the VM calls this at back-edges.
    bool pollSafepoint() {
        if (hooks_.safepointPoll) return hooks_.safepointPoll();
        return false;
    }

    void notifyPreAllocate(size_t bytes, uint8_t typeTag) {
        if (hooks_.preAllocate) hooks_.preAllocate(bytes, typeTag);
    }

    void notifyPostAllocate(uintptr_t addr, size_t bytes) {
        if (hooks_.postAllocate) hooks_.postAllocate(addr, bytes);
        bytesAllocated_ += bytes;
        allocCount_++;

        // Trigger incremental GC step every 4KB allocated.
        if (bytesAllocated_ >= incrementalBudget_) {
            if (hooks_.instructionTick) {
                hooks_.instructionTick(static_cast<uint32_t>(allocCount_));
            }
            bytesAllocated_ = 0;
        }
    }

    void notifyStore(uintptr_t obj, uintptr_t oldRef, uintptr_t newRef) {
        if (hooks_.onStore) hooks_.onStore(obj, oldRef, newRef);
    }

    void notifyCall(uintptr_t framePtr) {
        if (hooks_.onCall) hooks_.onCall(framePtr);
    }

    void notifyReturn() {
        if (hooks_.onReturn) hooks_.onReturn();
    }

    void notifyException() {
        if (hooks_.onException) hooks_.onException();
    }

    void setIncrementalBudget(size_t bytes) { incrementalBudget_ = bytes; }

private:
    HookSet hooks_;
    size_t bytesAllocated_ = 0;
    uint64_t allocCount_ = 0;
    size_t incrementalBudget_ = 4096;
};

} // namespace Zepra::Runtime
