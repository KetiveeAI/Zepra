/**
 * @file TaskRunner.h
 * @brief Event Loop Integration
 * 
 * Browser event loop integration:
 * - Microtask/macrotask queues
 * - Idle task scheduling
 * - Promise resolution
 */

#pragma once

#include <functional>
#include <queue>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

namespace Zepra {

// =============================================================================
// Task Types
// =============================================================================

using Task = std::function<void()>;

enum class TaskPriority : uint8_t {
    Microtask,      // Promise reactions, queueMicrotask
    UserBlocking,   // User input, animations
    UserVisible,    // Rendering, layout
    Background,     // Prefetch, analytics
    Idle            // Run when idle
};

/**
 * @brief Delayed task with deadline
 */
struct DelayedTask {
    Task task;
    std::chrono::steady_clock::time_point runAt;
    TaskPriority priority;
    
    bool operator>(const DelayedTask& other) const {
        return runAt > other.runAt;
    }
};

// =============================================================================
// Microtask Queue
// =============================================================================

/**
 * @brief Queue for microtasks (Promise, queueMicrotask)
 */
class MicrotaskQueue {
public:
    void Enqueue(Task task) {
        std::lock_guard lock(mutex_);
        queue_.push(std::move(task));
    }
    
    void RunAll() {
        while (true) {
            Task task;
            {
                std::lock_guard lock(mutex_);
                if (queue_.empty()) break;
                task = std::move(queue_.front());
                queue_.pop();
            }
            task();
        }
    }
    
    bool IsEmpty() const {
        std::lock_guard lock(mutex_);
        return queue_.empty();
    }
    
    size_t Size() const {
        std::lock_guard lock(mutex_);
        return queue_.size();
    }
    
private:
    mutable std::mutex mutex_;
    std::queue<Task> queue_;
};

// =============================================================================
// Task Queue
// =============================================================================

/**
 * @brief Priority-based task queue
 */
class TaskQueue {
public:
    void Post(Task task, TaskPriority priority = TaskPriority::UserVisible) {
        std::lock_guard lock(mutex_);
        queues_[static_cast<size_t>(priority)].push(std::move(task));
    }
    
    void PostDelayed(Task task, std::chrono::milliseconds delay,
                     TaskPriority priority = TaskPriority::UserVisible) {
        DelayedTask dt;
        dt.task = std::move(task);
        dt.runAt = std::chrono::steady_clock::now() + delay;
        dt.priority = priority;
        
        std::lock_guard lock(mutex_);
        delayedQueue_.push(std::move(dt));
    }
    
    // Run one task from highest priority queue
    bool RunOne() {
        // Check delayed tasks first
        ProcessDelayedTasks();
        
        std::lock_guard lock(mutex_);
        
        for (size_t i = 0; i < kNumPriorities; i++) {
            if (!queues_[i].empty()) {
                Task task = std::move(queues_[i].front());
                queues_[i].pop();
                
                // Unlock before running
                mutex_.unlock();
                task();
                mutex_.lock();
                
                return true;
            }
        }
        return false;
    }
    
    bool IsEmpty() const {
        std::lock_guard lock(mutex_);
        for (const auto& q : queues_) {
            if (!q.empty()) return false;
        }
        return delayedQueue_.empty();
    }
    
private:
    void ProcessDelayedTasks() {
        auto now = std::chrono::steady_clock::now();
        
        std::lock_guard lock(mutex_);
        while (!delayedQueue_.empty() && delayedQueue_.top().runAt <= now) {
            DelayedTask dt = std::move(const_cast<DelayedTask&>(delayedQueue_.top()));
            delayedQueue_.pop();
            queues_[static_cast<size_t>(dt.priority)].push(std::move(dt.task));
        }
    }
    
    static constexpr size_t kNumPriorities = 5;
    mutable std::mutex mutex_;
    std::queue<Task> queues_[kNumPriorities];
    std::priority_queue<DelayedTask, std::vector<DelayedTask>, std::greater<>> delayedQueue_;
};

// =============================================================================
// Idle Task Runner
// =============================================================================

/**
 * @brief Run tasks during idle time
 */
class IdleTaskRunner {
public:
    struct IdleDeadline {
        std::chrono::steady_clock::time_point deadline;
        bool didTimeout;
        
