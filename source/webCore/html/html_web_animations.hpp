/**
 * @file html_web_animations.hpp
 * @brief Web Animations API
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <vector>
#include <functional>
#include <any>

namespace Zepra::WebCore {

/**
 * @brief Animation play state
 */
enum class AnimationPlayState {
    Idle,
    Running,
    Paused,
    Finished
};

/**
 * @brief Fill mode
 */
enum class FillMode {
    None,
    Forwards,
    Backwards,
    Both,
    Auto
};

/**
 * @brief Playback direction
 */
enum class PlaybackDirection {
    Normal,
    Reverse,
    Alternate,
    AlternateReverse
};

/**
 * @brief Keyframe
 */
struct Keyframe {
    double offset = 0;  // 0 to 1
    std::string easing = "linear";
    std::string composite = "replace";
    std::unordered_map<std::string, std::string> properties;
};

/**
 * @brief Effect timing
 */
struct EffectTiming {
    double delay = 0;
    double endDelay = 0;
    double duration = 0;
    std::string easing = "linear";
    double iterations = 1;
    double iterationStart = 0;
    FillMode fill = FillMode::Auto;
    PlaybackDirection direction = PlaybackDirection::Normal;
};

/**
 * @brief Computed effect timing
 */
struct ComputedEffectTiming : EffectTiming {
    double progress = 0;
    double currentIteration = 0;
    double activeDuration = 0;
    double endTime = 0;
    double localTime = 0;
};

/**
 * @brief Animation effect
 */
class AnimationEffect {
public:
    virtual ~AnimationEffect() = default;
    
    EffectTiming getTiming() const { return timing_; }
    ComputedEffectTiming getComputedTiming() const;
    void updateTiming(const EffectTiming& timing);
    
protected:
    EffectTiming timing_;
};

/**
 * @brief Keyframe effect
 */
class KeyframeEffect : public AnimationEffect {
public:
    KeyframeEffect(HTMLElement* target, const std::vector<Keyframe>& keyframes,
                   const EffectTiming& timing = {});
    ~KeyframeEffect() override = default;
    
    HTMLElement* target() const { return target_; }
    void setTarget(HTMLElement* t) { target_ = t; }
    
    std::string composite() const { return composite_; }
    void setComposite(const std::string& c) { composite_ = c; }
    
    std::vector<Keyframe> getKeyframes() const { return keyframes_; }
    void setKeyframes(const std::vector<Keyframe>& kf) { keyframes_ = kf; }
    
private:
    HTMLElement* target_;
    std::vector<Keyframe> keyframes_;
    std::string composite_ = "replace";
};

/**
 * @brief Animation
 */
class Animation {
public:
    Animation(AnimationEffect* effect = nullptr, class DocumentTimeline* timeline = nullptr);
    ~Animation();
    
    // Properties
    std::string id() const { return id_; }
    void setId(const std::string& id) { id_ = id; }
    
    AnimationEffect* effect() const { return effect_; }
    void setEffect(AnimationEffect* e) { effect_ = e; }
    
    double startTime() const { return startTime_; }
    void setStartTime(double t) { startTime_ = t; }
    
    double currentTime() const { return currentTime_; }
    void setCurrentTime(double t) { currentTime_ = t; }
    
    double playbackRate() const { return playbackRate_; }
    void setPlaybackRate(double r) { playbackRate_ = r; }
    
    AnimationPlayState playState() const { return playState_; }
    bool pending() const { return pending_; }
    
    // Methods
    void play();
    void pause();
    void finish();
    void cancel();
    void reverse();
    void updatePlaybackRate(double rate);
    
    void commitStyles();
    void persist();
    
    // Events
    std::function<void()> onFinish;
    std::function<void()> onCancel;
    std::function<void()> onRemove;
    
private:
    std::string id_;
    AnimationEffect* effect_ = nullptr;
    double startTime_ = 0;
    double currentTime_ = 0;
    double playbackRate_ = 1;
    AnimationPlayState playState_ = AnimationPlayState::Idle;
    bool pending_ = false;
};

/**
 * @brief Document timeline
 */
class DocumentTimeline {
public:
    DocumentTimeline();
    ~DocumentTimeline();
    
    double currentTime() const { return currentTime_; }
    
private:
    double currentTime_ = 0;
};

/**
 * @brief Animatable mixin
 */
class Animatable {
public:
    virtual ~Animatable() = default;
    
    Animation* animate(const std::vector<Keyframe>& keyframes,
                       const EffectTiming& options = {});
    Animation* animate(const std::vector<Keyframe>& keyframes, double duration);
    
    std::vector<Animation*> getAnimations();
    
protected:
    std::vector<std::unique_ptr<Animation>> animations_;
};

} // namespace Zepra::WebCore
