// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file scroll_physics.h
 * @brief Enhanced scroll animation physics
 * 
 * Provides adaptive scroll animation with bezier timing,
 * velocity preservation, and momentum.
 */

#pragma once

#include "../animation/easing.h"
#include <chrono>
#include <array>
#include <cmath>
#include <algorithm>

namespace NXRender {

/**
 * @brief Scroll physics settings
 */
struct ScrollPhysicsSettings {
    int minDurationMs = 150;
    int maxDurationMs = 500;
    double intervalRatio = 2.0;  // Duration vs event interval
    float decelerationRate = 0.998f;  // Per-frame momentum decay
    float snapThreshold = 0.5f;  // Pixels to consider at rest
};

/**
 * @brief Adaptive bezier scroll physics
 * 
 * Automatically adjusts animation duration based on scroll rate
 * and preserves velocity for smooth momentum scrolling.
 */
class AdaptiveScrollPhysics {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::milliseconds;
    
    explicit AdaptiveScrollPhysics(const ScrollPhysicsSettings& settings = {})
        : settings_(settings) {
        reset();
    }
    
    /**
     * @brief Update scroll target
     * @param destination New scroll destination
     * @param velocity Current velocity (for momentum)
     */
    void update(float destination, float velocity = 0) {
        auto now = std::chrono::steady_clock::now();
        
        startPos_ = currentPos_;
        destination_ = destination;
        startVelocity_ = velocity;
        startTime_ = now;
        duration_ = computeDuration(now);
        
        // Initialize bezier control points for velocity preservation
        initTimingFunction(velocity);
        
        // Update event history
        shiftEventHistory(now);
    }
    
    /**
     * @brief Get position at current time
     */
    float position() const {
        return positionAt(std::chrono::steady_clock::now());
    }
    
    /**
     * @brief Get position at specific time
     */
    float positionAt(TimePoint time) const {
        float t = progress(time);
        if (t >= 1.0f) return destination_;
        
        // Apply bezier easing
        float eased = cubicBezier(t, cp1_, cp2_);
        return startPos_ + (destination_ - startPos_) * eased;
    }
    
    /**
     * @brief Get velocity at current time
     */
    float velocity() const {
        return velocityAt(std::chrono::steady_clock::now());
    }
    
    /**
     * @brief Get velocity at specific time
     */
    float velocityAt(TimePoint time) const {
        float t = progress(time);
        if (t >= 1.0f || duration_.count() == 0) return 0;
        
        // Derivative of bezier * distance / duration
        float dt = 0.001f;
        float t1 = std::max(0.0f, t - dt);
        float t2 = std::min(1.0f, t + dt);
        
        float p1 = cubicBezier(t1, cp1_, cp2_);
        float p2 = cubicBezier(t2, cp1_, cp2_);
        
        float distance = destination_ - startPos_;
        float durationSec = duration_.count() / 1000.0f;
        
        return (p2 - p1) * distance / ((t2 - t1) * durationSec);
    }
    
    /**
     * @brief Check if animation is finished
     */
    bool isFinished() const {
        return isFinishedAt(std::chrono::steady_clock::now());
    }
    
    bool isFinishedAt(TimePoint time) const {
        return time >= startTime_ + duration_;
    }
    
    /**
     * @brief Reset state
     */
    void reset() {
        startPos_ = 0;
        destination_ = 0;
        currentPos_ = 0;
        startVelocity_ = 0;
        duration_ = Duration(0);
        eventHistory_.fill(TimePoint());
        isFirstEvent_ = true;
    }
    
    /**
     * @brief Apply content shift (for scroll anchoring)
     */
    void applyShift(float delta) {
        startPos_ += delta;
        destination_ += delta;
        currentPos_ += delta;
    }
    
    float currentPosition() const { return currentPos_; }
    float targetPosition() const { return destination_; }
    
    /**
     * @brief Update current position (call each frame)
     */
    void tick() {
        currentPos_ = position();
    }
    
private:
    ScrollPhysicsSettings settings_;
    
    float startPos_ = 0;
    float destination_ = 0;
    float currentPos_ = 0;
    float startVelocity_ = 0;
    
    TimePoint startTime_;
    Duration duration_;
    
