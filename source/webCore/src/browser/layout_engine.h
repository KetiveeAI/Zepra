/**
 * layout_engine.h - CSS Layout Engine for ZepraBrowser
 * 
 * Implements Block Formatting Context (BFC) algorithm for laying out
 * HTML elements according to CSS box model rules.
 */

#pragma once

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <cstdint>

namespace ZepraBrowser {

// =============================================================================
// LAYOUT TYPES
// =============================================================================

enum class LayoutType {
    Block,       // Block-level element (div, p, h1, etc.)
    Inline,      // Inline element (span, a, etc.)
    InlineBlock, // Inline-block element
    Flex,        // Flexbox container
    FlexItem,    // Child of flexbox container
    None,        // display: none
    Text         // Text node
};

// =============================================================================
// LAYOUT BOX
// =============================================================================

/**
 * @brief Represents a box in the layout tree with position and dimensions
 * 
 * Each LayoutBox corresponds to a DOM element and contains:
 * - Position (x, y relative to parent)
 * - Dimensions (width, height)
 * - Box model properties (margin, padding, border)
 * - Style properties for rendering
 * - Children in document order
 */
struct LayoutBox {
    // Position and dimensions
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    
    // Cached screen position (calculated during paint)
    mutable float screenX = 0;
    mutable float screenY = 0;
    
    // Box model
    float marginTop = 0, marginRight = 0, marginBottom = 0, marginLeft = 0;
    float paddingTop = 0, paddingRight = 0, paddingBottom = 0, paddingLeft = 0;
    float borderTop = 0, borderRight = 0, borderBottom = 0, borderLeft = 0;
    
    // Display type
    LayoutType type = LayoutType::Block;
    int flexDirection = 0; // 0=Row, 1=Column
    
    // Content (for text nodes)
    std::string text;
    
    // Style properties for rendering
    uint32_t color = 0x000000;
    uint32_t bgColor = 0xFFFFFF;
    uint32_t borderColor = 0x000000;
    float fontSize = 16.0f;
    bool bold = false;
    bool italic = false;
    bool hasBgColor = false;
    
    // Input properties
    bool isInput = false;
    std::string inputType;
    std::string placeholder;
    
    // Link properties
    bool isLink = false;
    std::string href;
    std::string target;

    // Image properties
    bool isImage = false;
    uint32_t textureId = 0; // 0 = no texture
    
    
    // Children (using list for stable pointers - vector invalidates on realloc)
    std::list<LayoutBox> children;
    
    // Helper methods
    float contentX() const { return x + marginLeft + borderLeft + paddingLeft; }
    float contentY() const { return y + marginTop + borderTop + paddingTop; }
    float contentWidth() const { return width - paddingLeft - paddingRight - borderLeft - borderRight; }
    float contentHeight() const { return height - paddingTop - paddingBottom - borderTop - borderBottom; }
    
    float totalWidth() const { return width + marginLeft + marginRight; }
    float totalHeight() const { return height + marginTop + marginBottom; }
};

// =============================================================================
// LAYOUT FUNCTIONS
// =============================================================================

/**
 * @brief Initialize layout rendering callbacks
 * 
 * Must be called before using paintBox() to connect layout engine
 * to the browser's rendering system.
 */
void setLayoutCallbacks(
    void (*gfx_rect)(float x, float y, float w, float h, uint32_t color),
    void (*gfx_border)(float x, float y, float w, float h, uint32_t color, float thickness),
    void (*text_render)(const std::string& text, float x, float y, uint32_t color, float fontSize),
    float (*text_width)(const std::string& text, float fontSize),
    void (*register_link)(float x, float y, float w, float h, const std::string& href, const std::string& target),
    void (*gfx_texture)(float x, float y, float w, float h, uint32_t textureId),
    void (*gfx_line)(float x1, float y1, float x2, float y2, uint32_t color, float thickness) = nullptr
);

/**
 * @brief Layout a block formatting context
 * 
 * Implements the CSS Block Formatting Context algorithm:
 * - Block boxes stack vertically
 * - Inline boxes flow horizontally with line wrapping
 * - Width is calculated from containing block
 * - Height is calculated from children
 * 
 * @param box The box to layout (modified in place)
 * @param containingWidth Width of the containing block
 * @param startY Starting Y position
 */
void layoutBlock(LayoutBox& box, float containingWidth, float startY = 0);

/**
 * @brief Paint a layout box and its children
 * 
 * Recursively renders the layout box tree:
 * 1. Draw background
 * 2. Draw border
 * 3. Draw text content (if text node)
 * 4. Paint children
 * 5. Register link hit boxes
 * 
 * @param box The box to paint
 * @param offsetX X offset from viewport origin
 * @param offsetY Y offset from viewport origin
 * @param viewportHeight Height of visible area (for culling)
 * @param scrollY Current scroll position
 */
void paintBox(const LayoutBox& box, float offsetX, float offsetY, 
              float viewportHeight, float scrollY);

/**
 * @brief Calculate text width using current font
 * 
 * @param text Text to measure
 * @param fontSize Font size in pixels
 * @return Width in pixels
 */
float measureTextWidth(const std::string& text, float fontSize);

/**
 * @brief Extract all text from the layout tree
 * 
 * @param root Root box
 * @return Concatenated text
 */
std::string getAllText(const LayoutBox& root);

/**
 * @brief Extract text within a screen rectangle
 * 
 * @param root Root box
 * @param x Selection X
 * @param y Selection Y
 * @param w Selection Width
 * @param h Selection Height
 * @return Text intersecting the rectangle
 */
std::string getTextInRect(const LayoutBox& root, float x, float y, float w, float h);

} // namespace ZepraBrowser
