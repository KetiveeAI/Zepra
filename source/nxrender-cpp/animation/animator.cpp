// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file animator.cpp
 * @brief Animation system implementation
 */

#include "animation/animator.h"
#include "widgets/widget.h"
#include <algorithm>

namespace NXRender {

// ============================================================================
// Animation
// ============================================================================

void Animation::start() {
    if (getter_ && !hasFrom_) {
        fromValue_ = getter_();
    }
    currentValue_ = fromValue_;
    elapsedMs_ = 0;
    currentRepeat_ = 0;
    yoyoReverse_ = false;
    state_ = AnimationState::Running;
    
    if (onStart_) onStart_();
}

void Animation::stop() {
    state_ = AnimationState::Idle;
}

void Animation::pause() {
    if (state_ == AnimationState::Running) {
        state_ = AnimationState::Paused;
    }
}

void Animation::resume() {
    if (state_ == AnimationState::Paused) {
        state_ = AnimationState::Running;
    }
}

bool Animation::update(float deltaMs) {
    if (state_ != AnimationState::Running) return state_ == AnimationState::Paused;
    
    elapsedMs_ += deltaMs;
    
    // Handle delay
    if (elapsedMs_ < delayMs_) return true;
    
    float activeTime = elapsedMs_ - delayMs_;
    float t = durationMs_ > 0 ? std::min(activeTime / durationMs_, 1.0f) : 1.0f;
    
    // Apply yoyo
    if (yoyoReverse_) t = 1.0f - t;
    
    // Apply easing
    float easedT = easingFn_ ? easingFn_(t) : t;
    
    // Interpolate value
    currentValue_ = fromValue_ + (toValue_ - fromValue_) * easedT;
    
    // Apply to widget
    if (setter_) setter_(currentValue_);
    if (onUpdate_) onUpdate_(currentValue_);
    
    // Check completion
    if (activeTime >= durationMs_) {
        if (repeatCount_ == -1 || currentRepeat_ < repeatCount_) {
            // Repeat
            currentRepeat_++;
            elapsedMs_ = delayMs_;
            if (yoyo_) {
                yoyoReverse_ = !yoyoReverse_;
            }
            return true;
        }
        
        // Completed
        state_ = AnimationState::Completed;
        if (onComplete_) onComplete_();
        return false;
    }
    
    return true;
}

float Animation::progress() const {
    if (durationMs_ <= 0) return 1.0f;
    float activeTime = std::max(0.0f, elapsedMs_ - delayMs_);
    return std::min(activeTime / durationMs_, 1.0f);
}

// ============================================================================
// AnimationGroup
// ============================================================================

AnimationGroup& AnimationGroup::add(std::shared_ptr<Animation> anim) {
    animations_.push_back(anim);
    return *this;
}

void AnimationGroup::start() {
    elapsedMs_ = 0;
    currentIndex_ = 0;
    
    if (mode_ == Mode::Parallel) {
        for (auto& anim : animations_) {
            anim->start();
        }
    } else if (!animations_.empty()) {
        animations_[0]->start();
    }
}

void AnimationGroup::stop() {
    for (auto& anim : animations_) {
        anim->stop();
    }
}

bool AnimationGroup::update(float deltaMs) {
    elapsedMs_ += deltaMs;
    
    if (elapsedMs_ < delayMs_) return true;
    
    if (mode_ == Mode::Parallel) {
        bool anyRunning = false;
        for (auto& anim : animations_) {
            if (anim->update(deltaMs)) anyRunning = true;
        }
        if (!anyRunning && onComplete_) onComplete_();
        return anyRunning;
    } else {
        // Sequential
        if (currentIndex_ >= animations_.size()) {
            if (onComplete_) onComplete_();
            return false;
        }
        
        if (!animations_[currentIndex_]->update(deltaMs)) {
            currentIndex_++;
            if (currentIndex_ < animations_.size()) {
                animations_[currentIndex_]->start();
            }
        }
        return currentIndex_ < animations_.size();
    }
}

bool AnimationGroup::isComplete() const {
    if (mode_ == Mode::Parallel) {
        for (const auto& anim : animations_) {
            if (anim->state() != AnimationState::Completed) return false;
        }
        return true;
    }
    return currentIndex_ >= animations_.size();
}

// ============================================================================
// Animator (Singleton)
// ============================================================================

Animator& Animator::instance() {
    static Animator instance;
    return instance;
}

std::shared_ptr<Animation> Animator::animate(Widget* widget, AnimatableProperty prop,
                                              float to, int durationMs,
                                              EasingFunction easing) {
    auto anim = std::make_shared<Animation>();
    anim->to(to).duration(durationMs).easing(easing);
    
    // Set up getter/setter based on property
    switch (prop) {
        case AnimatableProperty::X:
            anim->getter([widget]() { return widget->bounds().x; });
            anim->setter([widget](float v) { 
                widget->setPosition(v, widget->bounds().y); 
            });
            break;
        case AnimatableProperty::Y:
            anim->getter([widget]() { return widget->bounds().y; });
            anim->setter([widget](float v) { 
                widget->setPosition(widget->bounds().x, v); 
            });
            break;
        case AnimatableProperty::Width:
            anim->getter([widget]() { return widget->bounds().width; });
            anim->setter([widget](float v) { 
                widget->setSize(v, widget->bounds().height); 
            });
            break;
        case AnimatableProperty::Height:
            anim->getter([widget]() { return widget->bounds().height; });
            anim->setter([widget](float v) { 
                widget->setSize(widget->bounds().width, v); 
            });
            break;
        // Opacity would need to be added to Widget if needed
        case AnimatableProperty::Opacity:
            // Placeholder - Widget would need opacity() method
            break;
        default:
            break;
    }
    
    // Stop any existing animation on same property
    stopProperty(widget, prop);
    
    // Add and start
    animations_.push_back({widget, prop, anim});
    anim->start();
    
    return anim;
}

std::shared_ptr<Animation> Animator::animate(Widget* widget, const std::string& propertyName,
                                              float to, int durationMs,
                                              EasingFunction easing) {
    // Map string to property enum
    static std::unordered_map<std::string, AnimatableProperty> propMap = {
        {"x", AnimatableProperty::X},
        {"y", AnimatableProperty::Y},
        {"width", AnimatableProperty::Width},
        {"height", AnimatableProperty::Height},
        {"opacity", AnimatableProperty::Opacity},
        {"scale", AnimatableProperty::Scale},
        {"rotation", AnimatableProperty::Rotation}
    };
    
    auto it = propMap.find(propertyName);
    if (it != propMap.end()) {
        return animate(widget, it->second, to, durationMs, easing);
    }
    
    return nullptr;
}

std::shared_ptr<Animation> Animator::spring(Widget* widget, AnimatableProperty prop,
                                             float to, SpringConfig config) {
    auto anim = std::make_shared<Animation>();
    
    // Use spring easing
    anim->to(to).duration(1000).easing([config](float t) {
        return springEasing(t, config.stiffness, config.damping);
    });
    
    // Set up getter/setter using bounds() API
    switch (prop) {
        case AnimatableProperty::X:
            anim->getter([widget]() { return widget->bounds().x; });
            anim->setter([widget](float v) { 
                widget->setPosition(v, widget->bounds().y); 
            });
            break;
        case AnimatableProperty::Y:
            anim->getter([widget]() { return widget->bounds().y; });
            anim->setter([widget](float v) { 
                widget->setPosition(widget->bounds().x, v); 
            });
            break;
        case AnimatableProperty::Width:
            anim->getter([widget]() { return widget->bounds().width; });
            anim->setter([widget](float v) { 
                widget->setSize(v, widget->bounds().height); 
            });
            break;
        case AnimatableProperty::Height:
            anim->getter([widget]() { return widget->bounds().height; });
            anim->setter([widget](float v) { 
                widget->setSize(widget->bounds().width, v); 
            });
            break;
        default:
            break;
    }
    
    stopProperty(widget, prop);
    animations_.push_back({widget, prop, anim});
    anim->start();
    
    return anim;
}

void Animator::stopAll(Widget* widget) {
    animations_.erase(
        std::remove_if(animations_.begin(), animations_.end(),
            [widget](const AnimationEntry& e) { return e.widget == widget; }),
        animations_.end()
    );
}

void Animator::stopProperty(Widget* widget, AnimatableProperty prop) {
    animations_.erase(
        std::remove_if(animations_.begin(), animations_.end(),
            [widget, prop](const AnimationEntry& e) { 
                return e.widget == widget && e.property == prop; 
            }),
        animations_.end()
    );
}

void Animator::update(float deltaMs) {
    animations_.erase(
        std::remove_if(animations_.begin(), animations_.end(),
            [deltaMs](AnimationEntry& e) { return !e.animation->update(deltaMs); }),
        animations_.end()
    );
}

void Animator::update() {
    auto now = std::chrono::steady_clock::now();
    
    if (firstUpdate_) {
        lastUpdate_ = now;
        firstUpdate_ = false;
        return;
    }
    
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_);
    lastUpdate_ = now;
    
    update(static_cast<float>(delta.count()));
}

} // namespace NXRender
