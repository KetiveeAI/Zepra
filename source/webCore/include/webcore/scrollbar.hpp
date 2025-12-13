/**
 * @file scrollbar.hpp
 * @brief Scrollbar UI component
 */

#pragma once

#include "browser_ui.hpp"

namespace Zepra::WebCore {

/**
 * @brief Scrollbar orientation
 */
enum class ScrollbarOrientation {
    Vertical,
    Horizontal
};

/**
 * @brief Scrollbar component
 */
class Scrollbar : public UIComponent {
public:
    Scrollbar(ScrollbarOrientation orientation);
    
    // Configuration
    void setContentSize(float size) { contentSize_ = size; }
    void setViewportSize(float size) { viewportSize_ = size; updateThumb(); }
    void setScrollPosition(float pos);
    
    float scrollPosition() const { return scrollPos_; }
    float maxScrollPosition() const;
    
    // Events
    void setOnScroll(std::function<void(float)> handler) { onScroll_ = handler; }
    
    // Rendering
    void paint(PaintContext& ctx) override;
    
    // Input
    bool handleMouseDown(float x, float y, int button) override;
    bool handleMouseUp(float x, float y, int button) override;
    bool handleMouseMove(float x, float y) override;
    
private:
    void updateThumb();
    Rect trackRect() const;
    Rect thumbRect() const;
    
    ScrollbarOrientation orientation_;
    float contentSize_ = 0;
    float viewportSize_ = 0;
    float scrollPos_ = 0;
    
    // Thumb dragging
    bool dragging_ = false;
    float dragOffset_ = 0;
    
    // Appearance
    float thumbMinSize_ = 30;
    float trackPadding_ = 2;
    
    std::function<void(float)> onScroll_;
};

/**
 * @brief Scrollable container
 */
class ScrollContainer : public UIComponent {
public:
    ScrollContainer();
    
    // Content
    void setContentSize(float width, float height);
    float contentWidth() const { return contentWidth_; }
    float contentHeight() const { return contentHeight_; }
    
    // Scroll position
    void scrollTo(float x, float y);
    void scrollBy(float dx, float dy);
    float scrollX() const { return scrollX_; }
    float scrollY() const { return scrollY_; }
    
    // Scrollbars
    bool showVerticalScrollbar() const { return showVScroll_; }
    bool showHorizontalScrollbar() const { return showHScroll_; }
    void setShowScrollbars(bool vertical, bool horizontal);
    
    // Events
    void setOnScroll(std::function<void(float, float)> handler) { onScroll_ = handler; }
    
    // Rendering
    void paint(PaintContext& ctx) override;
    
    // Input
    bool handleMouseDown(float x, float y, int button) override;
    bool handleMouseUp(float x, float y, int button) override;
    bool handleMouseMove(float x, float y) override;
    bool handleWheel(float deltaX, float deltaY);
    
    // Content area (excluding scrollbars)
    Rect contentArea() const;
    
private:
    void updateScrollbars();
    
    float contentWidth_ = 0;
    float contentHeight_ = 0;
    float scrollX_ = 0;
    float scrollY_ = 0;
    
    bool showVScroll_ = true;
    bool showHScroll_ = false;
    
    std::unique_ptr<Scrollbar> vScrollbar_;
    std::unique_ptr<Scrollbar> hScrollbar_;
    
    std::function<void(float, float)> onScroll_;
};

} // namespace Zepra::WebCore
