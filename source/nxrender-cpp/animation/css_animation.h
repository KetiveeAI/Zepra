// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

namespace NXRender {

// ==================================================================
// Web Animations API — CSS Animations / Transitions timeline
// ==================================================================

enum class FillMode : uint8_t {
    None, Forwards, Backwards, Both, Auto
};

enum class PlaybackDirection : uint8_t {
    Normal, Reverse, Alternate, AlternateReverse
};

enum class CompositeOp : uint8_t {
    Replace, Add, Accumulate
};

enum class AnimationPlayState : uint8_t {
    Idle, Running, Paused, Finished
};

// ==================================================================
// CSS timing function
// ==================================================================

struct CSSTimingFunction {
    enum class Type : uint8_t {
        Linear, Ease, EaseIn, EaseOut, EaseInOut,
        CubicBezier, Steps, Spring
    } type = Type::Ease;

    // Cubic bezier
    float x1 = 0, y1 = 0, x2 = 1, y2 = 1;

    // Steps
    int stepCount = 1;
    enum class StepPosition { Start, End, JumpNone, JumpBoth } stepPos = StepPosition::End;

    // Spring
    float mass = 1, stiffness = 100, damping = 10;

    float evaluate(float t) const;
    static CSSTimingFunction ease();
    static CSSTimingFunction linear();
    static CSSTimingFunction easeIn();
    static CSSTimingFunction easeOut();
    static CSSTimingFunction easeInOut();
    static CSSTimingFunction cubicBezier(float x1, float y1, float x2, float y2);
    static CSSTimingFunction steps(int count, StepPosition pos = StepPosition::End);
    static CSSTimingFunction parse(const std::string& str);
};

// ==================================================================
// Keyframe
// ==================================================================

struct AnimationPropertyValue {
    std::string property;
    std::string value;
    CompositeOp composite = CompositeOp::Replace;
};

struct CSSKeyframe {
    float offset = 0;       // 0.0 - 1.0
    CSSTimingFunction easing;
    std::vector<AnimationPropertyValue> properties;
    CompositeOp composite = CompositeOp::Replace;
};

// ==================================================================
// Keyframe effect
// ==================================================================

class KeyframeEffect {
public:
    KeyframeEffect();
    ~KeyframeEffect();

    void setTarget(void* element) { target_ = element; }
    void* target() const { return target_; }

    void setKeyframes(const std::vector<CSSKeyframe>& keyframes) { keyframes_ = keyframes; }
    const std::vector<CSSKeyframe>& keyframes() const { return keyframes_; }

    void setDuration(double ms) { duration_ = ms; }
    double duration() const { return duration_; }

    void setDelay(double ms) { delay_ = ms; }
    double delay() const { return delay_; }

    void setEndDelay(double ms) { endDelay_ = ms; }
    double endDelay() const { return endDelay_; }

    void setIterations(double count) { iterations_ = count; }
    double iterations() const { return iterations_; }

    void setIterationStart(double start) { iterationStart_ = start; }
    void setDirection(PlaybackDirection dir) { direction_ = dir; }
    void setFillMode(FillMode fill) { fill_ = fill; }
    void setComposite(CompositeOp op) { composite_ = op; }

    PlaybackDirection direction() const { return direction_; }
    FillMode fillMode() const { return fill_; }

    // Compute the active duration
    double activeDuration() const;
    double endTime() const;

    // Get interpolated property values at given local time
    struct ComputedValue {
        std::string property;
        float numericValue = 0;
        std::string stringValue;
    };
    std::vector<ComputedValue> getComputedValues(double localTime) const;

    // Find bounding keyframes for a given progress
    void findKeyframes(float progress, const CSSKeyframe*& from,
                       const CSSKeyframe*& to, float& fraction) const;

private:
    void* target_ = nullptr;
    std::vector<CSSKeyframe> keyframes_;
    double duration_ = 0;
    double delay_ = 0;
    double endDelay_ = 0;
    double iterations_ = 1;
    double iterationStart_ = 0;
    PlaybackDirection direction_ = PlaybackDirection::Normal;
    FillMode fill_ = FillMode::None;
    CompositeOp composite_ = CompositeOp::Replace;
};

// ==================================================================
// Animation timeline
// ==================================================================

class AnimationTimeline {
public:
    virtual ~AnimationTimeline() = default;
    virtual double currentTime() const = 0;
    virtual bool isMonotonicallyIncreasing() const { return true; }
};

class DocumentTimeline : public AnimationTimeline {
public:
    DocumentTimeline();
    double currentTime() const override;
    void setOriginTime(double originMs) { originTime_ = originMs; }
    void tick(double nowMs);

private:
    double originTime_ = 0;
    double currentTime_ = 0;
};

class ScrollTimeline : public AnimationTimeline {
public:
    enum class Axis : uint8_t { Block, Inline, X, Y };

    ScrollTimeline();
    double currentTime() const override;
    bool isMonotonicallyIncreasing() const override { return false; }

    void setSource(void* scrollContainer) { source_ = scrollContainer; }
    void setAxis(Axis axis) { axis_ = axis; }
    void updateProgress(float progress) { progress_ = progress; }

private:
    void* source_ = nullptr;
    Axis axis_ = Axis::Block;
    float progress_ = 0;
};

class ViewTimeline : public ScrollTimeline {
public:
    enum class Inset { Auto, Length };

