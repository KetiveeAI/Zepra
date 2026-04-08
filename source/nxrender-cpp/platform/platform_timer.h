// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file platform_timer.h
 * @brief High-resolution timer and timer wheel for efficient bulk timer management.
 */

#pragma once

#include <cstdint>
#include <chrono>
#include <functional>
#include <vector>
#include <queue>

namespace NXRender {

using TimerCallback = std::function<void()>;

/**
 * @brief High-resolution monotonic timer.
 */
class HighResTimer {
public:
    HighResTimer();

    void reset();
    double elapsedMs() const;
    double elapsedUs() const;
    double elapsedSec() const;

    static uint64_t nowMs();
    static double nowSec();

private:
    std::chrono::steady_clock::time_point start_;
};

/**
 * @brief A single timer (one-shot or repeating).
 */
struct TimerEntry {
    uint32_t id = 0;
    uint64_t fireTimeMs = 0;
    uint64_t intervalMs = 0;   // 0 = one-shot
    TimerCallback callback;
    bool cancelled = false;
    bool repeating = false;
};

/**
 * @brief Timer manager with priority queue.
 *
 * Supports one-shot and repeating timers. Timers are fired when
 * processTimers() is called (typically once per frame).
 */
class TimerManager {
public:
    static TimerManager& instance();

    /**
     * @brief Schedule a one-shot timer.
     * @return Timer ID for cancellation.
     */
    uint32_t setTimeout(uint64_t delayMs, TimerCallback callback);

    /**
     * @brief Schedule a repeating timer.
     * @return Timer ID for cancellation.
     */
    uint32_t setInterval(uint64_t intervalMs, TimerCallback callback);

    /**
     * @brief Cancel a timer by ID.
     */
    void clearTimer(uint32_t id);

    /**
     * @brief Fire all timers whose deadline has passed.
     * Call once per frame.
     */
    void processTimers();

    /**
     * @brief Number of active timers.
     */
    size_t activeCount() const { return heap_.size(); }

    /**
     * @brief Time until next timer fires (ms). Returns UINT64_MAX if no timers.
     */
    uint64_t timeUntilNext() const;

    /**
     * @brief Cancel all timers.
     */
    void clearAll();

private:
    TimerManager() = default;

    struct HeapEntry {
        uint64_t fireTime;
        uint32_t id;
        bool operator>(const HeapEntry& other) const { return fireTime > other.fireTime; }
    };

    uint32_t nextId_ = 1;
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, std::greater<HeapEntry>> heap_;
    std::vector<TimerEntry> timers_;
};

} // namespace NXRender
