/**
 * @file gc_nursery.cpp
 * @brief Nursery (young generation) for fast object allocation
 *
 * The nursery is a dedicated region for new objects:
 * - Bump-pointer allocation (no free-list overhead)
 * - Collected by minor GC (scavenge)
 * - Objects surviving N collections are promoted to old gen
 *
 * Layout:
 *   ┌──────────────────────────────────────┐
 *   │ from-space                           │
 *   │ [obj][obj][obj]...[free]             │
 *   ├──────────────────────────────────────┤
 *   │ to-space (empty, used during GC)     │
 *   │                                      │
 *   └──────────────────────────────────────┘
 *
 * Scavenging (Cheney's algorithm):
 *   1. Swap from-space and to-space
 *   2. Copy roots from old from-space to to-space
 *   3. Scan copied objects, copy their children
 *   4. Promote objects that survived enough scavenges
 *   5. Old from-space is now garbage
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Zepra::Heap {

// =============================================================================
// Semi-Space (half of nursery)
// =============================================================================

class SemiSpace {
public:
    SemiSpace() : base_(nullptr), size_(0), cursor_(0) {}

    ~SemiSpace() { destroy(); }

    SemiSpace(const SemiSpace&) = delete;
    SemiSpace& operator=(const SemiSpace&) = delete;

    bool initialize(size_t size) {
        size_ = size;
#ifdef __linux__
        base_ = static_cast<uint8_t*>(
            mmap(nullptr, size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (base_ == MAP_FAILED) {
            base_ = nullptr;
            return false;
        }
#else
        base_ = static_cast<uint8_t*>(std::aligned_alloc(4096, size));
        if (!base_) return false;
#endif
        cursor_ = 0;
        return true;
    }

    void destroy() {
        if (!base_) return;
#ifdef __linux__
        munmap(base_, size_);
#else
        std::free(base_);
#endif
        base_ = nullptr;
        size_ = 0;
        cursor_ = 0;
    }

    /**
     * @brief Bump-pointer allocation (extremely fast)
     */
    uintptr_t allocate(size_t bytes) {
        bytes = (bytes + 7) & ~size_t(7);
        if (cursor_ + bytes > size_) return 0;

        uintptr_t addr = reinterpret_cast<uintptr_t>(base_) + cursor_;
        cursor_ += bytes;
        return addr;
    }

    /**
     * @brief Atomic bump-pointer (for TLAB refill)
     */
    uintptr_t allocateAtomic(size_t bytes) {
        bytes = (bytes + 7) & ~size_t(7);
        size_t old = atomicCursor_.load(std::memory_order_relaxed);
        while (true) {
            if (old + bytes > size_) return 0;
            if (atomicCursor_.compare_exchange_weak(old, old + bytes,
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
                return reinterpret_cast<uintptr_t>(base_) + old;
            }
        }
    }

    void reset() {
        cursor_ = 0;
        atomicCursor_.store(0, std::memory_order_release);
    }

    bool contains(uintptr_t addr) const {
        auto base = reinterpret_cast<uintptr_t>(base_);
        return addr >= base && addr < base + size_;
    }

    uintptr_t start() const { return reinterpret_cast<uintptr_t>(base_); }
    uintptr_t end() const { return start() + size_; }
    size_t used() const { return cursor_; }
    size_t remaining() const { return size_ - cursor_; }
    size_t capacity() const { return size_; }
    double utilization() const {
        return size_ > 0 ? static_cast<double>(cursor_) / size_ : 0;
    }

    void swapWith(SemiSpace& other) {
        std::swap(base_, other.base_);
        std::swap(size_, other.size_);
        std::swap(cursor_, other.cursor_);
        size_t tmp = atomicCursor_.load();
        atomicCursor_.store(other.atomicCursor_.load());
        other.atomicCursor_.store(tmp);
    }

private:
    uint8_t* base_;
    size_t size_;
    size_t cursor_;
    std::atomic<size_t> atomicCursor_{0};
};

// =============================================================================
// Nursery Allocator
// =============================================================================

class NurseryAllocator {
public:
    struct Config {
        size_t semiSpaceSize;
        uint8_t promotionAge;     // Age threshold for promotion
        bool zeroMemory;

        Config()
            : semiSpaceSize(2 * 1024 * 1024)
            , promotionAge(3)
            , zeroMemory(true) {}
    };

    struct Stats {
        uint64_t allocations;
        uint64_t bytesAllocated;
        uint64_t scavenges;
        uint64_t objectsCopied;
        uint64_t bytesCopied;
        uint64_t objectsPromoted;
        uint64_t bytesPromoted;
        double lastScavengeMs;
        double totalScavengeMs;
    };

    explicit NurseryAllocator(const Config& config = Config{})
        : config_(config) {}

    bool initialize() {
        if (!fromSpace_.initialize(config_.semiSpaceSize)) return false;
        if (!toSpace_.initialize(config_.semiSpaceSize)) return false;
        return true;
    }

    // -------------------------------------------------------------------------
    // Allocation
    // -------------------------------------------------------------------------

