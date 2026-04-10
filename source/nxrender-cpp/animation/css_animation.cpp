// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "animation/css_animation.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <sstream>

namespace NXRender {

// ==================================================================
// CSSTimingFunction
// ==================================================================

static float solveCubicBezier(float x1, float y1, float x2, float y2, float t) {
    // Newton-Raphson to solve cubic bezier for x, then evaluate y
    float x = t;
    for (int i = 0; i < 8; i++) {
        float cx = 3 * x1 * x * (1 - x) * (1 - x) + 3 * x2 * x * x * (1 - x) + x * x * x - t;
        float dx = 3 * x1 * (1 - x) * (1 - x) - 6 * x1 * x * (1 - x) +
                   6 * x2 * x * (1 - x) - 3 * x2 * x * x + 3 * x * x;
        if (std::abs(dx) < 1e-6f) break;
        x -= cx / dx;
    }
    x = std::clamp(x, 0.0f, 1.0f);
    return 3 * y1 * x * (1 - x) * (1 - x) + 3 * y2 * x * x * (1 - x) + x * x * x;
}

float CSSTimingFunction::evaluate(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);

    switch (type) {
        case Type::Linear:
            return t;
        case Type::Ease:
            return solveCubicBezier(0.25f, 0.1f, 0.25f, 1.0f, t);
        case Type::EaseIn:
            return solveCubicBezier(0.42f, 0.0f, 1.0f, 1.0f, t);
        case Type::EaseOut:
            return solveCubicBezier(0.0f, 0.0f, 0.58f, 1.0f, t);
        case Type::EaseInOut:
            return solveCubicBezier(0.42f, 0.0f, 0.58f, 1.0f, t);
        case Type::CubicBezier:
            return solveCubicBezier(x1, y1, x2, y2, t);
        case Type::Steps: {
            if (stepCount <= 0) return t;
            float step = 1.0f / stepCount;
            switch (stepPos) {
                case StepPosition::Start:
                    return std::ceil(t * stepCount) * step;
                case StepPosition::End:
                    return std::floor(t * stepCount) * step;
                case StepPosition::JumpNone:
                    return std::round(t * (stepCount - 1)) / (stepCount - 1);
                case StepPosition::JumpBoth:
                    return std::round(t * (stepCount + 1)) / (stepCount + 1);
            }
            return t;
        }
        case Type::Spring: {
            float w0 = std::sqrt(stiffness / mass);
            float zeta = damping / (2 * std::sqrt(stiffness * mass));
            float wd = w0 * std::sqrt(1 - zeta * zeta);
            float timeScale = 5.0f;
            float s = t * timeScale;
            float envelope = std::exp(-zeta * w0 * s);
            return 1.0f - envelope * std::cos(wd * s);
        }
    }
    return t;
}

CSSTimingFunction CSSTimingFunction::ease() {
    CSSTimingFunction tf; tf.type = Type::Ease; return tf;
}
CSSTimingFunction CSSTimingFunction::linear() {
    CSSTimingFunction tf; tf.type = Type::Linear; return tf;
}
CSSTimingFunction CSSTimingFunction::easeIn() {
    CSSTimingFunction tf; tf.type = Type::EaseIn; return tf;
}
CSSTimingFunction CSSTimingFunction::easeOut() {
    CSSTimingFunction tf; tf.type = Type::EaseOut; return tf;
}
CSSTimingFunction CSSTimingFunction::easeInOut() {
    CSSTimingFunction tf; tf.type = Type::EaseInOut; return tf;
}
CSSTimingFunction CSSTimingFunction::cubicBezier(float x1, float y1, float x2, float y2) {
    CSSTimingFunction tf;
    tf.type = Type::CubicBezier;
    tf.x1 = x1; tf.y1 = y1; tf.x2 = x2; tf.y2 = y2;
    return tf;
}
CSSTimingFunction CSSTimingFunction::steps(int count, StepPosition pos) {
    CSSTimingFunction tf;
    tf.type = Type::Steps;
    tf.stepCount = count;
    tf.stepPos = pos;
    return tf;
}

