/**
 * @file thread_pool.cpp
 * @brief Work-stealing thread pool implementation
 *
 * Fixed-size pool of worker threads with FIFO task queue.
 * Supports submit() returning std::future, graceful shutdown,
 * and waitForAll() barrier.
 *
 * Ref: Java ForkJoinPool, Tokio runtime
 */

#include "threading/thread_pool.h"
#include <cassert>
#include <stdexcept>

namespace Zepra::Threading {

ThreadPool::ThreadPool(size_t numThreads) {
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;
    }

    workers_.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ThreadPool::workerLoop, this, i);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_.load()) return;
        shutdown_.store(true);
    }
    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::waitForAll() {
    std::unique_lock<std::mutex> lock(mutex_);
    done_.wait(lock, [this] {
        return pendingCount_.load() == 0 && activeCount_.load() == 0;
    });
}

template<typename F, typename... Args>
auto ThreadPool::submit(F&& func, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    using ReturnType = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(func), std::forward<Args>(args)...)
    );

    std::future<ReturnType> future = task->get_future();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_.load()) {
            throw std::runtime_error("ThreadPool: submit after shutdown");
        }
        pendingCount_.fetch_add(1);
        tasks_.emplace_back([task] { (*task)(); });
    }

    condition_.notify_one();
    return future;
}

// Explicit template instantiation for common types
template std::future<void> ThreadPool::submit(std::function<void()>&&);
template std::future<int> ThreadPool::submit(std::function<int()>&&);
template std::future<bool> ThreadPool::submit(std::function<bool()>&&);

void ThreadPool::workerLoop(size_t threadId) {
    (void)threadId;

    for (;;) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] {
                return shutdown_.load() || !tasks_.empty();
            });

            if (shutdown_.load() && tasks_.empty()) return;

            task = std::move(tasks_.front());
            tasks_.pop_front();
        }

        activeCount_.fetch_add(1);
        try {
            task();
        } catch (...) {
            // Worker threads must not propagate exceptions
        }
        activeCount_.fetch_sub(1);
        pendingCount_.fetch_sub(1);

        // Wake up waitForAll() if no more work
        if (pendingCount_.load() == 0 && activeCount_.load() == 0) {
            done_.notify_all();
        }
    }
}

} // namespace Zepra::Threading
