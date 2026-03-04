/**
 * @file finalizer.cpp
 * @brief Weak References and Background Finalization Registry
 *
 * Implements the ES2024 WeakRef and FinalizationRegistry proposals.
 * Provides a Background Finalizer thread that invokes JS callbacks
 * when target objects are garbage collected.
 */

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <cstdio>

namespace Zepra::GC {

// =============================================================================
// Finalization Tasks
// =============================================================================

/**
 * Represents a pending finalization task.
 * When `target` is garbage collected, `callback` should be invoked with `heldValue`.
 */
struct FinalizationRecord {
    uint32_t id;
    uintptr_t targetAddr;  // The object being watched
    uint64_t heldValue;    // The value passed to the callback (NaN-boxed)
    std::function<void(uint64_t)> callback;
};

// =============================================================================
// Background Finalizer Thread
// =============================================================================

/**
 * Runs on a background thread.
 * Wakes up after a GC cycle to process any records whose targets died.
 */
class BackgroundFinalizer {
public:
    static BackgroundFinalizer& instance() {
        static BackgroundFinalizer inst;
        return inst;
    }

    BackgroundFinalizer(const BackgroundFinalizer&) = delete;
    void operator=(const BackgroundFinalizer&) = delete;

    ~BackgroundFinalizer() {
        stop();
    }

    /**
     * Register an object for finalization.
     */
    uint32_t registerTarget(uintptr_t target, uint64_t heldValue, std::function<void(uint64_t)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = nextId_++;
        records_.push_back({id, target, heldValue, callback});
        return id;
    }

    /**
     * Unregister an object from finalization.
     */
    bool unregister(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = records_.begin(); it != records_.end(); ++it) {
            if (it->id == id) {
                records_.erase(it);
                return true;
            }
        }
        return false;
    }

    /**
     * Called by the GC at the end of a cycle.
     * Passes a list of object addresses that were collected.
     */
    void notifyGarbageCollected(const std::vector<uintptr_t>& deadObjects) {
        std::vector<FinalizationRecord> readyToRun;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = records_.begin();
            while (it != records_.end()) {
                // If the target is in the dead list, extract it for processing
                bool found = false;
                for (uintptr_t dead : deadObjects) {
                    if (it->targetAddr == dead) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    readyToRun.push_back(*it);
                    it = records_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        if (!readyToRun.empty()) {
            std::lock_guard<std::mutex> lock(queueMutex_);
            pendingTasks_.insert(pendingTasks_.end(), readyToRun.begin(), readyToRun.end());
            cv_.notify_one();
        }
    }

    /**
     * Start the background thread.
     */
    void start() {
        if (running_) return;
        running_ = true;
        worker_ = std::thread(&BackgroundFinalizer::runLoop, this);
    }

    /**
     * Stop the background thread.
     */
    void stop() {
        if (!running_) return;
        running_ = false;
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

private:
    BackgroundFinalizer() : running_(false), nextId_(1) {
        start();
    }

    void runLoop() {
        while (running_) {
            std::vector<FinalizationRecord> tasksToRun;

            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                cv_.wait(lock, [this]() { return !pendingTasks_.empty() || !running_; });
                
                if (!running_ && pendingTasks_.empty()) {
                    break;
                }
                
                tasksToRun.swap(pendingTasks_);
            }

            // Execute callbacks outside the lock
            for (const auto& task : tasksToRun) {
                try {
                    task.callback(task.heldValue);
                } catch (...) {
                    // In a real engine, we'd report this to an unhandled rejection
                    // handler. For now, we catch to prevent the finalizer thread from crashing.
                    fprintf(stderr, "Exception in FinalizationRegistry callback\n");
                }
            }
        }
    }

    std::mutex mutex_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    std::thread worker_;
    std::atomic<bool> running_;
    
    uint32_t nextId_;
    std::vector<FinalizationRecord> records_;
    std::vector<FinalizationRecord> pendingTasks_;
};

// =============================================================================
// WeakRef Implementation
// =============================================================================

/**
 * Represents a WeakRef object in JS.
 * Weak references do not prevent an object from being garbage collected.
 */
class WeakRefNode {
public:
    explicit WeakRefNode(uintptr_t target) : target_(target), isDead_(false) {}

    /**
     * Returns the target if it is still alive, otherwise returns 0 (null/undefined).
     */
    uintptr_t deref() const {
        if (isDead_) return 0;
        return target_;
    }

    /**
     * Called by GC during sweep phase.
     */
    void markDead() {
        isDead_ = true;
        target_ = 0;
    }

    uintptr_t getAddress() const { return target_; }

private:
    uintptr_t target_;
    bool isDead_;
};

// A global registry for WeakRefs to coordinate with the GC
class WeakRefRegistry {
public:
    static WeakRefRegistry& instance() {
        static WeakRefRegistry inst;
        return inst;
    }

    void add(WeakRefNode* node) {
        std::lock_guard<std::mutex> lock(mutex_);
        weakRefs_.push_back(node);
    }

    void remove(WeakRefNode* node) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find(weakRefs_.begin(), weakRefs_.end(), node);
        if (it != weakRefs_.end()) {
            weakRefs_.erase(it);
        }
    }

    /**
     * Process all weak refs after marking phase.
     * Any weak ref pointing to an unmarked object is cleared.
     */
    void clearDeadReferences(const std::vector<uintptr_t>& deadObjects) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto* node : weakRefs_) {
            uintptr_t target = node->getAddress();
            for (uintptr_t dead : deadObjects) {
                if (target == dead) {
                    node->markDead();
                    break;
                }
            }
        }
    }

private:
    WeakRefRegistry() = default;
    
    std::mutex mutex_;
    std::vector<WeakRefNode*> weakRefs_;
};

} // namespace Zepra::GC
