// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file dropdown.cpp
 * @brief Dropdown menu implementation (pure C++, no SDL)
 */

#include "../../source/zepraEngine/include/engine/ui/dropdown.h"

namespace zepra {
namespace ui {

struct Dropdown::Impl {
    DropdownConfig config;
    std::vector<MenuItem> items;
    bool visible = false;
    float x = 0, y = 0;
    float width = 0, height = 0;
    
    int hoveredIndex = -1;
    int activeSubmenuIndex = -1;
    std::unique_ptr<Dropdown> activeSubmenu;
    
    SelectCallback selectCallback;
    CloseCallback closeCallback;
    
    Impl() : config() {}
    explicit Impl(const DropdownConfig& cfg) : config(cfg) {}
    
    void calculateSize() {
        width = config.minWidth;
        height = config.padding * 2;
        
        for (const auto& item : items) {
            if (item.type == MenuItemType::Separator) {
                height += 1;  // 1px separator
            } else {
                height += config.itemHeight;
                
                // Calculate width based on content
                float itemWidth = config.padding * 2 + 16;  // Icon space
                itemWidth += item.label.length() * 8;  // Approximate text width
                if (!item.shortcut.empty()) {
                    itemWidth += 60;  // Shortcut space
                }
                if (item.type == MenuItemType::Submenu) {
                    itemWidth += 16;  // Arrow
                }
                width = std::max(width, itemWidth);
            }
        }
        
        width = std::min(width, config.maxWidth);
    }
    
