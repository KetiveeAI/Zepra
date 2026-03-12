// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file animator.h
 * @brief Animation system for NXRender widgets
 * 
 * Provides property animation with timing, easing, and chaining.
 */

#pragma once

#include "easing.h"
#include "spring.h"
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>

namespace NXRender {

class Widget;

/**
 * @brief Animatable property types
 */
enum class AnimatableProperty {
    X, Y, Width, Height,
    Opacity, Scale, ScaleX, ScaleY,
    Rotation,
    BackgroundColor, ForegroundColor,
    BorderRadius, BorderWidth,
    PaddingTop, PaddingRight, PaddingBottom, PaddingLeft,
    MarginTop, MarginRight, MarginBottom, MarginLeft,
    FontSize,
    Custom  // For named custom properties
};

/**
 * @brief Animation state
 */
enum class AnimationState {
    Idle,
    Running,
    Paused,
    Completed
};

/**
 * @brief Single property animation
 */
class Animation {
public:
    using Callback = std::function<void()>;
    using ValueSetter = std::function<void(float)>;
    using ValueGetter = std::function<float()>;
    
    Animation() = default;
    
    // Configure animation
    Animation& from(float value) { fromValue_ = value; hasFrom_ = true; return *this; }
    Animation& to(float value) { toValue_ = value; return *this; }
    Animation& duration(int ms) { durationMs_ = ms; return *this; }
    Animation& easing(EasingFunction fn) { easingFn_ = fn; return *this; }
    Animation& delay(int ms) { delayMs_ = ms; return *this; }
    Animation& repeat(int count) { repeatCount_ = count; return *this; } // -1 = infinite
    Animation& yoyo(bool enabled) { yoyo_ = enabled; return *this; }
    
    // Callbacks
    Animation& onStart(Callback cb) { onStart_ = cb; return *this; }
    Animation& onUpdate(std::function<void(float)> cb) { onUpdate_ = cb; return *this; }
    Animation& onComplete(Callback cb) { onComplete_ = cb; return *this; }
    
    // Value accessors
    Animation& setter(ValueSetter fn) { setter_ = fn; return *this; }
    Animation& getter(ValueGetter fn) { getter_ = fn; return *this; }
    
    // Control
    void start();
    void stop();
    void pause();
    void resume();
    
    // Update (returns true if still running)
    bool update(float deltaMs);
    
    // State
    AnimationState state() const { return state_; }
    float currentValue() const { return currentValue_; }
    float progress() const;
    
private:
    float fromValue_ = 0;
    float toValue_ = 1;
    bool hasFrom_ = false;
    int durationMs_ = 300;
    int delayMs_ = 0;
    int repeatCount_ = 0;
    bool yoyo_ = false;
    
    EasingFunction easingFn_ = Easing::easeOut;
    ValueSetter setter_;
    ValueGetter getter_;
    Callback onStart_, onComplete_;
    std::function<void(float)> onUpdate_;
    
    AnimationState state_ = AnimationState::Idle;
    float elapsedMs_ = 0;
    float currentValue_ = 0;
    int currentRepeat_ = 0;
    bool yoyoReverse_ = false;
};

/**
 * @brief Animation group for sequencing
 */
class AnimationGroup {
public:
    enum class Mode { Parallel, Sequential };
    
    AnimationGroup& add(std::shared_ptr<Animation> anim);
    AnimationGroup& mode(Mode m) { mode_ = m; return *this; }
    AnimationGroup& delay(int ms) { delayMs_ = ms; return *this; }
    AnimationGroup& onComplete(std::function<void()> cb) { onComplete_ = cb; return *this; }
    
    void start();
    void stop();
    bool update(float deltaMs);
    bool isComplete() const;
    
private:
    std::vector<std::shared_ptr<Animation>> animations_;
    Mode mode_ = Mode::Parallel;
    int delayMs_ = 0;
    float elapsedMs_ = 0;
    size_t currentIndex_ = 0;
    std::function<void()> onComplete_;
};

/**
 * @brief Global animator managing all active animations
 */
class Animator {
public:
    static Animator& instance();
    
    // Create and start animation
    std::shared_ptr<Animation> animate(Widget* widget, AnimatableProperty prop,
                                        float to, int durationMs = 300,
                                        EasingFunction easing = Easing::easeOut);
    
    std::shared_ptr<Animation> animate(Widget* widget, const std::string& propertyName,
                                        float to, int durationMs = 300,
                                        EasingFunction easing = Easing::easeOut);
    
    // Spring animation
    std::shared_ptr<Animation> spring(Widget* widget, AnimatableProperty prop,
                                       float to, SpringConfig config = {});
    
    // Stop animations
    void stopAll(Widget* widget);
    void stopProperty(Widget* widget, AnimatableProperty prop);
    
    // Update all animations (call each frame)
    void update(float deltaMs);
    void update(); // Auto-calculates delta
    
    // Check if any animations running
    bool hasActiveAnimations() const { return !animations_.empty(); }
    
private:
    Animator() = default;
    
    struct AnimationEntry {
        Widget* widget;
        AnimatableProperty property;
        std::shared_ptr<Animation> animation;
    };
    
    std::vector<AnimationEntry> animations_;
    std::chrono::steady_clock::time_point lastUpdate_;
    bool firstUpdate_ = true;
};

// ============================================================================
// Convenience macros / functions
// ============================================================================

// Quick animate helper
inline std::shared_ptr<Animation> animate(Widget* widget, AnimatableProperty prop,
                                           float to, int durationMs = 300) {
    return Animator::instance().animate(widget, prop, to, durationMs);
}

// Quick spring helper
inline std::shared_ptr<Animation> spring(Widget* widget, AnimatableProperty prop,
                                          float to, SpringConfig config = {}) {
    return Animator::instance().spring(widget, prop, to, config);
}

} // namespace NXRender
