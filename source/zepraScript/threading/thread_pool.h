/**
 * @file thread_pool.h
 * @brief Work-stealing thread pool
 */

#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <deque>
#include <atomic>
#include <future>

namespace Zepra::Threading {

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = 0);
    ~ThreadPool();

    // Non-copyable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template<typename F, typename... Args>
    auto submit(F&& func, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    void shutdown();
    void waitForAll();

    size_t numThreads() const { return workers_.size(); }
    size_t pendingTasks() const { return pendingCount_.load(); }
    bool isShutdown() const { return shutdown_.load(); }

private:
    void workerLoop(size_t threadId);

    std::vector<std::thread> workers_;
    std::deque<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::condition_variable done_;
    std::atomic<bool> shutdown_{false};
    std::atomic<size_t> pendingCount_{0};
    std::atomic<size_t> activeCount_{0};
};

} // namespace Zepra::Threading
