// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "menu.h"
#include "nxgfx/context.h"
#include <algorithm>

namespace NXRender {
namespace Widgets {

MenuWidget::MenuWidget() {
    backgroundColor_ = Color::white();
    // Dynamically calculate bounds or expect parent to lay this out as absolute
}

MenuWidget::~MenuWidget() {}

void MenuWidget::addItem(const MenuItem& item) {
    items_.push_back(item);
    bounds_.width = calculateMenuWidth();
    bounds_.height = calculateMenuHeight();
}

void MenuWidget::addSeparator() {
    MenuItem sep;
    sep.isSeparator = true;
    addItem(sep);
}

float MenuWidget::calculateMenuWidth() {
    float maxW = 150.0f;
    for (const auto& item : items_) {
        if (!item.isSeparator) {
            float textLen = gpu()->measureText(item.text, 13.0f).width;
            float shortLen = item.shortcut.empty() ? 0 : gpu()->measureText(item.shortcut, 12.0f).width;
            maxW = std::max(maxW, textLen + shortLen + 40.0f);
        }
    }
    return maxW;
}

float MenuWidget::calculateMenuHeight() {
    float h = 4.0f; // Padding top
    for (const auto& item : items_) {
        h += item.isSeparator ? 8.0f : itemHeight_;
    }
    return h + 4.0f; // Padding bottom
}

EventResult MenuWidget::handleRoutedEvent(const Input::Event& event) {
    if (!state_.enabled || items_.empty()) return EventResult::Ignored;
    
    if (auto mouseEv = dynamic_cast<const Input::MouseEvent*>(&event)) {
        float ly = mouseEv->y() - bounds_.y;
        float lx = mouseEv->x() - bounds_.x;
        
        // If cliked completely off menu boundaries
        if (mouseEv->type() == Input::EventType::MouseDown && !bounds_.contains(mouseEv->x(), mouseEv->y())) {
            if (onDismiss_) onDismiss_();
            return EventResult::Ignored;
        }

        if (mouseEv->type() == Input::EventType::MouseLeave) {
            hoveredIndex_ = -1;
            return EventResult::NeedsRedraw;
        }

        // Hit testing items
        float cy = 4.0f;
        int hitIndex = -1;
        for (size_t i = 0; i < items_.size(); ++i) {
            float h = items_[i].isSeparator ? 8.0f : itemHeight_;
            if (ly >= cy && ly < cy + h) {
                if (!items_[i].isSeparator && items_[i].enabled) {
                    hitIndex = static_cast<int>(i);
                }
                break;
            }
            cy += h;
        }

        if (mouseEv->type() == Input::EventType::MouseMove) {
            if (hitIndex != hoveredIndex_) {
                hoveredIndex_ = hitIndex;
                return EventResult::NeedsRedraw;
            }
        } else if (mouseEv->type() == Input::EventType::MouseDown && mouseEv->button() == 0) {
            if (hitIndex >= 0 && items_[hitIndex].onTriggered) {
                items_[hitIndex].onTriggered();
                if (onDismiss_) onDismiss_(); // Close menu after action execution
            }
            return EventResult::NeedsRedraw;
        }
    }

    return Widget::handleRoutedEvent(event);
}

void MenuWidget::render(GpuContext* ctx) {
    if (!state_.visible) return;

    // Drop shadow
    ctx->drawShadow(bounds_, Color(0, 0, 0, 80), 12.0f, 0, 4.0f);
    
    // Background and border
    ctx->fillRect(bounds_, backgroundColor_);
    ctx->strokeRect(bounds_, Color(180, 180, 180, 255), 1.0f);

    float cy = bounds_.y + 4.0f;
    
    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        float h = item.isSeparator ? 8.0f : itemHeight_;
        
        Rect itemRect(bounds_.x, cy, bounds_.width, h);
        
        if (item.isSeparator) {
            ctx->fillRect(Rect(bounds_.x + 8.0f, cy + 4.0f, bounds_.width - 16.0f, 1.0f), Color(210, 210, 210, 255));
        } else {
            // Hover background
            if (static_cast<int>(i) == hoveredIndex_) {
                ctx->fillRect(itemRect, Color(20, 100, 250, 255));
            }

            Color textColor = !item.enabled ? Color(150, 150, 150, 255) : 
                              static_cast<int>(i) == hoveredIndex_ ? Color::white() : Color::black();
            
            // Draw Main Label
            ctx->drawText(item.text, bounds_.x + 20.0f, cy + 18.0f, textColor, 13.0f);
            
            // Draw Shortcut aligned right
            if (!item.shortcut.empty()) {
                float shortLen = ctx->measureText(item.shortcut, 12.0f).width;
                Color shortColor = static_cast<int>(i) == hoveredIndex_ ? Color(220, 230, 255, 255) : Color(140, 140, 140, 255);
                ctx->drawText(item.shortcut, bounds_.x + bounds_.width - shortLen - 12.0f, cy + 18.0f, shortColor, 12.0f);
            }
        }
        cy += h;
    }

    Widget::renderChildren(ctx);
}

} // namespace Widgets
} // namespace NXRender