CSSTimingFunction CSSTimingFunction::parse(const std::string& str) {
    if (str == "linear") return linear();
    if (str == "ease") return ease();
    if (str == "ease-in") return easeIn();
    if (str == "ease-out") return easeOut();
    if (str == "ease-in-out") return easeInOut();

    // cubic-bezier(x1, y1, x2, y2)
    if (str.find("cubic-bezier") == 0) {
        auto paren = str.find('(');
        auto end = str.rfind(')');
        if (paren != std::string::npos && end != std::string::npos) {
            std::string inner = str.substr(paren + 1, end - paren - 1);
            float x1 = 0, y1 = 0, x2 = 1, y2 = 1;
            if (sscanf(inner.c_str(), "%f,%f,%f,%f", &x1, &y1, &x2, &y2) == 4) {
                return cubicBezier(x1, y1, x2, y2);
            }
        }
    }

    // steps(n, position)
    if (str.find("steps") == 0) {
        auto paren = str.find('(');
        auto end = str.rfind(')');
        if (paren != std::string::npos && end != std::string::npos) {
            std::string inner = str.substr(paren + 1, end - paren - 1);
            int n = 1;
            sscanf(inner.c_str(), "%d", &n);
            StepPosition pos = StepPosition::End;
            if (inner.find("start") != std::string::npos) pos = StepPosition::Start;
            if (inner.find("jump-none") != std::string::npos) pos = StepPosition::JumpNone;
            if (inner.find("jump-both") != std::string::npos) pos = StepPosition::JumpBoth;
            return steps(n, pos);
        }
    }

    return ease();
}

// ==================================================================
// KeyframeEffect
// ==================================================================

KeyframeEffect::KeyframeEffect() {}
KeyframeEffect::~KeyframeEffect() {}

double KeyframeEffect::activeDuration() const {
    if (iterations_ <= 0 || duration_ <= 0) return 0;
    if (std::isinf(iterations_)) return std::numeric_limits<double>::infinity();
    return duration_ * iterations_;
}

double KeyframeEffect::endTime() const {
    return std::max(0.0, delay_ + activeDuration() + endDelay_);
}

void KeyframeEffect::findKeyframes(float progress, const CSSKeyframe*& from,
                                    const CSSKeyframe*& to, float& fraction) const {
    from = nullptr;
    to = nullptr;
    fraction = 0;

    if (keyframes_.empty()) return;
    if (keyframes_.size() == 1) {
        from = &keyframes_[0];
        to = &keyframes_[0];
        fraction = 0;
        return;
    }

    // Find bounding keyframes
    for (size_t i = 0; i < keyframes_.size() - 1; i++) {
        if (progress >= keyframes_[i].offset && progress <= keyframes_[i + 1].offset) {
            from = &keyframes_[i];
            to = &keyframes_[i + 1];
            float range = to->offset - from->offset;
            fraction = (range > 0) ? (progress - from->offset) / range : 0;
            // Apply easing
            fraction = from->easing.evaluate(fraction);
            return;
        }
    }

    // Before first or after last
    if (progress <= keyframes_.front().offset) {
        from = &keyframes_.front();
        to = &keyframes_.front();
    } else {
        from = &keyframes_.back();
        to = &keyframes_.back();
    }
}

std::vector<KeyframeEffect::ComputedValue> KeyframeEffect::getComputedValues(double localTime) const {
    std::vector<ComputedValue> values;
    if (keyframes_.empty()) return values;

    // Compute iteration progress
    double activeTime = localTime - delay_;
    if (activeTime < 0 || duration_ <= 0) return values;

    double overallProgress = activeTime / duration_;
    double iterationProgress = std::fmod(overallProgress, 1.0);

    // Direction
    int currentIteration = static_cast<int>(overallProgress);
    bool reverse = false;
    switch (direction_) {
        case PlaybackDirection::Normal: break;
        case PlaybackDirection::Reverse: reverse = true; break;
        case PlaybackDirection::Alternate: reverse = (currentIteration % 2 == 1); break;
        case PlaybackDirection::AlternateReverse: reverse = (currentIteration % 2 == 0); break;
    }
    if (reverse) iterationProgress = 1.0 - iterationProgress;

    float progress = static_cast<float>(iterationProgress);

    const CSSKeyframe* from;
    const CSSKeyframe* to;
    float fraction;
    findKeyframes(progress, from, to, fraction);

    if (!from || !to) return values;

    // Interpolate properties
    for (const auto& toProp : to->properties) {
        ComputedValue cv;
        cv.property = toProp.property;

        // Find matching property in 'from'
        const AnimationPropertyValue* fromProp = nullptr;
        for (const auto& fp : from->properties) {
            if (fp.property == toProp.property) { fromProp = &fp; break; }
        }

        if (fromProp) {
            // Numeric interpolation attempt
            float fromVal = std::strtof(fromProp->value.c_str(), nullptr);
            float toVal = std::strtof(toProp.value.c_str(), nullptr);
            cv.numericValue = fromVal + (toVal - fromVal) * fraction;
            cv.stringValue = std::to_string(cv.numericValue);
        } else {
            // Discrete: use 'to' value when fraction >= 0.5
            cv.stringValue = (fraction >= 0.5f) ? toProp.value : "";
        }

        values.push_back(cv);
    }

    return values;
}

// ==================================================================
// DocumentTimeline
// ==================================================================

