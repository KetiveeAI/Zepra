// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file tabview.cpp
 * @brief TabView widget implementation
 */

#include "widgets/tabview.h"
#include "nxgfx/context.h"

namespace NXRender {

TabView::TabView() {}

size_t TabView::addTab(const std::string& title, std::unique_ptr<Widget> content) {
    Tab tab;
    tab.title = title;
    tab.content = std::move(content);
    tab.closable = false;
    
    tabs_.push_back(std::move(tab));
    return tabs_.size() - 1;
}

void TabView::removeTab(size_t index) {
    if (index >= tabs_.size()) return;
    tabs_.erase(tabs_.begin() + index);
    if (activeTab_ >= tabs_.size() && !tabs_.empty()) {
        activeTab_ = tabs_.size() - 1;
    }
}

void TabView::clearTabs() {
    tabs_.clear();
    activeTab_ = 0;
}

void TabView::setActiveTab(size_t index) {
    if (index < tabs_.size() && index != activeTab_) {
        activeTab_ = index;
        if (onTabChange_) {
            onTabChange_(index);
        }
        invalidate();
    }
}

void TabView::setTabTitle(size_t index, const std::string& title) {
    if (index < tabs_.size()) {
        tabs_[index].title = title;
    }
}

std::string TabView::tabTitle(size_t index) const {
    return index < tabs_.size() ? tabs_[index].title : "";
}

void TabView::setTabClosable(size_t index, bool closable) {
    if (index < tabs_.size()) {
        tabs_[index].closable = closable;
    }
}

bool TabView::isTabClosable(size_t index) const {
    return index < tabs_.size() ? tabs_[index].closable : false;
}

void TabView::render(GpuContext* gpu) {
    if (!gpu) return;
    
    // Tab bar
    Rect bar = tabBarRect();
    gpu->fillRect(bar, tabBarColor_);
    
    if (!tabs_.empty()) {
        float tabW = bar.width / tabs_.size();
        if (tabW > 150) tabW = 150;
        
        for (size_t i = 0; i < tabs_.size(); ++i) {
            float tx = bar.x + i * tabW;
            Rect tr(tx, bar.y, tabW, tabHeight_);
            
            Color bg = tabBarColor_;
            if (i == activeTab_) {
                bg = tabActiveColor_;
            } else if (static_cast<int>(i) == hoveredTab_) {
                bg = tabHoverColor_;
            }
            gpu->fillRect(tr, bg);
            
            // Active indicator
            if (i == activeTab_) {
                Rect ind(tx, bar.y + tabHeight_ - 2, tabW, 2);
                gpu->fillRect(ind, Color(0, 122, 255));
            }
            
            // Title
            gpu->drawText(tabs_[i].title, tx + 12, bar.y + (tabHeight_ - 14) / 2 + 12, tabTextColor_, 14.0f);
        }
    }
    
    // Content area
    Rect content = contentRect();
    gpu->fillRect(content, contentColor_);
    
    if (activeTab_ < tabs_.size() && tabs_[activeTab_].content) {
        tabs_[activeTab_].content->render(gpu);
    }
}

EventResult TabView::handleEvent(const Event& event) {
    switch (event.type) {
        case EventType::MouseMove: {
            int old = hoveredTab_;
            hoveredTab_ = tabAtPoint(event.mouse.x, event.mouse.y);
            if (old != hoveredTab_) return EventResult::NeedsRedraw;
            break;
        }
        case EventType::MouseDown: {
            int clicked = tabAtPoint(event.mouse.x, event.mouse.y);
            if (clicked >= 0 && static_cast<size_t>(clicked) < tabs_.size()) {
                setActiveTab(clicked);
                return EventResult::NeedsRedraw;
            }
            break;
        }
        case EventType::MouseLeave:
            if (hoveredTab_ >= 0) {
                hoveredTab_ = -1;
                return EventResult::NeedsRedraw;
            }
            break;
        default:
            break;
    }
    
    // Forward to active tab content
    if (activeTab_ < tabs_.size() && tabs_[activeTab_].content) {
        return tabs_[activeTab_].content->handleEvent(event);
    }
    return EventResult::Ignored;
}

void TabView::layout() {
    Rect content = contentRect();
    if (activeTab_ < tabs_.size() && tabs_[activeTab_].content) {
        tabs_[activeTab_].content->setBounds(content);
        tabs_[activeTab_].content->layout();
    }
}

Size TabView::measure(const Size& available) {
    (void)available;
    return Size(400, 300);
}

Rect TabView::tabBarRect() const {
    return Rect(bounds_.x, bounds_.y, bounds_.width, tabHeight_);
}

Rect TabView::contentRect() const {
    return Rect(bounds_.x, bounds_.y + tabHeight_, bounds_.width, bounds_.height - tabHeight_);
}

Rect TabView::tabRect(size_t index) const {
    if (tabs_.empty()) return Rect();
    float tabW = bounds_.width / tabs_.size();
    if (tabW > 150) tabW = 150;
    return Rect(bounds_.x + index * tabW, bounds_.y, tabW, tabHeight_);
}

int TabView::tabAtPoint(float x, float y) const {
    Rect bar = tabBarRect();
    if (!bar.contains(x, y) || tabs_.empty()) return -1;
    
    float tabW = bar.width / tabs_.size();
    if (tabW > 150) tabW = 150;
    
    int idx = static_cast<int>((x - bar.x) / tabW);
    return idx >= 0 && static_cast<size_t>(idx) < tabs_.size() ? idx : -1;
}

} // namespace NXRender
