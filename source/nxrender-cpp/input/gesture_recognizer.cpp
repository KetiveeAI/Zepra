// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "input/gesture_recognizer.h"
#include <algorithm>

namespace NXRender {
namespace Input {

// ==================================================================
// GestureRecognizer base
// ==================================================================

void GestureRecognizer::reset() {
    state_ = GestureState::Possible;
    lastX_ = 0;
    lastY_ = 0;
}

// ==================================================================
// TapRecognizer
// ==================================================================

void TapRecognizer::feed(const PointerEvent& event) {
    if (!enabled_) return;

    if (event.down && !tracking_) {
        startX_ = event.x;
        startY_ = event.y;
        startTime_ = event.timestampMs;
        tracking_ = true;
        return;
    }

    if (tracking_ && event.moved) {
        float dx = event.x - startX_;
        float dy = event.y - startY_;
        if (std::sqrt(dx * dx + dy * dy) > maxDistance_) {
            tracking_ = false;
            setState(GestureState::Failed);
        }
        return;
    }

    if (tracking_ && !event.down) {
        tracking_ = false;
        uint64_t duration = event.timestampMs - startTime_;

        if (duration <= static_cast<uint64_t>(maxDurationMs_)) {
            float dx = event.x - startX_;
            float dy = event.y - startY_;
            if (std::sqrt(dx * dx + dy * dy) <= maxDistance_) {
                lastX_ = event.x;
                lastY_ = event.y;
                setState(GestureState::Ended);
                return;
            }
        }

        setState(GestureState::Failed);
    }
}

void TapRecognizer::reset() {
    GestureRecognizer::reset();
    tracking_ = false;
}

// ==================================================================
// DoubleTapRecognizer
// ==================================================================

void DoubleTapRecognizer::feed(const PointerEvent& event) {
    if (!enabled_) return;

    firstTap_.feed(event);

    if (firstTap_.state() == GestureState::Ended) {
        if (tapCount_ == 0) {
            tapCount_ = 1;
            firstTapTime_ = event.timestampMs;
            firstTapX_ = event.x;
            firstTapY_ = event.y;
            firstTap_.reset();
        } else if (tapCount_ == 1) {
            uint64_t interval = event.timestampMs - firstTapTime_;
            float dx = event.x - firstTapX_;
            float dy = event.y - firstTapY_;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (interval <= static_cast<uint64_t>(maxIntervalMs_) && dist <= 20.0f) {
                lastX_ = event.x;
                lastY_ = event.y;
                setState(GestureState::Ended);
            } else {
                setState(GestureState::Failed);
            }
            tapCount_ = 0;
            firstTap_.reset();
        }
    } else if (firstTap_.state() == GestureState::Failed) {
        tapCount_ = 0;
        firstTap_.reset();
    }

    // Timeout the first tap
    if (tapCount_ == 1 && event.timestampMs - firstTapTime_ > static_cast<uint64_t>(maxIntervalMs_)) {
        tapCount_ = 0;
    }
}

void DoubleTapRecognizer::reset() {
    GestureRecognizer::reset();
    firstTap_.reset();
    tapCount_ = 0;
}

// ==================================================================
// LongPressRecognizer
// ==================================================================

void LongPressRecognizer::feed(const PointerEvent& event) {
    if (!enabled_) return;

    if (event.down && !tracking_) {
        startX_ = event.x;
        startY_ = event.y;
        startTime_ = event.timestampMs;
        tracking_ = true;
        triggered_ = false;
        return;
    }

    if (tracking_ && event.moved) {
        float dx = event.x - startX_;
        float dy = event.y - startY_;
        if (std::sqrt(dx * dx + dy * dy) > maxDistance_) {
            tracking_ = false;
            if (!triggered_) setState(GestureState::Failed);
            else setState(GestureState::Cancelled);
        }
        return;
    }

    if (tracking_ && !event.down) {
        tracking_ = false;
        if (triggered_) {
            setState(GestureState::Ended);
        } else {
            setState(GestureState::Failed);
        }
    }
}

void LongPressRecognizer::checkTimeout(uint64_t currentTimeMs) {
    if (!tracking_ || triggered_) return;

    if (currentTimeMs - startTime_ >= static_cast<uint64_t>(minDurationMs_)) {
        triggered_ = true;
        lastX_ = startX_;
        lastY_ = startY_;
        setState(GestureState::Began);
    }
}

void LongPressRecognizer::reset() {
    GestureRecognizer::reset();
    tracking_ = false;
    triggered_ = false;
}

// ==================================================================
// PanRecognizer
// ==================================================================

void PanRecognizer::feed(const PointerEvent& event) {
    if (!enabled_) return;

    if (event.down && !tracking_) {
        startX_ = event.x;
        startY_ = event.y;
        currentX_ = event.x;
        currentY_ = event.y;
        prevX_ = event.x;
        prevY_ = event.y;
        prevTime_ = event.timestampMs;
        velocityX_ = 0;
        velocityY_ = 0;
        tracking_ = true;
        recognized_ = false;
        velocityIndex_ = 0;
        return;
    }

    if (tracking_ && event.moved) {
        currentX_ = event.x;
        currentY_ = event.y;
        updateVelocity(event.x, event.y, event.timestampMs);

        if (!recognized_) {
            float dx = event.x - startX_;
            float dy = event.y - startY_;
            if (std::sqrt(dx * dx + dy * dy) >= minDistance_) {
                recognized_ = true;
                lastX_ = event.x;
                lastY_ = event.y;
                setState(GestureState::Began);
                if (panCallback_) panCallback_(GestureState::Began, deltaX(), deltaY(), velocityX_, velocityY_);
            }
        } else {
            lastX_ = event.x;
            lastY_ = event.y;
            setState(GestureState::Changed);
            if (panCallback_) panCallback_(GestureState::Changed, deltaX(), deltaY(), velocityX_, velocityY_);
        }
        return;
    }

    if (tracking_ && !event.down) {
        tracking_ = false;
        if (recognized_) {
            lastX_ = event.x;
            lastY_ = event.y;
            setState(GestureState::Ended);
            if (panCallback_) panCallback_(GestureState::Ended, deltaX(), deltaY(), velocityX_, velocityY_);
        } else {
            setState(GestureState::Failed);
        }
    }
}

void PanRecognizer::updateVelocity(float x, float y, uint64_t time) {
    float dt = static_cast<float>(time - prevTime_);
    if (dt > 0) {
        float vx = (x - prevX_) / dt;
        float vy = (y - prevY_) / dt;

        velocityHistory_[velocityIndex_ % kVelocityHistorySize] = {vx, vy, time};
        velocityIndex_++;

        // Average recent velocity samples
        float sumVx = 0, sumVy = 0;
        int count = std::min(velocityIndex_, kVelocityHistorySize);
        for (int i = 0; i < count; i++) {
            sumVx += velocityHistory_[i].vx;
            sumVy += velocityHistory_[i].vy;
        }
        velocityX_ = sumVx / static_cast<float>(count);
        velocityY_ = sumVy / static_cast<float>(count);
    }

    prevX_ = x;
    prevY_ = y;
    prevTime_ = time;
}

void PanRecognizer::reset() {
    GestureRecognizer::reset();
    tracking_ = false;
    recognized_ = false;
    velocityX_ = 0;
    velocityY_ = 0;
}

// ==================================================================
// PinchRecognizer
// ==================================================================

void PinchRecognizer::feed(const PointerEvent& event) {
    if (!enabled_) return;

    // Track pointers
    if (event.down) {
        if (pointerCount_ < 2) {
            pointers_[pointerCount_] = {event.pointerId, event.x, event.y, true};
            pointerCount_++;
        }

        if (pointerCount_ == 2) {
            float dx = pointers_[1].x - pointers_[0].x;
            float dy = pointers_[1].y - pointers_[0].y;
            initialDistance_ = std::sqrt(dx * dx + dy * dy);
            scale_ = 1.0f;
            if (initialDistance_ > 1.0f) {
                recognized_ = true;
                float cx = (pointers_[0].x + pointers_[1].x) * 0.5f;
                float cy = (pointers_[0].y + pointers_[1].y) * 0.5f;
                lastX_ = cx;
                lastY_ = cy;
                setState(GestureState::Began);
                if (pinchCallback_) pinchCallback_(GestureState::Began, 1.0f, cx, cy);
            }
        }
        return;
    }

    if (event.moved && recognized_) {
        // Update pointer position
        for (int i = 0; i < 2; i++) {
            if (pointers_[i].id == event.pointerId) {
                pointers_[i].x = event.x;
                pointers_[i].y = event.y;
                break;
            }
        }

        float dx = pointers_[1].x - pointers_[0].x;
        float dy = pointers_[1].y - pointers_[0].y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (initialDistance_ > 1.0f) {
            scale_ = distance / initialDistance_;
            float cx = (pointers_[0].x + pointers_[1].x) * 0.5f;
            float cy = (pointers_[0].y + pointers_[1].y) * 0.5f;
            lastX_ = cx;
            lastY_ = cy;
            setState(GestureState::Changed);
            if (pinchCallback_) pinchCallback_(GestureState::Changed, scale_, cx, cy);
        }
        return;
    }

    if (!event.down) {
        for (int i = 0; i < 2; i++) {
            if (pointers_[i].id == event.pointerId) {
                pointers_[i].active = false;
                pointerCount_--;
                break;
            }
        }

        if (recognized_ && pointerCount_ < 2) {
            float cx = (pointers_[0].x + pointers_[1].x) * 0.5f;
            float cy = (pointers_[0].y + pointers_[1].y) * 0.5f;
            setState(GestureState::Ended);
            if (pinchCallback_) pinchCallback_(GestureState::Ended, scale_, cx, cy);
            recognized_ = false;
        }
    }
}

void PinchRecognizer::reset() {
    GestureRecognizer::reset();
    pointerCount_ = 0;
    scale_ = 1.0f;
    recognized_ = false;
}

// ==================================================================
// SwipeRecognizer
// ==================================================================

void SwipeRecognizer::feed(const PointerEvent& event) {
    if (!enabled_) return;

    if (event.down && !tracking_) {
        startX_ = event.x;
        startY_ = event.y;
        startTime_ = event.timestampMs;
        tracking_ = true;
        return;
    }

    if (tracking_ && !event.down) {
        tracking_ = false;

        float dx = event.x - startX_;
        float dy = event.y - startY_;
        float duration = static_cast<float>(event.timestampMs - startTime_);

        if (duration > static_cast<float>(maxDurationMs_) || duration <= 0) {
            setState(GestureState::Failed);
            return;
        }

        float absDx = std::abs(dx);
        float absDy = std::abs(dy);
        float vx = absDx / duration;
        float vy = absDy / duration;

        // Determine dominant direction
        if (absDx > absDy && vx >= minVelocity_) {
            Direction dir = (dx > 0) ? Direction::Right : Direction::Left;
            lastX_ = event.x;
            lastY_ = event.y;
            setState(GestureState::Ended);
            if (swipeCallback_) swipeCallback_(dir);
        } else if (absDy > absDx && vy >= minVelocity_) {
            Direction dir = (dy > 0) ? Direction::Down : Direction::Up;
            lastX_ = event.x;
            lastY_ = event.y;
            setState(GestureState::Ended);
            if (swipeCallback_) swipeCallback_(dir);
        } else {
            setState(GestureState::Failed);
        }
    }
}

void SwipeRecognizer::reset() {
    GestureRecognizer::reset();
    tracking_ = false;
}

} // namespace Input
} // namespace NXRender
