/**
 * @file sidebar.cpp
 * @brief Sidebar navigation component implementation
 */

#include "webcore/sidebar.hpp"
#include "webcore/svg_loader.hpp"
#include "webcore/theme.hpp"
#include <cmath>

namespace Zepra::WebCore {

Sidebar::Sidebar() {
    // Default sidebar items - matching the design
    addItem("home", "Home");
    addItem("avatar", "Profile");
    addItem("lab", "Labs");
    addItem("search", "Search");
    addItem("calendar", "Calendar");
    addItem("settings", "Settings");
}

void Sidebar::setBounds(float x, float y, float width, float height) {
    bounds_ = {x, y, width, height};
    width_ = width;
}

void Sidebar::addItem(const std::string& id, const std::string& tooltip) {
    items_.push_back({id, tooltip, false});
}

void Sidebar::setActiveItem(const std::string& id) {
    for (auto& item : items_) {
        item.active = (item.id == id);
    }
}

void Sidebar::paint(PaintContext& ctx) {
    if (collapsed_) {
        // Draw collapsed state - thin strip with expand indicator
        float stripWidth = collapsedWidth_;
        ctx.fillRect({bounds_.x, bounds_.y, stripWidth, bounds_.height}, {180, 160, 140, 150});  // Beige semi-transparent
        
        // Draw three dots indicator in center
        float dotY = bounds_.y + bounds_.height / 2 - 20;
        for (int i = 0; i < 3; i++) {
            ctx.fillRect({bounds_.x + 2, dotY + i * 12, 4, 4}, {255, 255, 255, 200});
        }
        return;
    }
    
    // === GLASSMORPHISM: Sidebar with frosted glass effect ===
    int numBands = 20;
    float bandHeight = bounds_.height / numBands;
    for (int i = 0; i < numBands; i++) {
        float t = static_cast<float>(i) / numBands;
        uint8_t r = 196 - static_cast<uint8_t>((196 - 139) * t);
        uint8_t g = 168 - static_cast<uint8_t>((168 - 115) * t);
        uint8_t b = 138 - static_cast<uint8_t>((138 - 85) * t);
        ctx.fillRect({bounds_.x, bounds_.y + i * bandHeight, bounds_.width, bandHeight + 1}, {r, g, b, 180});
    }
    
    // Glass overlay (frosted effect)
    ctx.fillRect(bounds_, {255, 255, 255, 30});
    
    // Left edge highlight (glass shine)
    ctx.fillRect({bounds_.x, bounds_.y, 1, bounds_.height}, {255, 255, 255, 80});
    
    // Right border - subtle dark line  
    ctx.fillRect({bounds_.x + bounds_.width - 1, bounds_.y, 1, bounds_.height}, 
                 {100, 80, 60, 80});  // Subtle border
    
    // Logo at top
    float logoSize = 32;
    float logoX = bounds_.x + (bounds_.width - logoSize) / 2;
    float logoY = bounds_.y + 20;
    
    // Logo background - primary purple
    ctx.fillRect({logoX, logoY, logoSize, logoSize}, Theme::primaryDark());  // #4300B1
    
    // "Z" letter
    float letterX = logoX + logoSize/2 - 6;
    float letterY = logoY + logoSize/2 - 8;
    ctx.drawText("Z", letterX, letterY, {255, 255, 255, 255}, 18);
    
    // Draw items
    float itemSize = 40;
    float itemSpacing = 16;
    float startY = logoY + logoSize + 40;
    
    for (size_t i = 0; i < items_.size(); i++) {
        float itemX = bounds_.x + (bounds_.width - itemSize) / 2;
        float itemY = startY + i * (itemSize + itemSpacing);
        
        Rect itemBounds = {itemX, itemY, itemSize, itemSize};
        
        // Hover/active background
        if (items_[i].active || static_cast<int>(i) == hoveredIndex_) {
            ctx.fillRect(itemBounds, Theme::secondaryLight());  // #DACAF6
        }
        
        // Draw icon using SVGLoader
        drawIcon(ctx, items_[i].id, itemBounds);
    }
}

void Sidebar::drawToggleButton(PaintContext& ctx) {
    (void)ctx;
    // Toggle button at bottom or on hover
}

void Sidebar::drawIcon(PaintContext& ctx, const std::string& id, const Rect& bounds, const Color&) {
    // Icon paths - using absolute path to resources/icons
    static const std::string iconDir = "/home/swana/Documents/zeprabrowser/resources/icons/";
    
    std::string iconPath;
    if (id == "home") {
        iconPath = iconDir + "home.svg";
    } else if (id == "search") {
        iconPath = iconDir + "search.svg";
    } else if (id == "calendar") {
        iconPath = iconDir + "Calendar.svg";
    } else if (id == "settings") {
        iconPath = iconDir + "settings.svg";
    } else if (id == "lab") {
        iconPath = iconDir + "Lab.svg";
    } else if (id == "avatar") {
        iconPath = iconDir + "Avtar.svg";
    } else if (id == "help") {
        iconPath = iconDir + "Idea.svg";
    } else {
        // Fallback: draw placeholder rect
        ctx.fillRect({bounds.x + 10, bounds.y + 10, bounds.width - 20, bounds.height - 20}, {139, 148, 158, 255});
        return;
    }
    
    // Load SVG using SVGLoader and draw as texture
    int iconSize = 24;  // Target icon size
    SVGTexture tex = SVGLoader::instance().loadSVG(iconPath, iconSize, iconSize);
    
    if (tex.textureId != 0) {
        // Center the icon within bounds
        float iconX = bounds.x + (bounds.width - iconSize) / 2;
        float iconY = bounds.y + (bounds.height - iconSize) / 2;
        ctx.drawTexture(tex.textureId, {iconX, iconY, (float)iconSize, (float)iconSize});
    } else {
        // SVG load failed - draw placeholder
        ctx.fillRect({bounds.x + 10, bounds.y + 10, bounds.width - 20, bounds.height - 20}, {139, 148, 158, 255});
    }
}

bool Sidebar::handleMouseDown(float x, float y, int button) {
    (void)button;
    
    // When collapsed, clicking on the strip expands it
    if (collapsed_) {
        if (x >= bounds_.x && x < bounds_.x + collapsedWidth_ &&
            y >= bounds_.y && y < bounds_.y + bounds_.height) {
            toggle();
            if (onToggle_) onToggle_(collapsed_);
            return true;
        }
        return false;
    }
    
    // Check if clicking outside expanded sidebar (to collapse it)
    if (x < bounds_.x || x > bounds_.x + width_ ||
        y < bounds_.y || y > bounds_.y + bounds_.height) {
        return false;
    }
    
    float itemSize = 40;
    float itemSpacing = 16;
    float startY = bounds_.y + 20 + 32 + 40; // logo offset
    
    for (size_t i = 0; i < items_.size(); i++) {
        float itemY = startY + i * (itemSize + itemSpacing);
        if (y >= itemY && y < itemY + itemSize) {
            if (onItemClick_) {
                onItemClick_(items_[i].id);
            }
            setActiveItem(items_[i].id);
            return true;
        }
    }
    
    return true;
}

bool Sidebar::handleMouseMove(float x, float y) {
    hoveredIndex_ = -1;
    
    if (x < bounds_.x || x > bounds_.x + bounds_.width ||
        y < bounds_.y || y > bounds_.y + bounds_.height) {
        return false;
    }
    
    float itemSize = 40;
    float itemSpacing = 16;
    float startY = bounds_.y + 20 + 32 + 40;
    
    for (size_t i = 0; i < items_.size(); i++) {
        float itemY = startY + i * (itemSize + itemSpacing);
        if (y >= itemY && y < itemY + itemSize) {
            hoveredIndex_ = static_cast<int>(i);
            return true;
        }
    }
    
    return false;
}

} // namespace Zepra::WebCore
