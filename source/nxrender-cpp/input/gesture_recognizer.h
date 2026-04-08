// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file gesture_recognizer.h
 * @brief Touch/pointer gesture detection: tap, double-tap, long-press, pan, pinch, swipe.
 *
 * Each recognizer runs a state machine fed by raw pointer events. Multiple
 * recognizers can coexist — the one that enters .Recognized state first wins.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <chrono>
#include <cmath>

namespace NXRender {
namespace Input {

enum class GestureState {
    Possible,   // Initial — waiting for input
    Began,      // Gesture started (continuous gestures)
    Changed,    // Gesture updated (continuous gestures)
    Ended,      // Gesture completed
    Cancelled,  // Gesture cancelled (e.g. another recognizer took over)
    Failed      // Did not match
};

struct PointerEvent {
    float x, y;
    int pointerId;
    bool down;    // true = press, false = release
    bool moved;   // true if this is a move event
    uint64_t timestampMs;
};

/**
 * @brief Base gesture recognizer.
 */
class GestureRecognizer {
public:
    using Callback = std::function<void(GestureState, float, float)>;

    virtual ~GestureRecognizer() = default;

    void setCallback(Callback cb) { callback_ = std::move(cb); }
    GestureState state() const { return state_; }
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }

    virtual void feed(const PointerEvent& event) = 0;
    virtual void reset();

protected:
    void setState(GestureState s) { state_ = s; if (callback_) callback_(s, lastX_, lastY_); }

    GestureState state_ = GestureState::Possible;
    Callback callback_;
    bool enabled_ = true;
    float lastX_ = 0, lastY_ = 0;
};

/**
 * @brief Single tap recognizer.
 */
class TapRecognizer : public GestureRecognizer {
public:
    void setMaxDistance(float px) { maxDistance_ = px; }
    void setMaxDuration(int ms) { maxDurationMs_ = ms; }

    void feed(const PointerEvent& event) override;
    void reset() override;

private:
    float startX_ = 0, startY_ = 0;
    uint64_t startTime_ = 0;
    bool tracking_ = false;
    float maxDistance_ = 10.0f;
    int maxDurationMs_ = 300;
};

/**
 * @brief Double tap recognizer.
 */
class DoubleTapRecognizer : public GestureRecognizer {
public:
    void setMaxInterval(int ms) { maxIntervalMs_ = ms; }

    void feed(const PointerEvent& event) override;
    void reset() override;

private:
    TapRecognizer firstTap_;
    int tapCount_ = 0;
    uint64_t firstTapTime_ = 0;
    float firstTapX_ = 0, firstTapY_ = 0;
    int maxIntervalMs_ = 400;
};

/**
 * @brief Long press recognizer.
 */
class LongPressRecognizer : public GestureRecognizer {
public:
    void setMinDuration(int ms) { minDurationMs_ = ms; }
    void setMaxDistance(float px) { maxDistance_ = px; }

    void feed(const PointerEvent& event) override;
    void checkTimeout(uint64_t currentTimeMs);
    void reset() override;

private:
    float startX_ = 0, startY_ = 0;
    uint64_t startTime_ = 0;
    bool tracking_ = false;
    bool triggered_ = false;
    int minDurationMs_ = 500;
    float maxDistance_ = 10.0f;
};

/**
 * @brief Pan (drag) recognizer with velocity tracking.
 */
class PanRecognizer : public GestureRecognizer {
public:
    using PanCallback = std::function<void(GestureState, float dx, float dy, float vx, float vy)>;

    void setPanCallback(PanCallback cb) { panCallback_ = std::move(cb); }
    void setMinDistance(float px) { minDistance_ = px; }

    float deltaX() const { return currentX_ - startX_; }
    float deltaY() const { return currentY_ - startY_; }
    float velocityX() const { return velocityX_; }
    float velocityY() const { return velocityY_; }

    void feed(const PointerEvent& event) override;
    void reset() override;

private:
    void updateVelocity(float x, float y, uint64_t time);

    PanCallback panCallback_;
    float startX_ = 0, startY_ = 0;
    float currentX_ = 0, currentY_ = 0;
    float prevX_ = 0, prevY_ = 0;
    uint64_t prevTime_ = 0;
    float velocityX_ = 0, velocityY_ = 0;
    bool tracking_ = false;
    bool recognized_ = false;
    float minDistance_ = 8.0f;

    // Velocity history for smoothing
    static constexpr int kVelocityHistorySize = 4;
    struct VelocitySample { float vx, vy; uint64_t time; };
    VelocitySample velocityHistory_[kVelocityHistorySize] = {};
    int velocityIndex_ = 0;
};

/**
 * @brief Pinch (zoom) recognizer for two-pointer distance changes.
 */
class PinchRecognizer : public GestureRecognizer {
public:
    using PinchCallback = std::function<void(GestureState, float scale, float centerX, float centerY)>;

    void setPinchCallback(PinchCallback cb) { pinchCallback_ = std::move(cb); }
    float currentScale() const { return scale_; }

    void feed(const PointerEvent& event) override;
    void reset() override;

private:
    struct Pointer { int id; float x, y; bool active; };

    PinchCallback pinchCallback_;
    Pointer pointers_[2] = {};
    int pointerCount_ = 0;
    float initialDistance_ = 0;
    float scale_ = 1.0f;
    bool recognized_ = false;
};

/**
 * @brief Swipe recognizer (fast directional gesture).
 */
class SwipeRecognizer : public GestureRecognizer {
public:
    enum class Direction { Left, Right, Up, Down };
    using SwipeCallback = std::function<void(Direction)>;

    void setSwipeCallback(SwipeCallback cb) { swipeCallback_ = std::move(cb); }
    void setMinVelocity(float pxPerMs) { minVelocity_ = pxPerMs; }
    void setMaxDuration(int ms) { maxDurationMs_ = ms; }

    void feed(const PointerEvent& event) override;
    void reset() override;

private:
    SwipeCallback swipeCallback_;
    float startX_ = 0, startY_ = 0;
    uint64_t startTime_ = 0;
    bool tracking_ = false;
    float minVelocity_ = 0.5f; // px/ms
    int maxDurationMs_ = 400;
};

} // namespace Input
} // namespace NXRender
