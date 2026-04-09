// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "animation/scroll_physics.h"
#include <cmath>
#include <algorithm>

namespace NXRender {

// ==================================================================
// ScrollPhysics
// ==================================================================

ScrollPhysics::ScrollPhysics() {}

void ScrollPhysics::setContentSize(float width, float height) {
    contentWidth_ = width;
    contentHeight_ = height;
}

void ScrollPhysics::setViewportSize(float width, float height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
}

void ScrollPhysics::setScrollPosition(float x, float y) {
    scrollX_ = clampX(x);
    scrollY_ = clampY(y);
    velocityX_ = 0;
    velocityY_ = 0;
    state_ = ScrollState::Idle;
}

float ScrollPhysics::clampX(float x) const {
    float maxX = std::max(0.0f, contentWidth_ - viewportWidth_);
    if (bounceEnabled_) {
        // Allow overscroll up to bounceLimit_
        return std::clamp(x, -bounceLimit_, maxX + bounceLimit_);
    }
    return std::clamp(x, 0.0f, maxX);
}

float ScrollPhysics::clampY(float y) const {
    float maxY = std::max(0.0f, contentHeight_ - viewportHeight_);
    if (bounceEnabled_) {
        return std::clamp(y, -bounceLimit_, maxY + bounceLimit_);
    }
    return std::clamp(y, 0.0f, maxY);
}

bool ScrollPhysics::isOverscrolledX() const {
    float maxX = std::max(0.0f, contentWidth_ - viewportWidth_);
    return scrollX_ < 0 || scrollX_ > maxX;
}

bool ScrollPhysics::isOverscrolledY() const {
    float maxY = std::max(0.0f, contentHeight_ - viewportHeight_);
    return scrollY_ < 0 || scrollY_ > maxY;
}

void ScrollPhysics::beginDrag(float x, float y) {
    dragStartX_ = x;
    dragStartY_ = y;
    dragScrollStartX_ = scrollX_;
    dragScrollStartY_ = scrollY_;
    velocityX_ = 0;
    velocityY_ = 0;
    state_ = ScrollState::Dragging;

    // Reset velocity tracker
    velocitySampleCount_ = 0;
}

void ScrollPhysics::updateDrag(float x, float y, float timestampMs) {
    if (state_ != ScrollState::Dragging) return;

    float dx = dragStartX_ - x;
    float dy = dragStartY_ - y;

    // Apply rubber-band effect when overscrolled
    float newX = dragScrollStartX_ + dx;
    float newY = dragScrollStartY_ + dy;

    if (bounceEnabled_) {
        float maxX = std::max(0.0f, contentWidth_ - viewportWidth_);
        float maxY = std::max(0.0f, contentHeight_ - viewportHeight_);

        if (newX < 0) {
            newX = -rubberBand(-newX);
        } else if (newX > maxX) {
            newX = maxX + rubberBand(newX - maxX);
        }

        if (newY < 0) {
            newY = -rubberBand(-newY);
        } else if (newY > maxY) {
            newY = maxY + rubberBand(newY - maxY);
        }
    }

    scrollX_ = clampX(newX);
    scrollY_ = clampY(newY);

    // Track velocity
    if (velocitySampleCount_ < kMaxVelocitySamples) {
        velocitySamples_[velocitySampleCount_] = {x, y, timestampMs};
        velocitySampleCount_++;
    } else {
        // Shift samples
        for (int i = 0; i < kMaxVelocitySamples - 1; i++) {
            velocitySamples_[i] = velocitySamples_[i + 1];
        }
        velocitySamples_[kMaxVelocitySamples - 1] = {x, y, timestampMs};
    }
}

void ScrollPhysics::endDrag(float x, float y, float timestampMs) {
    if (state_ != ScrollState::Dragging) return;

    // Compute fling velocity from recent samples
    updateDrag(x, y, timestampMs);

    if (velocitySampleCount_ >= 2) {
        auto& newest = velocitySamples_[velocitySampleCount_ - 1];
        auto& oldest = velocitySamples_[0];
        float dt = newest.timestamp - oldest.timestamp;

        if (dt > 0) {
            velocityX_ = -(newest.x - oldest.x) / dt * 1000.0f; // px/sec
            velocityY_ = -(newest.y - oldest.y) / dt * 1000.0f;

            // Clamp velocity
            float maxVel = 8000.0f;
            velocityX_ = std::clamp(velocityX_, -maxVel, maxVel);
            velocityY_ = std::clamp(velocityY_, -maxVel, maxVel);
        }
    }

    float speed = std::sqrt(velocityX_ * velocityX_ + velocityY_ * velocityY_);

    if (isOverscrolledX() || isOverscrolledY()) {
        state_ = ScrollState::Bouncing;
    } else if (speed > minFlingVelocity_) {
        state_ = ScrollState::Flinging;
    } else {
        state_ = ScrollState::Idle;
    }
}

float ScrollPhysics::rubberBand(float overscroll) const {
    // Rubber-band damping: diminishing returns as overscroll increases
    float dimension = std::max(viewportHeight_, 1.0f);
    float c = 0.55f;
    return dimension * (1.0f - std::exp(-overscroll * c / dimension));
}

bool ScrollPhysics::update(float deltaMs) {
    float dt = deltaMs / 1000.0f; // Convert to seconds

    switch (state_) {
        case ScrollState::Idle:
        case ScrollState::Dragging:
            return false;

        case ScrollState::Flinging: {
            // Apply deceleration
            float decay = std::pow(friction_, dt * 60.0f);
            velocityX_ *= decay;
            velocityY_ *= decay;

            scrollX_ += velocityX_ * dt;
            scrollY_ += velocityY_ * dt;

            // Clamp or start bounce
            float maxX = std::max(0.0f, contentWidth_ - viewportWidth_);
            float maxY = std::max(0.0f, contentHeight_ - viewportHeight_);

            if (bounceEnabled_) {
                if (scrollX_ < 0 || scrollX_ > maxX || scrollY_ < 0 || scrollY_ > maxY) {
                    state_ = ScrollState::Bouncing;
                    return true;
                }
            } else {
                scrollX_ = std::clamp(scrollX_, 0.0f, maxX);
                scrollY_ = std::clamp(scrollY_, 0.0f, maxY);
            }

            float speed = std::sqrt(velocityX_ * velocityX_ + velocityY_ * velocityY_);
            if (speed < 1.0f) {
                velocityX_ = 0;
                velocityY_ = 0;
                state_ = ScrollState::Idle;
                return false;
            }

            return true;
        }

        case ScrollState::Bouncing: {
            float maxX = std::max(0.0f, contentWidth_ - viewportWidth_);
            float maxY = std::max(0.0f, contentHeight_ - viewportHeight_);

            // Spring-back to valid range
            float targetX = std::clamp(scrollX_, 0.0f, maxX);
            float targetY = std::clamp(scrollY_, 0.0f, maxY);

            float springK = 200.0f; // Spring constant
            float damping = 20.0f;

            float forceX = -springK * (scrollX_ - targetX) - damping * velocityX_;
            float forceY = -springK * (scrollY_ - targetY) - damping * velocityY_;

            velocityX_ += forceX * dt;
            velocityY_ += forceY * dt;
            scrollX_ += velocityX_ * dt;
            scrollY_ += velocityY_ * dt;

            float distX = std::abs(scrollX_ - targetX);
            float distY = std::abs(scrollY_ - targetY);
            float speed = std::sqrt(velocityX_ * velocityX_ + velocityY_ * velocityY_);

            if (distX < 0.5f && distY < 0.5f && speed < 1.0f) {
                scrollX_ = targetX;
                scrollY_ = targetY;
                velocityX_ = 0;
                velocityY_ = 0;
                state_ = ScrollState::Idle;
                return false;
            }

            return true;
        }

        case ScrollState::Animating: {
            // Smooth scroll to target
            float dx = targetScrollX_ - scrollX_;
            float dy = targetScrollY_ - scrollY_;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 0.5f) {
                scrollX_ = targetScrollX_;
                scrollY_ = targetScrollY_;
                state_ = ScrollState::Idle;
                return false;
            }

            float speed = dist * 8.0f; // Proportional approach
            speed = std::min(speed, 4000.0f);
            float t = std::min(1.0f, speed * dt / dist);

            scrollX_ += dx * t;
            scrollY_ += dy * t;

            return true;
        }
    }

