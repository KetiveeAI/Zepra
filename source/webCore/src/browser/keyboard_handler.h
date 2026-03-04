/**
 * keyboard_handler.h - Keyboard Input Handling for ZepraBrowser
 */

#pragma once

#include <string>
#include <functional>
#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace ZepraBrowser {

// =============================================================================
// KEYBOARD HANDLER
// =============================================================================

class KeyboardHandler {
public:
    // Callbacks for keyboard actions
    using VoidCallback = std::function<void()>;
    using StringCallback = std::function<void(const std::string&)>;
    
    KeyboardHandler();
    
    // Set callbacks for various actions
    void onNewTab(VoidCallback cb) { onNewTab_ = cb; }
    void onCloseTab(VoidCallback cb) { onCloseTab_ = cb; }
    void onToggleSidebar(VoidCallback cb) { onToggleSidebar_ = cb; }
    void onToggleConsole(VoidCallback cb) { onToggleConsole_ = cb; }
    void onFocusAddressBar(VoidCallback cb) { onFocusAddressBar_ = cb; }
    void onNavigate(StringCallback cb) { onNavigate_ = cb; }
    void onExit(VoidCallback cb) { onExit_ = cb; }
    
    // Process a key press event
    // Returns true if the key was handled
    bool handleKeyPress(KeySym key, const char* str, bool ctrl, bool shift, bool alt);
    
    // Text input handling
    void setActiveInput(std::string* input) { activeInput_ = input; }
    void clearActiveInput() { activeInput_ = nullptr; }
    bool hasActiveInput() const { return activeInput_ != nullptr; }
    
    // Console state
    bool isConsoleVisible() const { return consoleVisible_; }
    void setConsoleVisible(bool visible) { consoleVisible_ = visible; }
    
private:
    // Callbacks
    VoidCallback onNewTab_;
    VoidCallback onCloseTab_;
    VoidCallback onToggleSidebar_;
    VoidCallback onToggleConsole_;
    VoidCallback onFocusAddressBar_;
    StringCallback onNavigate_;
    VoidCallback onExit_;
    
    // State
    std::string* activeInput_ = nullptr;
    bool consoleVisible_ = false;
};

// =============================================================================
// GLOBAL KEYBOARD SHORTCUTS
// =============================================================================

// Keyboard shortcut definitions
struct KeyShortcut {
    KeySym key;
    bool ctrl;
    bool shift;
    bool alt;
    const char* description;
};

// List of global shortcuts
extern const KeyShortcut SHORTCUTS[];
extern const int SHORTCUT_COUNT;

} // namespace ZepraBrowser
