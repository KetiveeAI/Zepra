// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * layout_engine.cpp - CSS Layout Engine Implementation
 */

#include "layout_engine.h"
#include <algorithm>
#include <iostream>

namespace ZepraBrowser {

// =============================================================================
// RENDERING CALLBACKS (set by main browser)
// =============================================================================

// These are set by the main browser code to connect layout to rendering
static void (*s_gfx_rect)(float, float, float, float, uint32_t) = nullptr;
static void (*s_gfx_border)(float, float, float, float, uint32_t, float) = nullptr;
static void (*s_text_render)(const std::string&, float, float, uint32_t, float) = nullptr;
static float (*s_text_width)(const std::string&, float) = nullptr;
static void (*s_register_link)(float, float, float, float, const std::string&, const std::string&) = nullptr;
static void (*s_gfx_texture)(float, float, float, float, uint32_t) = nullptr;
static void (*s_gfx_line)(float, float, float, float, uint32_t, float) = nullptr;

// Set rendering callbacks
void setLayoutCallbacks(
    void (*gfx_rect)(float, float, float, float, uint32_t),
    void (*gfx_border)(float, float, float, float, uint32_t, float),
    void (*text_render)(const std::string&, float, float, uint32_t, float),
    float (*text_width)(const std::string&, float),
    void (*register_link)(float, float, float, float, const std::string&, const std::string&),
    void (*gfx_texture)(float, float, float, float, uint32_t),
    void (*gfx_line)(float, float, float, float, uint32_t, float)
) {
    s_gfx_rect = gfx_rect;
    s_gfx_border = gfx_border;
    s_text_render = text_render;
    s_text_width = text_width;
    s_register_link = register_link;
    s_gfx_texture = gfx_texture;
    s_gfx_line = gfx_line;
}

// =============================================================================
// LAYOUT ALGORITHM
// =============================================================================

void layoutBlock(LayoutBox& box, float containingWidth, float startY) {
    // Calculate width (block elements take full containing width)
    if (box.type == LayoutType::Block) {
        box.width = containingWidth - box.marginLeft - box.marginRight;
    }
    
    // Position
    box.x = box.marginLeft;
    box.y = startY + box.marginTop;
    
    // Layout children
    float childY = box.paddingTop + box.borderTop;
    float childX = box.paddingLeft + box.borderLeft;
    float lineHeight = 0;
    float lineWidth = 0;
    float contentWidth = box.width - box.paddingLeft - box.paddingRight - box.borderLeft - box.borderRight;
    
    for (auto& child : box.children) {
        if (child.type == LayoutType::None) continue;
        
        // Flexbox: horizontal layout
        if (box.type == LayoutType::Flex) {
            // Flex children are laid out horizontally
            if (child.width == 0) {
                // Auto width: measure text or give default
                if (!child.text.empty()) {
                    child.width = measureTextWidth(child.text, child.fontSize) + 8;
                } else {
                    child.width = 100; // Default
                }
            }
            if (child.height == 0) {
                child.height = child.fontSize + 8;
            }
            
            // Recursively layout child to determine size and position children
            // Use a tentative width (e.g. contentWidth / number_of_children or just contentWidth for auto)
            // Ideally Flex layout is multi-pass, but for this simple engine:
            float childAvailableWidth = (box.width > 0) ? box.width : 200; 
            if (contentWidth > 0) childAvailableWidth = contentWidth; // Constrain to parent width
            
            layoutBlock(child, childAvailableWidth, 0); // Recursive layout
            
            // Re-measure after layout (in case it expanded)
            if (child.width == 0 && !child.text.empty()) {
                 child.width = measureTextWidth(child.text, child.fontSize) + 8;
            }
            if (child.height == 0) child.height = child.fontSize + 8;
            
            if (box.flexDirection == 1) { // Column
                child.x = childX + child.marginLeft;
                child.y = childY + child.marginTop;
                
                // Advance Y cursor for next item
                childY += child.height + child.marginTop + child.marginBottom;
                // Update container height? (LayoutBlock does this at end using childY)
            } else { // Row (Default)
                child.x = childX + lineWidth + child.marginLeft;
                child.y = childY + child.marginTop;
            
                // Advance X cursor
                lineWidth += child.width + 8 + child.marginLeft + child.marginRight; // Flex gap + margins
                lineHeight = std::max(lineHeight, child.height + child.marginTop + child.marginBottom);
            }
            child.type = LayoutType::FlexItem;
            continue;
        }
        
        if (child.type == LayoutType::Block) {
            // Block elements: stack vertically
            // Flush any inline content first
            if (lineHeight > 0) {
                childY += lineHeight;
                lineHeight = 0;
                lineWidth = 0;
            }
            
            // Layout child block
            layoutBlock(child, contentWidth, childY);
            child.x += box.paddingLeft + box.borderLeft;
            
            childY = child.y + child.totalHeight();
            
        } else if (child.type == LayoutType::Inline || child.type == LayoutType::Text || child.type == LayoutType::InlineBlock) {
            // Inline/Text/InlineBlock: flow horizontally with wrapping
            
            // Measure width/height
            if (child.isInput) {
                // Input defaults
                if (child.width == 0) child.width = 200; // Default input width
                if (child.height == 0) child.height = std::max(24.0f, child.fontSize + 8);
                
                // Add placeholder if text is empty
                if (child.text.empty() && !child.placeholder.empty()) {
                     // We don't change child.text to keep value clean, but paint will use placeholder
                }
            } else {
                float textW = measureTextWidth(child.text, child.fontSize);
                child.width = textW;
                child.height = child.fontSize + 4;
            }
            
            // Check for line wrap
            if (lineWidth > 0 && lineWidth + child.width > contentWidth) {
                // Start new line
                childY += lineHeight;
                lineWidth = 0;
                lineHeight = 0;
            }
            
            child.x = childX + lineWidth;
            child.y = childY;
            
            lineWidth += child.width + 4; // Spacing
            lineHeight = std::max(lineHeight, child.height);
        }
    }
    
    // Add remaining line height
    if (lineHeight > 0) {
        childY += lineHeight;
    }
    
    // Calculate height from children (auto height)
    box.height = childY + box.paddingBottom + box.borderBottom;
    
    // If block has no children but has text (leaf block), measure text height
    if (box.type == LayoutType::Block && box.children.empty() && (!box.text.empty() || box.isInput)) {
        box.height = std::max(box.height, box.fontSize + box.paddingTop + box.paddingBottom + 4);
    } else {
         if (box.height < box.fontSize + box.paddingTop + box.paddingBottom) {
            // Ensure min height for empty blocks? Maybe not.
            // box.height = box.fontSize + box.paddingTop + box.paddingBottom + 4;
         }
    }
}

// =============================================================================
// PAINTING
// =============================================================================

void paintBox(const LayoutBox& box, float offsetX, float offsetY,
              float viewportHeight, float scrollY) {
    if (box.type == LayoutType::None) return;
    
    float screenX = offsetX + box.x;
    float screenY = offsetY + box.y - scrollY;
    
    // Save for hit testing
    box.screenX = screenX;
    box.screenY = screenY;
    
    // Skip if off-screen (culling)
    if (screenY + box.height < 0 || screenY > viewportHeight) {
        return;
    }
    
    // Draw background
    if (box.hasBgColor && s_gfx_rect) {
        s_gfx_rect(screenX, screenY, box.width, box.height, box.bgColor);
    }
    
    // Draw border
    if ((box.borderTop > 0 || box.borderRight > 0 || box.borderBottom > 0 || box.borderLeft > 0) && s_gfx_border) {
        float thickness = std::max({box.borderTop, box.borderRight, box.borderBottom, box.borderLeft});
        s_gfx_border(screenX, screenY, box.width, box.height, box.borderColor, thickness);
    }
    
    // Draw input style
    // Draw input style (Unified with standard box rendering)
    if (box.isInput && s_gfx_border && s_gfx_rect) {
        // Use box properties (bgColor, borderColor) which are now set from CSS
        // If specific input styling is needed beyond standard box props, add it here
        // without overriding colors.
        
        // Ensure inputs have at least a border if CSS didn't set one?
        // For now, trust CSS or defaults set in buildLayoutFromDOM
    }
    
    // Draw image placeholder
    // Draw image
    if (box.isImage) {
        if (box.textureId > 0 && s_gfx_texture) {
            // Draw real textue
            s_gfx_texture(screenX, screenY, box.width, box.height, box.textureId);
        } else if (s_gfx_rect && s_gfx_border) {
            // Draw placeholder
            s_gfx_rect(screenX, screenY, box.width, box.height, box.bgColor); // Use bgColor (Success/Fail color)
            s_gfx_border(screenX, screenY, box.width, box.height, 0x000000, 1.0f);
        }
    }

    // Draw text content
    if ((!box.text.empty() || !box.placeholder.empty()) && 
        (box.type == LayoutType::Text || box.type == LayoutType::Inline || box.type == LayoutType::Block || box.type == LayoutType::InlineBlock)) {
        
        float textX = screenX + box.paddingLeft + (box.isInput ? 4 : 0);
        float textY = screenY + box.paddingTop + box.fontSize + (box.isInput ? 2 : 0);
        
        // Use placeholder if text is empty for inputs
        std::string drawText = box.text;
        uint32_t drawColor = box.color;
        
        if (box.isInput && drawText.empty() && !box.placeholder.empty()) {
            drawText = box.placeholder;
            drawColor = 0x999999; // Gray placeholder
        }
        
        if (s_text_render && !drawText.empty()) s_text_render(drawText, textX, textY, drawColor, box.fontSize);
        
        // Draw underline for links
        if (box.isLink && !drawText.empty()) {
            float textW = s_text_width ? s_text_width(drawText, box.fontSize) : (drawText.length() * box.fontSize * 0.5f);
            float underlineY = textY + 2;  // 2px below baseline
            if (s_gfx_line) {
                s_gfx_line(textX, underlineY, textX + textW, underlineY, box.color, 1.0f);
            } else if (s_gfx_rect) {
                // Fallback: use thin rect as line
                s_gfx_rect(textX, underlineY, textW, 1.0f, box.color);
            }
        }
        
        // Register link hit box
        if (box.isLink && !box.href.empty() && s_register_link) {
            s_register_link(screenX, screenY, box.width, box.height, box.href, box.target);
        }
    }
    
    // Paint children
    for (const auto& child : box.children) {
        paintBox(child, screenX + box.paddingLeft + box.borderLeft,
                 screenY + box.paddingTop + box.borderTop,
                 viewportHeight, 0);  // Children use parent-relative scroll
    }
}

// =============================================================================
// TEXT MEASUREMENT
// =============================================================================

float measureTextWidth(const std::string& text, float fontSize) {
    // Use callback if set, otherwise estimate
    if (s_text_width) return s_text_width(text, fontSize);
    return text.length() * fontSize * 0.5f;  // Rough estimate
}


// =============================================================================
// TEXT EXTRACTION
// =============================================================================

std::string getAllText(const LayoutBox& root) {
    std::string s;
    if (!root.text.empty()) s += root.text;
    
    for (const auto& child : root.children) {
        s += getAllText(child);
    }
    
    if (root.type == LayoutType::Block) s += "\n";
    return s;
}

std::string getTextInRect(const LayoutBox& root, float x, float y, float w, float h) {
    std::string s;
    
    // Check intersection (AABB)
    bool intersects = (root.screenX < x + w && root.screenX + root.width > x &&
                       root.screenY < y + h && root.screenY + root.height > y);
                       
    if (intersects && !root.text.empty()) {
        // Optimization: Could check specific character bounds, but box-level is okay for now
        s += root.text; 
    }
    
    // Optimization: If parent doesn't intersect at all, children probably don't either? 
    // BUT children are relative to parent content. If overflow is hidden... 
    // And cached screenX/Y are absolute.
    // If the box is completely outside, we might skip children, BUT layout overflow exists.
    // Culling is usually safe if we check cached screen coordinates.
    // However, generous bounds or just traversing all is safer to catch everything inside the rect.
    
    for (const auto& child : root.children) {
        s += getTextInRect(child, x, y, w, h);
    }
    
    if (intersects && root.type == LayoutType::Block && !s.empty()) {
        // Avoid double newlines if children already added them
        if (s.back() != '\n') s += "\n";
    }
    
    return s;
}

} // namespace ZepraBrowser
