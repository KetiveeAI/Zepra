// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file sidebar.cpp
 * @brief Collapsible sidebar implementation (pure C++, no SDL)
 */

#include "../../source/zepraEngine/include/engine/ui/sidebar.h"
#include <algorithm>

namespace zepra {
namespace ui {

struct Sidebar::Impl {
    SidebarConfig config;
    bool visible = false;
    float currentWidth = 0;
    float targetWidth = 0;
    float animationProgress = 1.0f;
    
    SidebarSection currentSection = SidebarSection::Bookmarks;
    std::string searchQuery;
    
    float x = 0, y = 0, height = 0;
    float scrollOffset = 0;
    
    // Items per section
    std::vector<SidebarItem> bookmarks;
    std::vector<SidebarItem> history;
    std::vector<SidebarItem> downloads;
    std::vector<SidebarItem> readingList;
    
    int hoveredItemIndex = -1;
    std::string selectedItemId;
    
    ItemClickCallback itemClickCallback;
    SectionCallback sectionCallback;
    ToggleCallback toggleCallback;
    
    Impl() : config() {
        currentWidth = config.startCollapsed ? 0 : config.width;
        targetWidth = currentWidth;
    }
    
    explicit Impl(const SidebarConfig& cfg) : config(cfg) {
        currentWidth = config.startCollapsed ? 0 : config.width;
        targetWidth = currentWidth;
    }
    
    std::vector<SidebarItem>& getItems(SidebarSection section) {
        switch (section) {
            case SidebarSection::Bookmarks: return bookmarks;
            case SidebarSection::History: return history;
            case SidebarSection::Downloads: return downloads;
            case SidebarSection::ReadingList: return readingList;
        }
        return bookmarks;
    }
    
    const std::vector<SidebarItem>& getItems(SidebarSection section) const {
        switch (section) {
            case SidebarSection::Bookmarks: return bookmarks;
            case SidebarSection::History: return history;
            case SidebarSection::Downloads: return downloads;
            case SidebarSection::ReadingList: return readingList;
        }
        return bookmarks;
    }
    
