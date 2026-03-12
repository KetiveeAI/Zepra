// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file scrollview.cpp
 * @brief ScrollView widget implementation
 */

#include "widgets/scrollview.h"
#include "nxgfx/context.h"
#include "theme/theme.h"
#include <algorithm>

namespace NXRender {

ScrollView::ScrollView() {}
ScrollView::~ScrollView() = default;

void ScrollView::setContent(std::unique_ptr<Widget> content) {
    ownedContent_ = std::move(content);
    content_ = ownedContent_.get();
    if (content_) {
        content_->setBounds(Rect(0, 0, bounds_.width, bounds_.height));
    }
}

void ScrollView::setScrollPosition(float x, float y) {
    scrollX_ = std::max(0.0f, std::min(x, contentWidth_ - bounds_.width));
    scrollY_ = std::max(0.0f, std::min(y, contentHeight_ - bounds_.height));
}

void ScrollView::scrollTo(float x, float y, bool animated) {
    (void)animated;  // Animation not implemented
    setScrollPosition(x, y);
}

void ScrollView::scrollBy(float dx, float dy) {
    setScrollPosition(scrollX_ + dx, scrollY_ + dy);
}

void ScrollView::render(GpuContext* ctx) {
    if (!isVisible()) return;
    
    // Draw background
    if (backgroundColor_.a > 0) {
        ctx->fillRect(bounds_, backgroundColor_);
    }
    
    // Clip to bounds
    ctx->pushClip(bounds_);
    
    // Render content with scroll offset
    if (content_) {
        ctx->pushTransform();
        ctx->translate(-scrollX_, -scrollY_);
        content_->render(ctx);
        ctx->popTransform();
    }
    
    ctx->popClip();
    
    // Draw scrollbars
    if (showScrollbars_) {
        Theme* t = currentTheme();
        Color scrollbarColor = t ? t->colors.border : Color(0xCCCCCC);
        Color thumbColor = t ? t->colors.textSecondary : Color(0x888888);
        
        // Vertical scrollbar
        if (contentHeight_ > bounds_.height) {
            float trackHeight = bounds_.height - 4;
            float thumbHeight = std::max(20.0f, trackHeight * (bounds_.height / contentHeight_));
            float thumbY = (scrollY_ / (contentHeight_ - bounds_.height)) * (trackHeight - thumbHeight);
            
            Rect track(bounds_.right() - 10, bounds_.y + 2, 8, trackHeight);
            Rect thumb(bounds_.right() - 10, bounds_.y + 2 + thumbY, 8, thumbHeight);
            
            ctx->fillRoundedRect(track, scrollbarColor, 4);
            ctx->fillRoundedRect(thumb, thumbColor, 4);
        }
        
        // Horizontal scrollbar
        if (contentWidth_ > bounds_.width && direction_ != ScrollDirection::Vertical) {
            float trackWidth = bounds_.width - 4;
            float thumbWidth = std::max(20.0f, trackWidth * (bounds_.width / contentWidth_));
            float thumbX = (scrollX_ / (contentWidth_ - bounds_.width)) * (trackWidth - thumbWidth);
            
            Rect track(bounds_.x + 2, bounds_.bottom() - 10, trackWidth, 8);
            Rect thumb(bounds_.x + 2 + thumbX, bounds_.bottom() - 10, thumbWidth, 8);
            
            ctx->fillRoundedRect(track, scrollbarColor, 4);
            ctx->fillRoundedRect(thumb, thumbColor, 4);
        }
    }
}

Size ScrollView::measure(const Size& available) {
    return available;  // ScrollView fills available space
}

void ScrollView::layout() {
    if (content_) {
        Size contentSize = content_->measure(Size(1e6f, 1e6f));
        contentWidth_ = contentSize.width;
        contentHeight_ = contentSize.height;
        content_->setBounds(Rect(0, 0, contentWidth_, contentHeight_));
        content_->layout();
    }
}

EventResult ScrollView::onMouseDown(float x, float y, MouseButton button) {
    if (button != MouseButton::Left) return EventResult::Ignored;
    
    // Check if clicking on scrollbar thumb
    if (contentHeight_ > bounds_.height) {
        float trackHeight = bounds_.height - 4;
        float thumbHeight = std::max(20.0f, trackHeight * (bounds_.height / contentHeight_));
        float thumbY = (scrollY_ / (contentHeight_ - bounds_.height)) * (trackHeight - thumbHeight);
        
        Rect thumb(bounds_.right() - 10, bounds_.y + 2 + thumbY, 8, thumbHeight);
        if (thumb.contains(x, y)) {
            draggingVScroll_ = true;
            dragStartY_ = y;
            dragStartScrollY_ = scrollY_;
            return EventResult::Handled;
        }
    }
    
    return EventResult::Ignored;
}

EventResult ScrollView::onMouseUp(float x, float y, MouseButton button) {
    (void)x; (void)y; (void)button;
    draggingVScroll_ = false;
    draggingHScroll_ = false;
    return EventResult::Handled;
}

EventResult ScrollView::onMouseMove(float x, float y) {
    (void)x;
    if (draggingVScroll_) {
        float delta = y - dragStartY_;
        float trackHeight = bounds_.height - 4;
        float thumbHeight = std::max(20.0f, trackHeight * (bounds_.height / contentHeight_));
        float scrollRange = contentHeight_ - bounds_.height;
        float scrollDelta = (delta / (trackHeight - thumbHeight)) * scrollRange;
        setScrollPosition(scrollX_, dragStartScrollY_ + scrollDelta);
        return EventResult::NeedsRedraw;
    }
    return EventResult::Ignored;
}

} // namespace NXRender