    /**
     * @brief Allocate in nursery (bump pointer, very fast)
     */
    uintptr_t allocate(size_t size) {
        size = (size + 7) & ~size_t(7);
        uintptr_t addr = fromSpace_.allocate(size);
        if (addr == 0) return 0;  // Nursery full → trigger scavenge

        if (config_.zeroMemory) {
            std::memset(reinterpret_cast<void*>(addr), 0, size);
        }

        stats_.allocations++;
        stats_.bytesAllocated += size;
        return addr;
    }

    bool isFull() const {
        return fromSpace_.remaining() < 64;
    }

    bool isInNursery(uintptr_t addr) const {
        return fromSpace_.contains(addr) || toSpace_.contains(addr);
    }

    // -------------------------------------------------------------------------
    // Scavenge (minor GC)
    // -------------------------------------------------------------------------

    using CopyCallback = std::function<uintptr_t(uintptr_t oldAddr,
                                                   size_t size)>;
    using PromoteCallback = std::function<uintptr_t(uintptr_t oldAddr,
                                                      size_t size)>;
    using RootVisitor = std::function<void(
        std::function<void(uintptr_t* rootSlot)>)>;
    using ObjectTracer = std::function<void(uintptr_t addr,
        std::function<void(uintptr_t* refSlot)>)>;
    using ObjectSizer = std::function<size_t(uintptr_t addr)>;
    using AgeGetter = std::function<uint8_t(uintptr_t addr)>;
    using AgeSetter = std::function<void(uintptr_t addr, uint8_t age)>;

    struct ScavengeCallbacks {
        RootVisitor enumerateRoots;
        ObjectTracer traceObject;
        ObjectSizer objectSize;
        PromoteCallback promote;
        AgeGetter getAge;
        AgeSetter setAge;
    };

    /**
     * @brief Run scavenge (Cheney's semi-space copying)
     */
    void scavenge(const ScavengeCallbacks& cb) {
        auto start = std::chrono::steady_clock::now();
        stats_.scavenges++;

        // to-space becomes the destination
        toSpace_.reset();

        size_t scanCursor = 0;

        // Lambda: copy one object from from-space to to-space or promote
        auto copyOrPromote = [&](uintptr_t oldAddr) -> uintptr_t {
            if (!fromSpace_.contains(oldAddr)) return oldAddr;

            // Check forwarding pointer (already copied?)
            auto* fwd = reinterpret_cast<uintptr_t*>(oldAddr);
            if (toSpace_.contains(*fwd)) return *fwd;

            size_t size = cb.objectSize ? cb.objectSize(oldAddr) : 32;

            // Check age for promotion
            uint8_t age = cb.getAge ? cb.getAge(oldAddr) : 0;
            if (age >= config_.promotionAge) {
                // Promote to old gen
                uintptr_t promoted = cb.promote ?
                    cb.promote(oldAddr, size) : 0;
                if (promoted != 0) {
                    std::memcpy(reinterpret_cast<void*>(promoted),
                                reinterpret_cast<void*>(oldAddr), size);
                    *fwd = promoted;  // Forwarding pointer
                    stats_.objectsPromoted++;
                    stats_.bytesPromoted += size;
                    return promoted;
                }
            }

            // Copy to to-space
            uintptr_t newAddr = toSpace_.allocate(size);
            if (newAddr == 0) return oldAddr;  // To-space overflow

            std::memcpy(reinterpret_cast<void*>(newAddr),
                        reinterpret_cast<void*>(oldAddr), size);
            *fwd = newAddr;  // Forwarding pointer

            // Increment age
            if (cb.setAge) cb.setAge(newAddr, age + 1);

            stats_.objectsCopied++;
            stats_.bytesCopied += size;
            return newAddr;
        };

        // Phase 1: Copy root-referenced objects
        if (cb.enumerateRoots) {
            cb.enumerateRoots([&](uintptr_t* rootSlot) {
                if (*rootSlot != 0 && fromSpace_.contains(*rootSlot)) {
                    *rootSlot = copyOrPromote(*rootSlot);
                }
            });
        }

        // Phase 2: Scan copied objects (breadth-first)
        while (scanCursor < toSpace_.used()) {
            uintptr_t objAddr = toSpace_.start() + scanCursor;
            size_t objSize = cb.objectSize ?
                cb.objectSize(objAddr) : 32;

            if (cb.traceObject) {
                cb.traceObject(objAddr, [&](uintptr_t* refSlot) {
                    if (*refSlot != 0 && fromSpace_.contains(*refSlot)) {
                        *refSlot = copyOrPromote(*refSlot);
                    }
                });
            }

            scanCursor += objSize;
        }

        // Swap spaces
        fromSpace_.swapWith(toSpace_);

        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        stats_.lastScavengeMs = ms;
        stats_.totalScavengeMs += ms;
    }

    // -------------------------------------------------------------------------
    // Stats
    // -------------------------------------------------------------------------

    const Stats& stats() const { return stats_; }
    size_t used() const { return fromSpace_.used(); }
    size_t capacity() const { return fromSpace_.capacity(); }
    double utilization() const { return fromSpace_.utilization(); }

private:
    Config config_;
    SemiSpace fromSpace_;
    SemiSpace toSpace_;
    Stats stats_{};
};

} // namespace Zepra::Heap
