/**
 * @file browser_components.h
 * @brief Unified browser UI component imports
 * 
 * This header provides easy access to all ZepraBrowser UI components.
 * All components are pure C++ with no SDL dependencies.
 */

#ifndef ZEPRA_BROWSER_COMPONENTS_H
#define ZEPRA_BROWSER_COMPONENTS_H

// UI Components
#include "ui/searchbox.h"
#include "ui/navbar.h"
#include "ui/dropdown.h"
#include "ui/sidebar.h"
#include "ui/context_menu.h"
#include "ui/ai_chat_window.h"
#include "ui/zepra_webview.h"

// Input System
#include "input/keyboard_handler.h"
#include "input/shortcut_manager.h"
#include "input/voice_input.h"

namespace zepra {

/**
 * BrowserUIManager - Coordinates all UI components
 * 
 * This class manages the lifecycle and interaction
 * between browser UI components.
 */
class BrowserUIManager {
public:
    BrowserUIManager() {
        // Initialize shortcut manager with defaults
        shortcuts_.registerBrowserDefaults();
    }
    
    // Access components
    ui::SearchBox& searchBox() { return searchBox_; }
    ui::NavBar& navBar() { return navBar_; }
    ui::Sidebar& sidebar() { return sidebar_; }
    ui::ContextMenu& contextMenu() { return contextMenu_; }
    ui::AIChatWindow& aiChat() { return aiChat_; }
    ui::Dropdown& dropdown() { return dropdown_; }
    
    input::KeyboardHandler& keyboard() { return keyboard_; }
    input::ShortcutManager& shortcuts() { return shortcuts_; }
    input::VoiceInput& voice() { return voice_; }
    
    // Event routing
    bool handleKeyEvent(const input::KeyEvent& event) {
        // First check shortcuts
        if (shortcuts_.handleKeyEvent(event)) {
            return true;
        }
        
        // Then route to focused widget
        if (searchBox_.isFocused()) {
            return searchBox_.handleKeyPress(event.keyCode, 
                event.modifiers.ctrl, event.modifiers.shift, event.modifiers.alt);
        }
        
        if (contextMenu_.isVisible()) {
            return contextMenu_.handleKeyPress(event.keyCode);
        }
        
        if (dropdown_.isVisible()) {
            return dropdown_.handleKeyPress(event.keyCode);
        }
        
        return false;
    }
    
    bool handleMouseClick(float x, float y) {
        // Check overlays first
        if (contextMenu_.isVisible()) {
            return contextMenu_.handleMouseClick(x, y);
        }
        
        if (dropdown_.isVisible()) {
            return dropdown_.handleMouseClick(x, y);
        }
        
        if (aiChat_.isVisible()) {
            if (aiChat_.handleMouseClick(x, y)) return true;
        }
        
        if (sidebar_.isVisible()) {
            if (sidebar_.handleMouseClick(x, y)) return true;
        }
        
        // Check main UI components
        // (searchbox, navbar handled by main browser render loop)
        return false;
    }
    
    void update(float deltaTime) {
        sidebar_.update(deltaTime);
        aiChat_.update(deltaTime);
        voice_.update();
    }
    
private:
    ui::SearchBox searchBox_;
    ui::NavBar navBar_;
    ui::Sidebar sidebar_;
    ui::ContextMenu contextMenu_;
    ui::AIChatWindow aiChat_;
    ui::Dropdown dropdown_;
    
    input::KeyboardHandler keyboard_;
    input::ShortcutManager shortcuts_;
    input::VoiceInput voice_;
};

} // namespace zepra

#endif // ZEPRA_BROWSER_COMPONENTS_H
