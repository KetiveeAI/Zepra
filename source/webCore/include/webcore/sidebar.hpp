/**
 * @file sidebar.hpp
 * @brief Sidebar navigation component for Zepra Browser
 */

#pragma once

#include "paint_context.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Sidebar navigation item
 */
struct SidebarItem {
    std::string id;
    std::string tooltip;
    bool active = false;
};

/**
 * @brief Vertical sidebar navigation component
 */
class Sidebar {
public:
    Sidebar();
    
    // Layout
    void setBounds(float x, float y, float width, float height);
    Rect bounds() const { return bounds_; }
    float width() const { return collapsed_ ? collapsedWidth_ : width_; }
    float expandedWidth() const { return width_; }
    
    // Collapsed/Expanded state - hidden by default
    bool isCollapsed() const { return collapsed_; }
    void setCollapsed(bool collapsed) { collapsed_ = collapsed; }
    void toggle() { collapsed_ = !collapsed_; }
    
    // Items
    void addItem(const std::string& id, const std::string& tooltip);
    void setActiveItem(const std::string& id);
    
    // Events
    void setOnItemClick(std::function<void(const std::string&)> handler) { onItemClick_ = handler; }
    void setOnToggle(std::function<void(bool)> handler) { onToggle_ = handler; }
    
    // Rendering
    void paint(PaintContext& ctx);
    
    // Input handling
    bool handleMouseDown(float x, float y, int button);
    bool handleMouseMove(float x, float y);
    
private:
    void drawIcon(PaintContext& ctx, const std::string& id, const Rect& bounds, const Color& color = {139, 148, 158, 255});
    void drawToggleButton(PaintContext& ctx);
    
    Rect bounds_;
    float width_ = 60.0f;
    float collapsedWidth_ = 8.0f;  // Thin strip when collapsed
    bool collapsed_ = true;  // Hidden by default
    std::vector<SidebarItem> items_;
    int hoveredIndex_ = -1;
    bool hoveringToggle_ = false;
    std::function<void(const std::string&)> onItemClick_;
    std::function<void(bool)> onToggle_;
};

} // namespace Zepra::WebCore
