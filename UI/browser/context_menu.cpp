// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file context_menu.cpp
 * @brief Context menu implementation (pure C++, no SDL)
 */

#include "../../source/zepraEngine/include/engine/ui/context_menu.h"

namespace zepra {
namespace ui {

struct ContextMenu::Impl {
    std::unique_ptr<Dropdown> dropdown;
    ContextData currentContext;
    ActionCallback actionCallback;
    
    std::vector<std::pair<ContextType, MenuItem>> customItems;
    
    Impl() : dropdown(std::make_unique<Dropdown>()) {}
};

ContextMenu::ContextMenu() : impl_(std::make_unique<Impl>()) {
    impl_->dropdown->setSelectCallback([this](const std::string& id) {
        if (impl_->actionCallback) {
            impl_->actionCallback(id, impl_->currentContext);
        }
    });
}

ContextMenu::~ContextMenu() = default;

void ContextMenu::show(float x, float y, const ContextData& context) {
    impl_->currentContext = context;
    
    auto items = buildMenuItems(context);
    impl_->dropdown->setItems(items);
    impl_->dropdown->show(x, y);
}

void ContextMenu::hide() {
    impl_->dropdown->hide();
}

bool ContextMenu::isVisible() const {
    return impl_->dropdown->isVisible();
}

void ContextMenu::setActionCallback(ActionCallback callback) {
    impl_->actionCallback = std::move(callback);
}

void ContextMenu::addCustomItem(ContextType contextType, const MenuItem& item) {
    impl_->customItems.emplace_back(contextType, item);
}

void ContextMenu::clearCustomItems() {
    impl_->customItems.clear();
}

void ContextMenu::render() {
    impl_->dropdown->render();
}

bool ContextMenu::handleMouseClick(float x, float y) {
    return impl_->dropdown->handleMouseClick(x, y);
}

bool ContextMenu::handleMouseMove(float x, float y) {
    return impl_->dropdown->handleMouseMove(x, y);
}

bool ContextMenu::handleKeyPress(int keyCode) {
    return impl_->dropdown->handleKeyPress(keyCode);
}

std::vector<MenuItem> ContextMenu::buildMenuItems(const ContextData& context) {
    std::vector<MenuItem> items;
    
    switch (context.type) {
        case ContextType::Page:
            items.push_back(createMenuItem("back", "Back", "resources/icons/arrow-back.svg"));
            items.push_back(createMenuItem("forward", "Forward", "resources/icons/arrow-forward.svg"));
            items.push_back(createMenuItem("refresh", "Refresh", "resources/icons/refresh.svg", "Ctrl+R"));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("save", "Save Page As...", "", "Ctrl+S"));
            items.push_back(createMenuItem("print", "Print...", "", "Ctrl+P"));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("view_source", "View Page Source", "", "Ctrl+U"));
            items.push_back(createMenuItem("inspect", "Inspect", "resources/icons/devtool.svg", "Ctrl+Shift+I"));
            break;
            
        case ContextType::Link:
            items.push_back(createMenuItem("open_link", "Open Link"));
            items.push_back(createMenuItem("open_link_new_tab", "Open Link in New Tab"));
            items.push_back(createMenuItem("open_link_new_window", "Open Link in New Window"));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("copy_link", "Copy Link Address"));
            items.push_back(createMenuItem("save_link", "Save Link As..."));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("inspect", "Inspect Element"));
            break;
            
        case ContextType::Image:
            items.push_back(createMenuItem("open_image", "Open Image in New Tab"));
            items.push_back(createMenuItem("save_image", "Save Image As..."));
            items.push_back(createMenuItem("copy_image", "Copy Image"));
            items.push_back(createMenuItem("copy_image_url", "Copy Image Address"));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("inspect", "Inspect Element"));
            break;
            
        case ContextType::Selection:
            items.push_back(createMenuItem("copy", "Copy", "", "Ctrl+C"));
            if (!context.selectedText.empty()) {
                items.push_back(createMenuItem("search", "Search for \"" + 
                    context.selectedText.substr(0, 20) + 
                    (context.selectedText.length() > 20 ? "...\"" : "\"")));
            }
            items.push_back(createSeparator());
            items.push_back(createMenuItem("inspect", "Inspect Element"));
            break;
            
        case ContextType::Input:
            if (context.canCut) {
                items.push_back(createMenuItem("cut", "Cut", "", "Ctrl+X"));
            }
            if (context.canCopy) {
                items.push_back(createMenuItem("copy", "Copy", "", "Ctrl+C"));
            }
            if (context.canPaste) {
                items.push_back(createMenuItem("paste", "Paste", "", "Ctrl+V"));
            }
            items.push_back(createSeparator());
            items.push_back(createMenuItem("select_all", "Select All", "", "Ctrl+A"));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("inspect", "Inspect Element"));
            break;
            
        case ContextType::Tab:
            items.push_back(createMenuItem("new_tab", "New Tab", "", "Ctrl+T"));
            items.push_back(createMenuItem("duplicate_tab", "Duplicate Tab"));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("pin_tab", "Pin Tab"));
            items.push_back(createMenuItem("mute_tab", "Mute Tab"));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("close_tab", "Close Tab", "", "Ctrl+W"));
            items.push_back(createMenuItem("close_other_tabs", "Close Other Tabs"));
            items.push_back(createMenuItem("close_tabs_right", "Close Tabs to the Right"));
            break;
            
        case ContextType::Bookmark:
            items.push_back(createMenuItem("open_bookmark", "Open"));
            items.push_back(createMenuItem("open_bookmark_new_tab", "Open in New Tab"));
            items.push_back(createSeparator());
            items.push_back(createMenuItem("edit_bookmark", "Edit..."));
            items.push_back(createMenuItem("delete_bookmark", "Delete"));
            break;
    }
    
    // Add custom items for this context type
    for (const auto& [type, item] : impl_->customItems) {
        if (type == context.type) {
            items.push_back(createSeparator());
            items.push_back(item);
        }
    }
    
    return items;
}

} // namespace ui
} // namespace zepra
