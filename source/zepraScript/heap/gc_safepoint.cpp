/**
 * @file gc_safepoint.cpp
 * @brief Safe-point infrastructure for GC synchronization
 *
 * Safe-points are locations in code where a thread can be safely
 * stopped for GC. At a safe-point, all heap references are in
 * known locations (described by stack maps).
 *
 * Mechanisms:
 *
 * 1. Polling safe-points:
 *    JIT and interpreter periodically read a "safepoint page".
 *    When GC wants to stop threads, it mprotects this page
 *    to trigger SIGSEGV → signal handler parks the thread.
 *
 * 2. Voluntary safe-points:
 *    Threads call checkSafepoint() at function calls,
 *    loop back-edges, and allocation slow paths.
 *
 * 3. Thread-local safe-point state:
 *    Each thread has a SafePointState with:
 *    - Current phase (running, parked, in-native)
 *    - Stack top/bottom for conservative scanning
 *    - Register save area for precise scanning
 *
 * 4. Safe-point synchronization:
 *    The GC coordinator requests a "global safe-point".
 *    All threads must reach a safe-point before GC begins.
 *    This uses a phaser barrier (threads count down).
 *
 * 5. Biased handshake:
 *    For operations that need only one thread to stop
 *    (e.g., deoptimization), use per-thread handshake
 *    instead of global stop-the-world.
 */

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <functional>
#include <chrono>
#include <csignal>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#endif

namespace Zepra::Heap {

// =============================================================================
// Thread Safe-Point State
// =============================================================================

enum class ThreadPhase : uint8_t {
    Running,        // Executing JS code (must be stopped for GC)
    Parked,         // Stopped at safe-point (GC can proceed)
    InNative,       // Executing native code (GC can proceed)
    Blocked,        // Waiting on lock/IO (GC can proceed)
    Transitioning,  // Moving between phases
};

struct SafePointState {
    std::atomic<ThreadPhase> phase{ThreadPhase::Running};

    // Stack bounds for conservative scanning
    uintptr_t stackTop = 0;
    uintptr_t stackBottom = 0;

    // Register save area (filled when parking)
    static constexpr size_t MAX_REGISTERS = 32;
    uint64_t savedRegisters[MAX_REGISTERS] = {};
    size_t savedRegisterCount = 0;

    // Thread ID
    uint32_t threadId = 0;
    std::thread::id nativeId;

    // Per-thread handshake flag
    std::atomic<bool> handshakeRequested{false};
    std::function<void()> handshakeCallback;

    // Stats
    uint64_t parkCount = 0;
    uint64_t totalParkNs = 0;

    bool isAtSafePoint() const {
        auto p = phase.load(std::memory_order_acquire);
        return p == ThreadPhase::Parked ||
               p == ThreadPhase::InNative ||
               p == ThreadPhase::Blocked;
    }
};

// =============================================================================
// Safe-Point Page (polling mechanism)
// =============================================================================

/**
 * @brief Memory page used for polling safe-points
 *
 * When GC wants to stop threads:
 * 1. mprotect the page to PROT_NONE
 * 2. Threads polling this page get SIGSEGV
 * 3. Signal handler recognizes the address → parks thread
 * 4. After GC, mprotect back to PROT_READ
 */
class SafePointPage {
public:
    SafePointPage() : page_(nullptr), pageSize_(0), armed_(false) {}

    ~SafePointPage() { destroy(); }

    SafePointPage(const SafePointPage&) = delete;
    SafePointPage& operator=(const SafePointPage&) = delete;

    bool initialize() {
#ifdef __linux__
        pageSize_ = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        page_ = mmap(nullptr, pageSize_,
                      PROT_READ,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1, 0);
        if (page_ == MAP_FAILED) {
            page_ = nullptr;
            return false;
        }
        return true;
#else
        pageSize_ = 4096;
        page_ = std::aligned_alloc(pageSize_, pageSize_);
        return page_ != nullptr;
#endif
    }

    void destroy() {
        if (!page_) return;
#ifdef __linux__
        munmap(page_, pageSize_);
#else
        std::free(page_);
#endif
        page_ = nullptr;
    }

