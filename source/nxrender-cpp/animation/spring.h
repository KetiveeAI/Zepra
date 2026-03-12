// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file spring.h
 * @brief Spring physics for animations
 * 
 * Based on critically damped spring physics for smooth animations.
 */

#pragma once

#include <cmath>

namespace NXRender {

/**
 * @brief Spring configuration
 */
struct SpringConfig {
    float stiffness = 170.0f;   // Spring constant (k)
    float damping = 26.0f;      // Damping coefficient
    float mass = 1.0f;          // Mass of the object
    float initialVelocity = 0;  // Starting velocity
    
    // Presets
    static SpringConfig gentle() { return {120, 14, 1, 0}; }
    static SpringConfig wobbly() { return {180, 12, 1, 0}; }
    static SpringConfig stiff() { return {210, 20, 1, 0}; }
    static SpringConfig slow() { return {280, 60, 1, 0}; }
    static SpringConfig molasses() { return {280, 120, 1, 0}; }
};

/**
 * @brief Spring animation state
 */
class Spring {
public:
    Spring(float from = 0, float to = 1, SpringConfig config = {})
        : from_(from), to_(to), config_(config), 
          position_(from), velocity_(config.initialVelocity) {}
    
    /**
     * @brief Update spring state
     * @param deltaTime Time step in seconds
     * @return Current position
     */
    float update(float deltaTime) {
        // Spring force: F = -k * (x - target)
        // Damping force: F = -d * velocity
        // F = ma => a = F/m
        
        float displacement = position_ - to_;
        float springForce = -config_.stiffness * displacement;
        float dampingForce = -config_.damping * velocity_;
        float acceleration = (springForce + dampingForce) / config_.mass;
        
        velocity_ += acceleration * deltaTime;
        position_ += velocity_ * deltaTime;
        
        elapsed_ += deltaTime;
        return position_;
    }
    
    /**
     * @brief Check if spring is at rest
     */
    bool isAtRest() const {
        return std::abs(position_ - to_) < 0.001f && 
               std::abs(velocity_) < 0.001f;
    }
    
    /**
     * @brief Get current value
     */
    float value() const { return position_; }
    
    /**
     * @brief Get current velocity
     */
    float velocity() const { return velocity_; }
    
    /**
     * @brief Reset spring
     */
    void reset(float from, float to) {
        from_ = from;
        to_ = to;
        position_ = from;
        velocity_ = config_.initialVelocity;
        elapsed_ = 0;
    }
    
    /**
     * @brief Set target value (for gesture following)
     */
    void setTarget(float target) { to_ = target; }
    
    /**
     * @brief Get normalized progress [0, 1] (approximate)
     */
    float progress() const {
        if (std::abs(to_ - from_) < 0.001f) return 1.0f;
        return (position_ - from_) / (to_ - from_);
    }
    
private:
    float from_, to_;
    SpringConfig config_;
    float position_;
    float velocity_;
    float elapsed_ = 0;
};

/**
 * @brief Spring easing function generator
 * @param stiffness Spring stiffness
 * @param damping Damping coefficient
 * @return Easing function
 */
inline float springEasing(float t, float stiffness = 170, float damping = 26) {
    // Approximate spring using damped oscillation formula
    // For critically damped: x(t) = (1 + (c1 + c2*t) * e^(-omega*t))
    float omega = std::sqrt(stiffness);
    float zeta = damping / (2 * omega);
    
    if (zeta < 1) {
        // Underdamped - oscillates
        float omegaD = omega * std::sqrt(1 - zeta * zeta);
        float decay = std::exp(-zeta * omega * t * 3);
        return 1 - decay * std::cos(omegaD * t * 3);
    } else {
        // Critically damped or overdamped
        float decay = std::exp(-omega * t * 3);
        return 1 - decay * (1 + omega * t * 3);
    }
}

} // namespace NXRender
