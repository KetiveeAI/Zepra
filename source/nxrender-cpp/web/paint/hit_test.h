// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "../box/box_tree.h"
#include "paint_ops.h"
#include <vector>
#include <functional>

namespace NXRender {
namespace Web {

// ==================================================================
// Transform-aware hit testing
// ==================================================================

struct HitTestRequest {
    float x = 0, y = 0;
    bool ignorePointerEvents = false;
    bool stopAtFirstHit = true;

    enum class Type { Default, Hover, Click, Touch, Drag } type = Type::Default;
};

struct HitTestEntry {
    const BoxNode* node = nullptr;
    float localX = 0, localY = 0;
    int depth = 0;
    float zIndex = 0;
    bool isScrollbar = false;
    bool isOverflowClipped = false;
};

class HitTestEngine {
public:
    std::vector<HitTestEntry> hitTest(const BoxNode* root, const HitTestRequest& request);
    HitTestEntry hitTestFirst(const BoxNode* root, float x, float y);

private:
    void traverse(const BoxNode* node, float x, float y, int depth,
                  const HitTestRequest& request, std::vector<HitTestEntry>& results);
    bool isPointInNode(const BoxNode* node, float x, float y,
                       float& localX, float& localY);
    bool isPointInRoundedRect(float px, float py, float rx, float ry, float rw, float rh,
                               float tl, float tr, float br, float bl);
    bool shouldIgnoreNode(const BoxNode* node, const HitTestRequest& request);
    void transformPoint(const BoxNode* node, float& x, float& y);
    void inverseTransformPoint(const BoxNode* node, float& x, float& y);
};

// ==================================================================
// Scroll container
// ==================================================================

enum class OverflowMode : uint8_t {
    Visible, Hidden, Scroll, Auto, Clip
};

enum class ScrollBehavior : uint8_t {
    Auto, Smooth, Instant
};

enum class OverscrollBehavior : uint8_t {
    Auto, Contain, None
};

enum class ScrollSnapType : uint8_t {
    None, X, Y, Block, Inline, Both
};

enum class ScrollSnapAlign : uint8_t {
    None, Start, End, Center
};

struct ScrollSnapPoint {
    float position = 0;
    ScrollSnapAlign align = ScrollSnapAlign::None;
};

struct ScrollState {
    float scrollLeft = 0;
    float scrollTop = 0;
    float scrollWidth = 0;     // total content width
    float scrollHeight = 0;    // total content height
    float clientWidth = 0;     // visible width
    float clientHeight = 0;    // visible height
    float maxScrollLeft = 0;
    float maxScrollTop = 0;
    bool scrollingX = false;
    bool scrollingY = false;
};

struct ScrollbarMetrics {
    float trackSize = 0;
    float thumbSize = 0;
    float thumbPosition = 0;
    float scrollbarWidth = 12;
    bool visible = false;
    bool hovered = false;
    bool dragging = false;
    float dragStart = 0;
    float dragScrollStart = 0;
};

class ScrollContainer {
public:
    ScrollContainer();
    ~ScrollContainer();

    void setOverflow(OverflowMode x, OverflowMode y);
    OverflowMode overflowX() const { return overflowX_; }
    OverflowMode overflowY() const { return overflowY_; }

    void setContentSize(float w, float h);
    void setViewportSize(float w, float h);
    const ScrollState& state() const { return state_; }

    // Scrolling
    void scrollTo(float x, float y, ScrollBehavior behavior = ScrollBehavior::Auto);
    void scrollBy(float dx, float dy, ScrollBehavior behavior = ScrollBehavior::Auto);
    void scrollToElement(const BoxNode* target, ScrollBehavior behavior = ScrollBehavior::Smooth);

    // Smooth scroll animation
    void tickAnimation(double dt);
    bool isAnimating() const { return animating_; }

    // Scroll snap
    void setScrollSnap(ScrollSnapType type) { snapType_ = type; }
    void addSnapPoint(float position, ScrollSnapAlign align);
    void clearSnapPoints() { snapPointsX_.clear(); snapPointsY_.clear(); }
    float findSnapPosition(float current, float target, bool horizontal);

    // Overscroll
    void setOverscrollBehavior(OverscrollBehavior x, OverscrollBehavior y);
    float overscrollX() const { return overscrollX_; }
    float overscrollY() const { return overscrollY_; }
    void applyOverscrollElastic(float dx, float dy);
    void releaseOverscroll();

    // Scrollbar
    ScrollbarMetrics horizontalScrollbar() const;
    ScrollbarMetrics verticalScrollbar() const;
    bool hitTestScrollbar(float x, float y, bool& horizontal, float& trackPos);
    void startScrollbarDrag(bool horizontal, float trackPos);
    void updateScrollbarDrag(float trackPos);
    void endScrollbarDrag();

    // Wheel
    void handleWheel(float deltaX, float deltaY, bool precise);

    // Touch
    void handleTouchStart(float x, float y);
    void handleTouchMove(float x, float y);
    void handleTouchEnd();

    // Callbacks
    using ScrollCallback = std::function<void(float scrollLeft, float scrollTop)>;
    void onScroll(ScrollCallback cb) { onScroll_ = cb; }

    // Visibility queries
    bool isElementVisible(const BoxNode* element) const;
    float scrollProgress() const;

private:
    OverflowMode overflowX_ = OverflowMode::Visible;
    OverflowMode overflowY_ = OverflowMode::Visible;
    ScrollState state_;
    OverscrollBehavior osBehaviorX_ = OverscrollBehavior::Auto;
    OverscrollBehavior osBehaviorY_ = OverscrollBehavior::Auto;
    float overscrollX_ = 0, overscrollY_ = 0;

    // Smooth scroll animation
    bool animating_ = false;
    float targetScrollX_ = 0, targetScrollY_ = 0;
    float velocityX_ = 0, velocityY_ = 0;

    // Scroll snap
    ScrollSnapType snapType_ = ScrollSnapType::None;
    std::vector<ScrollSnapPoint> snapPointsX_;
    std::vector<ScrollSnapPoint> snapPointsY_;

    // Touch tracking
    bool touching_ = false;
    float touchStartX_ = 0, touchStartY_ = 0;
    float touchLastX_ = 0, touchLastY_ = 0;

    // Scrollbar drag
    bool draggingH_ = false, draggingV_ = false;
    float dragTrackStart_ = 0, dragScrollStart_ = 0;

    ScrollCallback onScroll_;

    void clampScroll();
    void fireScrollEvent();
    void computeMaxScroll();
};

// ==================================================================
// Scroll chain — scroll propagation through nested containers
// ==================================================================

class ScrollChain {
public:
    void addContainer(ScrollContainer* container);
    void removeContainer(ScrollContainer* container);

    // Dispatch wheel event through chain
    void dispatchWheel(float deltaX, float deltaY, bool precise);

    // Find scroll container at point
    ScrollContainer* containerAt(const BoxNode* root, float x, float y);

private:
    std::vector<ScrollContainer*> chain_;
};

} // namespace Web
} // namespace NXRender
