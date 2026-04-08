// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "core/frame_scheduler.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace NXRender {

FrameScheduler::FrameScheduler() {
    setTargetFrameRate(60.0f);
}

void FrameScheduler::setTargetFrameRate(float fps) {
    if (fps <= 0.0f) fps = 60.0f;
    targetFps_ = fps;
    budgetMs_ = 1000.0 / static_cast<double>(fps);
}

void FrameScheduler::beginFrame() {
    frameStart_ = SteadyClock::now();
    inFrame_ = true;
    frameCount_++;
}

void FrameScheduler::beginLayout() {
    layoutStart_ = SteadyClock::now();
}

void FrameScheduler::endLayout() {
    layoutEnd_ = SteadyClock::now();
}

void FrameScheduler::beginPaint() {
    paintStart_ = SteadyClock::now();
}

void FrameScheduler::endPaint() {
    paintEnd_ = SteadyClock::now();
}

void FrameScheduler::beginComposite() {
    compositeStart_ = SteadyClock::now();
}

void FrameScheduler::endComposite() {
    compositeEnd_ = SteadyClock::now();
}

void FrameScheduler::endFrame() {
    if (!inFrame_) return;
    inFrame_ = false;

    auto frameEnd = SteadyClock::now();

    FrameTiming timing;
    timing.frameNumber = frameCount_;
    timing.frameStart = frameStart_;
    timing.frameEnd = frameEnd;
    timing.totalMs = Duration(frameEnd - frameStart_).count();

    // Phase timings (only valid if the begin/end were called)
    if (layoutEnd_ > layoutStart_) {
        timing.layoutMs = Duration(layoutEnd_ - layoutStart_).count();
    }
    if (paintEnd_ > paintStart_) {
        timing.paintMs = Duration(paintEnd_ - paintStart_).count();
    }
    if (compositeEnd_ > compositeStart_) {
        timing.compositeMs = Duration(compositeEnd_ - compositeStart_).count();
    }

    timing.janked = timing.totalMs > budgetMs_;
    if (timing.janked) {
        jankCount_++;
    }

    // Detect dropped frames: if frame time > 2x budget, we dropped at least one frame
    if (timing.totalMs > budgetMs_ * 2.0) {
        uint64_t dropped = static_cast<uint64_t>(timing.totalMs / budgetMs_) - 1;
        droppedFrames_ += dropped;
        timing.dropped = true;
    }

    lastTiming_ = timing;
    recordTiming(timing);

    // Run idle tasks in remaining budget
    processIdleTasks();
}

bool FrameScheduler::withinBudget() const {
    if (!inFrame_) return true;
    auto now = SteadyClock::now();
    double elapsed = Duration(now - frameStart_).count();
    return elapsed < budgetMs_;
}

double FrameScheduler::remainingBudgetMs() const {
    if (!inFrame_) return budgetMs_;
    auto now = SteadyClock::now();
    double elapsed = Duration(now - frameStart_).count();
    return std::max(0.0, budgetMs_ - elapsed);
}

double FrameScheduler::elapsedMs() const {
    if (!inFrame_) return 0.0;
    auto now = SteadyClock::now();
    return Duration(now - frameStart_).count();
}

void FrameScheduler::scheduleIdleTask(IdleTask task) {
    // Insert in priority order (High first)
    auto it = idleQueue_.begin();
    while (it != idleQueue_.end() && it->priority >= task.priority) {
        ++it;
    }
    idleQueue_.insert(it, std::move(task));
}

void FrameScheduler::processIdleTasks() {
    if (idleQueue_.empty()) return;

    auto now = SteadyClock::now();
    double remaining = budgetMs_ - Duration(now - frameStart_).count();

    // Reserve at least 1ms for frame swap
    remaining -= 1.0;

    while (!idleQueue_.empty() && remaining > 0.5) {
        auto task = std::move(idleQueue_.front());
        idleQueue_.pop_front();

        auto taskStart = SteadyClock::now();
        task.callback(remaining);
        auto taskEnd = SteadyClock::now();

        double taskTime = Duration(taskEnd - taskStart).count();
        remaining -= taskTime;
    }
}

void FrameScheduler::recordTiming(const FrameTiming& timing) {
    frameTimeHistory_.push_back(timing.totalMs);
    while (frameTimeHistory_.size() > kHistorySize) {
        frameTimeHistory_.pop_front();
    }
}

double FrameScheduler::averageFrameTimeMs() const {
    if (frameTimeHistory_.empty()) return 0.0;
    double sum = std::accumulate(frameTimeHistory_.begin(), frameTimeHistory_.end(), 0.0);
    return sum / static_cast<double>(frameTimeHistory_.size());
}

double FrameScheduler::averageFps() const {
    double avg = averageFrameTimeMs();
    if (avg <= 0.0) return 0.0;
    return 1000.0 / avg;
}

double FrameScheduler::p95FrameTimeMs() const {
    if (frameTimeHistory_.empty()) return 0.0;
    std::vector<double> sorted(frameTimeHistory_.begin(), frameTimeHistory_.end());
    std::sort(sorted.begin(), sorted.end());
    size_t idx = static_cast<size_t>(sorted.size() * 0.95);
    if (idx >= sorted.size()) idx = sorted.size() - 1;
    return sorted[idx];
}

float FrameScheduler::jankRatio() const {
    if (frameCount_ == 0) return 0.0f;
    return static_cast<float>(jankCount_) / static_cast<float>(frameCount_);
}

void FrameScheduler::resetStats() {
    frameCount_ = 0;
    jankCount_ = 0;
    droppedFrames_ = 0;
    frameTimeHistory_.clear();
    lastTiming_ = FrameTiming{};
}

} // namespace NXRender
