// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_handle_table.cpp — Persistent and weak handle management

#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>

namespace Zepra::Heap {

// Handles allow native (C++) code to hold references to JS heap objects
// that survive GC. Two kinds:
// - Persistent: prevents collection (strong ref)
// - Weak: allows collection, gets nulled after GC

enum class HandleType : uint8_t {
    Persistent,
    Weak
};

struct HandleEntry {
    uintptr_t* location;
    HandleType type;
    uint32_t id;
    bool alive;

    HandleEntry()
        : location(nullptr), type(HandleType::Persistent)
        , id(0), alive(true) {}
};

class HandleTable {
public:
    uint32_t createPersistent(uintptr_t* location) {
        return create(location, HandleType::Persistent);
    }

    uint32_t createWeak(uintptr_t* location) {
        return create(location, HandleType::Weak);
    }

    void destroy(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& h : handles_) {
            if (h.id == id) { h.alive = false; return; }
        }
    }

    // Called by GC: enumerate persistent handles as roots.
    void enumeratePersistent(std::function<void(uintptr_t)> visitor) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& h : handles_) {
            if (h.alive && h.type == HandleType::Persistent &&
                h.location && *h.location != 0) {
                visitor(*h.location);
            }
        }
    }

    // Called by GC: null out weak handles pointing to dead objects.
    void processWeakHandles(std::function<bool(uintptr_t)> isLive) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& h : handles_) {
            if (h.alive && h.type == HandleType::Weak &&
                h.location && *h.location != 0) {
                if (!isLive(*h.location)) {
                    *h.location = 0;
                    stats_.weakCleared++;
                }
            }
        }
    }

    // Called by GC: update handle locations after compaction.
    void updateHandles(std::function<uintptr_t(uintptr_t)> forward) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& h : handles_) {
            if (h.alive && h.location && *h.location != 0) {
                uintptr_t fwd = forward(*h.location);
                if (fwd != *h.location) {
                    *h.location = fwd;
                    stats_.forwarded++;
                }
            }
        }
    }

    // Compact dead entries periodically.
    void compact() {
        std::lock_guard<std::mutex> lock(mutex_);
        handles_.erase(
            std::remove_if(handles_.begin(), handles_.end(),
                [](const HandleEntry& h) { return !h.alive; }),
            handles_.end());
    }

    size_t persistentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t c = 0;
        for (auto& h : handles_) {
            if (h.alive && h.type == HandleType::Persistent) c++;
        }
        return c;
    }

    size_t weakCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t c = 0;
        for (auto& h : handles_) {
            if (h.alive && h.type == HandleType::Weak) c++;
        }
        return c;
    }

    struct Stats { uint64_t weakCleared; uint64_t forwarded; };
    Stats stats() const { return stats_; }

private:
    uint32_t create(uintptr_t* location, HandleType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        HandleEntry h;
        h.location = location;
        h.type = type;
        h.id = nextId_++;
        h.alive = true;
        handles_.push_back(h);
        return h.id;
    }

    mutable std::mutex mutex_;
    std::vector<HandleEntry> handles_;
    uint32_t nextId_ = 1;
    Stats stats_{};
};

} // namespace Zepra::Heap
