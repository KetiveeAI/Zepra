// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file scrollview.h
 * @brief Scrollable container widget
 */

#pragma once

#include "widget.h"

namespace NXRender {

/**
 * @brief Scroll direction
 */
enum class ScrollDirection {
    Vertical,
    Horizontal,
    Both
};

/**
 * @brief Scrollable container widget
 */
class ScrollView : public Widget {
public:
    ScrollView();
    ~ScrollView() override;
    
    // Content
    Widget* content() const { return content_; }
    void setContent(std::unique_ptr<Widget> content);
    
    // Scroll position
    float scrollX() const { return scrollX_; }
    float scrollY() const { return scrollY_; }
    void setScrollPosition(float x, float y);
    void scrollTo(float x, float y, bool animated = false);
    void scrollBy(float dx, float dy);
    
    // Configuration
    ScrollDirection direction() const { return direction_; }
    void setDirection(ScrollDirection dir) { direction_ = dir; }
    
    bool showScrollbars() const { return showScrollbars_; }
    void setShowScrollbars(bool show) { showScrollbars_ = show; }
    
    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;
    void layout() override;
    
    // Events
    EventResult onMouseDown(float x, float y, MouseButton button) override;
    EventResult onMouseUp(float x, float y, MouseButton button) override;
    EventResult onMouseMove(float x, float y) override;
    
private:
    Widget* content_ = nullptr;
    std::unique_ptr<Widget> ownedContent_;
    float scrollX_ = 0;
    float scrollY_ = 0;
    float contentWidth_ = 0;
    float contentHeight_ = 0;
    ScrollDirection direction_ = ScrollDirection::Vertical;
    bool showScrollbars_ = true;
    
    // Scrollbar dragging
    bool draggingVScroll_ = false;
    bool draggingHScroll_ = false;
    float dragStartY_ = 0;
    float dragStartScrollY_ = 0;
};

} // namespace NXRender
