// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "scroll_snap.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace NXRender {

// ==================================================================
// ScrollSnapEngine
// ==================================================================

ScrollSnapEngine::ScrollSnapEngine() {}
ScrollSnapEngine::~ScrollSnapEngine() {}

void ScrollSnapEngine::clear() {
    snapPointsX_.clear();
    snapPointsY_.clear();
}

void ScrollSnapEngine::addSnapPoint(const ScrollSnapPoint& point) {
    snapPointsX_.push_back(point);
    snapPointsY_.push_back(point);
}

void ScrollSnapEngine::addSnapPointX(float x, ScrollSnapAlign align, ScrollSnapStop stop) {
    ScrollSnapPoint p;
    p.position = x;
    p.align = align;
    p.stop = stop;
    snapPointsX_.push_back(p);
}

void ScrollSnapEngine::addSnapPointY(float y, ScrollSnapAlign align, ScrollSnapStop stop) {
    ScrollSnapPoint p;
    p.position = y;
    p.align = align;
    p.stop = stop;
    snapPointsY_.push_back(p);
}

float ScrollSnapEngine::snapPosition(float scroll, float viewport,
                                       const ScrollSnapPoint& point,
                                       ScrollSnapAlign align) const {
    ScrollSnapAlign effectiveAlign = (point.align != ScrollSnapAlign::None) ?
        point.align : align;

    float pos = point.position;
    switch (effectiveAlign) {
        case ScrollSnapAlign::Start:
            return pos - config_.snapPadding[3]; // left padding
        case ScrollSnapAlign::Center:
            return pos - viewport / 2;
        case ScrollSnapAlign::End:
            return pos - viewport + config_.snapPadding[1]; // right padding
        case ScrollSnapAlign::None:
            return scroll; // No snap
    }
    return scroll;
}

bool ScrollSnapEngine::shouldSnap(float distance, float viewport) const {
    if (config_.type == ScrollSnapType::Mandatory) return true;
    if (config_.type == ScrollSnapType::Proximity) {
        return std::abs(distance) < viewport * config_.proximityThreshold;
    }
    return false;
}

ScrollSnapEngine::SnapResult ScrollSnapEngine::findSnapTarget(
    float scrollX, float scrollY,
    float velocityX, float velocityY,
    float viewportWidth, float viewportHeight) const {

    SnapResult result;
    result.targetX = scrollX;
    result.targetY = scrollY;

    if (config_.type == ScrollSnapType::None) return result;

    // Predicted end position from velocity (deceleration = 0.998)
    float predictedX = scrollX + velocityX * 0.3f;
    float predictedY = scrollY + velocityY * 0.3f;

    // Find best X snap
    if (config_.axis == ScrollSnapAxis::Both || config_.axis == ScrollSnapAxis::X ||
        config_.axis == ScrollSnapAxis::Inline) {
        float bestDist = std::numeric_limits<float>::max();
        int bestIdx = -1;

        for (size_t i = 0; i < snapPointsX_.size(); i++) {
            float snapPos = snapPosition(predictedX, viewportWidth,
                                           snapPointsX_[i], ScrollSnapAlign::Start);
            float dist = std::abs(snapPos - predictedX);

            // For mandatory snap-stop: always, must land on this point if passing through
            if (snapPointsX_[i].stop == ScrollSnapStop::Always) {
                if ((velocityX > 0 && snapPos > scrollX && snapPos <= predictedX) ||
                    (velocityX < 0 && snapPos < scrollX && snapPos >= predictedX)) {
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestIdx = static_cast<int>(i);
                        result.targetX = snapPos;
                    }
                    continue;
                }
            }

            if (dist < bestDist && shouldSnap(dist, viewportWidth)) {
                bestDist = dist;
                bestIdx = static_cast<int>(i);
                result.targetX = snapPos;
            }
        }
        result.snapIndexX = bestIdx;
    }

    // Find best Y snap
    if (config_.axis == ScrollSnapAxis::Both || config_.axis == ScrollSnapAxis::Y ||
        config_.axis == ScrollSnapAxis::Block) {
        float bestDist = std::numeric_limits<float>::max();
        int bestIdx = -1;

        for (size_t i = 0; i < snapPointsY_.size(); i++) {
            float snapPos = snapPosition(predictedY, viewportHeight,
                                           snapPointsY_[i], ScrollSnapAlign::Start);
            float dist = std::abs(snapPos - predictedY);

            if (snapPointsY_[i].stop == ScrollSnapStop::Always) {
                if ((velocityY > 0 && snapPos > scrollY && snapPos <= predictedY) ||
                    (velocityY < 0 && snapPos < scrollY && snapPos >= predictedY)) {
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestIdx = static_cast<int>(i);
                        result.targetY = snapPos;
                    }
                    continue;
                }
            }

            if (dist < bestDist && shouldSnap(dist, viewportHeight)) {
                bestDist = dist;
                bestIdx = static_cast<int>(i);
                result.targetY = snapPos;
            }
        }
        result.snapIndexY = bestIdx;
    }

    result.snapped = (result.snapIndexX >= 0 || result.snapIndexY >= 0);
    return result;
}

float ScrollSnapEngine::nearestSnapX(float scrollX, float viewportWidth) const {
    float best = scrollX;
    float bestDist = std::numeric_limits<float>::max();
    for (const auto& p : snapPointsX_) {
        float pos = snapPosition(scrollX, viewportWidth, p, ScrollSnapAlign::Start);
        float dist = std::abs(pos - scrollX);
        if (dist < bestDist) { bestDist = dist; best = pos; }
    }
    return best;
}

