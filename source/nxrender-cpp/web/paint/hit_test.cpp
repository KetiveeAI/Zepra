// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "hit_test.h"
#include <algorithm>
#include <cmath>

namespace NXRender {
namespace Web {

// ==================================================================
// HitTestEngine
// ==================================================================

std::vector<HitTestEntry> HitTestEngine::hitTest(const BoxNode* root, const HitTestRequest& request) {
    std::vector<HitTestEntry> results;
    if (!root) return results;
    traverse(root, request.x, request.y, 0, request, results);
    // Sort by depth descending (front-most first)
    std::sort(results.begin(), results.end(), [](const HitTestEntry& a, const HitTestEntry& b) {
        if (a.zIndex != b.zIndex) return a.zIndex > b.zIndex;
        return a.depth > b.depth;
    });
    return results;
}

HitTestEntry HitTestEngine::hitTestFirst(const BoxNode* root, float x, float y) {
    HitTestRequest request;
    request.x = x;
    request.y = y;
    request.stopAtFirstHit = true;
    auto results = hitTest(root, request);
    if (!results.empty()) return results[0];
    return HitTestEntry();
}

void HitTestEngine::traverse(const BoxNode* node, float x, float y, int depth,
                               const HitTestRequest& request,
                               std::vector<HitTestEntry>& results) {
    if (shouldIgnoreNode(node, request)) return;

    // Transform point to local coordinates
    float localX = x, localY = y;
    inverseTransformPoint(node, localX, localY);

    // Test overflow clip
    bool clipped = false;
    if (node->hasOverflowClip()) {
        Rect clipRect = node->contentRect();
        if (localX < clipRect.x || localX > clipRect.x + clipRect.width ||
            localY < clipRect.y || localY > clipRect.y + clipRect.height) {
            clipped = true;
        }
    }

    // Children first (front to back)
    if (!clipped) {
        for (int i = static_cast<int>(node->childCount()) - 1; i >= 0; i--) {
            if (const BoxNode* child = node->childAt(i)) {
                traverse(child, localX, localY, depth + 1, request, results);
                if (request.stopAtFirstHit && !results.empty()) return;
            }
        }
    }

    // Test this node
    float hitX, hitY;
    if (isPointInNode(node, localX, localY, hitX, hitY)) {
        HitTestEntry entry;
        entry.node = node;
        entry.localX = hitX;
        entry.localY = hitY;
        entry.depth = depth;
        entry.zIndex = node->zIndex();
        entry.isOverflowClipped = clipped;
        results.push_back(entry);
    }
}

bool HitTestEngine::isPointInNode(const BoxNode* node, float x, float y,
                                    float& localX, float& localY) {
    Rect b = node->borderRect();
    if (x < b.x || x > b.x + b.width || y < b.y || y > b.y + b.height) return false;

    // Check border-radius
    if (node->hasBorderRadius()) {
        auto& radii = node->borderRadii();
        if (!isPointInRoundedRect(x, y, b.x, b.y, b.width, b.height,
                                   radii.topLeft, radii.topRight,
                                   radii.bottomRight, radii.bottomLeft)) {
            return false;
        }
    }

    localX = x - b.x;
    localY = y - b.y;
    return true;
}

bool HitTestEngine::isPointInRoundedRect(float px, float py,
                                           float rx, float ry, float rw, float rh,
                                           float tl, float tr, float br, float bl) {
    // Check each corner
    float cx, cy, r;

    // Top-left
    if (px < rx + tl && py < ry + tl) {
        cx = rx + tl; cy = ry + tl; r = tl;
        float dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy > r * r) return false;
    }
    // Top-right
    if (px > rx + rw - tr && py < ry + tr) {
        cx = rx + rw - tr; cy = ry + tr; r = tr;
        float dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy > r * r) return false;
    }
    // Bottom-right
    if (px > rx + rw - br && py > ry + rh - br) {
        cx = rx + rw - br; cy = ry + rh - br; r = br;
        float dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy > r * r) return false;
    }
    // Bottom-left
    if (px < rx + bl && py > ry + rh - bl) {
        cx = rx + bl; cy = ry + rh - bl; r = bl;
        float dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy > r * r) return false;
    }

    return true;
}

bool HitTestEngine::shouldIgnoreNode(const BoxNode* node, const HitTestRequest& request) {
    if (!node->isVisible()) return true;
    if (!request.ignorePointerEvents && node->pointerEventsNone()) return true;
    return false;
}

