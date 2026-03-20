// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file context_menu.h
 * @brief Right-click context menu component
 */

#ifndef ZEPRA_UI_CONTEXT_MENU_H
#define ZEPRA_UI_CONTEXT_MENU_H

#include "dropdown.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zepra {
namespace ui {

/**
 * Context menu types for different contexts
 */
enum class ContextType {
    Page,           // General page context
    Link,           // Right-click on a link
    Image,          // Right-click on an image
    Selection,      // Right-click on selected text
    Input,          // Right-click on input field
    Tab,            // Right-click on a tab
    Bookmark        // Right-click on bookmark
};

/**
 * Context data passed when showing menu
 */
struct ContextData {
    ContextType type = ContextType::Page;
    std::string linkUrl;
    std::string linkText;
    std::string imageUrl;
    std::string selectedText;
    std::string pageUrl;
    std::string pageTitle;
    bool isEditable = false;
    bool canCopy = false;
    bool canPaste = false;
    bool canCut = false;
};

/**
 * ContextMenu - Right-click menu for browser
 * 
 * Features:
 * - Context-aware menu items
 * - Standard browser actions
 * - Extensible item system
 */
class ContextMenu {
public:
    using ActionCallback = std::function<void(const std::string& action, const ContextData& data)>;

    ContextMenu();
    ~ContextMenu();

    // Show menu at position with context
    void show(float x, float y, const ContextData& context);
    void hide();
    bool isVisible() const;

    // Callbacks
    void setActionCallback(ActionCallback callback);

    // Custom items
    void addCustomItem(ContextType contextType, const MenuItem& item);
    void clearCustomItems();

    // Rendering
    void render();

    // Event handling
    bool handleMouseClick(float x, float y);
    bool handleMouseMove(float x, float y);
    bool handleKeyPress(int keyCode);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::vector<MenuItem> buildMenuItems(const ContextData& context);
};

} // namespace ui
} // namespace zepra

#endif // ZEPRA_UI_CONTEXT_MENU_H
