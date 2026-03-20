/**
 * @file browser_ui_components.h
 * @brief Browser-specific UI components using ZepraUI
 * 
 * Tab bar, address bar, navigation buttons, and content area.
 * 
 * Copyright (c) 2025 KetiveeAI
 */

#ifndef BROWSER_UI_COMPONENTS_H
#define BROWSER_UI_COMPONENTS_H

#include "zepra_ui.h"
#include <vector>
#include <string>

namespace ZepraUI {

// ============================================================================
// Tab Bar
// ============================================================================

struct TabInfo {
    std::string title;
    std::string url;
    bool active = false;
    bool loading = false;
};

class TabBar : public Widget {
public:
    TabBar(std::vector<TabInfo>* tabs, int* activeIndex)
        : tabs_(tabs), activeIndex_(activeIndex) {}
    
    TabBar& onNewTab(std::function<void()> cb) { onNewTab_ = cb; return *this; }
    TabBar& onCloseTab(std::function<void(int)> cb) { onCloseTab_ = cb; return *this; }
    TabBar& onSelectTab(std::function<void(int)> cb) { onSelectTab_ = cb; return *this; }
    
    RxNode* build() override {
        RxNode* container = rx_node_create(RX_NODE_HSTACK);
        container->gap = 4;
        container->padding = 8;
        container->background = Color(28, 28, 30).toNx();
        container->height = 40;
        
        if (tabs_) {
            int idx = 0;
            for (const auto& tab : *tabs_) {
                RxNode* tabNode = rx_node_create(RX_NODE_BUTTON);
                tabNode->text = strdup(tab.title.substr(0, 20).c_str());
                
                if (activeIndex_ && idx == *activeIndex_) {
                    tabNode->background = Color(60, 60, 62).toNx();
                } else {
                    tabNode->background = Color(44, 44, 46).toNx();
                }
                tabNode->corner_radius = 8;
                tabNode->padding = 8;
                
                rx_node_add_child(container, tabNode);
                idx++;
            }
        }
        
        // New Tab button
        RxNode* newTab = rx_node_create(RX_NODE_BUTTON);
        newTab->text = strdup("+");
        newTab->background = Color::transparent().toNx();
        newTab->foreground = Color(150, 150, 150).toNx();
        newTab->padding = 8;
        rx_node_add_child(container, newTab);
        
        return container;
    }
    
private:
    std::vector<TabInfo>* tabs_;
    int* activeIndex_;
    std::function<void()> onNewTab_;
    std::function<void(int)> onCloseTab_;
    std::function<void(int)> onSelectTab_;
};

// ============================================================================
// Traffic Light Buttons (macOS-style)
// ============================================================================

class TrafficLights : public Widget {
public:
    TrafficLights& onClose(std::function<void()> cb) { onClose_ = cb; return *this; }
    TrafficLights& onMinimize(std::function<void()> cb) { onMinimize_ = cb; return *this; }
    TrafficLights& onMaximize(std::function<void()> cb) { onMaximize_ = cb; return *this; }
    
    RxNode* build() override {
        RxNode* container = rx_node_create(RX_NODE_HSTACK);
        container->gap = 8;
        container->padding = 12;
        
        // Close (red)
        RxNode* close = rx_node_create(RX_NODE_BUTTON);
        close->text = strdup("");
        close->width = 12;
        close->height = 12;
        close->background = {255, 95, 87, 255};
        close->corner_radius = 6;
        rx_node_add_child(container, close);
        
        // Minimize (yellow)
        RxNode* minimize = rx_node_create(RX_NODE_BUTTON);
        minimize->text = strdup("");
        minimize->width = 12;
        minimize->height = 12;
        minimize->background = {255, 189, 46, 255};
        minimize->corner_radius = 6;
        rx_node_add_child(container, minimize);
        
        // Maximize (green)
        RxNode* maximize = rx_node_create(RX_NODE_BUTTON);
        maximize->text = strdup("");
        maximize->width = 12;
        maximize->height = 12;
        maximize->background = {40, 201, 65, 255};
        maximize->corner_radius = 6;
        rx_node_add_child(container, maximize);
        
        return container;
    }
    
private:
    std::function<void()> onClose_;
    std::function<void()> onMinimize_;
    std::function<void()> onMaximize_;
};

// ============================================================================
// Navigation Bar (Back, Forward, Refresh, Address)
// ============================================================================

class NavigationBar : public Widget {
public:
    NavigationBar(std::string* addressBinding) : address_(addressBinding) {}
    
    NavigationBar& onBack(std::function<void()> cb) { onBack_ = cb; return *this; }
    NavigationBar& onForward(std::function<void()> cb) { onForward_ = cb; return *this; }
    NavigationBar& onRefresh(std::function<void()> cb) { onRefresh_ = cb; return *this; }
    NavigationBar& onNavigate(std::function<void(const std::string&)> cb) { onNavigate_ = cb; return *this; }
    