DocumentTimeline::DocumentTimeline() {
    auto now = std::chrono::high_resolution_clock::now();
    originTime_ = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

double DocumentTimeline::currentTime() const {
    return currentTime_;
}

void DocumentTimeline::tick(double nowMs) {
    currentTime_ = nowMs - originTime_;
}

// ==================================================================
// ScrollTimeline
// ==================================================================

ScrollTimeline::ScrollTimeline() {}

double ScrollTimeline::currentTime() const {
    return static_cast<double>(progress_ * 100.0);
}

// ==================================================================
// ViewTimeline
// ==================================================================

ViewTimeline::ViewTimeline() {}

// ==================================================================
// CSSAnimation
// ==================================================================

CSSAnimation::CSSAnimation() {}
CSSAnimation::~CSSAnimation() {}

void CSSAnimation::play() {
    if (playState_ == AnimationPlayState::Finished) {
        currentTime_ = (playbackRate_ >= 0) ? 0 : effect_->endTime();
    }
    pendingPlay_ = true;
    pendingPause_ = false;
    playState_ = AnimationPlayState::Running;
}

void CSSAnimation::pause() {
    if (playState_ == AnimationPlayState::Paused) return;
    pendingPause_ = true;
    pendingPlay_ = false;
    holdTime_ = currentTime_;
    playState_ = AnimationPlayState::Paused;
}

void CSSAnimation::cancel() {
    playState_ = AnimationPlayState::Idle;
    currentTime_ = 0;
    startTime_ = 0;
    if (onCancel_) onCancel_();
}

void CSSAnimation::finish() {
    if (!effect_) return;
    if (playbackRate_ > 0) {
        currentTime_ = effect_->endTime();
    } else if (playbackRate_ < 0) {
        currentTime_ = 0;
    }
    playState_ = AnimationPlayState::Finished;
    if (onFinish_) onFinish_();
}

void CSSAnimation::reverse() {
    playbackRate_ = -playbackRate_;
    if (playState_ == AnimationPlayState::Finished) play();
}

void CSSAnimation::updatePlaybackRate(double rate) {
    double prevTime = currentTime_;
    playbackRate_ = rate;
    currentTime_ = prevTime;
}

void CSSAnimation::setCurrentTime(double ms) {
    currentTime_ = ms;
    holdTime_ = ms;
}

void CSSAnimation::setStartTime(double ms) {
    startTime_ = ms;
    pendingPlay_ = false;
}

void CSSAnimation::tick(double timelineTime) {
    if (playState_ == AnimationPlayState::Idle ||
        playState_ == AnimationPlayState::Paused) return;

    if (pendingPlay_) {
        startTime_ = timelineTime - currentTime_ / playbackRate_;
        pendingPlay_ = false;
    }

    currentTime_ = calculateCurrentTime(timelineTime);
    updateFinishedState();
}

double CSSAnimation::calculateCurrentTime(double timelineTime) const {
    return (timelineTime - startTime_) * playbackRate_;
}

void CSSAnimation::updateFinishedState() {
    if (!effect_) return;

    double endTime = effect_->endTime();
    if (playbackRate_ > 0 && currentTime_ >= endTime) {
        currentTime_ = endTime;
        playState_ = AnimationPlayState::Finished;
        if (onFinish_) onFinish_();
    } else if (playbackRate_ < 0 && currentTime_ <= 0) {
        currentTime_ = 0;
        playState_ = AnimationPlayState::Finished;
        if (onFinish_) onFinish_();
    }
}

bool CSSAnimation::isRelevant() const {
    if (playState_ == AnimationPlayState::Idle) return false;
    if (playState_ == AnimationPlayState::Finished) {
        if (!effect_) return false;
        return effect_->fillMode() == FillMode::Forwards ||
               effect_->fillMode() == FillMode::Both;
    }
    return true;
}

std::vector<KeyframeEffect::ComputedValue> CSSAnimation::computedValues() const {
    if (!effect_ || !isRelevant()) return {};
    return effect_->getComputedValues(currentTime_);
}

// ==================================================================
// CSSAnimationController
// ==================================================================

CSSAnimationController::CSSAnimationController() {}
CSSAnimationController::~CSSAnimationController() {}

std::string CSSAnimationController::generateId() {
    return "anim_" + std::to_string(nextId_++);
}

std::shared_ptr<CSSAnimation> CSSAnimationController::createAnimation(
    void* target,
    const std::string& name,
    const std::vector<CSSKeyframe>& keyframes,
    double durationMs,
    double delayMs,
    double iterationCount,
    PlaybackDirection direction,
    FillMode fill,
    const CSSTimingFunction& /*easing*/) {

    auto anim = std::make_shared<CSSAnimation>();
    anim->setId(generateId());
    anim->setAnimationName(name);

    auto effect = std::make_shared<KeyframeEffect>();
    effect->setTarget(target);
    effect->setKeyframes(keyframes);
    effect->setDuration(durationMs);
    effect->setDelay(delayMs);
    effect->setIterations(iterationCount);
    effect->setDirection(direction);
    effect->setFillMode(fill);

    anim->setEffect(effect);
    anim->setTimeline(docTimeline_);
    anim->play();

    animations_.push_back(anim);
    return anim;
}

std::shared_ptr<CSSAnimation> CSSAnimationController::createTransition(
    void* target,
    const std::string& property,
    const std::string& fromValue,
    const std::string& toValue,
    double durationMs,
    double delayMs,
    const CSSTimingFunction& easing) {

    std::vector<CSSKeyframe> keyframes(2);
    keyframes[0].offset = 0;
    keyframes[0].easing = easing;
    keyframes[0].properties.push_back({property, fromValue, CompositeOp::Replace});
    keyframes[1].offset = 1;
    keyframes[1].properties.push_back({property, toValue, CompositeOp::Replace});

    return createAnimation(target, "transition_" + property, keyframes,
                           durationMs, delayMs, 1,
                           PlaybackDirection::Normal, FillMode::Forwards, easing);
}

std::shared_ptr<CSSAnimation> CSSAnimationController::createScrollAnimation(
    void* target,
    const std::vector<CSSKeyframe>& keyframes,
    std::shared_ptr<ScrollTimeline> scrollTimeline) {

    auto anim = std::make_shared<CSSAnimation>();
    anim->setId(generateId());

    auto effect = std::make_shared<KeyframeEffect>();
    effect->setTarget(target);
    effect->setKeyframes(keyframes);
    effect->setDuration(100); // Scroll timelines use 0-100 progress
    anim->setEffect(effect);
    anim->setTimeline(scrollTimeline);
    anim->play();

    animations_.push_back(anim);
    return anim;
}

void CSSAnimationController::cancelAnimation(const std::string& id) {
    for (auto it = animations_.begin(); it != animations_.end(); ++it) {
        if ((*it)->id() == id) {
            (*it)->cancel();
            animations_.erase(it);
            return;
        }
    }
}

void CSSAnimationController::cancelAnimationsForTarget(void* target) {
    animations_.erase(
        std::remove_if(animations_.begin(), animations_.end(),
                        [&](const std::shared_ptr<CSSAnimation>& a) {
                            if (a->effect() && a->effect()->target() == target) {
                                a->cancel();
                                return true;
                            }
                            return false;
                        }),
        animations_.end());
}

void CSSAnimationController::cancelAll() {
    for (auto& a : animations_) a->cancel();
    animations_.clear();
}

void CSSAnimationController::tick(double nowMs) {
    if (docTimeline_) docTimeline_->tick(nowMs);

    for (auto& anim : animations_) {
        if (anim->timeline()) {
            anim->tick(anim->timeline()->currentTime());
        }
    }

    removeFinished();
}

void CSSAnimationController::removeFinished() {
    animations_.erase(
        std::remove_if(animations_.begin(), animations_.end(),
                        [](const std::shared_ptr<CSSAnimation>& a) {
                            return a->playState() == AnimationPlayState::Finished &&
                                   !a->isRelevant();
                        }),
        animations_.end());
}

std::vector<CSSAnimation*> CSSAnimationController::animationsForTarget(void* target) const {
    std::vector<CSSAnimation*> result;
    for (const auto& a : animations_) {
        if (a->effect() && a->effect()->target() == target) {
            result.push_back(a.get());
        }
    }
    return result;
}

bool CSSAnimationController::hasActiveAnimations(void* target) const {
    for (const auto& a : animations_) {
        if (a->effect() && a->effect()->target() == target &&
            a->playState() == AnimationPlayState::Running) {
            return true;
        }
    }
    return false;
}

std::vector<KeyframeEffect::ComputedValue> CSSAnimationController::getAnimatedValues(
    void* target) const {
    std::vector<KeyframeEffect::ComputedValue> all;
    for (const auto& a : animations_) {
        if (a->effect() && a->effect()->target() == target && a->isRelevant()) {
            auto vals = a->computedValues();
            all.insert(all.end(), vals.begin(), vals.end());
        }
    }
    return all;
}

// ==================================================================
// KeyframesRegistry
// ==================================================================

void KeyframesRegistry::registerKeyframes(const std::string& name,
                                            const std::vector<CSSKeyframe>& keyframes) {
    registry_[name] = keyframes;
}

const std::vector<CSSKeyframe>* KeyframesRegistry::find(const std::string& name) const {
    auto it = registry_.find(name);
    return (it != registry_.end()) ? &it->second : nullptr;
}

void KeyframesRegistry::unregister(const std::string& name) {
    registry_.erase(name);
}

void KeyframesRegistry::clear() {
    registry_.clear();
}

} // namespace NXRender