    return false;
}

void ScrollPhysics::scrollTo(float x, float y, bool animated) {
    float maxX = std::max(0.0f, contentWidth_ - viewportWidth_);
    float maxY = std::max(0.0f, contentHeight_ - viewportHeight_);

    x = std::clamp(x, 0.0f, maxX);
    y = std::clamp(y, 0.0f, maxY);

    if (animated) {
        targetScrollX_ = x;
        targetScrollY_ = y;
        state_ = ScrollState::Animating;
    } else {
        scrollX_ = x;
        scrollY_ = y;
        velocityX_ = 0;
        velocityY_ = 0;
        state_ = ScrollState::Idle;
    }
}

void ScrollPhysics::scrollBy(float dx, float dy, bool animated) {
    scrollTo(scrollX_ + dx, scrollY_ + dy, animated);
}

void ScrollPhysics::fling(float velocityX, float velocityY) {
    velocityX_ = velocityX;
    velocityY_ = velocityY;
    state_ = ScrollState::Flinging;
}

void ScrollPhysics::stop() {
    velocityX_ = 0;
    velocityY_ = 0;
    state_ = ScrollState::Idle;

    // Snap to valid range if overscrolled
    if (!bounceEnabled_) {
        float maxX = std::max(0.0f, contentWidth_ - viewportWidth_);
        float maxY = std::max(0.0f, contentHeight_ - viewportHeight_);
        scrollX_ = std::clamp(scrollX_, 0.0f, maxX);
        scrollY_ = std::clamp(scrollY_, 0.0f, maxY);
    }
}

float ScrollPhysics::maxScrollX() const {
    return std::max(0.0f, contentWidth_ - viewportWidth_);
}

float ScrollPhysics::maxScrollY() const {
    return std::max(0.0f, contentHeight_ - viewportHeight_);
}

float ScrollPhysics::scrollFractionX() const {
    float maxX = maxScrollX();
    if (maxX <= 0) return 0;
    return std::clamp(scrollX_ / maxX, 0.0f, 1.0f);
}

float ScrollPhysics::scrollFractionY() const {
    float maxY = maxScrollY();
    if (maxY <= 0) return 0;
    return std::clamp(scrollY_ / maxY, 0.0f, 1.0f);
}

} // namespace NXRender
