// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_pressure_api.cpp — Embedder-facing memory pressure API

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>

namespace Zepra::Runtime {

// Embedder (browser, Node-like) uses this API to:
// - Notify the engine of OS memory pressure
// - Request heap size limits
// - Get allocation failure callbacks

enum class PressureLevel : uint8_t {
    None,
    Moderate,   // Trim caches, run incremental GC
    Critical    // Full GC, release pages to OS, reduce heap
};

class GCPressureAPI {
public:
    using PressureCallback = std::function<void(PressureLevel)>;
    using OOMCallback = std::function<bool(size_t requestedBytes)>;

    void notifyPressure(PressureLevel level) {
        currentLevel_.store(static_cast<uint8_t>(level),
            std::memory_order_release);

        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& cb : pressureCallbacks_) {
            try { cb(level); } catch (...) {}
        }
    }

    uint32_t onPressure(PressureCallback cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        pressureCallbacks_.push_back(std::move(cb));
        return static_cast<uint32_t>(pressureCallbacks_.size());
    }

    void setOOMHandler(OOMCallback cb) { oomHandler_ = std::move(cb); }

    // Called by allocator when allocation fails.
    bool handleOOM(size_t bytes) {
        if (oomHandler_) return oomHandler_(bytes);
        return false;
    }

    void setHeapLimit(size_t maxBytes) { heapLimit_ = maxBytes; }
    size_t heapLimit() const { return heapLimit_; }

    void setExternalMemory(size_t bytes) {
        externalMemory_.store(bytes, std::memory_order_release);
    }

    size_t externalMemory() const {
        return externalMemory_.load(std::memory_order_acquire);
    }

    void adjustExternalMemory(int64_t delta) {
        if (delta >= 0) {
            externalMemory_.fetch_add(static_cast<size_t>(delta),
                std::memory_order_relaxed);
        } else {
            size_t sub = static_cast<size_t>(-delta);
            size_t cur = externalMemory_.load(std::memory_order_relaxed);
            externalMemory_.store(cur > sub ? cur - sub : 0,
                std::memory_order_relaxed);
        }
    }

    PressureLevel currentLevel() const {
        return static_cast<PressureLevel>(
            currentLevel_.load(std::memory_order_acquire));
    }

private:
    std::atomic<uint8_t> currentLevel_{0};
    std::atomic<size_t> externalMemory_{0};
    size_t heapLimit_ = 0;
    std::mutex mutex_;
    std::vector<PressureCallback> pressureCallbacks_;
    OOMCallback oomHandler_;
};

} // namespace Zepra::Runtime
