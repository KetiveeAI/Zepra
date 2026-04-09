// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "animation/spring.h"
#include <cmath>

namespace NXRender {

// ==================================================================
// SpringAnimation
// ==================================================================

SpringAnimation::SpringAnimation() {}

SpringAnimation::SpringAnimation(float stiffness, float damping, float mass)
    : stiffness_(stiffness), damping_(damping), mass_(mass) {}

void SpringAnimation::start(float fromValue, float toValue) {
    currentValue_ = fromValue;
    targetValue_ = toValue;
    velocity_ = 0;
    elapsed_ = 0;
    running_ = true;
    settled_ = false;
}

void SpringAnimation::startWithVelocity(float fromValue, float toValue, float initialVelocity) {
    currentValue_ = fromValue;
    targetValue_ = toValue;
    velocity_ = initialVelocity;
    elapsed_ = 0;
    running_ = true;
    settled_ = false;
}

bool SpringAnimation::update(float deltaMs) {
    if (!running_ || settled_) return false;

    float dt = deltaMs / 1000.0f;
    elapsed_ += dt;

    // Critically damped / overdamped / underdamped spring using Verlet integration
    // F = -k * (x - target) - d * v
    float displacement = currentValue_ - targetValue_;
    float springForce = -stiffness_ * displacement;
    float dampingForce = -damping_ * velocity_;
    float acceleration = (springForce + dampingForce) / mass_;

    velocity_ += acceleration * dt;
    currentValue_ += velocity_ * dt;

    // Check if settled
    float absDist = std::abs(currentValue_ - targetValue_);
    float absVel = std::abs(velocity_);

    if (absDist < restThreshold_ && absVel < velocityThreshold_) {
        currentValue_ = targetValue_;
        velocity_ = 0;
        settled_ = true;
        running_ = false;
        if (onComplete_) onComplete_();
        return false;
    }

    if (onUpdate_) onUpdate_(currentValue_);
    return true;
}

void SpringAnimation::stop() {
    running_ = false;
}

void SpringAnimation::snapTo(float value) {
    currentValue_ = value;
    targetValue_ = value;
    velocity_ = 0;
    running_ = false;
    settled_ = true;
}

float SpringAnimation::dampingRatio() const {
    // ζ = c / (2 * sqrt(k * m))
    float criticalDamping = 2.0f * std::sqrt(stiffness_ * mass_);
    if (criticalDamping < 0.0001f) return 0;
    return damping_ / criticalDamping;
}

float SpringAnimation::naturalFrequency() const {
    // ω₀ = sqrt(k / m)
    if (mass_ <= 0) return 0;
    return std::sqrt(stiffness_ / mass_);
}

float SpringAnimation::estimatedDuration() const {
    // Approximate: time for amplitude to decay below threshold
    float ratio = dampingRatio();
    float omega = naturalFrequency();
    if (omega < 0.0001f) return 0;

    if (ratio >= 1.0f) {
        // Overdamped / critically damped
        return 4.0f / (ratio * omega);
    } else {
        // Underdamped: envelope decay time
        float decayRate = ratio * omega;
        if (decayRate < 0.0001f) return 10.0f; // Very low damping
        return -std::log(restThreshold_ / std::max(std::abs(currentValue_ - targetValue_), 0.001f)) / decayRate;
    }
}

SpringAnimation SpringAnimation::gentle() {
    return SpringAnimation(100.0f, 15.0f, 1.0f);
}

SpringAnimation SpringAnimation::responsive() {
    return SpringAnimation(300.0f, 20.0f, 1.0f);
}

SpringAnimation SpringAnimation::bouncy() {
    return SpringAnimation(400.0f, 10.0f, 1.0f);
}

SpringAnimation SpringAnimation::stiff() {
    return SpringAnimation(800.0f, 30.0f, 1.0f);
}

SpringAnimation SpringAnimation::criticallyDamped(float stiffness) {
    float mass = 1.0f;
    float damping = 2.0f * std::sqrt(stiffness * mass);
    return SpringAnimation(stiffness, damping, mass);
}

} // namespace NXRender
