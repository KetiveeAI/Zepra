// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file keyboard_handler.cpp
 * @brief Implementation of keyboard input handling
 */

#include "input/keyboard_handler.h"
#include <iostream>
#include <algorithm>

namespace ZepraBrowser {

KeyboardHandler::KeyboardHandler() {
    std::cout << "[KeyboardHandler] Initialized" << std::endl;
}

KeyboardHandler::~KeyboardHandler() = default;

void KeyboardHandler::handleKeyPress(NXRender::KeyCode key, const std::string& text, 
                                    bool ctrl, bool shift) {
    std::cout << "[KeyboardHandler] key=" << (int)key << " text='" << text 
              << "' ctrl=" << ctrl << " shift=" << shift << std::endl;
    
    // Handle keyboard shortcuts first
    if (ctrl) {
        if (key == NXRender::KeyCode::C) {
            handleCtrlC();
            return;
        } else if (key == NXRender::KeyCode::V) {
            handleCtrlV();
            return;
        } else if (key == NXRender::KeyCode::T) {
            handleCtrlT();
            return;
        } else if (key == NXRender::KeyCode::W) {
            handleCtrlW();
            return;
        } else if (key == NXRender::KeyCode::L) {
            handleCtrlL();
            return;
        }
    }
    
    // Handle text input for focused elements
    if (!activeInput_ && !focusedBox_) {
        return;  // No focus, ignore input
    }
    
    if (key == NXRender::KeyCode::Enter) {
        handleEnter();
    } else if (key == NXRender::KeyCode::Backspace) {
        handleBackspace();
    } else if (key == NXRender::KeyCode::Escape) {
        handleEscape();
    } else if (!text.empty()) {
        handleTextInput(text);
    }
}

void KeyboardHandler::setAddressFocused(bool focused, std::string* inputBuffer) {
    addressFocused_ = focused;
    if (focused) {
        searchFocused_ = false;
        activeInput_ = inputBuffer;
        focusedBox_ = nullptr;
    } else if (activeInput_ == inputBuffer) {
        activeInput_ = nullptr;
    }
}

void KeyboardHandler::setSearchFocused(bool focused, std::string* inputBuffer) {
    searchFocused_ = focused;
    if (focused) {
        addressFocused_ = false;
        activeInput_ = inputBuffer;
        focusedBox_ = nullptr;
    } else if (activeInput_ == inputBuffer) {
        activeInput_ = nullptr;
    }
}

void KeyboardHandler::setFormInputFocused(bool focused, struct LayoutBox* box) {
    if (focused) {
        addressFocused_ = false;
        searchFocused_ = false;
        focusedBox_ = box;
        activeInput_ = box ? &box->text : nullptr;
    } else {
        focusedBox_ = nullptr;
        if (activeInput_ == (box ? &box->text : nullptr)) {
            activeInput_ = nullptr;
        }
    }
}

void KeyboardHandler::onNavigate(NavigateCallback callback) {
    navigateCallback_ = callback;
}

void KeyboardHandler::onNewTab(NewTabCallback callback) {
    newTabCallback_ = callback;
}

void KeyboardHandler::onCloseTab(CloseTabCallback callback) {
    closeTabCallback_ = callback;
}

void KeyboardHandler::onAddressFocus(std::function<void()> callback) {
    addressFocusCallback_ = callback;
}

// Private methods

void KeyboardHandler::handleTextInput(const std::string& text) {
    if (activeInput_) {
        *activeInput_ += text;
    }
}

void KeyboardHandler::handleBackspace() {
    if (activeInput_ && !activeInput_->empty()) {
        activeInput_->pop_back();
    }
}

void KeyboardHandler::handleEnter() {
    if (activeInput_ && navigateCallback_ && !activeInput_->empty()) {
        std::cout << "[KeyboardHandler] Enter - navigating to: " << *activeInput_ << std::endl;
        navigateCallback_(*activeInput_);
        
        // Unfocus but don't clear (preserve URL in address bar)
        addressFocused_ = false;
        searchFocused_ = false;
    }
}

void KeyboardHandler::handleEscape() {
    if (activeInput_) {
        activeInput_->clear();
    }
    addressFocused_ = false;
    searchFocused_ = false;
    focusedBox_ = nullptr;
    activeInput_ = nullptr;
}

void KeyboardHandler::handleCtrlC() {
    // TODO: Implement clipboard copy
    std::cout << "[KeyboardHandler] Ctrl+C (copy not implemented)" << std::endl;
}

void KeyboardHandler::handleCtrlV() {
    // TODO: Implement clipboard paste
    std::cout << "[KeyboardHandler] Ctrl+V (paste not implemented)" << std::endl;
}

void KeyboardHandler::handleCtrlT() {
    if (newTabCallback_) {
        std::cout << "[KeyboardHandler] Ctrl+T - new tab" << std::endl;
        newTabCallback_();
    }
}

void KeyboardHandler::handleCtrlW() {
    if (closeTabCallback_) {
        std::cout << "[KeyboardHandler] Ctrl+W - close tab" << std::endl;
        closeTabCallback_(-1);  // -1 means current tab
    }
}

void KeyboardHandler::handleCtrlL() {
    if (addressFocusCallback_) {
        std::cout << "[KeyboardHandler] Ctrl+L - focus address bar" << std::endl;
        addressFocusCallback_();
    }
}

} // namespace ZepraBrowser
