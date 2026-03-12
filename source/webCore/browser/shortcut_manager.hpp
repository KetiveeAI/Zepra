/**
 * @file shortcut_manager.hpp
 * @brief Keyboard shortcut manager
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include <vector>

namespace Zepra::WebCore {

struct Shortcut {
    int key;
    int modifiers; // KMOD_CTRL, KMOD_SHIFT, KMOD_ALT
    std::function<void()> action;
    std::string description;
};

class ShortcutManager {
public:
    void registerShortcut(int key, int modifiers, const std::string& description, std::function<void()> action);
    bool handleKeyDown(int key, int modifiers);
    
    const std::vector<Shortcut>& shortcuts() const { return shortcuts_; }

private:
    std::vector<Shortcut> shortcuts_;
};

} // namespace Zepra::WebCore