float ScrollSnapEngine::nearestSnapY(float scrollY, float viewportHeight) const {
    float best = scrollY;
    float bestDist = std::numeric_limits<float>::max();
    for (const auto& p : snapPointsY_) {
        float pos = snapPosition(scrollY, viewportHeight, p, ScrollSnapAlign::Start);
        float dist = std::abs(pos - scrollY);
        if (dist < bestDist) { bestDist = dist; best = pos; }
    }
    return best;
}

bool ScrollSnapEngine::isSnapped(float scrollX, float scrollY, float tolerance) const {
    bool xSnapped = snapPointsX_.empty();
    bool ySnapped = snapPointsY_.empty();

    for (const auto& p : snapPointsX_) {
        if (std::abs(p.position - scrollX) <= tolerance) { xSnapped = true; break; }
    }
    for (const auto& p : snapPointsY_) {
        if (std::abs(p.position - scrollY) <= tolerance) { ySnapped = true; break; }
    }
    return xSnapped && ySnapped;
}

ScrollSnapEngine::SnapResult ScrollSnapEngine::resnap(
    float scrollX, float scrollY,
    float viewportWidth, float viewportHeight) const {
    return findSnapTarget(scrollX, scrollY, 0, 0, viewportWidth, viewportHeight);
}

// ==================================================================
// SmoothScroller
// ==================================================================

SmoothScroller::SmoothScroller() {}

void SmoothScroller::scrollTo(float targetX, float targetY) {
    targetX_ = std::clamp(targetX, minX_, maxX_);
    targetY_ = std::clamp(targetY, minY_, maxY_);
    if (!smooth_) {
        currentX_ = targetX_;
        currentY_ = targetY_;
        if (onScroll_) onScroll_(currentX_, currentY_);
    } else {
        animating_ = true;
    }
}

void SmoothScroller::scrollBy(float deltaX, float deltaY) {
    scrollTo(targetX_ + deltaX, targetY_ + deltaY);
}

void SmoothScroller::snapTo(float targetX, float targetY) {
    targetX_ = std::clamp(targetX, minX_, maxX_);
    targetY_ = std::clamp(targetY, minY_, maxY_);
    animating_ = true;
}

void SmoothScroller::stop() {
    targetX_ = currentX_;
    targetY_ = currentY_;
    velocityX_ = 0;
    velocityY_ = 0;
    animating_ = false;
}

void SmoothScroller::tick(float deltaMs) {
    if (!animating_) return;

    float dt = deltaMs / 1000.0f;
    if (dt <= 0 || dt > 0.1f) dt = 0.016f;

    // Spring physics
    float dx = targetX_ - currentX_;
    float dy = targetY_ - currentY_;

    float forceX = SPRING_STIFFNESS * dx - SPRING_DAMPING * velocityX_;
    float forceY = SPRING_STIFFNESS * dy - SPRING_DAMPING * velocityY_;

    velocityX_ += forceX * dt;
    velocityY_ += forceY * dt;

    currentX_ += velocityX_ * dt;
    currentY_ += velocityY_ * dt;

    // Overscroll bounce
    if (currentX_ < minX_) {
        if (overscrollX_ == OverscrollBehavior::None) {
            currentX_ = minX_;
            velocityX_ = 0;
        } else {
            overscrollAmountX_ = currentX_ - minX_;
        }
    } else if (currentX_ > maxX_) {
        if (overscrollX_ == OverscrollBehavior::None) {
            currentX_ = maxX_;
            velocityX_ = 0;
        } else {
            overscrollAmountX_ = currentX_ - maxX_;
        }
    } else {
        overscrollAmountX_ = 0;
    }

    if (currentY_ < minY_) {
        if (overscrollY_ == OverscrollBehavior::None) {
            currentY_ = minY_;
            velocityY_ = 0;
        } else {
            overscrollAmountY_ = currentY_ - minY_;
        }
    } else if (currentY_ > maxY_) {
        if (overscrollY_ == OverscrollBehavior::None) {
            currentY_ = maxY_;
            velocityY_ = 0;
        } else {
            overscrollAmountY_ = currentY_ - maxY_;
        }
    } else {
        overscrollAmountY_ = 0;
    }

    // Check convergence
    if (std::abs(dx) < 0.5f && std::abs(dy) < 0.5f &&
        std::abs(velocityX_) < VELOCITY_THRESHOLD &&
        std::abs(velocityY_) < VELOCITY_THRESHOLD) {
        currentX_ = targetX_;
        currentY_ = targetY_;
        velocityX_ = 0;
        velocityY_ = 0;
        animating_ = false;

        // Snap integration
        if (snapEngine_) {
            auto snap = snapEngine_->findSnapTarget(currentX_, currentY_, 0, 0,
                                                      viewportWidth_, viewportHeight_);
            if (snap.snapped) {
                targetX_ = snap.targetX;
                targetY_ = snap.targetY;
                if (std::abs(targetX_ - currentX_) > 0.5f ||
                    std::abs(targetY_ - currentY_) > 0.5f) {
                    animating_ = true;
                }
            }
        }
    }

    if (onScroll_) onScroll_(currentX_, currentY_);
}

void SmoothScroller::setScrollBounds(float minX, float minY, float maxX, float maxY) {
    minX_ = minX; minY_ = minY;
    maxX_ = maxX; maxY_ = maxY;
}

void SmoothScroller::setBehavior(const std::string& behavior) {
    smooth_ = (behavior == "smooth");
}

} // namespace NXRender