    std::vector<SidebarItem> getFilteredItems() const {
        const auto& items = getItems(currentSection);
        if (searchQuery.empty()) return items;
        
        std::vector<SidebarItem> filtered;
        for (const auto& item : items) {
            // Simple case-insensitive search
            std::string lowerTitle = item.title;
            std::string lowerQuery = searchQuery;
            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
            std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
            
            if (lowerTitle.find(lowerQuery) != std::string::npos ||
                item.url.find(searchQuery) != std::string::npos) {
                filtered.push_back(item);
            }
        }
        return filtered;
    }
};

Sidebar::Sidebar() : impl_(std::make_unique<Impl>()) {}

Sidebar::Sidebar(const SidebarConfig& config) 
    : impl_(std::make_unique<Impl>(config)) {}

Sidebar::~Sidebar() = default;

void Sidebar::setConfig(const SidebarConfig& config) {
    impl_->config = config;
}

SidebarConfig Sidebar::getConfig() const {
    return impl_->config;
}

void Sidebar::show() {
    if (!impl_->visible) {
        impl_->visible = true;
        impl_->targetWidth = impl_->config.width;
        impl_->animationProgress = 0;
        if (impl_->toggleCallback) {
            impl_->toggleCallback(true);
        }
    }
}

void Sidebar::hide() {
    if (impl_->visible) {
        impl_->visible = false;
        impl_->targetWidth = 0;
        impl_->animationProgress = 0;
        if (impl_->toggleCallback) {
            impl_->toggleCallback(false);
        }
    }
}

void Sidebar::toggle() {
    if (impl_->visible) {
        hide();
    } else {
        show();
    }
}

bool Sidebar::isVisible() const {
    return impl_->visible;
}

bool Sidebar::isAnimating() const {
    return impl_->animationProgress < 1.0f;
}

float Sidebar::getCurrentWidth() const {
    return impl_->currentWidth;
}

void Sidebar::setSection(SidebarSection section) {
    if (impl_->currentSection != section) {
        impl_->currentSection = section;
        impl_->scrollOffset = 0;
        impl_->hoveredItemIndex = -1;
        if (impl_->sectionCallback) {
            impl_->sectionCallback(section);
        }
    }
}

SidebarSection Sidebar::getCurrentSection() const {
    return impl_->currentSection;
}

void Sidebar::setItems(SidebarSection section, const std::vector<SidebarItem>& items) {
    impl_->getItems(section) = items;
}

void Sidebar::addItem(SidebarSection section, const SidebarItem& item) {
    impl_->getItems(section).push_back(item);
}

void Sidebar::removeItem(SidebarSection section, const std::string& id) {
    auto& items = impl_->getItems(section);
    items.erase(
        std::remove_if(items.begin(), items.end(),
            [&id](const SidebarItem& i) { return i.id == id; }),
        items.end()
    );
}

void Sidebar::updateItem(SidebarSection section, const SidebarItem& item) {
    auto& items = impl_->getItems(section);
    for (auto& existing : items) {
        if (existing.id == item.id) {
            existing = item;
            break;
        }
    }
}

void Sidebar::clearItems(SidebarSection section) {
    impl_->getItems(section).clear();
}

void Sidebar::expandFolder(const std::string& id) {
    auto& items = impl_->getItems(impl_->currentSection);
    for (auto& item : items) {
        if (item.id == id && item.isFolder) {
            item.isExpanded = true;
            break;
        }
    }
}

void Sidebar::collapseFolder(const std::string& id) {
    auto& items = impl_->getItems(impl_->currentSection);
    for (auto& item : items) {
        if (item.id == id && item.isFolder) {
            item.isExpanded = false;
            break;
        }
    }
}

void Sidebar::toggleFolder(const std::string& id) {
    auto& items = impl_->getItems(impl_->currentSection);
    for (auto& item : items) {
        if (item.id == id && item.isFolder) {
            item.isExpanded = !item.isExpanded;
            break;
        }
    }
}

void Sidebar::setSearchQuery(const std::string& query) {
    impl_->searchQuery = query;
    impl_->scrollOffset = 0;
}

std::string Sidebar::getSearchQuery() const {
    return impl_->searchQuery;
}

void Sidebar::setItemClickCallback(ItemClickCallback callback) {
    impl_->itemClickCallback = std::move(callback);
}

void Sidebar::setSectionCallback(SectionCallback callback) {
    impl_->sectionCallback = std::move(callback);
}

void Sidebar::setToggleCallback(ToggleCallback callback) {
    impl_->toggleCallback = std::move(callback);
}

void Sidebar::setBounds(float x, float y, float width, float height) {
    impl_->x = x;
    impl_->y = y;
    impl_->height = height;
}

void Sidebar::render() {
    // Note: Actual rendering is done by the browser's OpenGL code
    // This component provides state and callbacks
}

void Sidebar::update(float deltaTime) {
    // Animate width change
    if (impl_->animationProgress < 1.0f) {
        float animSpeed = 1000.0f / impl_->config.animationDuration;
        impl_->animationProgress += deltaTime * animSpeed;
        impl_->animationProgress = std::min(impl_->animationProgress, 1.0f);
        
        // Ease out animation
        float t = impl_->animationProgress;
        float eased = 1.0f - (1.0f - t) * (1.0f - t);
        
        float startWidth = impl_->visible ? 0 : impl_->config.width;
        impl_->currentWidth = startWidth + (impl_->targetWidth - startWidth) * eased;
    }
}

bool Sidebar::handleMouseClick(float x, float y) {
    if (impl_->currentWidth < 10) return false;  // Too narrow to interact
    
    if (x < impl_->x || x > impl_->x + impl_->currentWidth) return false;
    if (y < impl_->y || y > impl_->y + impl_->height) return false;
    
    // Check section tabs (at top)
    float tabY = impl_->y + 8;
    float tabHeight = 32;
    if (y >= tabY && y <= tabY + tabHeight) {
        float tabWidth = impl_->currentWidth / 4;
        int tabIndex = static_cast<int>((x - impl_->x) / tabWidth);
        if (tabIndex >= 0 && tabIndex < 4) {
            setSection(static_cast<SidebarSection>(tabIndex));
            return true;
        }
    }
    
    // Check items
    auto items = impl_->getFilteredItems();
    float itemY = impl_->y + 60 - impl_->scrollOffset;  // After tabs and search
    float itemHeight = 40;
    
    for (size_t i = 0; i < items.size(); i++) {
        if (y >= itemY && y < itemY + itemHeight) {
            const auto& item = items[i];
            
            if (item.isFolder) {
                toggleFolder(item.id);
            } else {
                impl_->selectedItemId = item.id;
                if (impl_->itemClickCallback) {
                    impl_->itemClickCallback(item);
                }
            }
            return true;
        }
        itemY += itemHeight;
    }
    
    return true;  // Consumed click even if no item hit
}

bool Sidebar::handleMouseMove(float x, float y) {
    if (impl_->currentWidth < 10) return false;
    
    if (x < impl_->x || x > impl_->x + impl_->currentWidth) {
        impl_->hoveredItemIndex = -1;
        return false;
    }
    
    // Calculate hovered item
    auto items = impl_->getFilteredItems();
    float itemY = impl_->y + 60 - impl_->scrollOffset;
    float itemHeight = 40;
    
    int newHovered = -1;
    for (size_t i = 0; i < items.size(); i++) {
        if (y >= itemY && y < itemY + itemHeight) {
            newHovered = static_cast<int>(i);
            break;
        }
        itemY += itemHeight;
    }
    
    if (newHovered != impl_->hoveredItemIndex) {
        impl_->hoveredItemIndex = newHovered;
        return true;  // Needs redraw
    }
    
    return false;
}

bool Sidebar::handleMouseDown(float, float) {
    return false;
}

bool Sidebar::handleMouseUp(float, float) {
    return false;
}

bool Sidebar::handleKeyPress(int keyCode, bool ctrl, bool) {
    // Handle search input
    if (impl_->searchQuery.length() > 0 && keyCode == 0xFF08) { // Backspace
        impl_->searchQuery.pop_back();
        return true;
    }
    
    return false;
}

bool Sidebar::handleScroll(float x, float, float delta) {
    if (x < impl_->x || x > impl_->x + impl_->currentWidth) return false;
    
    impl_->scrollOffset -= delta * 40;  // 40px per scroll unit
    impl_->scrollOffset = std::max(0.0f, impl_->scrollOffset);
    
    // Limit scroll to content height
    auto items = impl_->getFilteredItems();
    float contentHeight = items.size() * 40;
    float viewHeight = impl_->height - 60;  // Minus header
    float maxScroll = std::max(0.0f, contentHeight - viewHeight);
    impl_->scrollOffset = std::min(impl_->scrollOffset, maxScroll);
    
    return true;
}

} // namespace ui
} // namespace zepra
