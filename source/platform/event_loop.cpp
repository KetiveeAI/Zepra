// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file event_loop.cpp
 * @brief Event loop implementation
 */

#include "platform/event_loop.hpp"
#include <thread>

namespace Zepra::Platform {

EventLoop::EventLoop() = default;
EventLoop::~EventLoop() = default;

void EventLoop::run() {
    running_.store(true);
    
    while (running_.load()) {
        runOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void EventLoop::runOnce() {
    processTimers();
    processTasks();
    
    if (taskQueue_.empty() && timerQueue_.empty() && idleCallback_) {
        idleCallback_();
    }
}

void EventLoop::quit() {
    running_.store(false);
}

void EventLoop::postTask(Task task) {
    std::lock_guard<std::mutex> lock(mutex_);
    taskQueue_.push(std::move(task));
}

void EventLoop::postDelayedTask(Task task, std::chrono::milliseconds delay) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    DelayedTask dt;
    dt.runAt = std::chrono::steady_clock::now() + delay;
    dt.task = std::move(task);
    dt.id = nextTimerId_++;
    dt.repeat = false;
    dt.interval = delay;
    
    timerQueue_.push(std::move(dt));
}

void EventLoop::postRepeatingTask(Task task, std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    DelayedTask dt;
    dt.runAt = std::chrono::steady_clock::now() + interval;
    dt.task = std::move(task);
    dt.id = nextTimerId_++;
    dt.repeat = true;
    dt.interval = interval;
    
    timerQueue_.push(std::move(dt));
}

int64_t EventLoop::setTimer(std::chrono::milliseconds interval, Task callback, bool repeat) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    DelayedTask dt;
    dt.runAt = std::chrono::steady_clock::now() + interval;
    dt.task = std::move(callback);
    dt.id = nextTimerId_++;
    dt.repeat = repeat;
    dt.interval = interval;
    
    int64_t id = dt.id;
    timerQueue_.push(std::move(dt));
    return id;
}

void EventLoop::clearTimer(int64_t timerId) {
    // Note: In a real implementation, we'd need a cancel set
    // For now, timers can't be truly cancelled
}

void EventLoop::processTasks() {
    std::queue<Task> tasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::swap(tasks, taskQueue_);
    }
    
    while (!tasks.empty()) {
        Task task = std::move(tasks.front());
        tasks.pop();
        if (task) task();
    }
}

void EventLoop::processTimers() {
    auto now = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    while (!timerQueue_.empty() && timerQueue_.top().runAt <= now) {
        DelayedTask dt = timerQueue_.top();
        timerQueue_.pop();
        
        if (dt.task) dt.task();
        
        if (dt.repeat) {
            dt.runAt = now + dt.interval;
            timerQueue_.push(std::move(dt));
        }
    }
}

EventLoop& EventLoop::main() {
    static EventLoop instance;
    return instance;
}

} // namespace Zepra::Platform