void HitTestEngine::transformPoint(const BoxNode* node, float& x, float& y) {
    if (!node->hasTransform()) return;
    const float* m = node->transformMatrix();
    float nx = m[0] * x + m[2] * y + m[4];
    float ny = m[1] * x + m[3] * y + m[5];
    x = nx; y = ny;
}

void HitTestEngine::inverseTransformPoint(const BoxNode* node, float& x, float& y) {
    if (!node->hasTransform()) return;
    const float* m = node->transformMatrix();
    float det = m[0] * m[3] - m[1] * m[2];
    if (std::abs(det) < 1e-6f) return;
    float invDet = 1.0f / det;
    float tx = x - m[4], ty = y - m[5];
    float nx = (m[3] * tx - m[2] * ty) * invDet;
    float ny = (-m[1] * tx + m[0] * ty) * invDet;
    x = nx; y = ny;
}

// ==================================================================
// ScrollContainer
// ==================================================================

ScrollContainer::ScrollContainer() {}
ScrollContainer::~ScrollContainer() {}

void ScrollContainer::setOverflow(OverflowMode x, OverflowMode y) {
    overflowX_ = x;
    overflowY_ = y;
    computeMaxScroll();
}

void ScrollContainer::setContentSize(float w, float h) {
    state_.scrollWidth = w;
    state_.scrollHeight = h;
    computeMaxScroll();
}

void ScrollContainer::setViewportSize(float w, float h) {
    state_.clientWidth = w;
    state_.clientHeight = h;
    computeMaxScroll();
}

void ScrollContainer::computeMaxScroll() {
    state_.maxScrollLeft = std::max(0.0f, state_.scrollWidth - state_.clientWidth);
    state_.maxScrollTop = std::max(0.0f, state_.scrollHeight - state_.clientHeight);
    state_.scrollingX = (overflowX_ == OverflowMode::Scroll || overflowX_ == OverflowMode::Auto) &&
                        state_.scrollWidth > state_.clientWidth;
    state_.scrollingY = (overflowY_ == OverflowMode::Scroll || overflowY_ == OverflowMode::Auto) &&
                        state_.scrollHeight > state_.clientHeight;
    clampScroll();
}

void ScrollContainer::clampScroll() {
    state_.scrollLeft = std::clamp(state_.scrollLeft, 0.0f, state_.maxScrollLeft);
    state_.scrollTop = std::clamp(state_.scrollTop, 0.0f, state_.maxScrollTop);
}

void ScrollContainer::scrollTo(float x, float y, ScrollBehavior behavior) {
    if (behavior == ScrollBehavior::Smooth) {
        targetScrollX_ = std::clamp(x, 0.0f, state_.maxScrollLeft);
        targetScrollY_ = std::clamp(y, 0.0f, state_.maxScrollTop);
        animating_ = true;
    } else {
        state_.scrollLeft = std::clamp(x, 0.0f, state_.maxScrollLeft);
        state_.scrollTop = std::clamp(y, 0.0f, state_.maxScrollTop);
        fireScrollEvent();
    }
}

void ScrollContainer::scrollBy(float dx, float dy, ScrollBehavior behavior) {
    scrollTo(state_.scrollLeft + dx, state_.scrollTop + dy, behavior);
}

void ScrollContainer::scrollToElement(const BoxNode* target, ScrollBehavior behavior) {
    if (!target) return;
    Rect b = target->borderRect();
    scrollTo(b.x, b.y, behavior);
}

void ScrollContainer::tickAnimation(double dt) {
    if (!animating_) return;

    float smoothing = 8.0f;
    float f = static_cast<float>(dt) * smoothing;
    f = std::min(f, 1.0f);

    state_.scrollLeft += (targetScrollX_ - state_.scrollLeft) * f;
    state_.scrollTop += (targetScrollY_ - state_.scrollTop) * f;

    // Check convergence
    if (std::abs(state_.scrollLeft - targetScrollX_) < 0.5f &&
        std::abs(state_.scrollTop - targetScrollY_) < 0.5f) {
        state_.scrollLeft = targetScrollX_;
        state_.scrollTop = targetScrollY_;
        animating_ = false;
    }

    clampScroll();
    fireScrollEvent();
}

void ScrollContainer::addSnapPoint(float position, ScrollSnapAlign align) {
    snapPointsX_.push_back({position, align});
    snapPointsY_.push_back({position, align});
}

float ScrollContainer::findSnapPosition(float current, float target, bool horizontal) {
    const auto& points = horizontal ? snapPointsX_ : snapPointsY_;
    if (points.empty() || snapType_ == ScrollSnapType::None) return target;

    float closest = target;
    float minDist = std::abs(target - current);

    for (const auto& sp : points) {
        float dist = std::abs(sp.position - target);
        if (dist < minDist) {
            minDist = dist;
            closest = sp.position;
        }
    }
    return closest;
}

