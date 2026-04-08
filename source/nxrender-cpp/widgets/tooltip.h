// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file tooltip.h
 * @brief Delayed tooltip widget with arrow and auto-dismiss.
 */

#pragma once

#include "widget.h"
#include <chrono>

namespace NXRender {

class Tooltip : public Widget {
public:
    enum class Placement { Top, Bottom, Left, Right, Auto };

    Tooltip();

    void setText(const std::string& text) { text_ = text; }
    const std::string& text() const { return text_; }

    void setPlacement(Placement p) { placement_ = p; }
    Placement placement() const { return placement_; }

    void setShowDelay(int ms) { showDelayMs_ = ms; }
    void setDismissDelay(int ms) { dismissDelayMs_ = ms; }
    void setMaxWidth(float w) { maxWidth_ = w; }
    void setFontSize(float size) { fontSize_ = size; }

    void setTextColor(const Color& c) { textColor_ = c; }
    void setArrowSize(float s) { arrowSize_ = s; }

    /**
     * @brief Show the tooltip anchored to the given widget.
     */
    void showAt(Widget* anchor);

    /**
     * @brief Begin show timer (called on hover).
     */
    void scheduleShow(Widget* anchor);

    /**
     * @brief Cancel pending show and hide immediately.
     */
    void cancelAndHide();

    /**
     * @brief Update timer state. Call each frame.
     */
    void update(float deltaMs);

    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;

private:
    Placement computeAutoPlacement(Widget* anchor) const;
    void positionRelativeTo(Widget* anchor);
    std::vector<std::string> wrapText(float maxWidth) const;

    std::string text_;
    Placement placement_ = Placement::Top;
    int showDelayMs_ = 500;
    int dismissDelayMs_ = 3000;
    float maxWidth_ = 300.0f;
    float fontSize_ = 12.0f;
    Color textColor_ = Color(255, 255, 255);
    float arrowSize_ = 6.0f;

    Widget* anchor_ = nullptr;
    bool showPending_ = false;
    bool visible_ = false;
    float showTimer_ = 0;
    float dismissTimer_ = 0;
    float opacity_ = 0.0f;
};

} // namespace NXRender
