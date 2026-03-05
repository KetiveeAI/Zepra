// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_heap_init.cpp — Heap initialization and subsystem wiring

#include <mutex>
#include <memory>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Bootstraps the entire GC system. Called once during VM creation.
// Allocates the initial heap segments, creates nursery/old-gen/LOS,
// wires the GC controller, and registers with the runtime.

struct HeapInitConfig {
    size_t nurserySize;
    size_t oldGenInitSize;
    size_t oldGenMaxSize;
    size_t largeObjectThreshold;
    size_t tlabSize;
    uint8_t tenuringThreshold;
    bool concurrentMarking;
    bool concurrentSweeping;
    bool compactionEnabled;
    bool incrementalMarking;

    static HeapInitConfig defaults() {
        HeapInitConfig c;
        c.nurserySize = 2 * 1024 * 1024;
        c.oldGenInitSize = 8 * 1024 * 1024;
        c.oldGenMaxSize = 512 * 1024 * 1024;
        c.largeObjectThreshold = 8192;
        c.tlabSize = 32 * 1024;
        c.tenuringThreshold = 6;
        c.concurrentMarking = true;
        c.concurrentSweeping = true;
        c.compactionEnabled = true;
        c.incrementalMarking = true;
        return c;
    }
};

class HeapInitializer {
public:
    struct Callbacks {
        // Allocate a segment from the OS.
        std::function<uintptr_t(size_t)> allocateSegment;
        // Register the nursery with the runtime.
        std::function<void(uintptr_t base, size_t size)> registerNursery;
        // Register old-gen with the runtime.
        std::function<void(uintptr_t base, size_t size)> registerOldGen;
        // Set up TLAB manager.
        std::function<void(size_t tlabSize)> initTLABs;
        // Wire GC controller.
        std::function<void()> wireController;
        // Register safepoint polling page.
        std::function<void(uintptr_t addr)> registerSafepointPage;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    bool initialize(const HeapInitConfig& config) {
        config_ = config;

        // Allocate nursery segment.
        uintptr_t nurseryBase = 0;
        if (cb_.allocateSegment) {
            nurseryBase = cb_.allocateSegment(config.nurserySize);
            if (nurseryBase == 0) {
                fprintf(stderr, "[gc-init] Failed to allocate nursery\n");
                return false;
            }
        }

        if (cb_.registerNursery) {
            cb_.registerNursery(nurseryBase, config.nurserySize);
        }

        // Allocate old-gen segment.
        uintptr_t oldGenBase = 0;
        if (cb_.allocateSegment) {
            oldGenBase = cb_.allocateSegment(config.oldGenInitSize);
            if (oldGenBase == 0) {
                fprintf(stderr, "[gc-init] Failed to allocate old gen\n");
                return false;
            }
        }

        if (cb_.registerOldGen) {
            cb_.registerOldGen(oldGenBase, config.oldGenInitSize);
        }

        // Initialize TLABs.
        if (cb_.initTLABs) {
            cb_.initTLABs(config.tlabSize);
        }

        // Wire GC controller.
        if (cb_.wireController) {
            cb_.wireController();
        }

        // Allocate safepoint polling page.
        if (cb_.allocateSegment && cb_.registerSafepointPage) {
            uintptr_t safepointPage = cb_.allocateSegment(4096);
            if (safepointPage != 0) {
                cb_.registerSafepointPage(safepointPage);
            }
        }

        initialized_ = true;
        return true;
    }

    bool isInitialized() const { return initialized_; }
    const HeapInitConfig& config() const { return config_; }

private:
    Callbacks cb_;
    HeapInitConfig config_;
    bool initialized_ = false;
};

} // namespace Zepra::Heap
