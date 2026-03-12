// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file tabview.h
 * @brief TabView widget with tab bar and content switching
 */

#pragma once

#include "widget.h"
#include "../nxgfx/color.h"
#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace NXRender {

/**
 * @brief Tab change callback
 */
using TabChangeCallback = std::function<void(size_t index)>;

/**
 * @brief Tab information
 */
struct Tab {
    std::string title;
    std::unique_ptr<Widget> content;
    bool closable = false;
};

/**
 * @brief TabView with tab bar and content area
 */
class TabView : public Widget {
public:
    TabView();
    ~TabView() override = default;
    
    // Tab management
    size_t addTab(const std::string& title, std::unique_ptr<Widget> content);
    void removeTab(size_t index);
    void clearTabs();
    size_t tabCount() const { return tabs_.size(); }
    
    // Selection
    size_t activeTab() const { return activeTab_; }
    void setActiveTab(size_t index);
    
    // Tab properties
    void setTabTitle(size_t index, const std::string& title);
    std::string tabTitle(size_t index) const;
    
    void setTabClosable(size_t index, bool closable);
    bool isTabClosable(size_t index) const;
    
    // Styling
    float tabHeight() const { return tabHeight_; }
    void setTabHeight(float h) { tabHeight_ = h; }
    
    Color tabBarColor() const { return tabBarColor_; }
    void setTabBarColor(const Color& c) { tabBarColor_ = c; }
    
    Color tabActiveColor() const { return tabActiveColor_; }
    void setTabActiveColor(const Color& c) { tabActiveColor_ = c; }
    
    Color tabHoverColor() const { return tabHoverColor_; }
    void setTabHoverColor(const Color& c) { tabHoverColor_ = c; }
    
    Color tabTextColor() const { return tabTextColor_; }
    void setTabTextColor(const Color& c) { tabTextColor_ = c; }
    
    Color contentColor() const { return contentColor_; }
    void setContentColor(const Color& c) { contentColor_ = c; }
    
    // Callbacks
    void onTabChange(TabChangeCallback cb) { onTabChange_ = cb; }
    
    // Widget overrides
    void render(GpuContext* gpu) override;
    EventResult handleEvent(const Event& event) override;
    void layout() override;
    Size measure(const Size& available) override;
    
private:
    std::vector<Tab> tabs_;
    size_t activeTab_ = 0;
    int hoveredTab_ = -1;
    
    float tabHeight_ = 36;
    Color tabBarColor_ = Color(40, 40, 45);
    Color tabActiveColor_ = Color(30, 30, 35);
    Color tabHoverColor_ = Color(55, 55, 60);
    Color tabTextColor_ = Color(200, 200, 205);
    Color contentColor_ = Color(30, 30, 35);
    
    TabChangeCallback onTabChange_;
    
    Rect tabBarRect() const;
    Rect contentRect() const;
    Rect tabRect(size_t index) const;
    int tabAtPoint(float x, float y) const;
};

} // namespace NXRender
