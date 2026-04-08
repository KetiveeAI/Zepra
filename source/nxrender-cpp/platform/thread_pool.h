// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file thread_pool.h
 * @brief Fixed-size worker thread pool with priority task queue.
 *
 * Used by async image decode, tile rasterization, and font loading.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <future>

namespace NXRender {

/**
 * @brief Task priority levels.
 */
enum class TaskPriority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief Fixed-size worker thread pool.
 */
class ThreadPool {
public:
    /**
     * @brief Get the global thread pool instance.
     * Initializes with hardware_concurrency - 1 threads (min 2).
     */
    static ThreadPool& instance();

    /**
     * @brief Create a thread pool with a specific number of workers.
     */
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Submit a task to the pool.
     * @return Future for the task result.
     */
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        return submitWithPriority(TaskPriority::Normal, std::forward<F>(f), std::forward<Args>(args)...);
    }

    /**
     * @brief Submit a task with a specific priority.
     */
    template <typename F, typename... Args>
    auto submitWithPriority(TaskPriority priority, F&& f, Args&&... args)
        -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReturnType> result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (stopped_) return result;

            taskQueue_.push({priority, [task]() { (*task)(); }});
        }

        condition_.notify_one();
        return result;
    }

    /**
     * @brief Submit a fire-and-forget task.
     */
    void submitVoid(std::function<void()> task, TaskPriority priority = TaskPriority::Normal);

    /**
     * @brief Number of worker threads.
     */
    size_t workerCount() const { return workers_.size(); }

    /**
     * @brief Number of pending tasks in the queue.
     */
    size_t pendingTasks() const;

    /**
     * @brief Number of tasks currently being executed.
     */
    size_t activeTasks() const { return activeTasks_.load(); }

    /**
     * @brief Wait for all pending tasks to complete.
     */
    void waitAll();

    /**
     * @brief Shutdown the pool. Waits for active tasks to finish.
     */
    void shutdown();

    bool isShutdown() const { return stopped_; }

private:
    void workerLoop();

    struct Task {
        TaskPriority priority;
        std::function<void()> func;

        bool operator<(const Task& other) const {
            return static_cast<uint8_t>(priority) < static_cast<uint8_t>(other.priority);
        }
    };

    std::vector<std::thread> workers_;
    std::priority_queue<Task> taskQueue_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::condition_variable allDone_;
    std::atomic<bool> stopped_{false};
    std::atomic<size_t> activeTasks_{0};
};

} // namespace NXRender
