// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file listview.cpp
 * @brief ListView widget implementation
 */

#include "widgets/listview.h"
#include "nxgfx/context.h"

namespace NXRender {

ListView::ListView(size_t itemCount, float rowHeight)
    : itemCount_(itemCount), rowHeight_(rowHeight) {}

void ListView::setItemCount(size_t count) {
    itemCount_ = count;
    if (selectedIndex_ >= count) {
        selectedIndex_ = SIZE_MAX;
    }
}

void ListView::reloadData() {
    invalidate();
}

void ListView::setSelectedIndex(size_t index) {
    if (index < itemCount_) {
        selectedIndex_ = index;
        if (onSelect_) {
            onSelect_(index);
        }
    }
}

void ListView::scrollTo(size_t index) {
    if (index >= itemCount_) return;
    float targetY = index * rowHeight_;
    float viewHeight = bounds_.height;
    if (targetY < scrollOffset_) {
        scrollOffset_ = targetY;
    } else if (targetY + rowHeight_ > scrollOffset_ + viewHeight) {
        scrollOffset_ = targetY + rowHeight_ - viewHeight;
    }
}

void ListView::scrollBy(float delta) {
    scrollOffset_ += delta;
    if (scrollOffset_ < 0) scrollOffset_ = 0;
    float maxScroll = contentHeight() - bounds_.height;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset_ > maxScroll) scrollOffset_ = maxScroll;
}

void ListView::render(GpuContext* gpu) {
    if (!gpu) return;
    
    gpu->fillRect(bounds_, backgroundColor_);
    gpu->pushClip(bounds_);
    
    size_t first = static_cast<size_t>(scrollOffset_ / rowHeight_);
    size_t visible = static_cast<size_t>(bounds_.height / rowHeight_) + 2;
    size_t last = std::min(first + visible, itemCount_);
    
    for (size_t i = first; i < last; ++i) {
        float rowY = bounds_.y + (i * rowHeight_) - scrollOffset_;
        Rect rowRect(bounds_.x, rowY, bounds_.width, rowHeight_);
        
        if (i == selectedIndex_) {
            gpu->fillRect(rowRect, selectedColor_);
        } else if (i == hoveredIndex_) {
            gpu->fillRect(rowRect, hoverColor_);
        }
        
        if (showsSeparators_ && i < itemCount_ - 1) {
            Rect sep(bounds_.x + 8, rowY + rowHeight_ - 1, bounds_.width - 16, 1);
            gpu->fillRect(sep, separatorColor_);
        }
    }
    
    gpu->popClip();
}

EventResult ListView::handleEvent(const Event& event) {
    switch (event.type) {
        case EventType::MouseDown:
            if (bounds_.contains(event.mouse.x, event.mouse.y)) {
                size_t clicked = indexAtPoint(event.mouse.x, event.mouse.y);
                if (clicked < itemCount_) {
                    setSelectedIndex(clicked);
                    return EventResult::NeedsRedraw;
                }
            }
            break;
        case EventType::MouseMove:
            if (bounds_.contains(event.mouse.x, event.mouse.y)) {
                size_t old = hoveredIndex_;
                hoveredIndex_ = indexAtPoint(event.mouse.x, event.mouse.y);
                if (old != hoveredIndex_) return EventResult::NeedsRedraw;
            } else {
                if (hoveredIndex_ != SIZE_MAX) {
                    hoveredIndex_ = SIZE_MAX;
                    return EventResult::NeedsRedraw;
                }
            }
            break;
        default:
            break;
    }
    return EventResult::Ignored;
}

Size ListView::measure(const Size& available) {
    (void)available;
    return Size(200, contentHeight());
}

size_t ListView::indexAtPoint(float x, float y) const {
    (void)x;
    float relY = y - bounds_.y + scrollOffset_;
    return static_cast<size_t>(relY / rowHeight_);
}

} // namespace NXRender
