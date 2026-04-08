// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/splitter.h"
#include "nxgfx/context.h"
#include <algorithm>

namespace NXRender {

Splitter::Splitter() {
    backgroundColor_ = Color::transparent();
}

void Splitter::setSplitRatio(float ratio) {
    splitRatio_ = clampRatio(ratio);
    layout();
    invalidate();
}

void Splitter::setFirst(std::unique_ptr<Widget> widget) {
    first_ = widget.get();
    if (!children_.empty() && children_.size() >= 1) {
        children_[0] = std::move(widget);
    } else {
        addChild(std::move(widget));
    }
    layout();
}

void Splitter::setSecond(std::unique_ptr<Widget> widget) {
    second_ = widget.get();
    if (children_.size() >= 2) {
        children_[1] = std::move(widget);
    } else {
        addChild(std::move(widget));
    }
    layout();
}

float Splitter::clampRatio(float ratio) const {
    const Rect& b = bounds();
    float totalSize = (orientation_ == Orientation::Horizontal) ? b.width : b.height;
    float usable = totalSize - handleWidth_;
    if (usable <= 0) return 0.5f;

    float firstSize = ratio * usable;
    float secondSize = usable - firstSize;

    if (firstSize < minFirstPx_) {
        firstSize = minFirstPx_;
        ratio = firstSize / usable;
    }
    if (secondSize < minSecondPx_) {
        secondSize = minSecondPx_;
        ratio = (usable - secondSize) / usable;
    }

    return std::clamp(ratio, 0.0f, 1.0f);
}

Rect Splitter::handleRect() const {
    const Rect& b = bounds();
    float usable = (orientation_ == Orientation::Horizontal)
                   ? b.width - handleWidth_
                   : b.height - handleWidth_;
    float handlePos = usable * splitRatio_;

    if (orientation_ == Orientation::Horizontal) {
        return Rect(b.x + handlePos, b.y, handleWidth_, b.height);
    } else {
        return Rect(b.x, b.y + handlePos, b.width, handleWidth_);
    }
}

bool Splitter::isOnHandle(float x, float y) const {
    Rect hr = handleRect();
    // Expand hit area slightly for usability
    float expand = 2.0f;
    Rect expanded(hr.x - expand, hr.y - expand,
                  hr.width + expand * 2, hr.height + expand * 2);
    return expanded.contains(x, y);
}

Size Splitter::measure(const Size& available) {
    return Size(available.width > 0 ? available.width : 400.0f,
                available.height > 0 ? available.height : 300.0f);
}

void Splitter::layout() {
    const Rect& b = bounds();
    if (b.width <= 0 || b.height <= 0) return;

    splitRatio_ = clampRatio(splitRatio_);
    float usable, firstSize;

    if (orientation_ == Orientation::Horizontal) {
        usable = b.width - handleWidth_;
        firstSize = usable * splitRatio_;

        if (first_) {
            first_->setBounds(Rect(b.x, b.y, firstSize, b.height));
            first_->layout();
        }
        if (second_) {
            float secondX = b.x + firstSize + handleWidth_;
            float secondW = usable - firstSize;
            second_->setBounds(Rect(secondX, b.y, secondW, b.height));
            second_->layout();
        }
    } else {
        usable = b.height - handleWidth_;
        firstSize = usable * splitRatio_;

        if (first_) {
            first_->setBounds(Rect(b.x, b.y, b.width, firstSize));
            first_->layout();
        }
        if (second_) {
            float secondY = b.y + firstSize + handleWidth_;
            float secondH = usable - firstSize;
            second_->setBounds(Rect(b.x, secondY, b.width, secondH));
            second_->layout();
        }
    }
}

EventResult Splitter::onMouseDown(float x, float y, MouseButton button) {
    if (button == MouseButton::Left && isOnHandle(x, y)) {
        dragging_ = true;
        dragStart_ = (orientation_ == Orientation::Horizontal) ? x : y;
        ratioStart_ = splitRatio_;
        return EventResult::Handled;
    }
    return EventResult::Ignored;
}

EventResult Splitter::onMouseMove(float x, float y) {
    if (dragging_) {
        const Rect& b = bounds();
        float usable = (orientation_ == Orientation::Horizontal)
                       ? b.width - handleWidth_
                       : b.height - handleWidth_;
        if (usable <= 0) return EventResult::Handled;

        float current = (orientation_ == Orientation::Horizontal) ? x : y;
        float delta = current - dragStart_;
        float ratioChange = delta / usable;
        splitRatio_ = clampRatio(ratioStart_ + ratioChange);
        layout();
        return EventResult::NeedsRedraw;
    }

    // Update hover state
    bool wasHovered = handleHovered_;
    handleHovered_ = isOnHandle(x, y);
    if (wasHovered != handleHovered_) return EventResult::NeedsRedraw;

    return EventResult::Ignored;
}

EventResult Splitter::onMouseUp(float x, float y, MouseButton button) {
    if (dragging_) {
        dragging_ = false;
        return EventResult::Handled;
    }
    return EventResult::Ignored;
}

void Splitter::render(GpuContext* ctx) {
    if (!ctx || !isVisible()) return;

    // Render first panel
    if (first_) first_->render(ctx);

    // Render handle
    Rect hr = handleRect();
    Color hc = handleHovered_ || dragging_ ? handleHoverColor_ : handleColor_;
    ctx->fillRect(hr, hc);

    // Draw grip dots on handle
    float gripSize = 2.0f;
    int dotCount = 3;
    if (orientation_ == Orientation::Horizontal) {
        float startY = hr.y + (hr.height - dotCount * gripSize * 3) * 0.5f;
        for (int i = 0; i < dotCount; i++) {
            float dy = startY + static_cast<float>(i) * gripSize * 3;
            ctx->fillCircle(hr.x + hr.width * 0.5f, dy, gripSize * 0.5f, Color(0x999999));
        }
    } else {
        float startX = hr.x + (hr.width - dotCount * gripSize * 3) * 0.5f;
        for (int i = 0; i < dotCount; i++) {
            float dx = startX + static_cast<float>(i) * gripSize * 3;
            ctx->fillCircle(dx, hr.y + hr.height * 0.5f, gripSize * 0.5f, Color(0x999999));
        }
    }

    // Render second panel
    if (second_) second_->render(ctx);
}

} // namespace NXRender