void ScrollContainer::setOverscrollBehavior(OverscrollBehavior x, OverscrollBehavior y) {
    osBehaviorX_ = x;
    osBehaviorY_ = y;
}

void ScrollContainer::applyOverscrollElastic(float dx, float dy) {
    if (osBehaviorX_ != OverscrollBehavior::None) {
        if ((state_.scrollLeft <= 0 && dx < 0) ||
            (state_.scrollLeft >= state_.maxScrollLeft && dx > 0)) {
            overscrollX_ += dx * 0.3f; // Rubber band factor
        }
    }
    if (osBehaviorY_ != OverscrollBehavior::None) {
        if ((state_.scrollTop <= 0 && dy < 0) ||
            (state_.scrollTop >= state_.maxScrollTop && dy > 0)) {
            overscrollY_ += dy * 0.3f;
        }
    }
}

void ScrollContainer::releaseOverscroll() {
    overscrollX_ *= 0.85f;
    overscrollY_ *= 0.85f;
    if (std::abs(overscrollX_) < 0.5f) overscrollX_ = 0;
    if (std::abs(overscrollY_) < 0.5f) overscrollY_ = 0;
}

ScrollbarMetrics ScrollContainer::horizontalScrollbar() const {
    ScrollbarMetrics sb;
    if (!state_.scrollingX) return sb;
    sb.visible = true;
    sb.trackSize = state_.clientWidth;
    float ratio = state_.clientWidth / state_.scrollWidth;
    sb.thumbSize = std::max(20.0f, sb.trackSize * ratio);
    float scrollableTrack = sb.trackSize - sb.thumbSize;
    sb.thumbPosition = (state_.maxScrollLeft > 0) ?
        (state_.scrollLeft / state_.maxScrollLeft) * scrollableTrack : 0;
    return sb;
}

ScrollbarMetrics ScrollContainer::verticalScrollbar() const {
    ScrollbarMetrics sb;
    if (!state_.scrollingY) return sb;
    sb.visible = true;
    sb.trackSize = state_.clientHeight;
    float ratio = state_.clientHeight / state_.scrollHeight;
    sb.thumbSize = std::max(20.0f, sb.trackSize * ratio);
    float scrollableTrack = sb.trackSize - sb.thumbSize;
    sb.thumbPosition = (state_.maxScrollTop > 0) ?
        (state_.scrollTop / state_.maxScrollTop) * scrollableTrack : 0;
    return sb;
}

bool ScrollContainer::hitTestScrollbar(float x, float y, bool& horizontal, float& trackPos) {
    auto vsb = verticalScrollbar();
    if (vsb.visible) {
        float sbX = state_.clientWidth - vsb.scrollbarWidth;
        if (x >= sbX && x <= sbX + vsb.scrollbarWidth && y >= 0 && y <= state_.clientHeight) {
            horizontal = false;
            trackPos = y;
            return true;
        }
    }
    auto hsb = horizontalScrollbar();
    if (hsb.visible) {
        float sbY = state_.clientHeight - hsb.scrollbarWidth;
        if (y >= sbY && y <= sbY + hsb.scrollbarWidth && x >= 0 && x <= state_.clientWidth) {
            horizontal = true;
            trackPos = x;
            return true;
        }
    }
    return false;
}

void ScrollContainer::startScrollbarDrag(bool horizontal, float trackPos) {
    if (horizontal) {
        draggingH_ = true;
        dragTrackStart_ = trackPos;
        dragScrollStart_ = state_.scrollLeft;
    } else {
        draggingV_ = true;
        dragTrackStart_ = trackPos;
        dragScrollStart_ = state_.scrollTop;
    }
}

void ScrollContainer::updateScrollbarDrag(float trackPos) {
    if (draggingH_) {
        auto sb = horizontalScrollbar();
        float scrollableTrack = sb.trackSize - sb.thumbSize;
        if (scrollableTrack > 0) {
            float delta = trackPos - dragTrackStart_;
            float scrollDelta = (delta / scrollableTrack) * state_.maxScrollLeft;
            state_.scrollLeft = std::clamp(dragScrollStart_ + scrollDelta,
                                            0.0f, state_.maxScrollLeft);
            fireScrollEvent();
        }
    }
    if (draggingV_) {
        auto sb = verticalScrollbar();
        float scrollableTrack = sb.trackSize - sb.thumbSize;
        if (scrollableTrack > 0) {
            float delta = trackPos - dragTrackStart_;
            float scrollDelta = (delta / scrollableTrack) * state_.maxScrollTop;
            state_.scrollTop = std::clamp(dragScrollStart_ + scrollDelta,
                                           0.0f, state_.maxScrollTop);
            fireScrollEvent();
        }
    }
}

