// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "platform/platform_timer.h"
#include <algorithm>

namespace NXRender {

// ==================================================================
// HighResTimer
// ==================================================================

HighResTimer::HighResTimer() : start_(std::chrono::steady_clock::now()) {}

void HighResTimer::reset() {
    start_ = std::chrono::steady_clock::now();
}

double HighResTimer::elapsedMs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now - start_).count();
}

double HighResTimer::elapsedUs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::micro>(now - start_).count();
}

double HighResTimer::elapsedSec() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - start_).count();
}

uint64_t HighResTimer::nowMs() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

double HighResTimer::nowSec() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

// ==================================================================
// TimerManager
// ==================================================================

TimerManager& TimerManager::instance() {
    static TimerManager mgr;
    return mgr;
}

uint32_t TimerManager::setTimeout(uint64_t delayMs, TimerCallback callback) {
    uint32_t id = nextId_++;
    uint64_t fireTime = HighResTimer::nowMs() + delayMs;

    TimerEntry entry;
    entry.id = id;
    entry.fireTimeMs = fireTime;
    entry.intervalMs = 0;
    entry.callback = std::move(callback);
    entry.cancelled = false;
    entry.repeating = false;
    timers_.push_back(std::move(entry));

    heap_.push({fireTime, id});
    return id;
}

uint32_t TimerManager::setInterval(uint64_t intervalMs, TimerCallback callback) {
    uint32_t id = nextId_++;
    uint64_t fireTime = HighResTimer::nowMs() + intervalMs;

    TimerEntry entry;
    entry.id = id;
    entry.fireTimeMs = fireTime;
    entry.intervalMs = intervalMs;
    entry.callback = std::move(callback);
    entry.cancelled = false;
    entry.repeating = true;
    timers_.push_back(std::move(entry));

    heap_.push({fireTime, id});
    return id;
}

void TimerManager::clearTimer(uint32_t id) {
    for (auto& timer : timers_) {
        if (timer.id == id) {
            timer.cancelled = true;
            break;
        }
    }
}

void TimerManager::processTimers() {
    uint64_t now = HighResTimer::nowMs();

    while (!heap_.empty() && heap_.top().fireTime <= now) {
        HeapEntry top = heap_.top();
        heap_.pop();

        // Find the timer
        TimerEntry* found = nullptr;
        for (auto& timer : timers_) {
            if (timer.id == top.id && !timer.cancelled) {
                found = &timer;
                break;
            }
        }

        if (!found) continue;

        // Fire the callback
        if (found->callback) {
            found->callback();
        }

        // Re-schedule if repeating
        if (found->repeating && !found->cancelled) {
            found->fireTimeMs = now + found->intervalMs;
            heap_.push({found->fireTimeMs, found->id});
        } else {
            found->cancelled = true; // Mark for cleanup
        }
    }

    // Garbage-collect cancelled timers
    timers_.erase(
        std::remove_if(timers_.begin(), timers_.end(),
                       [](const TimerEntry& t) { return t.cancelled; }),
        timers_.end());
}

uint64_t TimerManager::timeUntilNext() const {
    if (heap_.empty()) return UINT64_MAX;
    uint64_t now = HighResTimer::nowMs();
    if (heap_.top().fireTime <= now) return 0;
    return heap_.top().fireTime - now;
}

void TimerManager::clearAll() {
    while (!heap_.empty()) heap_.pop();
    timers_.clear();
}

} // namespace NXRender