    /**
     * @brief Arm the safe-point (threads will trap on poll)
     */
    bool arm() {
        if (!page_ || armed_) return false;
#ifdef __linux__
        int result = mprotect(page_, pageSize_, PROT_NONE);
        if (result != 0) return false;
#endif
        armed_ = true;
        return true;
    }

    /**
     * @brief Disarm the safe-point (threads can poll freely)
     */
    bool disarm() {
        if (!page_ || !armed_) return false;
#ifdef __linux__
        int result = mprotect(page_, pageSize_, PROT_READ);
        if (result != 0) return false;
#endif
        armed_ = false;
        return true;
    }

    /**
     * @brief Poll the safe-point (called by JIT/interpreter)
     *
     * If armed, this will either fault (SIGSEGV) or be a no-op
     * depending on the platform's mprotect implementation.
     */
    void poll() const {
        if (page_) {
            volatile uint8_t val = *static_cast<volatile uint8_t*>(page_);
            (void)val;
        }
    }

    /**
     * @brief Check if an address is the safe-point page
     */
    bool isInPage(uintptr_t addr) const {
        if (!page_) return false;
        auto base = reinterpret_cast<uintptr_t>(page_);
        return addr >= base && addr < base + pageSize_;
    }

    void* pageAddress() const { return page_; }
    bool isArmed() const { return armed_; }

private:
    void* page_;
    size_t pageSize_;
    bool armed_;
};

// =============================================================================
// Safe-Point Coordinator
// =============================================================================

/**
 * @brief Coordinates global and per-thread safe-points
 *
 * Manages the stop-the-world protocol:
 * 1. requestStop() → arm safe-point page
 * 2. Wait for all threads to park
 * 3. GC runs
 * 4. resumeAll() → disarm page, wake threads
 */
class SafePointCoordinator {
public:
    struct Config {
        size_t maxThreads;
        uint64_t stopTimeoutMs;    // Max time to wait for threads

        Config()
            : maxThreads(256)
            , stopTimeoutMs(5000) {}
    };

    struct Stats {
        uint64_t totalStops;
        uint64_t totalResumes;
        uint64_t stopTimeoutCount;
        double lastStopMs;
        double maxStopMs;
        double avgStopMs;
    };

    explicit SafePointCoordinator(const Config& config = Config{})
        : config_(config)
        , stopRequested_(false) {}

    bool initialize() {
        return safepointPage_.initialize();
    }

    // -------------------------------------------------------------------------
    // Thread registration
    // -------------------------------------------------------------------------

    SafePointState* registerThread(uint32_t threadId) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto state = std::make_unique<SafePointState>();
        state->threadId = threadId;
        state->nativeId = std::this_thread::get_id();
        state->phase.store(ThreadPhase::Running, std::memory_order_release);

        SafePointState* ptr = state.get();
        threads_.push_back(std::move(state));

        return ptr;
    }

    void unregisterThread(uint32_t threadId) {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_.erase(
            std::remove_if(threads_.begin(), threads_.end(),
                [threadId](const std::unique_ptr<SafePointState>& s) {
                    return s->threadId == threadId;
                }),
            threads_.end());
    }

    // -------------------------------------------------------------------------
    // Global stop-the-world
    // -------------------------------------------------------------------------