        std::chrono::milliseconds TimeRemaining() const {
            auto now = std::chrono::steady_clock::now();
            if (now >= deadline) return std::chrono::milliseconds(0);
            return std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        }
    };
    
    using IdleCallback = std::function<void(const IdleDeadline&)>;
    
    void PostIdleTask(IdleCallback callback, 
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
        std::lock_guard lock(mutex_);
        idleTasks_.push({std::move(callback), timeout});
    }
    
    // Called by browser when idle time available
    void RunIdleTasks(std::chrono::milliseconds idleBudget) {
        auto deadline = std::chrono::steady_clock::now() + idleBudget;
        
        while (std::chrono::steady_clock::now() < deadline) {
            IdleCallback callback;
            {
                std::lock_guard lock(mutex_);
                if (idleTasks_.empty()) break;
                callback = std::move(idleTasks_.front().callback);
                idleTasks_.pop();
            }
            
            IdleDeadline id{deadline, false};
            callback(id);
        }
    }
    
private:
    struct IdleTask {
        IdleCallback callback;
        std::chrono::milliseconds timeout;
    };
    
    mutable std::mutex mutex_;
    std::queue<IdleTask> idleTasks_;
};

// =============================================================================
// Event Loop Integration
// =============================================================================

/**
 * @brief Main event loop integration for browser
 */
class EventLoopRunner {
public:
    EventLoopRunner() = default;
    
    // Post tasks
    void PostTask(Task task) {
        taskQueue_.Post(std::move(task));
    }
    
    void PostDelayedTask(Task task, std::chrono::milliseconds delay) {
        taskQueue_.PostDelayed(std::move(task), delay);
    }
    
    void PostMicrotask(Task task) {
        microtaskQueue_.Enqueue(std::move(task));
    }
    
    void PostIdleTask(IdleTaskRunner::IdleCallback callback) {
        idleRunner_.PostIdleTask(std::move(callback));
    }
    
    // Run one iteration of event loop
    void RunOnce() {
        // 1. Run all microtasks
        microtaskQueue_.RunAll();
        
        // 2. Run one macrotask
        taskQueue_.RunOne();
        
        // 3. Microtasks again (may have been queued)
        microtaskQueue_.RunAll();
    }
    
    // Run until no more tasks
    void RunUntilEmpty() {
        while (!taskQueue_.IsEmpty() || !microtaskQueue_.IsEmpty()) {
            RunOnce();
        }
    }
    
    // Run with idle time
    void RunWithIdleTime(std::chrono::milliseconds idleBudget) {
        RunOnce();
        
        if (taskQueue_.IsEmpty() && microtaskQueue_.IsEmpty()) {
            idleRunner_.RunIdleTasks(idleBudget);
        }
    }
    
    // Pending work check
    bool HasPendingWork() const {
        return !taskQueue_.IsEmpty() || !microtaskQueue_.IsEmpty();
    }
    
    // Shutdown
    void Shutdown() {
        shutdown_.store(true);
    }
    
    bool IsShuttingDown() const {
        return shutdown_.load();
    }
    
private:
    TaskQueue taskQueue_;
    MicrotaskQueue microtaskQueue_;
    IdleTaskRunner idleRunner_;
    std::atomic<bool> shutdown_{false};
};

// =============================================================================
// Global Event Loop Access
// =============================================================================

/**
 * @brief Get the current thread's event loop
 */
EventLoopRunner* GetCurrentEventLoop();

/**
 * @brief Set the current thread's event loop
 */
void SetCurrentEventLoop(EventLoopRunner* loop);

// =============================================================================
// Promise Resolution
// =============================================================================

/**
 * @brief Helper for Promise microtask scheduling
 */
class PromiseResolutionScope {
public:
    explicit PromiseResolutionScope(EventLoopRunner* loop) : loop_(loop) {}
    
    ~PromiseResolutionScope() {
        // Drain microtasks after promise operations
        if (loop_) {
            // Would run microtasks
        }
    }
    
private:
    EventLoopRunner* loop_;
};

} // namespace Zepra
