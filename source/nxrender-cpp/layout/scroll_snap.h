// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "../nxgfx/primitives.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace NXRender {

// ==================================================================
// CSS Scroll Snap Engine
// ==================================================================

enum class ScrollSnapType : uint8_t {
    None, Mandatory, Proximity
};

enum class ScrollSnapAxis : uint8_t {
    Both, X, Y, Block, Inline
};

enum class ScrollSnapAlign : uint8_t {
    None, Start, End, Center
};

enum class ScrollSnapStop : uint8_t {
    Normal, Always
};

struct ScrollSnapPoint {
    float position = 0;        // Snap position in scroll space
    ScrollSnapAlign align = ScrollSnapAlign::Start;
    ScrollSnapStop stop = ScrollSnapStop::Normal;
    Rect area;                 // Snap area rect
    int elementIndex = -1;     // Source element index
};

struct ScrollSnapConfig {
    ScrollSnapType type = ScrollSnapType::None;
    ScrollSnapAxis axis = ScrollSnapAxis::Both;
    float snapPadding[4] = {0, 0, 0, 0}; // T R B L
    float proximityThreshold = 0.3f;       // Fraction of viewport
};

class ScrollSnapEngine {
public:
    ScrollSnapEngine();
    ~ScrollSnapEngine();

    void setConfig(const ScrollSnapConfig& config) { config_ = config; }
    const ScrollSnapConfig& config() const { return config_; }

    // Register snap points from child elements
    void clear();
    void addSnapPoint(const ScrollSnapPoint& point);
    void addSnapPointX(float x, ScrollSnapAlign align = ScrollSnapAlign::Start,
                        ScrollSnapStop stop = ScrollSnapStop::Normal);
    void addSnapPointY(float y, ScrollSnapAlign align = ScrollSnapAlign::Start,
                        ScrollSnapStop stop = ScrollSnapStop::Normal);

    // Find snap destination from current scroll + velocity
    struct SnapResult {
        float targetX = 0, targetY = 0;
        bool snapped = false;
        int snapIndexX = -1, snapIndexY = -1;
    };

    SnapResult findSnapTarget(float scrollX, float scrollY,
                               float velocityX, float velocityY,
                               float viewportWidth, float viewportHeight) const;

    // Find nearest snap point to a position
    float nearestSnapX(float scrollX, float viewportWidth) const;
    float nearestSnapY(float scrollY, float viewportHeight) const;

    // Check if currently snapped
    bool isSnapped(float scrollX, float scrollY, float tolerance = 1.0f) const;

    // Resnap after layout change
    SnapResult resnap(float scrollX, float scrollY,
                       float viewportWidth, float viewportHeight) const;

    const std::vector<ScrollSnapPoint>& snapPointsX() const { return snapPointsX_; }
    const std::vector<ScrollSnapPoint>& snapPointsY() const { return snapPointsY_; }

private:
    ScrollSnapConfig config_;
    std::vector<ScrollSnapPoint> snapPointsX_;
    std::vector<ScrollSnapPoint> snapPointsY_;

    float snapPosition(float scroll, float viewport, const ScrollSnapPoint& point,
                        ScrollSnapAlign align) const;
    bool shouldSnap(float distance, float viewport) const;
};

// ==================================================================
// Smooth scroll animation
// ==================================================================

class SmoothScroller {
public:
    SmoothScroller();

    void scrollTo(float targetX, float targetY);
    void scrollBy(float deltaX, float deltaY);
    void snapTo(float targetX, float targetY);
    void stop();

    void tick(float deltaMs);

    float scrollX() const { return currentX_; }
    float scrollY() const { return currentY_; }
    bool isAnimating() const { return animating_; }

    // Limits
    void setScrollBounds(float minX, float minY, float maxX, float maxY);
    void setBehavior(const std::string& behavior); // "auto", "smooth", "instant"

    // Overscroll
    enum class OverscrollBehavior : uint8_t { Auto, Contain, None };
    void setOverscrollX(OverscrollBehavior b) { overscrollX_ = b; }
    void setOverscrollY(OverscrollBehavior b) { overscrollY_ = b; }
    float overscrollAmountX() const { return overscrollAmountX_; }
    float overscrollAmountY() const { return overscrollAmountY_; }

    // Snap integration
    void setSnapEngine(ScrollSnapEngine* engine) { snapEngine_ = engine; }

    // Scroll event
    using ScrollCallback = std::function<void(float x, float y)>;
    void onScroll(ScrollCallback cb) { onScroll_ = cb; }

private:
    float currentX_ = 0, currentY_ = 0;
    float targetX_ = 0, targetY_ = 0;
    float velocityX_ = 0, velocityY_ = 0;
    float minX_ = 0, minY_ = 0, maxX_ = 0, maxY_ = 0;
    bool animating_ = false;
    bool smooth_ = true;

    OverscrollBehavior overscrollX_ = OverscrollBehavior::Auto;
    OverscrollBehavior overscrollY_ = OverscrollBehavior::Auto;
    float overscrollAmountX_ = 0, overscrollAmountY_ = 0;

    ScrollSnapEngine* snapEngine_ = nullptr;
    ScrollCallback onScroll_;

    float viewportWidth_ = 0, viewportHeight_ = 0;

    static constexpr float SPRING_STIFFNESS = 200.0f;
    static constexpr float SPRING_DAMPING = 20.0f;
    static constexpr float VELOCITY_THRESHOLD = 0.5f;
};

} // namespace NXRender
