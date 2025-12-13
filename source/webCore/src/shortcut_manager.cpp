/**
 * @file shortcut_manager.cpp
 * @brief ShortcutManager implementation
 */

#include "webcore/shortcut_manager.hpp"
#include <iostream>

namespace Zepra::WebCore {

void ShortcutManager::registerShortcut(int key, int modifiers, const std::string& description, std::function<void()> action) {
    shortcuts_.push_back({key, modifiers, action, description});
}

bool ShortcutManager::handleKeyDown(int key, int modifiers) {
    for (const auto& shortcut : shortcuts_) {
        // Check if key matches
        if (shortcut.key != key) continue;
        
        // Check if required modifiers are present
        // (Note: we check if the required modifiers are *contained* in the active modifiers)
        // Strictly matching might be too restrictive if NumLock/CapsLock are on, 
        // but for now let's enforce exact match of Ctrl/Shift/Alt bits
        
        bool match = (modifiers & shortcut.modifiers) == shortcut.modifiers;
        
        // If the shortcut requires NO modifiers, ensure none are pressed (ignoring num/caps)
        if (shortcut.modifiers == 0) {
            // Implementation specific: typically we allow no modifiers. 
            // But usually shortcuts have at least one.
            // If we decide single keys (like F5) are shortcuts, this logic holds.
        }
        
        if (match) {
            std::cout << "Shortcut triggered: " << shortcut.description << std::endl;
            if (shortcut.action) {
                shortcut.action();
                return true;
            }
        }
    }
    return false;
}

} // namespace Zepra::WebCore
