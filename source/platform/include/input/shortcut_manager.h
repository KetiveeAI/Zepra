/**
 * @file shortcut_manager.h
 * @brief Keyboard shortcut management system
 */

#ifndef ZEPRA_INPUT_SHORTCUT_MANAGER_H
#define ZEPRA_INPUT_SHORTCUT_MANAGER_H

#include "keyboard_handler.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zepra {
namespace input {

/**
 * Single keyboard shortcut
 */
struct Shortcut {
    std::string id;             // Unique identifier, e.g., "new_tab"
    std::string label;          // Display name, e.g., "New Tab"
    int keyCode;                // Primary key
    Modifiers modifiers;        // Required modifiers
    std::string category;       // e.g., "tabs", "navigation", "editing"
    bool enabled = true;
    bool global = false;        // Works even when input focused
};

/**
 * Shortcut action callback
 */
using ShortcutCallback = std::function<void(const std::string& shortcutId)>;

/**
 * Standard browser shortcuts
 */
namespace BrowserShortcuts {
    // Tab management
    constexpr const char* NewTab = "new_tab";
    constexpr const char* CloseTab = "close_tab";
    constexpr const char* NextTab = "next_tab";
    constexpr const char* PrevTab = "prev_tab";
    constexpr const char* ReopenTab = "reopen_tab";
    
    // Navigation
    constexpr const char* Back = "back";
    constexpr const char* Forward = "forward";
    constexpr const char* Refresh = "refresh";
    constexpr const char* HardRefresh = "hard_refresh";
    constexpr const char* Home = "home";
    constexpr const char* FocusAddress = "focus_address";
    
    // Zoom
    constexpr const char* ZoomIn = "zoom_in";
    constexpr const char* ZoomOut = "zoom_out";
    constexpr const char* ZoomReset = "zoom_reset";
    
    // View
    constexpr const char* Fullscreen = "fullscreen";
    constexpr const char* DevTools = "devtools";
    constexpr const char* InspectElement = "inspect_element";
    constexpr const char* ViewSource = "view_source";
    
    // Page actions
    constexpr const char* Bookmark = "bookmark";
    constexpr const char* Find = "find";
    constexpr const char* Print = "print";
    constexpr const char* Save = "save";
    
    // Panels
    constexpr const char* History = "history";
    constexpr const char* Downloads = "downloads";
    constexpr const char* Settings = "settings";
    constexpr const char* ToggleSidebar = "toggle_sidebar";
    
    // Editing
    constexpr const char* Copy = "copy";
    constexpr const char* Cut = "cut";
    constexpr const char* Paste = "paste";
    constexpr const char* Undo = "undo";
    constexpr const char* Redo = "redo";
    constexpr const char* SelectAll = "select_all";
}

/**
 * ShortcutManager - Keyboard shortcut system
 * 
 * Features:
 * - Register custom shortcuts
 * - Default browser shortcuts
 * - Conflict detection
 * - Customizable shortcuts
 * - Category organization
 */
class ShortcutManager {
public:
    ShortcutManager();
    ~ShortcutManager();

    // Registration
    void registerShortcut(const Shortcut& shortcut);
    void unregisterShortcut(const std::string& id);
    void clearAll();

    // Defaults
    void registerBrowserDefaults();

    // Lookup
    Shortcut* findShortcut(const std::string& id);
    const Shortcut* findShortcut(const std::string& id) const;
    std::vector<Shortcut> getShortcuts() const;
    std::vector<Shortcut> getShortcutsByCategory(const std::string& category) const;

    // Customization
    bool setShortcutKey(const std::string& id, int keyCode, const Modifiers& modifiers);
    bool resetToDefault(const std::string& id);
    void resetAllToDefaults();

    // Enable/disable
    void setEnabled(const std::string& id, bool enabled);
    bool isEnabled(const std::string& id) const;

    // Conflict detection
    std::string findConflict(int keyCode, const Modifiers& modifiers) const;

    // Event handling (returns true if handled)
    bool handleKeyEvent(const KeyEvent& event);

    // Callback
    void setCallback(ShortcutCallback callback);

    // Serialization
    std::string toJson() const;
    bool fromJson(const std::string& json);

    // Display helpers
    static std::string formatShortcut(const Shortcut& shortcut);
    static std::string formatKey(int keyCode, const Modifiers& modifiers);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Helper to create shortcuts
Shortcut createShortcut(const std::string& id, const std::string& label,
                        int keyCode, bool ctrl = false, bool shift = false,
                        bool alt = false, const std::string& category = "");

} // namespace input
} // namespace zepra

#endif // ZEPRA_INPUT_SHORTCUT_MANAGER_H
