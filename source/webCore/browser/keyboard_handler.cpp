// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * keyboard_handler.cpp - Keyboard Input Implementation
 */

#include "keyboard_handler.h"
#include <cstring>
#include <iostream>

namespace ZepraBrowser {

// Global shortcuts
const KeyShortcut SHORTCUTS[] = {
    { XK_t, true, false, false, "New Tab" },
    { XK_w, true, false, false, "Close Tab" },
    { XK_l, true, false, false, "Focus Address Bar" },
    { XK_b, true, false, false, "Toggle Sidebar" },
    { XK_i, true, false, false, "Toggle Console" },
    { XK_F2, false, false, false, "Toggle Sidebar" },
    { XK_F12, false, false, false, "Toggle Console" },
    { XK_Escape, false, false, false, "Close/Exit" },
};
const int SHORTCUT_COUNT = sizeof(SHORTCUTS) / sizeof(SHORTCUTS[0]);

KeyboardHandler::KeyboardHandler() {}

bool KeyboardHandler::handleKeyPress(KeySym key, const char* str, 
                                     bool ctrl, bool shift, bool alt) {
    // Handle text input first
    if (activeInput_) {
        if (key == XK_Return) {
            // Submit input
            if (onNavigate_ && !activeInput_->empty()) {
                onNavigate_(*activeInput_);
            }
            activeInput_->clear();
            activeInput_ = nullptr;
            return true;
        }
        else if (key == XK_BackSpace) {
            if (!activeInput_->empty()) {
                activeInput_->pop_back();
            }
            return true;
        }
        else if (key == XK_Escape) {
            activeInput_->clear();
            activeInput_ = nullptr;
            return true;
        }
        else if (ctrl && key == XK_a) {
            // Select all - clear for now
            std::cout << "[Keyboard] Ctrl+A: Select all (not yet implemented)" << std::endl;
            return true;
        }
        else if (str && strlen(str) == 1 && str[0] >= 32) {
            *activeInput_ += str[0];
            return true;
        }
        return false;
    }
    
    // Global shortcuts (when no input focused)
    if (ctrl && key == XK_t) {
        if (onNewTab_) onNewTab_();
        return true;
    }
    else if (ctrl && key == XK_w) {
        if (onCloseTab_) onCloseTab_();
        return true;
    }
    else if (ctrl && key == XK_l) {
        if (onFocusAddressBar_) onFocusAddressBar_();
        return true;
    }
    else if (ctrl && key == XK_b) {
        if (onToggleSidebar_) onToggleSidebar_();
        return true;
    }
    else if (key == XK_F2) {
        if (onToggleSidebar_) onToggleSidebar_();
        return true;
    }
    else if (key == XK_F12) {
        consoleVisible_ = !consoleVisible_;
        if (onToggleConsole_) onToggleConsole_();
        std::cout << "[Browser] Console " << (consoleVisible_ ? "opened" : "closed") << std::endl;
        return true;
    }
    else if (ctrl && key == XK_i) {
        consoleVisible_ = !consoleVisible_;
        if (onToggleConsole_) onToggleConsole_();
        std::cout << "[Browser] Console " << (consoleVisible_ ? "opened" : "closed") << std::endl;
        return true;
    }
    else if (key == XK_Escape) {
        if (consoleVisible_) {
            consoleVisible_ = false;
            if (onToggleConsole_) onToggleConsole_();
        } else {
            if (onExit_) onExit_();
        }
        return true;
    }
    
    return false;
}

} // namespace ZepraBrowser
