// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file splitter.h
 * @brief Split view widget with draggable divider.
 */

#pragma once

#include "widget.h"

namespace NXRender {

class Splitter : public Widget {
public:
    enum class Orientation { Horizontal, Vertical };

    Splitter();

    void setOrientation(Orientation o) { orientation_ = o; invalidate(); }
    Orientation orientation() const { return orientation_; }

    /**
     * @brief Set the split ratio (0.0 - 1.0). 0.5 = even split.
     */
    void setSplitRatio(float ratio);
    float splitRatio() const { return splitRatio_; }

    /**
     * @brief Set minimum sizes for both panels.
     */
    void setMinFirst(float px) { minFirstPx_ = px; }
    void setMinSecond(float px) { minSecondPx_ = px; }

    /**
     * @brief Set the handle width (divider thickness).
     */
    void setHandleWidth(float w) { handleWidth_ = w; }
    float handleWidth() const { return handleWidth_; }

    /**
     * @brief Set the first and second panel content widgets.
     */
    void setFirst(std::unique_ptr<Widget> widget);
    void setSecond(std::unique_ptr<Widget> widget);

    Widget* first() const { return first_; }
    Widget* second() const { return second_; }

    // Colors
    void setHandleColor(const Color& color) { handleColor_ = color; }
    void setHandleHoverColor(const Color& color) { handleHoverColor_ = color; }

    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;
    void layout() override;

    // Events
    EventResult onMouseDown(float x, float y, MouseButton button) override;
    EventResult onMouseMove(float x, float y) override;
    EventResult onMouseUp(float x, float y, MouseButton button) override;

private:
    Rect handleRect() const;
    bool isOnHandle(float x, float y) const;
    float clampRatio(float ratio) const;

    Orientation orientation_ = Orientation::Horizontal;
    float splitRatio_ = 0.5f;
    float minFirstPx_ = 50.0f;
    float minSecondPx_ = 50.0f;
    float handleWidth_ = 6.0f;

    Color handleColor_ = Color(0xD0D0D0);
    Color handleHoverColor_ = Color(0x90CAF9);

    Widget* first_ = nullptr;
    Widget* second_ = nullptr;

    bool dragging_ = false;
    float dragStart_ = 0;
    float ratioStart_ = 0;
    bool handleHovered_ = false;
};

} // namespace NXRender
