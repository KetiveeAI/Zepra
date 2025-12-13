/**
 * @file event_loop.hpp
 * @brief Platform event loop
 */

#pragma once

#include <functional>
#include <chrono>
#include <queue>
#include <mutex>
#include <atomic>

namespace Zepra::Platform {

/**
 * @class EventLoop
 * @brief Main event loop for the application
 */
class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    
    // Run the event loop
    void run();
    void runOnce();
    void quit();
    
    bool isRunning() const { return running_.load(); }
    
    // Schedule tasks
    using Task = std::function<void()>;
    
    void postTask(Task task);
    void postDelayedTask(Task task, std::chrono::milliseconds delay);
    void postRepeatingTask(Task task, std::chrono::milliseconds interval);
    
    // Timers
    int64_t setTimer(std::chrono::milliseconds interval, Task callback, bool repeat = false);
    void clearTimer(int64_t timerId);
    
    // Idle callback (runs when no other events)
    void setIdleCallback(Task callback) { idleCallback_ = std::move(callback); }
    
    // Singleton access
    static EventLoop& main();
    
private:
    void processEvents();
    void processTasks();
    void processTimers();
    
    struct DelayedTask {
        std::chrono::steady_clock::time_point runAt;
        Task task;
        int64_t id;
        bool repeat;
        std::chrono::milliseconds interval;
        
        bool operator>(const DelayedTask& other) const {
            return runAt > other.runAt;
        }
    };
    
    std::queue<Task> taskQueue_;
    std::priority_queue<DelayedTask, std::vector<DelayedTask>, std::greater<>> timerQueue_;
    std::mutex mutex_;
    
    std::atomic<bool> running_{false};
    int64_t nextTimerId_ = 1;
    
    Task idleCallback_;
};

} // namespace Zepra::Platform
