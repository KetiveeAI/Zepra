// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/tooltip.h"
#include "nxgfx/context.h"
#include "nxgfx/text.h"
#include "platform/display_info.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace NXRender {

Tooltip::Tooltip() {
    backgroundColor_ = Color(50, 50, 50, 230);
    state_.visible = false;
}

void Tooltip::showAt(Widget* anchor) {
    if (!anchor || text_.empty()) return;
    anchor_ = anchor;
    visible_ = true;
    showPending_ = false;
    showTimer_ = 0;
    dismissTimer_ = 0;
    opacity_ = 0.0f;
    positionRelativeTo(anchor);
    invalidate();
}

void Tooltip::scheduleShow(Widget* anchor) {
    if (!anchor || text_.empty()) return;
    anchor_ = anchor;
    showPending_ = true;
    showTimer_ = 0;
    visible_ = false;
}

void Tooltip::cancelAndHide() {
    showPending_ = false;
    visible_ = false;
    anchor_ = nullptr;
    showTimer_ = 0;
    dismissTimer_ = 0;
    opacity_ = 0.0f;
    invalidate();
}

void Tooltip::update(float deltaMs) {
    if (showPending_) {
        showTimer_ += deltaMs;
        if (showTimer_ >= static_cast<float>(showDelayMs_)) {
            showAt(anchor_);
        }
        return;
    }

    if (visible_) {
        // Fade in
        if (opacity_ < 1.0f) {
            opacity_ = std::min(1.0f, opacity_ + deltaMs / 150.0f);
        }

        // Auto-dismiss timer
        dismissTimer_ += deltaMs;
        if (dismissTimer_ >= static_cast<float>(dismissDelayMs_)) {
            // Fade out
            opacity_ -= deltaMs / 200.0f;
            if (opacity_ <= 0.0f) {
                cancelAndHide();
            }
        }
    }
}

Tooltip::Placement Tooltip::computeAutoPlacement(Widget* anchor) const {
    if (!anchor) return Placement::Top;

    const auto& di = DisplayInfo::instance().metrics();
    float anchorCenterY = anchor->bounds().y + anchor->bounds().height * 0.5f;
    float viewHeight = static_cast<float>(di.windowHeight);

    // Prefer top, fall back to bottom if near screen top
    if (anchorCenterY < viewHeight * 0.3f) return Placement::Bottom;
    return Placement::Top;
}

void Tooltip::positionRelativeTo(Widget* anchor) {
    if (!anchor) return;

    Size tooltipSize = measure(Size(maxWidth_, 0));
    Placement p = (placement_ == Placement::Auto) ? computeAutoPlacement(anchor) : placement_;

    const Rect& ab = anchor->bounds();
    float x = 0, y = 0;

    switch (p) {
        case Placement::Top:
            x = ab.x + (ab.width - tooltipSize.width) * 0.5f;
            y = ab.y - tooltipSize.height - arrowSize_ - 4;
            break;
        case Placement::Bottom:
            x = ab.x + (ab.width - tooltipSize.width) * 0.5f;
            y = ab.y + ab.height + arrowSize_ + 4;
            break;
        case Placement::Left:
            x = ab.x - tooltipSize.width - arrowSize_ - 4;
            y = ab.y + (ab.height - tooltipSize.height) * 0.5f;
            break;
        case Placement::Right:
            x = ab.x + ab.width + arrowSize_ + 4;
            y = ab.y + (ab.height - tooltipSize.height) * 0.5f;
            break;
        default:
            break;
    }

    // Clamp to viewport
    const auto& di = DisplayInfo::instance().metrics();
    x = std::max(4.0f, std::min(x, static_cast<float>(di.windowWidth) - tooltipSize.width - 4));
    y = std::max(4.0f, std::min(y, static_cast<float>(di.windowHeight) - tooltipSize.height - 4));

    setBounds(Rect(x, y, tooltipSize.width, tooltipSize.height));
}

std::vector<std::string> Tooltip::wrapText(float maxW) const {
    std::vector<std::string> lines;
    if (text_.empty()) return lines;

    // Simple word-wrap
    std::istringstream stream(text_);
    std::string word;
    std::string currentLine;
    float charWidth = fontSize_ * 0.6f; // Approximate

    while (stream >> word) {
        float testWidth = static_cast<float>(currentLine.size() + word.size() + 1) * charWidth;
        if (!currentLine.empty() && testWidth > maxW) {
            lines.push_back(currentLine);
            currentLine = word;
        } else {
            if (!currentLine.empty()) currentLine += " ";
            currentLine += word;
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }

    return lines;
}

Size Tooltip::measure(const Size& available) {
    float maxW = maxWidth_;
    if (available.width > 0 && available.width < maxW) maxW = available.width;

    auto lines = wrapText(maxW - 16); // 8px padding each side
    float lineHeight = fontSize_ + 4;
    float contentHeight = static_cast<float>(lines.size()) * lineHeight;
    float contentWidth = 0;
    float charWidth = fontSize_ * 0.6f;
    for (const auto& line : lines) {
        float w = static_cast<float>(line.size()) * charWidth;
        contentWidth = std::max(contentWidth, w);
    }

    return Size(contentWidth + 16, contentHeight + 12); // padding
}

void Tooltip::render(GpuContext* ctx) {
    if (!ctx || !visible_ || opacity_ <= 0.0f) return;

    const Rect& b = bounds();

    // Background with opacity
    Color bg = backgroundColor_;
    bg.a = static_cast<uint8_t>(static_cast<float>(bg.a) * opacity_);
    ctx->fillRoundedRect(b, bg, 4.0f);

    // Arrow (simple triangle) — only for top/bottom placement
    Placement p = (placement_ == Placement::Auto && anchor_)
                  ? computeAutoPlacement(anchor_) : placement_;

    if (p == Placement::Top && anchor_) {
        float arrowX = b.x + b.width * 0.5f;
        float arrowY = b.y + b.height;
        std::vector<Point> arrow = {
            Point(arrowX - arrowSize_, arrowY),
            Point(arrowX + arrowSize_, arrowY),
            Point(arrowX, arrowY + arrowSize_)
        };
        ctx->fillPath(arrow, bg);
    } else if (p == Placement::Bottom && anchor_) {
        float arrowX = b.x + b.width * 0.5f;
        float arrowY = b.y;
        std::vector<Point> arrow = {
            Point(arrowX - arrowSize_, arrowY),
            Point(arrowX + arrowSize_, arrowY),
            Point(arrowX, arrowY - arrowSize_)
        };
        ctx->fillPath(arrow, bg);
    }

    // Text lines
    auto lines = wrapText(b.width - 16);
    float lineHeight = fontSize_ + 4;
    float textY = b.y + 6;
    Color textCol = textColor_;
    textCol.a = static_cast<uint8_t>(static_cast<float>(textCol.a) * opacity_);

    for (const auto& line : lines) {
        ctx->drawText(line, b.x + 8, textY, textCol, fontSize_);
        textY += lineHeight;
    }
}

} // namespace NXRender
