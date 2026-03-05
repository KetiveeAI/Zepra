/**
 * @file gc_lifecycle_hooks.cpp
 * @brief Runtime-side GC lifecycle hooks
 *
 * Allows the runtime/embedder to hook into GC events:
 *   - Pre-GC: save state, flush caches, enter safepoint
 *   - Post-GC: update caches, run finalizers, adjust budgets
 *   - Allocation failure: OOM handling
 *   - Heap resize: adjust configurations
 *
 * These are the extension points for browser integration:
 *   DOM → GC integration, worker heap isolation, etc.
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <string>

namespace Zepra::Runtime {

// =============================================================================
// GC Phase
// =============================================================================

enum class GCPhase : uint8_t {
    None,
    PreMark,
    Mark,
    PostMark,
    PreSweep,
    Sweep,
    PostSweep,
    PreCompact,
    Compact,
    PostCompact
};

const char* gcPhaseName(GCPhase phase) {
    switch (phase) {
        case GCPhase::None:        return "none";
        case GCPhase::PreMark:     return "pre-mark";
        case GCPhase::Mark:        return "mark";
        case GCPhase::PostMark:    return "post-mark";
        case GCPhase::PreSweep:    return "pre-sweep";
        case GCPhase::Sweep:       return "sweep";
        case GCPhase::PostSweep:   return "post-sweep";
        case GCPhase::PreCompact:  return "pre-compact";
        case GCPhase::Compact:     return "compact";
        case GCPhase::PostCompact: return "post-compact";
    }
    return "unknown";
}

// =============================================================================
// GC Event Info (passed to hooks)
// =============================================================================

struct GCEventInfo {
    enum class Type : uint8_t {
        Minor,
        Major,
        Full,
        Compact
    };

    Type type;
    GCPhase phase;
    size_t heapUsed;
    size_t heapCapacity;
    double elapsedMs;
    size_t bytesReclaimed;
    bool isSTW;

    GCEventInfo()
        : type(Type::Minor), phase(GCPhase::None)
        , heapUsed(0), heapCapacity(0)
        , elapsedMs(0), bytesReclaimed(0), isSTW(false) {}
};

// =============================================================================
// Hook Registry
// =============================================================================

class GCHookRegistry {
public:
    using HookFn = std::function<void(const GCEventInfo&)>;

    struct Hook {
        uint32_t id;
        std::string name;
        HookFn callback;
        bool active;

        Hook() : id(0), active(false) {}
        Hook(uint32_t i, const std::string& n, HookFn cb)
            : id(i), name(n), callback(std::move(cb)), active(true) {}
    };

    uint32_t addHook(const std::string& name, HookFn callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = nextId_++;
        hooks_.push_back({id, name, std::move(callback)});
        return id;
    }

    void removeHook(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        hooks_.erase(
            std::remove_if(hooks_.begin(), hooks_.end(),
                [id](const Hook& h) { return h.id == id; }),
            hooks_.end());
    }

    void enableHook(uint32_t id, bool enable) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& h : hooks_) {
            if (h.id == id) { h.active = enable; break; }
        }
    }

    void fireAll(const GCEventInfo& info) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& h : hooks_) {
            if (h.active && h.callback) {
                try {
                    h.callback(info);
                } catch (...) {
                    fprintf(stderr, "[gc-hooks] Exception in hook '%s'\n",
                        h.name.c_str());
                }
            }
        }
    }

    size_t hookCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return hooks_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<Hook> hooks_;
    uint32_t nextId_ = 1;
};

// =============================================================================
// GC Lifecycle Manager
// =============================================================================

class GCLifecycleManager {
public:
    GCLifecycleManager() = default;

    // -------------------------------------------------------------------------
    // Hook registration (one per phase pair)
    // -------------------------------------------------------------------------

    uint32_t onPreGC(const std::string& name,
                      GCHookRegistry::HookFn fn) {
        return preGC_.addHook(name, std::move(fn));
    }

    uint32_t onPostGC(const std::string& name,
                       GCHookRegistry::HookFn fn) {
        return postGC_.addHook(name, std::move(fn));
    }

    uint32_t onAllocationFailure(const std::string& name,
                                   GCHookRegistry::HookFn fn) {
        return oomHooks_.addHook(name, std::move(fn));
    }

    uint32_t onHeapResize(const std::string& name,
                            GCHookRegistry::HookFn fn) {
        return resizeHooks_.addHook(name, std::move(fn));
    }

    // -------------------------------------------------------------------------
    // Fire events (called by GC pipeline)
    // -------------------------------------------------------------------------

    void firePreGC(const GCEventInfo& info) {
        currentPhase_.store(info.phase, std::memory_order_release);
        preGC_.fireAll(info);
    }

    void firePostGC(const GCEventInfo& info) {
        postGC_.fireAll(info);
        currentPhase_.store(GCPhase::None, std::memory_order_release);

        gcCount_++;
        totalGCMs_ += info.elapsedMs;
        totalReclaimed_ += info.bytesReclaimed;
    }

    void fireAllocationFailure(size_t requestedBytes) {
        GCEventInfo info;
        info.heapUsed = requestedBytes;
        oomHooks_.fireAll(info);
    }

    void fireHeapResize(size_t oldSize, size_t newSize) {
        GCEventInfo info;
        info.heapUsed = newSize;
        info.heapCapacity = oldSize;
        resizeHooks_.fireAll(info);
    }

    // -------------------------------------------------------------------------
    // State queries
    // -------------------------------------------------------------------------

    GCPhase currentPhase() const {
        return currentPhase_.load(std::memory_order_acquire);
    }

    bool isInGC() const {
        return currentPhase() != GCPhase::None;
    }

    uint64_t gcCount() const { return gcCount_; }
    double totalGCMs() const { return totalGCMs_; }
    size_t totalReclaimed() const { return totalReclaimed_; }

private:
    GCHookRegistry preGC_;
    GCHookRegistry postGC_;
    GCHookRegistry oomHooks_;
    GCHookRegistry resizeHooks_;

    std::atomic<GCPhase> currentPhase_{GCPhase::None};
    uint64_t gcCount_ = 0;
    double totalGCMs_ = 0;
    size_t totalReclaimed_ = 0;
};

} // namespace Zepra::Runtime
