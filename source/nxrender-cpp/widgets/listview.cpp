// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/listview.h"
#include "nxgfx/context.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

ListView::ListView(size_t itemCount, float rowHeight)
    : itemCount_(itemCount), rowHeight_(rowHeight) {}

void ListView::setItemCount(size_t count) {
    itemCount_ = count;
    if (selectedIndex_ >= count) {
        selectedIndex_ = SIZE_MAX;
    }
    // Prune multi-select
    for (auto it = selectedIndices_.begin(); it != selectedIndices_.end(); ) {
        if (*it >= count) it = selectedIndices_.erase(it);
        else ++it;
    }
    // Clamp scroll
    clampScrollOffset();
}

void ListView::reloadData() {
    invalidate();
}

void ListView::setSelectedIndex(size_t index) {
    if (index < itemCount_) {
        selectedIndex_ = index;
        if (!multiSelect_) {
            selectedIndices_.clear();
        }
        if (onSelect_) {
            onSelect_(index);
        }
    }
}

void ListView::toggleSelection(size_t index) {
    if (!multiSelect_ || index >= itemCount_) return;

    auto it = selectedIndices_.find(index);
    if (it != selectedIndices_.end()) {
        selectedIndices_.erase(it);
    } else {
        selectedIndices_.insert(index);
    }
    selectedIndex_ = index;
}

void ListView::selectRange(size_t from, size_t to) {
    if (!multiSelect_) return;
    size_t lo = std::min(from, to);
    size_t hi = std::min(std::max(from, to), itemCount_ - 1);
    for (size_t i = lo; i <= hi; i++) {
        selectedIndices_.insert(i);
    }
    selectedIndex_ = to;
}

bool ListView::isSelected(size_t index) const {
    if (index == selectedIndex_) return true;
    if (multiSelect_) {
        return selectedIndices_.count(index) > 0;
    }
    return false;
}

// ==================================================================
// Scrolling
// ==================================================================

void ListView::scrollTo(size_t index) {
    if (index >= itemCount_) return;
    float targetY = index * rowHeight_;
    float viewHeight = bounds_.height;
    if (targetY < scrollOffset_) {
        scrollOffset_ = targetY;
    } else if (targetY + rowHeight_ > scrollOffset_ + viewHeight) {
        scrollOffset_ = targetY + rowHeight_ - viewHeight;
    }
    clampScrollOffset();
}

void ListView::scrollBy(float delta) {
    scrollOffset_ += delta;
    clampScrollOffset();
}

void ListView::clampScrollOffset() {
    if (scrollOffset_ < 0) scrollOffset_ = 0;
    float maxScroll = contentHeight() - bounds_.height;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset_ > maxScroll) scrollOffset_ = maxScroll;
}

void ListView::ensureVisible(size_t index) {
    if (index >= itemCount_) return;
    float itemTop = index * rowHeight_;
    float itemBot = itemTop + rowHeight_;

    if (itemTop < scrollOffset_) {
        scrollOffset_ = itemTop;
    } else if (itemBot > scrollOffset_ + bounds_.height) {
        scrollOffset_ = itemBot - bounds_.height;
    }
    clampScrollOffset();
}

// ==================================================================
// Rendering
// ==================================================================

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

        // Row background
        if (isSelected(i)) {
            gpu->fillRect(rowRect, selectedColor_);
        } else if (i == hoveredIndex_) {
            gpu->fillRect(rowRect, hoverColor_);
        } else if (alternatingRows_ && (i % 2 == 1)) {
            Color altBg(backgroundColor_.r + 5, backgroundColor_.g + 5,
                        backgroundColor_.b + 5, backgroundColor_.a);
            gpu->fillRect(rowRect, altBg);
        }

        // Cell configuration callback
        if (onCellConfig_) {
            // onCellConfig_ would configure a reusable cell widget
            // For now, just draw index text as placeholder
        }

        // Separator
        if (showsSeparators_ && i < itemCount_ - 1) {
            float sepIndent = separatorIndent_;
            Rect sep(bounds_.x + sepIndent, rowY + rowHeight_ - 1,
                     bounds_.width - sepIndent * 2, 1);
            gpu->fillRect(sep, separatorColor_);
        }
    }

    // Scroll indicator
    if (contentHeight() > bounds_.height && showScrollIndicator_) {
        float trackHeight = bounds_.height;
        float thumbFraction = bounds_.height / contentHeight();
        float thumbHeight = std::max(20.0f, trackHeight * thumbFraction);
        float maxScroll = contentHeight() - bounds_.height;
        float thumbY = (maxScroll > 0)
            ? (scrollOffset_ / maxScroll) * (trackHeight - thumbHeight)
            : 0;

        Rect thumb(bounds_.x + bounds_.width - 6, bounds_.y + thumbY, 4, thumbHeight);
        gpu->fillRoundedRect(thumb, Color(128, 128, 128, 120), 2);
    }

    gpu->popClip();
}