    RxNode* build() override {
        RxNode* container = rx_node_create(RX_NODE_HSTACK);
        container->gap = 8;
        container->padding = 8;
        container->background = Color(28, 28, 30).toNx();
        container->height = 44;
        
        // Back button
        RxNode* back = rx_node_create(RX_NODE_BUTTON);
        back->text = strdup("←");
        back->width = 32;
        back->height = 32;
        back->background = Color::transparent().toNx();
        back->corner_radius = 6;
        rx_node_add_child(container, back);
        
        // Forward button
        RxNode* forward = rx_node_create(RX_NODE_BUTTON);
        forward->text = strdup("→");
        forward->width = 32;
        forward->height = 32;
        forward->background = Color::transparent().toNx();
        forward->corner_radius = 6;
        rx_node_add_child(container, forward);
        
        // Refresh button
        RxNode* refresh = rx_node_create(RX_NODE_BUTTON);
        refresh->text = strdup("⟳");
        refresh->width = 32;
        refresh->height = 32;
        refresh->background = Color::transparent().toNx();
        refresh->corner_radius = 6;
        rx_node_add_child(container, refresh);
        
        // Address bar
        RxNode* addressBar = rx_node_create(RX_NODE_TEXTFIELD);
        addressBar->text = strdup(address_ ? address_->c_str() : "");
        addressBar->background = Color(44, 44, 46).toNx();
        addressBar->foreground = Color::white().toNx();
        addressBar->corner_radius = 8;
        addressBar->padding = 8;
        addressBar->height = 32;
        rx_node_add_child(container, addressBar);
        
        return container;
    }
    
private:
    std::string* address_;
    std::function<void()> onBack_;
    std::function<void()> onForward_;
    std::function<void()> onRefresh_;
    std::function<void(const std::string&)> onNavigate_;
};

// ============================================================================
// Content Area
// ============================================================================

class ContentArea : public Widget {
public:
    ContentArea& setLoading(bool loading) { loading_ = loading; return *this; }
    ContentArea& setError(const std::string& error) { error_ = error; return *this; }
    
    RxNode* build() override {
        RxNode* container = rx_node_create(RX_NODE_ZSTACK);
        container->background = Color(255, 255, 255).toNx();
        
        if (loading_) {
            RxNode* loadingText = rx_node_create(RX_NODE_TEXT);
            loadingText->text = strdup("Loading...");
            loadingText->foreground = Color(100, 100, 100).toNx();
            rx_node_add_child(container, loadingText);
        } else if (!error_.empty()) {
            RxNode* errorText = rx_node_create(RX_NODE_TEXT);
            errorText->text = strdup(error_.c_str());
            errorText->foreground = Color(255, 59, 48).toNx();
            rx_node_add_child(container, errorText);
        }
        
        return container;
    }
    
private:
    bool loading_ = false;
    std::string error_;
};

// ============================================================================
// Complete Browser UI
// ============================================================================

class BrowserUI {
public:
    BrowserUI(uint32_t width, uint32_t height)
        : window_(width, height, "Zepra Browser") {}
    
    bool init() { return window_.init(); }
    
    void build() {
        auto root = std::make_unique<VStack>(0);
        
        // Tab bar at top
        root->add(std::make_unique<HStack>(0))
            .add(std::make_unique<TrafficLights>())
            .add(std::make_unique<TabBar>(&tabs_, &activeTab_));
        
        // Navigation bar
        root->add(std::make_unique<NavigationBar>(&addressText_));
        
        // Content area (fills remaining space)
        root->add(std::make_unique<ContentArea>());
        
        window_.setContent(std::move(root));
    }
    
    void frame() { window_.frame(); }
    void resize(uint32_t w, uint32_t h) { window_.resize(w, h); }
    void handleMouseMove(float x, float y) { window_.handleMouseMove(x, y); }
    void handleMouseDown(float x, float y) { window_.handleMouseDown(x, y); }
    void handleMouseUp(float x, float y) { window_.handleMouseUp(x, y); }
    
    // Tab management
    void addTab(const std::string& url) {
        tabs_.push_back({url, url, false, true});
        activeTab_ = static_cast<int>(tabs_.size()) - 1;
    }
    
    void closeTab(int index) {
        if (index >= 0 && index < static_cast<int>(tabs_.size())) {
            tabs_.erase(tabs_.begin() + index);
            if (activeTab_ >= static_cast<int>(tabs_.size())) {
                activeTab_ = static_cast<int>(tabs_.size()) - 1;
            }
        }
    }
    
    void setAddress(const std::string& url) { addressText_ = url; }
    const std::string& address() const { return addressText_; }

private:
    Window window_;
    std::vector<TabInfo> tabs_;
    int activeTab_ = -1;
    std::string addressText_;
};

} // namespace ZepraUI

#endif // BROWSER_UI_COMPONENTS_H