    ViewTimeline();
    void setSubject(void* element) { subject_ = element; }
    void setInsetStart(float px) { insetStart_ = px; }
    void setInsetEnd(float px) { insetEnd_ = px; }

private:
    void* subject_ = nullptr;
    float insetStart_ = 0;
    float insetEnd_ = 0;
};

// ==================================================================
// CSS Animation
// ==================================================================

class CSSAnimation {
public:
    CSSAnimation();
    ~CSSAnimation();

    // Identity
    void setId(const std::string& id) { id_ = id; }
    const std::string& id() const { return id_; }
    void setAnimationName(const std::string& name) { name_ = name; }
    const std::string& animationName() const { return name_; }

    // Timeline
    void setTimeline(std::shared_ptr<AnimationTimeline> tl) { timeline_ = tl; }
    AnimationTimeline* timeline() const { return timeline_.get(); }

    // Effect
    void setEffect(std::shared_ptr<KeyframeEffect> effect) { effect_ = effect; }
    KeyframeEffect* effect() const { return effect_.get(); }

    // Playback
    void play();
    void pause();
    void cancel();
    void finish();
    void reverse();
    void updatePlaybackRate(double rate);

    double currentTime() const { return currentTime_; }
    void setCurrentTime(double ms);
    double startTime() const { return startTime_; }
    void setStartTime(double ms);
    double playbackRate() const { return playbackRate_; }
    AnimationPlayState playState() const { return playState_; }

    // Pending play/pause
    bool pending() const { return pendingPlay_ || pendingPause_; }

    // Events
    using EventCallback = std::function<void()>;
    void onFinish(EventCallback cb) { onFinish_ = cb; }
    void onCancel(EventCallback cb) { onCancel_ = cb; }
    void onRemove(EventCallback cb) { onRemove_ = cb; }

    // Update — call each frame
    void tick(double timelineTime);

    // Is relevant (contributes to rendering)
    bool isRelevant() const;

    // Computed values at current time
    std::vector<KeyframeEffect::ComputedValue> computedValues() const;

private:
    std::string id_;
    std::string name_;
    std::shared_ptr<AnimationTimeline> timeline_;
    std::shared_ptr<KeyframeEffect> effect_;

    AnimationPlayState playState_ = AnimationPlayState::Idle;
    double currentTime_ = 0;
    double startTime_ = 0;
    double playbackRate_ = 1.0;
    double holdTime_ = 0;
    bool pendingPlay_ = false;
    bool pendingPause_ = false;

    EventCallback onFinish_;
    EventCallback onCancel_;
    EventCallback onRemove_;

    double calculateCurrentTime(double timelineTime) const;
    void updateFinishedState();
};

// ==================================================================
// Animation controller — manages all animations
// ==================================================================

class CSSAnimationController {
public:
    CSSAnimationController();
    ~CSSAnimationController();

    void setDocumentTimeline(std::shared_ptr<DocumentTimeline> tl) { docTimeline_ = tl; }

    // Create animation from @keyframes
    std::shared_ptr<CSSAnimation> createAnimation(
        void* target,
        const std::string& name,
        const std::vector<CSSKeyframe>& keyframes,
        double durationMs,
        double delayMs = 0,
        double iterationCount = 1,
        PlaybackDirection direction = PlaybackDirection::Normal,
        FillMode fill = FillMode::None,
        const CSSTimingFunction& easing = CSSTimingFunction::ease()
    );

    // Create transition
    std::shared_ptr<CSSAnimation> createTransition(
        void* target,
        const std::string& property,
        const std::string& fromValue,
        const std::string& toValue,
        double durationMs,
        double delayMs = 0,
        const CSSTimingFunction& easing = CSSTimingFunction::ease()
    );

    // Scroll-driven animation
    std::shared_ptr<CSSAnimation> createScrollAnimation(
        void* target,
        const std::vector<CSSKeyframe>& keyframes,
        std::shared_ptr<ScrollTimeline> scrollTimeline
    );

    // Remove
    void cancelAnimation(const std::string& id);
    void cancelAnimationsForTarget(void* target);
    void cancelAll();

    // Tick all animations
    void tick(double nowMs);

    // Get active animations for element
    std::vector<CSSAnimation*> animationsForTarget(void* target) const;

    // Check if element has active animations
    bool hasActiveAnimations(void* target) const;

    // Get all computed styles from active animations
    std::vector<KeyframeEffect::ComputedValue> getAnimatedValues(void* target) const;

    size_t activeCount() const { return animations_.size(); }

private:
    std::shared_ptr<DocumentTimeline> docTimeline_;
    std::vector<std::shared_ptr<CSSAnimation>> animations_;

    void removeFinished();
    std::string generateId();
    int nextId_ = 0;
};

// ==================================================================
// @keyframes registry
// ==================================================================

class KeyframesRegistry {
public:
    void registerKeyframes(const std::string& name, const std::vector<CSSKeyframe>& keyframes);
    const std::vector<CSSKeyframe>* find(const std::string& name) const;
    void unregister(const std::string& name);
    void clear();

private:
    std::unordered_map<std::string, std::vector<CSSKeyframe>> registry_;
};

} // namespace NXRender
