/**
 * @file scrollbar.cpp
 * @brief Scrollbar implementation
 */

#include "webcore/scrollbar.hpp"
#include "webcore/theme.hpp"
#include <algorithm>

namespace Zepra::WebCore {

// =============================================================================
// Scrollbar
// =============================================================================

Scrollbar::Scrollbar(ScrollbarOrientation orientation) : orientation_(orientation) {}

void Scrollbar::setScrollPosition(float pos) {
    float maxPos = maxScrollPosition();
    scrollPos_ = std::max(0.0f, std::min(pos, maxPos));
}

float Scrollbar::maxScrollPosition() const {
    return std::max(0.0f, contentSize_ - viewportSize_);
}

void Scrollbar::updateThumb() {
    // Thumb size is proportional to viewport/content ratio
    // Handled in thumbRect()
}

Rect Scrollbar::trackRect() const {
    return {
        bounds_.x + trackPadding_,
        bounds_.y + trackPadding_,
        bounds_.width - trackPadding_ * 2,
        bounds_.height - trackPadding_ * 2
    };
}

Rect Scrollbar::thumbRect() const {
    Rect track = trackRect();
    
    if (contentSize_ <= viewportSize_) {
        // Content fits, thumb fills track
        return track;
    }
    
    float ratio = viewportSize_ / contentSize_;
    float scrollRatio = scrollPos_ / maxScrollPosition();
    
    if (orientation_ == ScrollbarOrientation::Vertical) {
        float thumbHeight = std::max(thumbMinSize_, track.height * ratio);
        float maxThumbY = track.height - thumbHeight;
        float thumbY = track.y + maxThumbY * scrollRatio;
        
        return {track.x, thumbY, track.width, thumbHeight};
    } else {
        float thumbWidth = std::max(thumbMinSize_, track.width * ratio);
        float maxThumbX = track.width - thumbWidth;
        float thumbX = track.x + maxThumbX * scrollRatio;
        
        return {thumbX, track.y, thumbWidth, track.height};
    }
}

void Scrollbar::paint(PaintContext& ctx) {
    if (!visible_ || contentSize_ <= viewportSize_) return;
    
    // Track background
    ctx.fillRect(bounds_, Theme::Content::surfaceAlt());
    
    // Track
    Rect track = trackRect();
    ctx.fillRect(track, Theme::Content::surface());
    
    // Thumb
    Rect thumb = thumbRect();
    Color thumbColor = dragging_ ? Theme::primary() :
                       hovered_ ? Theme::secondary() :
                                  Theme::Content::border();
    
    // Rounded thumb
    ctx.fillRect(thumb, thumbColor);
}

bool Scrollbar::handleMouseDown(float x, float y, int button) {
    if (button != 1 || !containsPoint(x, y) || contentSize_ <= viewportSize_) return false;
    
    Rect thumb = thumbRect();
    
    // Check if click is on thumb
    if (x >= thumb.x && x < thumb.x + thumb.width &&
        y >= thumb.y && y < thumb.y + thumb.height) {
        dragging_ = true;
        if (orientation_ == ScrollbarOrientation::Vertical) {
            dragOffset_ = y - thumb.y;
        } else {
            dragOffset_ = x - thumb.x;
        }
        return true;
    }
    
    // Click on track - jump to position
    Rect track = trackRect();
    float clickRatio;
    if (orientation_ == ScrollbarOrientation::Vertical) {
        clickRatio = (y - track.y) / track.height;
    } else {
        clickRatio = (x - track.x) / track.width;
    }
    
    float newPos = clickRatio * contentSize_ - viewportSize_ / 2;
    setScrollPosition(newPos);
    if (onScroll_) onScroll_(scrollPos_);
    
    return true;
}

bool Scrollbar::handleMouseUp(float x, float y, int button) {
    (void)x; (void)y;
    if (button == 1 && dragging_) {
        dragging_ = false;
        return true;
    }
    return false;
}

bool Scrollbar::handleMouseMove(float x, float y) {
    bool wasHovered = hovered_;
    hovered_ = containsPoint(x, y);
    
    if (dragging_) {
        Rect track = trackRect();
        Rect thumb = thumbRect();
        
        float thumbSize = (orientation_ == ScrollbarOrientation::Vertical) ? 
                          thumb.height : thumb.width;
        float trackSize = (orientation_ == ScrollbarOrientation::Vertical) ? 
                          track.height : track.width;
        
        float pos = (orientation_ == ScrollbarOrientation::Vertical) ?
                    y - track.y - dragOffset_ : x - track.x - dragOffset_;
        
        float maxPos = trackSize - thumbSize;
        float ratio = maxPos > 0 ? pos / maxPos : 0;
        ratio = std::max(0.0f, std::min(1.0f, ratio));
        
        float newScrollPos = ratio * maxScrollPosition();
        setScrollPosition(newScrollPos);
        if (onScroll_) onScroll_(scrollPos_);
        
        return true;
    }
    
    return hovered_ || wasHovered;
}

// =============================================================================
// ScrollContainer
// =============================================================================

ScrollContainer::ScrollContainer() {
    vScrollbar_ = std::make_unique<Scrollbar>(ScrollbarOrientation::Vertical);
    hScrollbar_ = std::make_unique<Scrollbar>(ScrollbarOrientation::Horizontal);
    
    vScrollbar_->setOnScroll([this](float pos) {
        scrollY_ = pos;
        if (onScroll_) onScroll_(scrollX_, scrollY_);
    });
    
    hScrollbar_->setOnScroll([this](float pos) {
        scrollX_ = pos;
        if (onScroll_) onScroll_(scrollX_, scrollY_);
    });
}

void ScrollContainer::setContentSize(float width, float height) {
    contentWidth_ = width;
    contentHeight_ = height;
    updateScrollbars();
}

void ScrollContainer::scrollTo(float x, float y) {
    scrollX_ = std::max(0.0f, std::min(x, contentWidth_ - bounds_.width));
    scrollY_ = std::max(0.0f, std::min(y, contentHeight_ - bounds_.height));
    updateScrollbars();
    if (onScroll_) onScroll_(scrollX_, scrollY_);
}

void ScrollContainer::scrollBy(float dx, float dy) {
    scrollTo(scrollX_ + dx, scrollY_ + dy);
}

void ScrollContainer::setShowScrollbars(bool vertical, bool horizontal) {
    showVScroll_ = vertical;
    showHScroll_ = horizontal;
}

void ScrollContainer::updateScrollbars() {
    const float scrollbarWidth = 12;
    Rect content = contentArea();
    
    if (showVScroll_) {
        vScrollbar_->setBounds(
            bounds_.x + bounds_.width - scrollbarWidth,
            bounds_.y,
            scrollbarWidth,
            bounds_.height - (showHScroll_ ? scrollbarWidth : 0)
        );
        vScrollbar_->setContentSize(contentHeight_);
        vScrollbar_->setViewportSize(content.height);
        vScrollbar_->setScrollPosition(scrollY_);
    }
    
    if (showHScroll_) {
        hScrollbar_->setBounds(
            bounds_.x,
            bounds_.y + bounds_.height - scrollbarWidth,
            bounds_.width - (showVScroll_ ? scrollbarWidth : 0),
            scrollbarWidth
        );
        hScrollbar_->setContentSize(contentWidth_);
        hScrollbar_->setViewportSize(content.width);
        hScrollbar_->setScrollPosition(scrollX_);
    }
}

Rect ScrollContainer::contentArea() const {
    const float scrollbarWidth = 12;
    return {
        bounds_.x,
        bounds_.y,
        bounds_.width - (showVScroll_ && contentHeight_ > bounds_.height ? scrollbarWidth : 0),
        bounds_.height - (showHScroll_ && contentWidth_ > bounds_.width ? scrollbarWidth : 0)
    };
}

void ScrollContainer::paint(PaintContext& ctx) {
    if (!visible_) return;
    
    // Background
    ctx.fillRect(bounds_, Theme::Content::background());
    
    // Scrollbars
    if (showVScroll_) vScrollbar_->paint(ctx);
    if (showHScroll_) hScrollbar_->paint(ctx);
    
    // Corner if both scrollbars visible
    if (showVScroll_ && showHScroll_ && 
        contentHeight_ > bounds_.height && contentWidth_ > bounds_.width) {
        const float scrollbarWidth = 12;
        Rect corner = {
            bounds_.x + bounds_.width - scrollbarWidth,
            bounds_.y + bounds_.height - scrollbarWidth,
            scrollbarWidth,
            scrollbarWidth
        };
        ctx.fillRect(corner, Theme::Content::surfaceAlt());
    }
}

bool ScrollContainer::handleMouseDown(float x, float y, int button) {
    if (!containsPoint(x, y)) return false;
    
    if (showVScroll_ && vScrollbar_->handleMouseDown(x, y, button)) return true;
    if (showHScroll_ && hScrollbar_->handleMouseDown(x, y, button)) return true;
    
    return false;
}

bool ScrollContainer::handleMouseUp(float x, float y, int button) {
    if (vScrollbar_->handleMouseUp(x, y, button)) return true;
    if (hScrollbar_->handleMouseUp(x, y, button)) return true;
    return false;
}

bool ScrollContainer::handleMouseMove(float x, float y) {
    bool handled = false;
    if (showVScroll_) handled |= vScrollbar_->handleMouseMove(x, y);
    if (showHScroll_) handled |= hScrollbar_->handleMouseMove(x, y);
    return handled;
}

bool ScrollContainer::handleWheel(float deltaX, float deltaY) {
    if (!containsPoint(bounds_.x + bounds_.width / 2, bounds_.y + bounds_.height / 2)) {
        return false;
    }
    
    scrollBy(deltaX, deltaY);
    return true;
}

} // namespace Zepra::WebCore