void ScrollContainer::endScrollbarDrag() {
    draggingH_ = false;
    draggingV_ = false;
}

void ScrollContainer::handleWheel(float deltaX, float deltaY, bool precise) {
    float multiplier = precise ? 1.0f : 40.0f;
    scrollBy(deltaX * multiplier, deltaY * multiplier);
}

void ScrollContainer::handleTouchStart(float x, float y) {
    touching_ = true;
    touchStartX_ = x; touchStartY_ = y;
    touchLastX_ = x; touchLastY_ = y;
    velocityX_ = 0; velocityY_ = 0;
    animating_ = false;
}

void ScrollContainer::handleTouchMove(float x, float y) {
    if (!touching_) return;
    float dx = touchLastX_ - x;
    float dy = touchLastY_ - y;
    velocityX_ = dx;
    velocityY_ = dy;

    scrollBy(dx, dy);

    // Overscroll elastic
    if (state_.scrollLeft <= 0 || state_.scrollLeft >= state_.maxScrollLeft) {
        applyOverscrollElastic(dx, 0);
    }
    if (state_.scrollTop <= 0 || state_.scrollTop >= state_.maxScrollTop) {
        applyOverscrollElastic(0, dy);
    }

    touchLastX_ = x; touchLastY_ = y;
}

void ScrollContainer::handleTouchEnd() {
    touching_ = false;
    releaseOverscroll();

    // Inertia scroll
    float speed = std::sqrt(velocityX_ * velocityX_ + velocityY_ * velocityY_);
    if (speed > 2.0f) {
        float duration = speed * 0.01f;
        targetScrollX_ = state_.scrollLeft + velocityX_ * duration;
        targetScrollY_ = state_.scrollTop + velocityY_ * duration;

        // Snap if applicable
        targetScrollX_ = findSnapPosition(state_.scrollLeft, targetScrollX_, true);
        targetScrollY_ = findSnapPosition(state_.scrollTop, targetScrollY_, false);

        targetScrollX_ = std::clamp(targetScrollX_, 0.0f, state_.maxScrollLeft);
        targetScrollY_ = std::clamp(targetScrollY_, 0.0f, state_.maxScrollTop);
        animating_ = true;
    }
}

bool ScrollContainer::isElementVisible(const BoxNode* element) const {
    if (!element) return false;
    Rect b = element->borderRect();
    return !(b.x + b.width < state_.scrollLeft ||
             b.x > state_.scrollLeft + state_.clientWidth ||
             b.y + b.height < state_.scrollTop ||
             b.y > state_.scrollTop + state_.clientHeight);
}

float ScrollContainer::scrollProgress() const {
    if (state_.maxScrollTop <= 0) return 0;
    return state_.scrollTop / state_.maxScrollTop;
}

void ScrollContainer::fireScrollEvent() {
    if (onScroll_) onScroll_(state_.scrollLeft, state_.scrollTop);
}

// ==================================================================
// ScrollChain
// ==================================================================

void ScrollChain::addContainer(ScrollContainer* container) {
    chain_.push_back(container);
}

void ScrollChain::removeContainer(ScrollContainer* container) {
    chain_.erase(std::remove(chain_.begin(), chain_.end(), container), chain_.end());
}

void ScrollChain::dispatchWheel(float deltaX, float deltaY, bool precise) {
    for (auto it = chain_.rbegin(); it != chain_.rend(); ++it) {
        auto& sc = *it;
        float oldLeft = sc->state().scrollLeft;
        float oldTop = sc->state().scrollTop;

        sc->handleWheel(deltaX, deltaY, precise);

        float consumedX = sc->state().scrollLeft - oldLeft;
        float consumedY = sc->state().scrollTop - oldTop;

        deltaX -= consumedX / (precise ? 1.0f : 40.0f);
        deltaY -= consumedY / (precise ? 1.0f : 40.0f);

        if (std::abs(deltaX) < 0.01f && std::abs(deltaY) < 0.01f) break;
    }
}

ScrollContainer* ScrollChain::containerAt(const BoxNode* /*root*/, float /*x*/, float /*y*/) {
    // Walk tree to find scroll container — requires box tree traversal
    // Returns the innermost scroll container at the given point
    if (!chain_.empty()) return chain_.back();
    return nullptr;
}

} // namespace Web
} // namespace NXRender
