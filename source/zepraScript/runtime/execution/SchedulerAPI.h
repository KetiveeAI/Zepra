/**
 * @file SchedulerAPI.h
 * @brief Task Scheduler Implementation
 */

#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>

namespace Zepra::Runtime {

// =============================================================================
// Task Priority
// =============================================================================

enum class TaskPriority {
    UserBlocking = 0,
    UserVisible = 1,
    Background = 2
};

// =============================================================================
// Abort Signal
// =============================================================================

class AbortSignal {
public:
    bool aborted() const { return aborted_; }
    
    void abort() {
        aborted_ = true;
        if (onAbort_) onAbort_();
    }
    
    void onAbort(std::function<void()> handler) { onAbort_ = std::move(handler); }

private:
    std::atomic<bool> aborted_{false};
    std::function<void()> onAbort_;
};

class AbortController {
public:
    AbortController() : signal_(std::make_shared<AbortSignal>()) {}
    
    std::shared_ptr<AbortSignal> signal() const { return signal_; }
    void abort() { signal_->abort(); }

private:
    std::shared_ptr<AbortSignal> signal_;
};

// =============================================================================
// Task Options
// =============================================================================

struct TaskOptions {
    TaskPriority priority = TaskPriority::UserVisible;
    std::shared_ptr<AbortSignal> signal;
    std::chrono::milliseconds delay{0};
};

// =============================================================================
// Task Handle
// =============================================================================

class TaskHandle {
public:
    virtual ~TaskHandle() = default;
    virtual bool isPending() const = 0;
    virtual bool isDone() const = 0;
    virtual void cancel() = 0;
};

// =============================================================================
// Scheduler
// =============================================================================

class Scheduler {
public:
    using Task = std::function<void()>;
    
    static Scheduler& instance() {
        static Scheduler scheduler;
        return scheduler;
    }
    
    std::shared_ptr<TaskHandle> postTask(Task task, TaskOptions options = {}) {
        auto handle = std::make_shared<TaskHandleImpl>(std::move(task), options.signal);
        
        if (options.delay.count() > 0) {
            std::thread([this, handle, delay = options.delay, priority = options.priority]() {
                std::this_thread::sleep_for(delay);
                enqueue(handle, priority);
            }).detach();
        } else {
            enqueue(handle, options.priority);
        }
        
        return handle;
    }
    
    void yield() {
        std::this_thread::yield();
    }
    
    void run() {
        while (!stopped_) {
            std::shared_ptr<TaskHandleImpl> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this]() { return stopped_ || !isEmpty(); });
                
                if (stopped_) return;
                task = dequeue();
            }
            
            if (task && !task->isCancelled()) {
                task->execute();
            }
        }
    }
    
    void stop() {
        stopped_ = true;
        cv_.notify_all();
    }

private:
    class TaskHandleImpl : public TaskHandle {
    public:
        TaskHandleImpl(Task task, std::shared_ptr<AbortSignal> signal)
            : task_(std::move(task)), signal_(std::move(signal)) {
            if (signal_) {
                signal_->onAbort([this]() { cancel(); });
            }
        }
        
        bool isPending() const override { return !done_ && !cancelled_; }
        bool isDone() const override { return done_; }
        bool isCancelled() const { return cancelled_; }
        
        void cancel() override { cancelled_ = true; }
        
        void execute() {
            if (cancelled_) return;
            if (signal_ && signal_->aborted()) return;
            task_();
            done_ = true;
        }

    private:
        Task task_;
        std::shared_ptr<AbortSignal> signal_;
        std::atomic<bool> done_{false};
        std::atomic<bool> cancelled_{false};
    };
    
    void enqueue(std::shared_ptr<TaskHandleImpl> task, TaskPriority priority) {
        std::lock_guard<std::mutex> lock(mutex_);
        queues_[static_cast<int>(priority)].push(std::move(task));
        cv_.notify_one();
    }
    
    std::shared_ptr<TaskHandleImpl> dequeue() {
        for (int i = 0; i < 3; ++i) {
            if (!queues_[i].empty()) {
                auto task = queues_[i].front();
                queues_[i].pop();
                return task;
            }
        }
        return nullptr;
    }
    
    bool isEmpty() const {
        for (int i = 0; i < 3; ++i) {
            if (!queues_[i].empty()) return false;
        }
        return true;
    }
    
    std::queue<std::shared_ptr<TaskHandleImpl>> queues_[3];
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stopped_{false};
};

// =============================================================================
// Global Functions
// =============================================================================

inline std::shared_ptr<TaskHandle> postTask(Scheduler::Task task, TaskOptions options = {}) {
    return Scheduler::instance().postTask(std::move(task), std::move(options));
}

inline void schedulerYield() {
    Scheduler::instance().yield();
}

} // namespace Zepra::Runtime
