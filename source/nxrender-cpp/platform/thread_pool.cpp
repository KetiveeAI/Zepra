// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "platform/thread_pool.h"
#include <algorithm>

namespace NXRender {

ThreadPool& ThreadPool::instance() {
    static ThreadPool pool(std::max(2u, std::thread::hardware_concurrency() - 1));
    return pool;
}

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; i++) {
        workers_.emplace_back([this]() { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::workerLoop() {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            condition_.wait(lock, [this]() {
                return stopped_ || !taskQueue_.empty();
            });

            if (stopped_ && taskQueue_.empty()) return;

            task = taskQueue_.top();
            taskQueue_.pop();
        }

        activeTasks_++;
        task.func();
        activeTasks_--;

        allDone_.notify_all();
    }
}

void ThreadPool::submitVoid(std::function<void()> task, TaskPriority priority) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stopped_) return;
        taskQueue_.push({priority, std::move(task)});
    }
    condition_.notify_one();
}

size_t ThreadPool::pendingTasks() const {
    return taskQueue_.size();
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    allDone_.wait(lock, [this]() {
        return taskQueue_.empty() && activeTasks_.load() == 0;
    });
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stopped_) return;
        stopped_ = true;
    }

    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

} // namespace NXRender
