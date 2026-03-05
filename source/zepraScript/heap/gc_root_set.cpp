// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_root_set.cpp — Explicit root set management for GC

#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>

namespace Zepra::Heap {

// The root set is the starting point for GC tracing. Roots include:
// - VM stack frames (handled by stack scanner)
// - Global object
// - Native handles (PersistentHandle API)
// - Finalizable weak refs
// This module manages the explicitly-registered roots.

enum class RootCategory : uint8_t {
    Global,
    Handle,
    InternalCache,
    JITCode,
    WASMInstance
};

struct RootEntry {
    uintptr_t* location;   // Pointer to the slot holding the heap ref
    RootCategory category;
    uint32_t id;
};

class RootSet {
public:
    uint32_t addRoot(uintptr_t* location, RootCategory cat) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = nextId_++;
        roots_.push_back({location, cat, id});
        return id;
    }

    void removeRoot(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        roots_.erase(
            std::remove_if(roots_.begin(), roots_.end(),
                [id](const RootEntry& e) { return e.id == id; }),
            roots_.end());
    }

    // Called by GC during root scanning phase.
    void enumerateRoots(std::function<void(uintptr_t)> visitor) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& entry : roots_) {
            if (entry.location && *entry.location != 0) {
                visitor(*entry.location);
            }
        }
    }

    // Update root locations after compaction.
    void updateRoots(std::function<uintptr_t(uintptr_t)> forwardingLookup) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& entry : roots_) {
            if (entry.location && *entry.location != 0) {
                uintptr_t forwarded = forwardingLookup(*entry.location);
                if (forwarded != *entry.location) {
                    *entry.location = forwarded;
                }
            }
        }
    }

    size_t rootCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return roots_.size();
    }

    size_t rootCountByCategory(RootCategory cat) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::count_if(roots_.begin(), roots_.end(),
            [cat](const RootEntry& e) { return e.category == cat; });
    }

private:
    mutable std::mutex mutex_;
    std::vector<RootEntry> roots_;
    uint32_t nextId_ = 1;
};

// Scoped root that auto-unregisters on destruction.
class ScopedRoot {
public:
    ScopedRoot(RootSet& set, uintptr_t* location, RootCategory cat)
        : set_(set) {
        id_ = set_.addRoot(location, cat);
    }

    ~ScopedRoot() { set_.removeRoot(id_); }

    ScopedRoot(const ScopedRoot&) = delete;
    ScopedRoot& operator=(const ScopedRoot&) = delete;

private:
    RootSet& set_;
    uint32_t id_;
};

} // namespace Zepra::Heap