    // Bezier control points (y1, y2 for cubic-bezier(0, y1, 1, y2))
    float cp1_ = 0.25f;
    float cp2_ = 0.1f;
    
    // Event history for rate detection
    std::array<TimePoint, 3> eventHistory_;
    bool isFirstEvent_ = true;
    
    float progress(TimePoint time) const {
        if (duration_.count() == 0) return 1.0f;
        auto elapsed = std::chrono::duration_cast<Duration>(time - startTime_);
        return std::clamp(static_cast<float>(elapsed.count()) / duration_.count(), 0.0f, 1.0f);
    }
    
    Duration computeDuration(TimePoint now) {
        if (isFirstEvent_) {
            initializeHistory(now);
            isFirstEvent_ = false;
            return Duration(settings_.maxDurationMs);
        }
        
        // Calculate average interval between recent events
        auto interval1 = std::chrono::duration_cast<Duration>(eventHistory_[2] - eventHistory_[1]);
        auto interval2 = std::chrono::duration_cast<Duration>(eventHistory_[1] - eventHistory_[0]);
        
        int avgInterval = (interval1.count() + interval2.count()) / 2;
        int duration = static_cast<int>(avgInterval * settings_.intervalRatio);
        
        return Duration(std::clamp(duration, settings_.minDurationMs, settings_.maxDurationMs));
    }
    
    void initializeHistory(TimePoint now) {
        Duration maxInterval(settings_.maxDurationMs);
        eventHistory_[2] = now;
        eventHistory_[1] = now - maxInterval;
        eventHistory_[0] = now - maxInterval - maxInterval;
    }
    
    void shiftEventHistory(TimePoint now) {
        eventHistory_[0] = eventHistory_[1];
        eventHistory_[1] = eventHistory_[2];
        eventHistory_[2] = now;
    }
    
    void initTimingFunction(float velocity) {
        // Adjust control points based on initial velocity
        // Higher velocity = more "ease-out" character
        float distance = std::abs(destination_ - startPos_);
        if (distance < 0.001f) {
            cp1_ = 0.25f;
            cp2_ = 0.1f;
            return;
        }
        
        float normalizedVel = std::clamp(std::abs(velocity) / 1000.0f, 0.0f, 1.0f);
        
        // More velocity = steeper start, gentler end
        cp1_ = 0.1f + 0.3f * normalizedVel;
        cp2_ = 0.4f - 0.3f * normalizedVel;
    }
    
    // Simplified cubic bezier for y = f(x) with x1=0, x2=1
    static float cubicBezier(float t, float y1, float y2) {
        float invT = 1.0f - t;
        return 3 * y1 * t * invT * invT + 3 * y2 * t * t * invT + t * t * t;
    }
};

/**
 * @brief Momentum scroll physics
 * 
 * Provides inertia scrolling with deceleration.
 */
class MomentumScrollPhysics {
public:
    explicit MomentumScrollPhysics(float decelerationRate = 0.998f)
        : decelerationRate_(decelerationRate) {}
    
    /**
     * @brief Start momentum with initial velocity
     */
    void start(float position, float velocity) {
        position_ = position;
        velocity_ = velocity;
        isActive_ = std::abs(velocity) > 0.1f;
    }
    
    /**
     * @brief Update position (call each frame)
     * @param deltaMs Delta time in milliseconds
     */
    void update(float deltaMs) {
        if (!isActive_) return;
        
        // Apply deceleration
        float frames = deltaMs / 16.67f;  // Normalize to ~60fps
        velocity_ *= std::pow(decelerationRate_, frames);
        position_ += velocity_ * (deltaMs / 1000.0f);
        
        // Stop when velocity is negligible
        if (std::abs(velocity_) < 0.1f) {
            velocity_ = 0;
            isActive_ = false;
        }
    }
    
    /**
     * @brief Stop momentum
     */
    void stop() {
        velocity_ = 0;
        isActive_ = false;
    }
    
    float position() const { return position_; }
    float velocity() const { return velocity_; }
    bool isActive() const { return isActive_; }
    
private:
    float position_ = 0;
    float velocity_ = 0;
    float decelerationRate_;
    bool isActive_ = false;
};

} // namespace NXRender