    int hitTest(float px, float py) const {
        if (!visible) return -1;
        if (px < x || px > x + width) return -1;
        
        float currentY = y + config.padding;
        for (size_t i = 0; i < items.size(); i++) {
            const auto& item = items[i];
            float itemH = (item.type == MenuItemType::Separator) ? 1 : config.itemHeight;
            
            if (py >= currentY && py < currentY + itemH) {
                if (item.type == MenuItemType::Separator || !item.enabled) {
                    return -1;
                }
                return static_cast<int>(i);
            }
            currentY += itemH;
        }
        return -1;
    }
};

Dropdown::Dropdown() : impl_(std::make_unique<Impl>()) {}

Dropdown::Dropdown(const DropdownConfig& config) 
    : impl_(std::make_unique<Impl>(config)) {}

Dropdown::~Dropdown() = default;

void Dropdown::setConfig(const DropdownConfig& config) {
    impl_->config = config;
}

DropdownConfig Dropdown::getConfig() const {
    return impl_->config;
}

void Dropdown::setItems(const std::vector<MenuItem>& items) {
    impl_->items = items;
    impl_->calculateSize();
}

void Dropdown::addItem(const MenuItem& item) {
    impl_->items.push_back(item);
    impl_->calculateSize();
}

void Dropdown::addSeparator() {
    MenuItem sep;
    sep.type = MenuItemType::Separator;
    impl_->items.push_back(sep);
    impl_->calculateSize();
}

void Dropdown::clear() {
    impl_->items.clear();
    impl_->width = 0;
    impl_->height = 0;
}

void Dropdown::setItemEnabled(const std::string& id, bool enabled) {
    for (auto& item : impl_->items) {
        if (item.id == id) {
            item.enabled = enabled;
            break;
        }
    }
}

void Dropdown::setItemChecked(const std::string& id, bool checked) {
    for (auto& item : impl_->items) {
        if (item.id == id) {
            item.checked = checked;
            break;
        }
    }
}

MenuItem* Dropdown::findItem(const std::string& id) {
    for (auto& item : impl_->items) {
        if (item.id == id) {
            return &item;
        }
        // Search in submenus
        for (auto& sub : item.submenu) {
            if (sub.id == id) {
                return &sub;
            }
        }
    }
    return nullptr;
}

void Dropdown::show(float x, float y) {
    impl_->x = x;
    impl_->y = y;
    impl_->visible = true;
    impl_->hoveredIndex = -1;
    impl_->activeSubmenuIndex = -1;
    impl_->activeSubmenu.reset();
}

void Dropdown::showBelow(float x, float y, float anchorWidth) {
    // Position menu below anchor, aligned to left
    impl_->x = x;
    impl_->y = y;
    
    // If menu is wider than anchor, keep it left-aligned
    // If needed, adjust x to keep menu on screen (would need screen dimensions)
    
    impl_->visible = true;
    impl_->hoveredIndex = -1;
}

void Dropdown::hide() {
    impl_->visible = false;
    impl_->activeSubmenu.reset();
    if (impl_->closeCallback) {
        impl_->closeCallback();
    }
}

bool Dropdown::isVisible() const {
    return impl_->visible;
}

void Dropdown::setSelectCallback(SelectCallback callback) {
    impl_->selectCallback = std::move(callback);
}

void Dropdown::setCloseCallback(CloseCallback callback) {
    impl_->closeCallback = std::move(callback);
}

void Dropdown::render() {
    // Note: Actual rendering is done by the browser's OpenGL code
    // This component provides state and callbacks
}

bool Dropdown::handleMouseClick(float x, float y) {
    if (!impl_->visible) return false;
    
    // Check submenu first
    if (impl_->activeSubmenu && impl_->activeSubmenu->handleMouseClick(x, y)) {
        return true;
    }
    
    int index = impl_->hitTest(x, y);
    if (index >= 0) {
        const auto& item = impl_->items[index];
        
        if (item.type == MenuItemType::Submenu) {
            // Open submenu on click
            return true;
        }
        
        if (item.type == MenuItemType::Checkbox) {
            // Toggle checkbox
            impl_->items[index].checked = !impl_->items[index].checked;
        }
        
        if (impl_->selectCallback) {
            impl_->selectCallback(item.id);
        }
        
        hide();
        return true;
    }
    
    // Click outside - close
    hide();
    return false;
}

bool Dropdown::handleMouseMove(float x, float y) {
    if (!impl_->visible) return false;
    
    int newHovered = impl_->hitTest(x, y);
    if (newHovered != impl_->hoveredIndex) {
        impl_->hoveredIndex = newHovered;
        
        // Handle submenu opening
        if (newHovered >= 0) {
            const auto& item = impl_->items[newHovered];
            if (item.type == MenuItemType::Submenu && !item.submenu.empty()) {
                if (impl_->activeSubmenuIndex != newHovered) {
                    impl_->activeSubmenuIndex = newHovered;
                    impl_->activeSubmenu = std::make_unique<Dropdown>(impl_->config);
                    impl_->activeSubmenu->setItems(item.submenu);
                    impl_->activeSubmenu->setSelectCallback(impl_->selectCallback);
                    // Position submenu to the right
                    float subY = impl_->y + impl_->config.padding + 
                                 newHovered * impl_->config.itemHeight;
                    impl_->activeSubmenu->show(impl_->x + impl_->width, subY);
                }
            } else {
                impl_->activeSubmenuIndex = -1;
                impl_->activeSubmenu.reset();
            }
        }
        
        return true;  // Needs redraw
    }
    
    // Forward to submenu
    if (impl_->activeSubmenu) {
        return impl_->activeSubmenu->handleMouseMove(x, y);
    }
    
    return false;
}

bool Dropdown::handleKeyPress(int keyCode) {
    if (!impl_->visible) return false;
    
    // Handle arrow keys
    if (keyCode == 0xFF54) { // Down
        impl_->hoveredIndex++;
        if (impl_->hoveredIndex >= static_cast<int>(impl_->items.size())) {
            impl_->hoveredIndex = 0;
        }
        // Skip separators and disabled items
        while (impl_->hoveredIndex < static_cast<int>(impl_->items.size())) {
            const auto& item = impl_->items[impl_->hoveredIndex];
            if (item.type != MenuItemType::Separator && item.enabled) break;
            impl_->hoveredIndex++;
        }
        return true;
    }
    
    if (keyCode == 0xFF52) { // Up
        impl_->hoveredIndex--;
        if (impl_->hoveredIndex < 0) {
            impl_->hoveredIndex = static_cast<int>(impl_->items.size()) - 1;
        }
        // Skip separators and disabled items
        while (impl_->hoveredIndex >= 0) {
            const auto& item = impl_->items[impl_->hoveredIndex];
            if (item.type != MenuItemType::Separator && item.enabled) break;
            impl_->hoveredIndex--;
        }
        return true;
    }
    
    if (keyCode == 0xFF0D || keyCode == '\r') { // Enter
        if (impl_->hoveredIndex >= 0) {
            const auto& item = impl_->items[impl_->hoveredIndex];
            if (impl_->selectCallback) {
                impl_->selectCallback(item.id);
            }
            hide();
            return true;
        }
    }
    
    if (keyCode == 0xFF1B) { // Escape
        hide();
        return true;
    }
    
    return false;
}

// Helper functions
MenuItem createMenuItem(const std::string& id, const std::string& label, 
                        const std::string& icon, const std::string& shortcut) {
    MenuItem item;
    item.id = id;
    item.label = label;
    item.icon = icon;
    item.shortcut = shortcut;
    item.type = MenuItemType::Normal;
    item.enabled = true;
    return item;
}

MenuItem createSeparator() {
    MenuItem item;
    item.type = MenuItemType::Separator;
    return item;
}

MenuItem createCheckbox(const std::string& id, const std::string& label, bool checked) {
    MenuItem item;
    item.id = id;
    item.label = label;
    item.type = MenuItemType::Checkbox;
    item.checked = checked;
    item.enabled = true;
    return item;
}

MenuItem createSubmenu(const std::string& id, const std::string& label, 
                       const std::vector<MenuItem>& items) {
    MenuItem item;
    item.id = id;
    item.label = label;
    item.type = MenuItemType::Submenu;
    item.submenu = items;
    item.enabled = true;
    return item;
}

} // namespace ui
} // namespace zepra
