// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file navbar.cpp
 * @brief Navigation bar implementation
 */

#include "../../source/zepraEngine/include/engine/ui/navbar.h"

namespace zepra {
namespace ui {

struct NavBar::Impl {
    NavBarConfig config;
    bool canBack = false;
    bool canForward = false;
    bool loading = false;
    
    float x = 0, y = 0, width = 200, height = 40;
    int hoveredButton = -1;  // -1 = none, 0 = back, 1 = forward, 2 = refresh, 3 = home
    int pressedButton = -1;
    
    ActionCallback actionCallback;
    
    Impl() : config() {}
    explicit Impl(const NavBarConfig& cfg) : config(cfg) {}
    
    float getButtonX(int index) const {
        return x + index * (config.buttonSize + config.spacing);
    }
    
    int hitTest(float px, float py) const {
        if (py < y || py > y + height) return -1;
        
        int buttonCount = config.showHome ? 4 : 3;
        for (int i = 0; i < buttonCount; i++) {
            float bx = getButtonX(i);
            if (px >= bx && px <= bx + config.buttonSize) {
                return i;
            }
        }
        return -1;
    }
    
    bool isButtonEnabled(int index) const {
        switch (index) {
            case 0: return canBack;
            case 1: return canForward;
            case 2: return true;  // Refresh/Stop always enabled
            case 3: return true;  // Home always enabled
            default: return false;
        }
    }
};

NavBar::NavBar() : impl_(std::make_unique<Impl>()) {}

NavBar::NavBar(const NavBarConfig& config) 
    : impl_(std::make_unique<Impl>(config)) {}

NavBar::~NavBar() = default;

void NavBar::setConfig(const NavBarConfig& config) {
    impl_->config = config;
}

NavBarConfig NavBar::getConfig() const {
    return impl_->config;
}

void NavBar::setCanGoBack(bool canGoBack) {
    impl_->canBack = canGoBack;
}

void NavBar::setCanGoForward(bool canGoForward) {
    impl_->canForward = canGoForward;
}

void NavBar::setIsLoading(bool isLoading) {
    impl_->loading = isLoading;
}

bool NavBar::canGoBack() const {
    return impl_->canBack;
}

bool NavBar::canGoForward() const {
    return impl_->canForward;
}

bool NavBar::isLoading() const {
    return impl_->loading;
}

void NavBar::setActionCallback(ActionCallback callback) {
    impl_->actionCallback = std::move(callback);
}

void NavBar::setBounds(float x, float y, float width, float height) {
    impl_->x = x;
    impl_->y = y;
    impl_->width = width;
    impl_->height = height;
}

void NavBar::render() {
    // Note: Actual rendering is done by the browser's OpenGL code
    // This is a logical component that provides state and callbacks
}

bool NavBar::handleMouseClick(float x, float y) {
    int button = impl_->hitTest(x, y);
    if (button < 0) return false;
    
    if (!impl_->isButtonEnabled(button)) return false;
    
    if (impl_->actionCallback) {
        NavAction action;
        switch (button) {
            case 0: action = NavAction::Back; break;
            case 1: action = NavAction::Forward; break;
            case 2: action = impl_->loading ? NavAction::Stop : NavAction::Refresh; break;
            case 3: action = NavAction::Home; break;
            default: return false;
        }
        impl_->actionCallback(action);
    }
    
    return true;
}

bool NavBar::handleMouseMove(float x, float y) {
    int newHovered = impl_->hitTest(x, y);
    if (newHovered != impl_->hoveredButton) {
        impl_->hoveredButton = newHovered;
        return true;  // Needs redraw for hover effect
    }
    return false;
}

bool NavBar::handleMouseDown(float x, float y) {
    int button = impl_->hitTest(x, y);
    if (button >= 0 && impl_->isButtonEnabled(button)) {
        impl_->pressedButton = button;
        return true;
    }
    return false;
}

bool NavBar::handleMouseUp(float x, float y) {
    impl_->pressedButton = -1;
    return handleMouseClick(x, y);
}

} // namespace ui
} // namespace zepra