    /**
     * @brief Request all threads to stop at safe-points
     */
    bool requestStop() {
        auto start = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopRequested_) return false;
            stopRequested_ = true;
        }

        // Arm the polling page
        safepointPage_.arm();

        // Wait for all threads to park
        bool allParked = waitForAllParked();

        double elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        stats_.totalStops++;
        stats_.lastStopMs = elapsed;
        if (elapsed > stats_.maxStopMs) stats_.maxStopMs = elapsed;

        // Running average
        stats_.avgStopMs = stats_.avgStopMs * 0.9 + elapsed * 0.1;

        if (!allParked) {
            stats_.stopTimeoutCount++;
        }

        return allParked;
    }

    /**
     * @brief Resume all parked threads
     */
    void resumeAll() {
        safepointPage_.disarm();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopRequested_ = false;
        }

        // Wake all parked threads
        parkCondition_.notify_all();
        stats_.totalResumes++;
    }

    // -------------------------------------------------------------------------
    // Per-thread safe-point check (called by mutator threads)
    // -------------------------------------------------------------------------

    /**
     * @brief Voluntary safe-point check
     *
     * Called at function calls, loop back-edges, allocation slow path.
     * If GC has requested a stop, parks this thread.
     */
    void checkSafepoint(SafePointState* state) {
        if (!stopRequested_.load(std::memory_order_acquire)) {
            // Check per-thread handshake
            if (state->handshakeRequested.load(std::memory_order_acquire)) {
                handleHandshake(state);
            }
            return;
        }

        parkThread(state);
    }

    /**
     * @brief Enter native code (GC can proceed without waiting)
     */
    void enterNative(SafePointState* state) {
        state->phase.store(ThreadPhase::InNative, std::memory_order_release);

        // If GC was waiting, check if we're the last thread
        if (stopRequested_.load(std::memory_order_acquire)) {
            parkCondition_.notify_all();
        }
    }

    /**
     * @brief Leave native code (back to running JS)
     */
    void leaveNative(SafePointState* state) {
        state->phase.store(ThreadPhase::Running, std::memory_order_release);

        // If GC is active, we must park
        if (stopRequested_.load(std::memory_order_acquire)) {
            parkThread(state);
        }
    }

    // -------------------------------------------------------------------------
    // Per-thread handshake
    // -------------------------------------------------------------------------

    /**
     * @brief Request a single thread to execute a callback
     *
     * The callback runs at the thread's next safe-point.
     * Used for deoptimization, IC patching, etc.
     */
    void requestHandshake(SafePointState* state,
                           std::function<void()> callback) {
        state->handshakeCallback = std::move(callback);
        state->handshakeRequested.store(true, std::memory_order_release);
    }

    // -------------------------------------------------------------------------
    // Polling
    // -------------------------------------------------------------------------

    /**
     * @brief Get the poll address for JIT code generation
     *
     * JIT inserts: load [this address]; at safe-points.
     */
    void* pollAddress() const { return safepointPage_.pageAddress(); }

    /**
     * @brief Check if an address is the safe-point page (for signal handler)
     */
    bool isSafepointFault(uintptr_t faultAddr) const {
        return safepointPage_.isInPage(faultAddr);
    }

    Stats stats() const { return stats_; }

    size_t threadCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return threads_.size();
    }

private:
    void parkThread(SafePointState* state) {
        auto parkStart = std::chrono::steady_clock::now();

        state->phase.store(ThreadPhase::Parked, std::memory_order_release);
        state->parkCount++;

        // Signal that we're parked
        parkCondition_.notify_all();

        // Wait until GC completes
        {
            std::unique_lock<std::mutex> lock(parkMutex_);
            parkCondition_.wait(lock, [this]() {
                return !stopRequested_.load(std::memory_order_acquire);
            });
        }

        state->phase.store(ThreadPhase::Running, std::memory_order_release);

        uint64_t parkNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - parkStart).count();
        state->totalParkNs += parkNs;
    }

    void handleHandshake(SafePointState* state) {
        state->handshakeRequested.store(false, std::memory_order_release);
        if (state->handshakeCallback) {
            state->handshakeCallback();
            state->handshakeCallback = nullptr;
        }
    }

    bool waitForAllParked() {
        auto deadline = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(config_.stopTimeoutMs);

        std::unique_lock<std::mutex> lock(parkMutex_);
        return parkCondition_.wait_until(lock, deadline, [this]() {
            std::lock_guard<std::mutex> tlock(mutex_);
            for (const auto& t : threads_) {
                if (!t->isAtSafePoint()) return false;
            }
            return true;
        });
    }

    Config config_;
    mutable std::mutex mutex_;
    std::mutex parkMutex_;
    std::condition_variable parkCondition_;
    std::atomic<bool> stopRequested_;
    SafePointPage safepointPage_;
    std::vector<std::unique_ptr<SafePointState>> threads_;
    Stats stats_{};
};

} // namespace Zepra::Heap
