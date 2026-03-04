/**
 * @file dropdown.h
 * @brief Dropdown menu component with icons and submenus
 */

#ifndef ZEPRA_UI_DROPDOWN_H
#define ZEPRA_UI_DROPDOWN_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zepra {
namespace ui {

/**
 * Menu item types
 */
enum class MenuItemType {
    Normal,
    Checkbox,
    Radio,
    Separator,
    Submenu
};

/**
 * Single menu item
 */
struct MenuItem {
    std::string id;
    std::string label;
    std::string icon;           // SVG path
    std::string shortcut;       // e.g., "Ctrl+N"
    MenuItemType type = MenuItemType::Normal;
    bool enabled = true;
    bool checked = false;       // For checkbox/radio
    std::string radioGroup;     // For radio items
    std::vector<MenuItem> submenu;
};

/**
 * Dropdown configuration
 */
struct DropdownConfig {
    float minWidth = 180.0f;
    float maxWidth = 320.0f;
    float itemHeight = 36.0f;
    float padding = 8.0f;
    float borderRadius = 8.0f;
    float shadowBlur = 16.0f;
    bool showIcons = true;
    bool showShortcuts = true;
};

/**
 * Dropdown - Popup menu component
 * 
 * Features:
 * - Menu items with icons
 * - Separators
 * - Submenus with hover-to-open
 * - Checkbox and radio items
 * - Keyboard navigation
 * - Click-outside-to-close
 */
class Dropdown {
public:
    using SelectCallback = std::function<void(const std::string& itemId)>;
    using CloseCallback = std::function<void()>;

    Dropdown();
    explicit Dropdown(const DropdownConfig& config);
    ~Dropdown();

    // Configuration
    void setConfig(const DropdownConfig& config);
    DropdownConfig getConfig() const;

    // Menu items
    void setItems(const std::vector<MenuItem>& items);
    void addItem(const MenuItem& item);
    void addSeparator();
    void clear();

    // Item state
    void setItemEnabled(const std::string& id, bool enabled);
    void setItemChecked(const std::string& id, bool checked);
    MenuItem* findItem(const std::string& id);

    // Show/hide
    void show(float x, float y);
    void showBelow(float x, float y, float anchorWidth);
    void hide();
    bool isVisible() const;

    // Callbacks
    void setSelectCallback(SelectCallback callback);
    void setCloseCallback(CloseCallback callback);

    // Rendering
    void render();

    // Event handling
    bool handleMouseClick(float x, float y);
    bool handleMouseMove(float x, float y);
    bool handleKeyPress(int keyCode);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Helper to create common menu items
MenuItem createMenuItem(const std::string& id, const std::string& label, 
                        const std::string& icon = "", const std::string& shortcut = "");
MenuItem createSeparator();
MenuItem createCheckbox(const std::string& id, const std::string& label, bool checked = false);
MenuItem createSubmenu(const std::string& id, const std::string& label, 
                       const std::vector<MenuItem>& items);

} // namespace ui
} // namespace zepra

#endif // ZEPRA_UI_DROPDOWN_H
