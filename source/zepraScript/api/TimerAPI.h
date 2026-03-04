/**
 * @file TimerAPI.h
 * @brief Timer API Implementation
 * 
 * HTML Standard Timers:
 * - setTimeout, clearTimeout
 * - setInterval, clearInterval
 * - requestAnimationFrame, cancelAnimationFrame
 * - queueMicrotask
 */

#pragma once

#include <functional>
#include <chrono>
#include <map>
#include <mutex>
#include <atomic>
#include <queue>

namespace Zepra::API {

// =============================================================================
// Timer ID
// =============================================================================

using TimerId = uint32_t;

// =============================================================================
// Timer Entry
// =============================================================================

struct TimerEntry {
    TimerId id;
    std::function<void()> callback;
    std::chrono::steady_clock::time_point fireTime;
    std::chrono::milliseconds interval{0};  // 0 = one-shot
    bool cancelled = false;
    
    bool operator>(const TimerEntry& other) const {
        return fireTime > other.fireTime;
    }
};

// =============================================================================
// Animation Frame Entry
// =============================================================================

struct AnimationFrameEntry {
    TimerId id;
    std::function<void(double)> callback;  // DOMHighResTimeStamp
    bool cancelled = false;
};

// =============================================================================
// Timer Manager
// =============================================================================

/**
 * @brief Manages all timers for an execution context
 */
class TimerManager {
public:
    TimerManager() = default;
    
    // === setTimeout / clearTimeout ===
    
    TimerId setTimeout(std::function<void()> callback, 
                       std::chrono::milliseconds delay) {
        std::lock_guard lock(mutex_);
        
        TimerId id = nextId_++;
        TimerEntry entry;
        entry.id = id;
        entry.callback = std::move(callback);
        entry.fireTime = std::chrono::steady_clock::now() + delay;
        entry.interval = std::chrono::milliseconds(0);
        
        timers_.push(std::move(entry));
        timerMap_[id] = true;
        
        return id;
    }
    
    void clearTimeout(TimerId id) {
        std::lock_guard lock(mutex_);
        timerMap_.erase(id);
    }
    
    // === setInterval / clearInterval ===
    
    TimerId setInterval(std::function<void()> callback,
                        std::chrono::milliseconds interval) {
        std::lock_guard lock(mutex_);
        
        TimerId id = nextId_++;
        TimerEntry entry;
        entry.id = id;
        entry.callback = std::move(callback);
        entry.fireTime = std::chrono::steady_clock::now() + interval;
        entry.interval = interval;
        
        timers_.push(std::move(entry));
        timerMap_[id] = true;
        
        return id;
    }
    
    void clearInterval(TimerId id) {
        clearTimeout(id);  // Same mechanism
    }
    
    // === requestAnimationFrame / cancelAnimationFrame ===
    
    TimerId requestAnimationFrame(std::function<void(double)> callback) {
        std::lock_guard lock(mutex_);
        
        TimerId id = nextId_++;
        AnimationFrameEntry entry;
        entry.id = id;
        entry.callback = std::move(callback);
        
        animationFrames_.push_back(std::move(entry));
        
        return id;
    }
    
    void cancelAnimationFrame(TimerId id) {
        std::lock_guard lock(mutex_);
        for (auto& entry : animationFrames_) {
            if (entry.id == id) {
                entry.cancelled = true;
                break;
            }
        }
    }
    
    // === queueMicrotask ===
    
    void queueMicrotask(std::function<void()> callback) {
        std::lock_guard lock(mutex_);
        microtasks_.push(std::move(callback));
    }
    
    // === Processing ===
    
    // Run pending timers (called by event loop)
    void runTimers() {
        auto now = std::chrono::steady_clock::now();
        
        std::lock_guard lock(mutex_);
        
        while (!timers_.empty()) {
            const auto& top = timers_.top();
            
            if (top.fireTime > now) break;
            
            TimerEntry entry = timers_.top();
            timers_.pop();
            
            // Check if cancelled
            if (timerMap_.find(entry.id) == timerMap_.end()) {
                continue;
            }
            
            // Execute callback
            if (entry.callback) {
                mutex_.unlock();
                entry.callback();
                mutex_.lock();
            }
            
            // Reschedule interval
            if (entry.interval.count() > 0 && 
                timerMap_.find(entry.id) != timerMap_.end()) {
                entry.fireTime = std::chrono::steady_clock::now() + entry.interval;
                timers_.push(std::move(entry));
            } else {
                timerMap_.erase(entry.id);
            }
        }
    }
    
    // Run animation frames (called before paint)
    void runAnimationFrames(double timestamp) {
        std::vector<AnimationFrameEntry> frames;
        {
            std::lock_guard lock(mutex_);
            frames = std::move(animationFrames_);
            animationFrames_.clear();
        }
        
        for (auto& entry : frames) {
            if (!entry.cancelled && entry.callback) {
                entry.callback(timestamp);
            }
        }
    }
    
    // Run microtasks
    void runMicrotasks() {
        while (true) {
            std::function<void()> task;
            {
                std::lock_guard lock(mutex_);
                if (microtasks_.empty()) break;
                task = std::move(microtasks_.front());
                microtasks_.pop();
            }
            if (task) task();
        }
    }
    
    // Check for pending work
    bool hasPendingTimers() const {
        std::lock_guard lock(mutex_);
        return !timers_.empty() || !animationFrames_.empty();
    }
    
    // Get time until next timer
    std::chrono::milliseconds timeUntilNextTimer() const {
        std::lock_guard lock(mutex_);
        if (timers_.empty()) {
            return std::chrono::milliseconds::max();
        }
        
        auto now = std::chrono::steady_clock::now();
        auto diff = timers_.top().fireTime - now;
        if (diff.count() < 0) return std::chrono::milliseconds(0);
        return std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    }
    
private:
    mutable std::mutex mutex_;
    std::atomic<TimerId> nextId_{1};
    
    std::priority_queue<TimerEntry, std::vector<TimerEntry>, std::greater<>> timers_;
    std::map<TimerId, bool> timerMap_;
    
    std::vector<AnimationFrameEntry> animationFrames_;
    std::queue<std::function<void()>> microtasks_;
};

// =============================================================================
// Global Timer Functions
// =============================================================================

TimerManager& getTimerManager();

inline TimerId setTimeout(std::function<void()> callback, int delayMs) {
    return getTimerManager().setTimeout(std::move(callback), 
        std::chrono::milliseconds(delayMs));
}

inline void clearTimeout(TimerId id) {
    getTimerManager().clearTimeout(id);
}

inline TimerId setInterval(std::function<void()> callback, int intervalMs) {
    return getTimerManager().setInterval(std::move(callback),
        std::chrono::milliseconds(intervalMs));
}

inline void clearInterval(TimerId id) {
    getTimerManager().clearInterval(id);
}

inline TimerId requestAnimationFrame(std::function<void(double)> callback) {
    return getTimerManager().requestAnimationFrame(std::move(callback));
}

inline void cancelAnimationFrame(TimerId id) {
    getTimerManager().cancelAnimationFrame(id);
}

inline void queueMicrotask(std::function<void()> callback) {
    getTimerManager().queueMicrotask(std::move(callback));
}

} // namespace Zepra::API
