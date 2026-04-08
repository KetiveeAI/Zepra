// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "animation/keyframe.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

// ==================================================================
// KeyframeSet
// ==================================================================

void KeyframeSet::addKeyframe(float offset, float value, EasingFunction easing) {
    keyframes.push_back(Keyframe(offset, value, easing));
    sortKeyframes();
}

void KeyframeSet::sortKeyframes() {
    std::sort(keyframes.begin(), keyframes.end(),
              [](const Keyframe& a, const Keyframe& b) {
                  return a.offset < b.offset;
              });
}

float KeyframeSet::interpolate(float progress) const {
    if (keyframes.empty()) return 0.0f;
    if (keyframes.size() == 1) return keyframes[0].value;

    // Clamp progress
    progress = std::clamp(progress, 0.0f, 1.0f);

    // Find the two keyframes surrounding progress
    const Keyframe* prev = &keyframes.front();
    const Keyframe* next = &keyframes.back();

    for (size_t i = 0; i < keyframes.size() - 1; i++) {
        if (progress >= keyframes[i].offset && progress <= keyframes[i + 1].offset) {
            prev = &keyframes[i];
            next = &keyframes[i + 1];
            break;
        }
    }

    // Compute local progress within this segment
    float segmentLength = next->offset - prev->offset;
    if (segmentLength <= 0.0f) return prev->value;

    float localT = (progress - prev->offset) / segmentLength;

    // Apply the easing function of the start keyframe
    float easedT = prev->easing(localT);

    // Linear interpolation between values
    return prev->value + (next->value - prev->value) * easedT;
}

// ==================================================================
// KeyframeAnimation
// ==================================================================

KeyframeAnimation::KeyframeAnimation() {}

void KeyframeAnimation::start() {
    running_ = true;
    paused_ = false;
    complete_ = false;
    elapsedMs_ = 0;
    currentRepeat_ = 0;
    reversed_ = false;

    // Apply fill mode backwards — set to first keyframe value
    if (fillMode_ >= 2 && !keyframes_.keyframes.empty()) {
        currentValue_ = keyframes_.keyframes.front().value;
        if (setter_) setter_(currentValue_);
    }
}

void KeyframeAnimation::stop() {
    running_ = false;
    paused_ = false;
}

void KeyframeAnimation::pause() {
    if (running_) paused_ = true;
}

void KeyframeAnimation::resume() {
    paused_ = false;
}

float KeyframeAnimation::progress() const {
    if (durationMs_ <= 0) return 1.0f;
    float effective = elapsedMs_ - static_cast<float>(delayMs_);
    if (effective < 0) return 0.0f;
    return std::clamp(effective / static_cast<float>(durationMs_), 0.0f, 1.0f);
}

bool KeyframeAnimation::update(float deltaMs) {
    if (!running_ || paused_) return running_;

    elapsedMs_ += deltaMs;

    // Handle delay
    float effective = elapsedMs_ - static_cast<float>(delayMs_);
    if (effective < 0) return true; // Still in delay

    float dur = static_cast<float>(durationMs_);
    if (dur <= 0) {
        complete_ = true;
        running_ = false;
        if (onComplete_) onComplete_();
        return false;
    }

    float p = effective / dur;

    if (p >= 1.0f) {
        // Iteration complete
        if (repeatCount_ == -1 || currentRepeat_ < repeatCount_) {
            currentRepeat_++;
            elapsedMs_ = static_cast<float>(delayMs_); // Reset elapsed
            if (alternate_) reversed_ = !reversed_;
            return true;
        }

        // Animation complete
        p = 1.0f;
        complete_ = true;
        running_ = false;

        // Apply fill mode forwards
        if (fillMode_ == 1 || fillMode_ == 3) {
            float finalP = reversed_ ? (1.0f - p) : p;
            currentValue_ = keyframes_.interpolate(finalP);
            if (setter_) setter_(currentValue_);
        }

        if (onComplete_) onComplete_();
        return false;
    }

    float interpP = reversed_ ? (1.0f - p) : p;
    currentValue_ = keyframes_.interpolate(interpP);
    if (setter_) setter_(currentValue_);

    return true;
}

// ==================================================================
// KeyframeRegistry
// ==================================================================

KeyframeRegistry& KeyframeRegistry::instance() {
    static KeyframeRegistry reg;
    return reg;
}

KeyframeRegistry::KeyframeRegistry() {
    // Register built-in animations
    registerSet(fadeIn());
    registerSet(fadeOut());
    registerSet(slideInLeft());
    registerSet(slideInRight());
    registerSet(slideInTop());
    registerSet(slideInBottom());
    registerSet(scaleUp());
    registerSet(scaleDown());
    registerSet(bounce());
    registerSet(shake());
    registerSet(pulse());
    registerSet(spin());
}