// ==================================================================
// Event handling
// ==================================================================

EventResult ListView::handleEvent(const Event& event) {
    switch (event.type) {
        case EventType::MouseDown:
            if (bounds_.contains(event.mouse.x, event.mouse.y)) {
                size_t clicked = indexAtPoint(event.mouse.x, event.mouse.y);
                if (clicked < itemCount_) {
                    if (multiSelect_ && event.mouse.modifiers.ctrl) {
                        toggleSelection(clicked);
                    } else if (multiSelect_ && event.mouse.modifiers.shift
                               && selectedIndex_ != SIZE_MAX) {
                        selectRange(selectedIndex_, clicked);
                    } else {
                        selectedIndices_.clear();
                        setSelectedIndex(clicked);
                    }
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

        case EventType::MouseWheel:
            if (bounds_.contains(event.mouse.x, event.mouse.y)) {
                scrollBy(-event.mouse.wheelDelta * scrollSpeed_);
                return EventResult::NeedsRedraw;
            }
            break;

        case EventType::KeyDown:
            return handleKeyEvent(event);

        default:
            break;
    }
    return EventResult::Ignored;
}

EventResult ListView::handleKeyEvent(const Event& event) {
    if (!isFocused()) return EventResult::Ignored;

    KeyCode key = event.key.key;

    if (key == KeyCode::Up) {
        if (selectedIndex_ == SIZE_MAX || selectedIndex_ == 0) {
            setSelectedIndex(0);
        } else {
            setSelectedIndex(selectedIndex_ - 1);
        }
        ensureVisible(selectedIndex_);
        return EventResult::NeedsRedraw;
    }

    if (key == KeyCode::Down) {
        if (selectedIndex_ == SIZE_MAX) {
            setSelectedIndex(0);
        } else if (selectedIndex_ + 1 < itemCount_) {
            setSelectedIndex(selectedIndex_ + 1);
        }
        ensureVisible(selectedIndex_);
        return EventResult::NeedsRedraw;
    }

    if (key == KeyCode::Home) {
        setSelectedIndex(0);
        ensureVisible(0);
        return EventResult::NeedsRedraw;
    }

    if (key == KeyCode::End && itemCount_ > 0) {
        setSelectedIndex(itemCount_ - 1);
        ensureVisible(itemCount_ - 1);
        return EventResult::NeedsRedraw;
    }

    if (key == KeyCode::PageUp) {
        size_t pageSize = static_cast<size_t>(bounds_.height / rowHeight_);
        if (pageSize == 0) pageSize = 1;
        size_t newIdx = (selectedIndex_ != SIZE_MAX && selectedIndex_ >= pageSize)
            ? selectedIndex_ - pageSize : 0;
        setSelectedIndex(newIdx);
        ensureVisible(newIdx);
        return EventResult::NeedsRedraw;
    }

    if (key == KeyCode::PageDown) {
        size_t pageSize = static_cast<size_t>(bounds_.height / rowHeight_);
        if (pageSize == 0) pageSize = 1;
        size_t newIdx = (selectedIndex_ != SIZE_MAX)
            ? std::min(selectedIndex_ + pageSize, itemCount_ - 1)
            : 0;
        setSelectedIndex(newIdx);
        ensureVisible(newIdx);
        return EventResult::NeedsRedraw;
    }

    if (key == KeyCode::A && event.key.modifiers.ctrl && multiSelect_) {
        // Select all
        for (size_t i = 0; i < itemCount_; i++) selectedIndices_.insert(i);
        return EventResult::NeedsRedraw;
    }

    return EventResult::Ignored;
}

// ==================================================================
// Measurement
// ==================================================================

Size ListView::measure(const Size& available) {
    float width = available.width > 0 ? available.width : 200;
    float height = std::min(contentHeight(), available.height > 0 ? available.height : 400.0f);
    return Size(width, height);
}

size_t ListView::indexAtPoint(float x, float y) const {
    (void)x;
    float relY = y - bounds_.y + scrollOffset_;
    if (relY < 0) return SIZE_MAX;
    size_t idx = static_cast<size_t>(relY / rowHeight_);
    return (idx < itemCount_) ? idx : SIZE_MAX;
}

} // namespace NXRender
