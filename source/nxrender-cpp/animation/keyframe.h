// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file keyframe.h
 * @brief CSS-style keyframe animation with multi-keyframe interpolation.
 */

#pragma once

#include "easing.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

namespace NXRender {

/**
 * @brief A single keyframe at a specific offset in the animation timeline.
 */
struct Keyframe {
    float offset;              // 0.0 - 1.0 (0% to 100%)
    float value;               // Property value at this offset
    EasingFunction easing;     // Easing to apply FROM this keyframe TO the next

    Keyframe() : offset(0), value(0), easing(Easing::linear) {}
    Keyframe(float off, float val, EasingFunction ease = Easing::linear)
        : offset(off), value(val), easing(ease) {}
};

/**
 * @brief A named set of keyframes (like @keyframes in CSS).
 */
struct KeyframeSet {
    std::string name;
    std::vector<Keyframe> keyframes; // Sorted by offset

    void addKeyframe(float offset, float value, EasingFunction easing = Easing::linear);
    void sortKeyframes();

    float interpolate(float progress) const;
};

/**
 * @brief Animation driven by keyframes.
 */
class KeyframeAnimation {
public:
    using ValueSetter = std::function<void(float)>;
    using Callback = std::function<void()>;

    KeyframeAnimation();

    // Configure
    KeyframeAnimation& keyframes(const KeyframeSet& ks) { keyframes_ = ks; return *this; }
    KeyframeAnimation& duration(int ms) { durationMs_ = ms; return *this; }
    KeyframeAnimation& delay(int ms) { delayMs_ = ms; return *this; }
    KeyframeAnimation& repeat(int count) { repeatCount_ = count; return *this; } // -1 = infinite
    KeyframeAnimation& alternate(bool enable) { alternate_ = enable; return *this; }
    KeyframeAnimation& fillMode(int mode) { fillMode_ = mode; return *this; } // 0=none, 1=forwards, 2=backwards, 3=both
    KeyframeAnimation& setter(ValueSetter fn) { setter_ = fn; return *this; }
    KeyframeAnimation& onComplete(Callback cb) { onComplete_ = cb; return *this; }

    // Control
    void start();
    void stop();
    void pause();
    void resume();
    bool update(float deltaMs);

    // State
    bool isRunning() const { return running_; }
    bool isPaused() const { return paused_; }
    bool isComplete() const { return complete_; }
    float currentValue() const { return currentValue_; }
    float progress() const;

private:
    KeyframeSet keyframes_;
    int durationMs_ = 300;
    int delayMs_ = 0;
    int repeatCount_ = 0;
    bool alternate_ = false;
    int fillMode_ = 0;
    ValueSetter setter_;
    Callback onComplete_;

    bool running_ = false;
    bool paused_ = false;
    bool complete_ = false;
    float elapsedMs_ = 0;
    float currentValue_ = 0;
    int currentRepeat_ = 0;
    bool reversed_ = false;
};

/**
 * @brief Global registry of named keyframe sets.
 */
class KeyframeRegistry {
public:
    static KeyframeRegistry& instance();

    void registerSet(const KeyframeSet& set);
    const KeyframeSet* find(const std::string& name) const;
    void remove(const std::string& name);
    void clear();

    // Built-in animations
    static KeyframeSet fadeIn();
    static KeyframeSet fadeOut();
    static KeyframeSet slideInLeft();
    static KeyframeSet slideInRight();
    static KeyframeSet slideInTop();
    static KeyframeSet slideInBottom();
    static KeyframeSet scaleUp();
    static KeyframeSet scaleDown();
    static KeyframeSet bounce();
    static KeyframeSet shake();
    static KeyframeSet pulse();
    static KeyframeSet spin();

private:
    KeyframeRegistry();
    std::unordered_map<std::string, KeyframeSet> sets_;
};

} // namespace NXRender
