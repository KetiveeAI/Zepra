// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file frame_scheduler.h
 * @brief VSync-aware frame scheduling with deadline tracking and jank detection.
 *
 * Manages frame pacing to ensure consistent frame times. Tracks frame budgets
 * and schedules idle tasks in remaining budget slack.
 */

#pragma once

#include <cstdint>
#include <chrono>
#include <functional>
#include <vector>
#include <deque>

namespace NXRender {

using SteadyClock = std::chrono::steady_clock;
using TimePoint = SteadyClock::time_point;
using Duration = std::chrono::duration<double, std::milli>;

/**
 * @brief Per-frame timing information.
 */
struct FrameTiming {
    uint64_t frameNumber = 0;
    TimePoint frameStart;
    TimePoint frameEnd;
    double totalMs = 0;
    double layoutMs = 0;
    double paintMs = 0;
    double compositeMs = 0;
    double gpuMs = 0;
    bool janked = false; // Exceeded budget
    bool dropped = false;
};

/**
 * @brief Idle task with priority.
 */
struct IdleTask {
    enum class Priority : uint8_t { Low, Normal, High };
    Priority priority = Priority::Normal;
    std::function<void(double remainingMs)> callback;
};

/**
 * @brief Frame scheduler with VSync alignment and budget management.
 */
class FrameScheduler {
public:
    FrameScheduler();

    /**
     * @brief Set target frame rate (typically 60 Hz from VSync).
     */
    void setTargetFrameRate(float fps);

    /**
     * @brief Begin a new frame. Records start time.
     */
    void beginFrame();

    /**
     * @brief Mark the layout phase start.
     */
    void beginLayout();
    void endLayout();

    /**
     * @brief Mark the paint phase start.
     */
    void beginPaint();
    void endPaint();

    /**
     * @brief Mark the composite phase start.
     */
    void beginComposite();
    void endComposite();

    /**
     * @brief End the current frame. Computes timing, runs idle tasks in slack.
     */
    void endFrame();

    /**
     * @brief Queue an idle task to run in frame budget slack.
     * Task receives remaining milliseconds as argument.
     */
    void scheduleIdleTask(IdleTask task);

    /**
     * @brief Check if we're within budget for the current frame.
     */
    bool withinBudget() const;

    /**
     * @brief Remaining budget in milliseconds.
     */
    double remainingBudgetMs() const;

    /**
     * @brief Current frame's elapsed time in milliseconds.
     */
    double elapsedMs() const;

    // ======================================================================
    // Statistics
    // ======================================================================

    const FrameTiming& lastFrameTiming() const { return lastTiming_; }
    uint64_t frameCount() const { return frameCount_; }
    uint64_t jankCount() const { return jankCount_; }
    uint64_t droppedFrames() const { return droppedFrames_; }

    /**
     * @brief Average frame time over the last N frames.
     */
    double averageFrameTimeMs() const;

    /**
     * @brief Average FPS over the last N frames.
     */
    double averageFps() const;

    /**
     * @brief 95th percentile frame time.
     */
    double p95FrameTimeMs() const;

    /**
     * @brief Percentage of frames that exceeded the budget.
     */
    float jankRatio() const;

    /**
     * @brief Get the frame budget in milliseconds.
     */
    double budgetMs() const { return budgetMs_; }

    /**
     * @brief Reset all statistics.
     */
    void resetStats();

private:
    void processIdleTasks();
    void recordTiming(const FrameTiming& timing);

    double budgetMs_ = 16.667; // 60 fps
    float targetFps_ = 60.0f;

    // Current frame state
    TimePoint frameStart_;
    TimePoint layoutStart_, layoutEnd_;
    TimePoint paintStart_, paintEnd_;
    TimePoint compositeStart_, compositeEnd_;
    bool inFrame_ = false;

    FrameTiming lastTiming_;
    uint64_t frameCount_ = 0;
    uint64_t jankCount_ = 0;
    uint64_t droppedFrames_ = 0;

    // Idle task queue (sorted by priority)
    std::deque<IdleTask> idleQueue_;

    // Rolling history for stats (last 120 frames)
    static constexpr size_t kHistorySize = 120;
    std::deque<double> frameTimeHistory_;
};

} // namespace NXRender
