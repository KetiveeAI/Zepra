// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file shortcut_manager.cpp
 * @brief Keyboard shortcut management (pure C++, no SDL)
 */

#include "../../source/platform/include/input/shortcut_manager.h"
#include <algorithm>
#include <sstream>

namespace zepra {
namespace input {

struct ShortcutManager::Impl {
    std::vector<Shortcut> shortcuts;
    std::vector<Shortcut> defaultShortcuts;
    ShortcutCallback callback;
    
    const Shortcut* findByKey(int keyCode, const Modifiers& mods) const {
        for (const auto& s : shortcuts) {
            if (s.enabled && s.keyCode == keyCode &&
                s.modifiers.ctrl == mods.ctrl &&
                s.modifiers.shift == mods.shift &&
                s.modifiers.alt == mods.alt) {
                return &s;
            }
        }
        return nullptr;
    }
};

ShortcutManager::ShortcutManager() : impl_(std::make_unique<Impl>()) {}

ShortcutManager::~ShortcutManager() = default;

void ShortcutManager::registerShortcut(const Shortcut& shortcut) {
    // Remove existing with same ID
    unregisterShortcut(shortcut.id);
    impl_->shortcuts.push_back(shortcut);
}

void ShortcutManager::unregisterShortcut(const std::string& id) {
    auto& shortcuts = impl_->shortcuts;
    shortcuts.erase(
        std::remove_if(shortcuts.begin(), shortcuts.end(),
            [&id](const Shortcut& s) { return s.id == id; }),
        shortcuts.end()
    );
}

void ShortcutManager::clearAll() {
    impl_->shortcuts.clear();
}

void ShortcutManager::registerBrowserDefaults() {
    using namespace BrowserShortcuts;
    
    // Tab management
    registerShortcut(createShortcut(NewTab, "New Tab", KeyCode::T, true, false, false, "tabs"));
    registerShortcut(createShortcut(CloseTab, "Close Tab", KeyCode::W, true, false, false, "tabs"));
    registerShortcut(createShortcut(NextTab, "Next Tab", KeyCode::Tab, true, false, false, "tabs"));
    registerShortcut(createShortcut(PrevTab, "Previous Tab", KeyCode::Tab, true, true, false, "tabs"));
    registerShortcut(createShortcut(ReopenTab, "Reopen Closed Tab", KeyCode::T, true, true, false, "tabs"));
    
    // Navigation
    registerShortcut(createShortcut(Back, "Back", KeyCode::Left, false, false, true, "navigation"));
    registerShortcut(createShortcut(Forward, "Forward", KeyCode::Right, false, false, true, "navigation"));
    registerShortcut(createShortcut(Refresh, "Refresh", KeyCode::R, true, false, false, "navigation"));
    registerShortcut(createShortcut(HardRefresh, "Hard Refresh", KeyCode::R, true, true, false, "navigation"));
    registerShortcut(createShortcut(FocusAddress, "Focus Address Bar", KeyCode::L, true, false, false, "navigation"));
    
    // F-key refresh
    Shortcut f5Refresh = createShortcut("refresh_f5", "Refresh", KeyCode::F5, false, false, false, "navigation");
    registerShortcut(f5Refresh);
    
    // Zoom
    registerShortcut(createShortcut(ZoomIn, "Zoom In", KeyCode::Plus, true, false, false, "zoom"));
    registerShortcut(createShortcut(ZoomOut, "Zoom Out", KeyCode::Minus, true, false, false, "zoom"));
    registerShortcut(createShortcut(ZoomReset, "Reset Zoom", KeyCode::Num0, true, false, false, "zoom"));
    
    // View
    registerShortcut(createShortcut(Fullscreen, "Fullscreen", KeyCode::F11, false, false, false, "view"));
    registerShortcut(createShortcut(DevTools, "Developer Tools", KeyCode::F12, false, false, false, "view"));
    registerShortcut(createShortcut(InspectElement, "Inspect Element", KeyCode::I, true, true, false, "view"));
    registerShortcut(createShortcut(ViewSource, "View Source", KeyCode::U, true, false, false, "view"));
    
    // Page actions
    registerShortcut(createShortcut(Bookmark, "Add Bookmark", KeyCode::D, true, false, false, "page"));
    registerShortcut(createShortcut(Find, "Find", KeyCode::F, true, false, false, "page"));
    registerShortcut(createShortcut(Print, "Print", KeyCode::P, true, false, false, "page"));
    registerShortcut(createShortcut(Save, "Save Page", KeyCode::S, true, false, false, "page"));
    
    // Panels
    registerShortcut(createShortcut(History, "History", KeyCode::H, true, false, false, "panels"));
    registerShortcut(createShortcut(Downloads, "Downloads", KeyCode::J, true, false, false, "panels"));
    registerShortcut(createShortcut(ToggleSidebar, "Toggle Sidebar", KeyCode::B, true, false, false, "panels"));
    
    // Editing (global shortcuts)
    Shortcut copy = createShortcut(Copy, "Copy", KeyCode::C, true, false, false, "editing");
    copy.global = true;
    registerShortcut(copy);
    
    Shortcut cut = createShortcut(Cut, "Cut", KeyCode::X, true, false, false, "editing");
    cut.global = true;
    registerShortcut(cut);
    
    Shortcut paste = createShortcut(Paste, "Paste", KeyCode::V, true, false, false, "editing");
    paste.global = true;
    registerShortcut(paste);
    
    Shortcut undo = createShortcut(Undo, "Undo", KeyCode::Z, true, false, false, "editing");
    undo.global = true;
    registerShortcut(undo);
    
    Shortcut redo = createShortcut(Redo, "Redo", KeyCode::Z, true, true, false, "editing");
    redo.global = true;
    registerShortcut(redo);
    
    Shortcut selectAll = createShortcut(SelectAll, "Select All", KeyCode::A, true, false, false, "editing");
    selectAll.global = true;
    registerShortcut(selectAll);
    
    // Store as defaults
    impl_->defaultShortcuts = impl_->shortcuts;
}

Shortcut* ShortcutManager::findShortcut(const std::string& id) {
    for (auto& s : impl_->shortcuts) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

const Shortcut* ShortcutManager::findShortcut(const std::string& id) const {
    for (const auto& s : impl_->shortcuts) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

std::vector<Shortcut> ShortcutManager::getShortcuts() const {
    return impl_->shortcuts;
}

std::vector<Shortcut> ShortcutManager::getShortcutsByCategory(const std::string& category) const {
    std::vector<Shortcut> result;
    for (const auto& s : impl_->shortcuts) {
        if (s.category == category) {
            result.push_back(s);
        }
    }
    return result;
}

bool ShortcutManager::setShortcutKey(const std::string& id, int keyCode, const Modifiers& modifiers) {
    // Check for conflicts
    std::string conflict = findConflict(keyCode, modifiers);
    if (!conflict.empty() && conflict != id) {
        return false;
    }
    
    Shortcut* shortcut = findShortcut(id);
    if (shortcut) {
        shortcut->keyCode = keyCode;
        shortcut->modifiers = modifiers;
        return true;
    }
    return false;
}

bool ShortcutManager::resetToDefault(const std::string& id) {
    for (const auto& def : impl_->defaultShortcuts) {
        if (def.id == id) {
            Shortcut* current = findShortcut(id);
            if (current) {
                current->keyCode = def.keyCode;
                current->modifiers = def.modifiers;
                return true;
            }
        }
    }
    return false;
}

void ShortcutManager::resetAllToDefaults() {
    impl_->shortcuts = impl_->defaultShortcuts;
}

void ShortcutManager::setEnabled(const std::string& id, bool enabled) {
    Shortcut* shortcut = findShortcut(id);
    if (shortcut) {
        shortcut->enabled = enabled;
    }
}

bool ShortcutManager::isEnabled(const std::string& id) const {
    const Shortcut* shortcut = findShortcut(id);
    return shortcut ? shortcut->enabled : false;
}

std::string ShortcutManager::findConflict(int keyCode, const Modifiers& modifiers) const {
    const Shortcut* existing = impl_->findByKey(keyCode, modifiers);
    return existing ? existing->id : "";
}

bool ShortcutManager::handleKeyEvent(const KeyEvent& event) {
    // Only handle key down events
    if (!event.pressed || event.repeat) return false;
    
    const Shortcut* shortcut = impl_->findByKey(event.keyCode, event.modifiers);
    if (shortcut && shortcut->enabled) {
        if (impl_->callback) {
            impl_->callback(shortcut->id);
        }
        return true;
    }
    return false;
}

void ShortcutManager::setCallback(ShortcutCallback callback) {
    impl_->callback = std::move(callback);
}

std::string ShortcutManager::toJson() const {
    std::ostringstream oss;
    oss << "{\n  \"shortcuts\": [\n";
    
    bool first = true;
    for (const auto& s : impl_->shortcuts) {
        if (!first) oss << ",\n";
        first = false;
        
        oss << "    {\n";
        oss << "      \"id\": \"" << s.id << "\",\n";
        oss << "      \"keyCode\": " << s.keyCode << ",\n";
        oss << "      \"ctrl\": " << (s.modifiers.ctrl ? "true" : "false") << ",\n";
        oss << "      \"shift\": " << (s.modifiers.shift ? "true" : "false") << ",\n";
        oss << "      \"alt\": " << (s.modifiers.alt ? "true" : "false") << ",\n";
        oss << "      \"enabled\": " << (s.enabled ? "true" : "false") << "\n";
        oss << "    }";
    }
    
    oss << "\n  ]\n}";
    return oss.str();
}

bool ShortcutManager::fromJson(const std::string& json) {
    // Basic JSON parsing - in production use a proper JSON library
    // For now, this is a placeholder
    return false;
}

std::string ShortcutManager::formatShortcut(const Shortcut& shortcut) {
    return formatKey(shortcut.keyCode, shortcut.modifiers);
}

std::string ShortcutManager::formatKey(int keyCode, const Modifiers& modifiers) {
    std::string result;
    
    if (modifiers.ctrl) result += "Ctrl+";
    if (modifiers.alt) result += "Alt+";
    if (modifiers.shift) result += "Shift+";
    if (modifiers.super) result += "Super+";
    
    result += keyCodeToString(keyCode);
    
    return result;
}

Shortcut createShortcut(const std::string& id, const std::string& label,
                        int keyCode, bool ctrl, bool shift,
                        bool alt, const std::string& category) {
    Shortcut s;
    s.id = id;
    s.label = label;
    s.keyCode = keyCode;
    s.modifiers.ctrl = ctrl;
    s.modifiers.shift = shift;
    s.modifiers.alt = alt;
    s.category = category;
    s.enabled = true;
    s.global = false;
    return s;
}

} // namespace input
} // namespace zepra