void KeyframeRegistry::registerSet(const KeyframeSet& set) {
    sets_[set.name] = set;
}

const KeyframeSet* KeyframeRegistry::find(const std::string& name) const {
    auto it = sets_.find(name);
    return it != sets_.end() ? &it->second : nullptr;
}

void KeyframeRegistry::remove(const std::string& name) {
    sets_.erase(name);
}

void KeyframeRegistry::clear() {
    sets_.clear();
}

KeyframeSet KeyframeRegistry::fadeIn() {
    KeyframeSet ks;
    ks.name = "fadeIn";
    ks.keyframes = {
        Keyframe(0.0f, 0.0f, Easing::easeOut),
        Keyframe(1.0f, 1.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::fadeOut() {
    KeyframeSet ks;
    ks.name = "fadeOut";
    ks.keyframes = {
        Keyframe(0.0f, 1.0f, Easing::easeIn),
        Keyframe(1.0f, 0.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::slideInLeft() {
    KeyframeSet ks;
    ks.name = "slideInLeft";
    ks.keyframes = {
        Keyframe(0.0f, -100.0f, Easing::easeOut),
        Keyframe(1.0f, 0.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::slideInRight() {
    KeyframeSet ks;
    ks.name = "slideInRight";
    ks.keyframes = {
        Keyframe(0.0f, 100.0f, Easing::easeOut),
        Keyframe(1.0f, 0.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::slideInTop() {
    KeyframeSet ks;
    ks.name = "slideInTop";
    ks.keyframes = {
        Keyframe(0.0f, -100.0f, Easing::easeOut),
        Keyframe(1.0f, 0.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::slideInBottom() {
    KeyframeSet ks;
    ks.name = "slideInBottom";
    ks.keyframes = {
        Keyframe(0.0f, 100.0f, Easing::easeOut),
        Keyframe(1.0f, 0.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::scaleUp() {
    KeyframeSet ks;
    ks.name = "scaleUp";
    ks.keyframes = {
        Keyframe(0.0f, 0.0f, Easing::easeOutBack),
        Keyframe(1.0f, 1.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::scaleDown() {
    KeyframeSet ks;
    ks.name = "scaleDown";
    ks.keyframes = {
        Keyframe(0.0f, 1.0f, Easing::easeIn),
        Keyframe(1.0f, 0.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::bounce() {
    KeyframeSet ks;
    ks.name = "bounce";
    ks.keyframes = {
        Keyframe(0.0f, 0.0f, Easing::linear),
        Keyframe(0.2f, 0.0f, Easing::easeOut),
        Keyframe(0.4f, -30.0f, Easing::easeIn),
        Keyframe(0.5f, 0.0f, Easing::easeOut),
        Keyframe(0.6f, -15.0f, Easing::easeIn),
        Keyframe(0.8f, 0.0f, Easing::easeOut),
        Keyframe(0.9f, -4.0f, Easing::easeIn),
        Keyframe(1.0f, 0.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::shake() {
    KeyframeSet ks;
    ks.name = "shake";
    ks.keyframes = {
        Keyframe(0.0f,  0.0f, Easing::linear),
        Keyframe(0.1f, -10.0f, Easing::linear),
        Keyframe(0.2f,  10.0f, Easing::linear),
        Keyframe(0.3f, -10.0f, Easing::linear),
        Keyframe(0.4f,  10.0f, Easing::linear),
        Keyframe(0.5f, -10.0f, Easing::linear),
        Keyframe(0.6f,  10.0f, Easing::linear),
        Keyframe(0.7f, -10.0f, Easing::linear),
        Keyframe(0.8f,  10.0f, Easing::linear),
        Keyframe(0.9f, -10.0f, Easing::linear),
        Keyframe(1.0f,  0.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::pulse() {
    KeyframeSet ks;
    ks.name = "pulse";
    ks.keyframes = {
        Keyframe(0.0f, 1.0f, Easing::easeInOut),
        Keyframe(0.5f, 1.05f, Easing::easeInOut),
        Keyframe(1.0f, 1.0f)
    };
    return ks;
}

KeyframeSet KeyframeRegistry::spin() {
    KeyframeSet ks;
    ks.name = "spin";
    ks.keyframes = {
        Keyframe(0.0f, 0.0f, Easing::linear),
        Keyframe(1.0f, 360.0f)
    };
    return ks;
}

} // namespace NXRender
