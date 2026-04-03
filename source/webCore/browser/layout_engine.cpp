// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * layout_engine.cpp - CSS Layout Engine Implementation
 */

#include "layout_engine.h"
#include <algorithm>
#include <iostream>
#include <cfloat>

// Viewport dimensions (defined in zepra_browser.cpp)
extern int g_width;
extern int g_height;

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

// Extended callbacks (set via setLayoutCallbacks2)
static void (*s_gfx_rrect)(float, float, float, float, float, uint32_t, uint8_t) = nullptr;
static void (*s_gfx_gradient)(float, float, float, float, uint32_t, uint32_t) = nullptr;
static void (*s_gfx_svg)(float, float, float, const std::string&) = nullptr;

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

void setLayoutCallbacks2(
    void (*gfx_rrect)(float, float, float, float, float, uint32_t, uint8_t),
    void (*gfx_gradient)(float, float, float, float, uint32_t, uint32_t),
    void (*gfx_svg)(float, float, float, const std::string&)
) {
    s_gfx_rrect = gfx_rrect;
    s_gfx_gradient = gfx_gradient;
    s_gfx_svg = gfx_svg;
}

// =============================================================================
// LAYOUT ALGORITHM
// =============================================================================

void layoutBlock(LayoutBox& box, float containingWidth, float startY) {
    // Guard against deep recursion (complex pages like github.com with 1800+ boxes)
    static thread_local int s_depth = 0;
    if (++s_depth > 64) { --s_depth; return; }
    struct DepthGuard { ~DepthGuard() { --s_depth; } } guard;
    
    float vpW = (float)g_width;
    float vpH = (float)g_height;
    
    // Resolve deferred CSS width
    if (box.cssWidth.isSet() && !box.cssWidth.isAuto()) {
        box.width = box.cssWidth.resolve(containingWidth, box.fontSize, vpW, vpH);
    } else if (box.type == LayoutType::Block || box.type == LayoutType::Flex) {
        // Block and Flex containers fill their containing block width (CSS2 §10.3.3)
        float w = containingWidth - box.marginLeft - box.marginRight;
        box.width = w > 0 ? w : containingWidth;
    }
    
    // Resolve deferred CSS height (container height is 0 for now — auto)
    if (box.cssHeight.isSet() && !box.cssHeight.isAuto()) {
        box.height = box.cssHeight.resolve(0, box.fontSize, vpW, vpH);
    }
    
    // Apply min/max width constraints
    if (box.cssMinWidth.isSet() && !box.cssMinWidth.isAuto()) {
        float minW = box.cssMinWidth.resolve(containingWidth, box.fontSize, vpW, vpH);
        if (box.width < minW) box.width = minW;
    }
    if (box.cssMaxWidth.isSet() && !box.cssMaxWidth.isAuto()) {
        float maxW = box.cssMaxWidth.resolve(containingWidth, box.fontSize, vpW, vpH);
        if (box.width > maxW) box.width = maxW;
    }
    
    // Resolve margin: auto (horizontal centering for block elements with explicit width)
    if (box.marginLeftAuto && box.marginRightAuto && box.width > 0 && box.width < containingWidth) {
        float remaining = containingWidth - box.width;
        box.marginLeft = remaining / 2.0f;
        box.marginRight = remaining / 2.0f;
    } else if (box.marginLeftAuto && box.width > 0) {
        box.marginLeft = containingWidth - box.width - box.marginRight;
        if (box.marginLeft < 0) box.marginLeft = 0;
    } else if (box.marginRightAuto && box.width > 0) {
        box.marginRight = containingWidth - box.width - box.marginLeft;
        if (box.marginRight < 0) box.marginRight = 0;
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
    if (contentWidth < 0) contentWidth = 0;
    
    // Flex containers: pre-loop pass handles all children at once
    // Resolve container height BEFORE layout if explicitly set or min-height applies
    // This is critical for flex containers using justify-content centering
    if (box.cssHeight.isSet() && !box.cssHeight.isAuto()) {
        box.height = std::max(box.height, box.cssHeight.resolve(0, box.fontSize, vpW, vpH));
    }
    if (box.cssMinHeight.isSet() && !box.cssMinHeight.isAuto()) {
        box.height = std::max(box.height, box.cssMinHeight.resolve(0, box.fontSize, vpW, vpH));
    }
    
    if (box.type == LayoutType::Flex) {
        float flexGap = box.gap;
        
        // Pre-resolve container height for column flex (needed for justify-content centering)
        // Without this, mainAxisSpace is 0 and centering has no effect
        if (box.flexDirection == 1) {
            float vpW_f = (float)g_width;
            float vpH_f = (float)g_height;
            if (box.cssHeight.isSet() && !box.cssHeight.isAuto()) {
                float h = box.cssHeight.resolve(0, box.fontSize, vpW_f, vpH_f);
                if (h > box.height) box.height = h;
            }
            if (box.cssMinHeight.isSet() && !box.cssMinHeight.isAuto()) {
                float minH = box.cssMinHeight.resolve(0, box.fontSize, vpW_f, vpH_f);
                if (box.height < minH) box.height = minH;
            }
        }
        
        // Pass 1: Measure all flex children
        struct FlexChild {
            LayoutBox* ptr;
            float mainSize;
            float crossSize;
        };
        std::vector<FlexChild> flexChildren;
        float totalMainSize = 0;
        float maxCrossSize = 0;
        int childCount = 0;
        
        for (auto& child : box.children) {
            if (child.type == LayoutType::None) continue;
            
            float childAvailableWidth = contentWidth > 0 ? contentWidth : 200;
            layoutBlock(child, childAvailableWidth, 0);
            
            if (child.width == 0 && !child.text.empty())
                child.width = measureTextWidth(child.text, child.fontSize) + 8;
            if (child.height == 0) child.height = child.fontSize + 8;
            
            float mainSize = (box.flexDirection == 1)
                ? (child.height + child.marginTop + child.marginBottom)
                : (child.width + child.marginLeft + child.marginRight);
            float crossSize = (box.flexDirection == 1)
                ? (child.width + child.marginLeft + child.marginRight)
                : (child.height + child.marginTop + child.marginBottom);
            
            flexChildren.push_back({&child, mainSize, crossSize});
            totalMainSize += mainSize;
            maxCrossSize = std::max(maxCrossSize, crossSize);
            childCount++;
        }
        
        float totalGap = (childCount > 1) ? flexGap * (childCount - 1) : 0;
        float containerMainSize = (box.flexDirection == 1)
            ? (box.height - box.paddingTop - box.paddingBottom - box.borderTop - box.borderBottom)
            : contentWidth;
        if (containerMainSize < 0) containerMainSize = 0;
        float freeSpace = containerMainSize - totalMainSize - totalGap;
        if (freeSpace < 0) freeSpace = 0;
        
        
        
        // Pass 2: Distribute space per justify-content
        float mainOffset = 0;
        float itemSpacing = flexGap;
        
        switch (box.justifyContent) {
            case 1: mainOffset = freeSpace; break; // flex-end
            case 2: mainOffset = freeSpace / 2.0f; break; // center
            case 3: // space-between
                if (childCount > 1) itemSpacing = flexGap + freeSpace / (childCount - 1);
                break;
            case 4: // space-around
                if (childCount > 0) {
                    float pad = freeSpace / (childCount * 2);
                    mainOffset = pad;
                    itemSpacing = flexGap + pad * 2;
                }
                break;
            case 5: // space-evenly
                if (childCount > 0) {
                    float pad = freeSpace / (childCount + 1);
                    mainOffset = pad;
                    itemSpacing = flexGap + pad;
                }
                break;
            default: break;
        }
        
        // Pass 3: Position children
        float cursor = mainOffset;
        for (size_t i = 0; i < flexChildren.size(); i++) {
            auto& fc = flexChildren[i];
            LayoutBox& child = *fc.ptr;
            
            float crossOffset = 0;
            float crossSpace = (box.flexDirection == 1) ? contentWidth : maxCrossSize;
            switch (box.alignItems) {
                case 1: crossOffset = 0; break;
                case 2: crossOffset = crossSpace - fc.crossSize; break;
                case 3: crossOffset = (crossSpace - fc.crossSize) / 2.0f; break;
                default:
                    if (box.flexDirection == 1)
                        child.width = contentWidth - child.marginLeft - child.marginRight;
                    else
                        child.height = maxCrossSize - child.marginTop - child.marginBottom;
                    break;
            }
            
            if (box.flexDirection == 1) {
                child.x = childX + child.marginLeft + crossOffset;
                child.y = box.paddingTop + box.borderTop + cursor + child.marginTop;
                cursor += fc.mainSize;
                if (i < flexChildren.size() - 1) cursor += itemSpacing;
            } else {
                child.x = childX + cursor + child.marginLeft;
                child.y = childY + child.marginTop + crossOffset;
                cursor += fc.mainSize;
                if (i < flexChildren.size() - 1) cursor += itemSpacing;
                lineHeight = std::max(lineHeight, fc.crossSize);
            }
            child.type = LayoutType::FlexItem;
        }
    } else {
    // Block/Inline flow layout
    for (auto& child : box.children) {
        if (child.type == LayoutType::None) continue;
        
        if (child.type == LayoutType::Block || child.type == LayoutType::Flex) {
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
                if (child.width == 0) child.width = 200;
                if (child.height == 0) child.height = std::max(24.0f, child.fontSize + 8);
            } else if (!child.children.empty()) {
                // Inline element with children — recursive layout
                float availWidth = contentWidth > 0 ? contentWidth : containingWidth;
                child.width = availWidth;
                layoutBlock(child, availWidth, 0);
                // Shrink-to-fit: compute content width from children
                float maxChildRight = 0;
                for (const auto& gc : child.children) {
                    float right = gc.x + gc.width + gc.marginRight;
                    maxChildRight = std::max(maxChildRight, right);
                }
                float contentW = maxChildRight + child.paddingRight + child.borderRight;
                if (contentW > 0 && contentW < availWidth)
                    child.width = contentW;
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
    } // end else (block/inline flow)
    
    // Apply text-align: center/right to inline children
    if (box.textAlign > 0 && contentWidth > 0) {
        // Group children into lines by Y position and shift their X
        float currentLineY = -1;
        float lineMaxRight = 0;
        std::vector<LayoutBox*> lineChildren;
        
        auto flushLine = [&]() {
            if (lineChildren.empty()) return;
            float lineContentWidth = lineMaxRight - (box.paddingLeft + box.borderLeft);
            float shift = 0;
            if (box.textAlign == 1) // center
                shift = (contentWidth - lineContentWidth) / 2.0f;
            else if (box.textAlign == 2) // right
                shift = contentWidth - lineContentWidth;
            if (shift > 0) {
                for (auto* lc : lineChildren) {
                    if (lc->type != LayoutType::Block && lc->type != LayoutType::Flex)
                        lc->x += shift;
                }
            }
            lineChildren.clear();
        };
        
        for (auto& child : box.children) {
            if (child.type == LayoutType::None) continue;
            if (child.type == LayoutType::Block || child.type == LayoutType::Flex) {
                flushLine();
                currentLineY = -1;
                continue;
            }
            if (child.y != currentLineY) {
                flushLine();
                currentLineY = child.y;
                lineMaxRight = 0;
            }
            lineChildren.push_back(&child);
            lineMaxRight = std::max(lineMaxRight, child.x + child.width);
        }
        flushLine();
    }
    
    // Add remaining line height
    if (lineHeight > 0) {
        childY += lineHeight;
    }
    
    // Calculate height from children (auto height)
    // For auto-height boxes, use children's computed height but never shrink below
    // a pre-resolved min-height (e.g. flex column pre-resolve sets height from min-height)
    float computedHeight = childY + box.paddingBottom + box.borderBottom;
    if (box.cssHeight.isAuto() || !box.cssHeight.isSet()) {
        if (box.height <= 0) {
            box.height = computedHeight;
        } else {
            box.height = std::max(box.height, computedHeight);
        }
    }
    
    // Apply min/max height constraints
    float vpW2 = (float)g_width;
    float vpH2 = (float)g_height;
    if (box.cssMinHeight.isSet() && !box.cssMinHeight.isAuto()) {
        float minH = box.cssMinHeight.resolve(0, box.fontSize, vpW2, vpH2);
        if (box.height < minH) box.height = minH;
    }
    if (box.cssMaxHeight.isSet() && !box.cssMaxHeight.isAuto()) {
        float maxH = box.cssMaxHeight.resolve(0, box.fontSize, vpW2, vpH2);
        if (box.height > maxH) box.height = maxH;
    }
    
    // If block has no children but has text (leaf block), ensure minimum height
    if (box.type == LayoutType::Block && box.children.empty() && (!box.text.empty() || box.isInput)) {
        box.height = std::max(box.height, box.fontSize + box.paddingTop + box.paddingBottom + 4);
    }
}

// =============================================================================
// PAINTING
// =============================================================================

void paintBox(const LayoutBox& box, float offsetX, float offsetY,
              float viewportHeight, float scrollY) {
    // Budget: limit total painted boxes per frame to prevent UI hang on complex pages
    static thread_local int s_paintCount = 0;
    static thread_local int s_paintDepth = 0;
    if (s_paintDepth == 0) s_paintCount = 0;  // Reset at top-level call
    if (s_paintCount++ > 4000 || s_paintDepth > 64) return;
    s_paintDepth++;
    struct PaintGuard { ~PaintGuard() { --s_paintDepth; } } pg;
    
    if (box.type == LayoutType::None) return;
    if (box.opacity <= 0.001f) return;
    if (box.visibilityHidden) {
        // visibility:hidden — skip painting this box but still paint children
        // (children inherit visibility but can override to visible)
        float screenX2 = offsetX + box.x;
        float screenY2 = offsetY + box.y - scrollY;
        box.screenX = screenX2;
        box.screenY = screenY2;
        for (const auto& child : box.children) {
            paintBox(child, screenX2 + box.paddingLeft + box.borderLeft,
                     screenY2 + box.paddingTop + box.borderTop,
                     viewportHeight, 0);
        }
        return;
    }
    
    float screenX = offsetX + box.x;
    float screenY = offsetY + box.y - scrollY;
    
    // Save for hit testing
    box.screenX = screenX;
    box.screenY = screenY;
    
    // Skip if off-screen (culling)
    if (screenY + box.height < 0 || screenY > viewportHeight) {
        return;
    }
    
    uint8_t alpha = (uint8_t)(box.opacity * 255.0f);
    
    // Draw background (gradient or solid)
    if (box.hasBgColor) {
        if (!box.backgroundImage.empty() && box.backgroundImage.find("gradient") != std::string::npos && s_gfx_gradient) {
            // Parse simple linear-gradient: extract two colors
            // e.g. "linear-gradient(to bottom, #1a1a2e, #16213e)"
            uint32_t c1 = box.bgColor;
            uint32_t c2 = box.bgColor;
            auto extractColors = [](const std::string& grad, uint32_t& out1, uint32_t& out2) {
                // Find hex colors in gradient string
                std::vector<uint32_t> colors;
                size_t pos = 0;
                while (pos < grad.size() && colors.size() < 2) {
                    if (grad[pos] == '#') {
                        pos++;
                        std::string hex;
                        while (pos < grad.size() && std::isxdigit(grad[pos])) {
                            hex += grad[pos++];
                        }
                        if (hex.size() == 3) {
                            hex = std::string(1,hex[0]) + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
                        }
                        if (hex.size() >= 6) {
                            colors.push_back((uint32_t)std::stoul(hex.substr(0,6), nullptr, 16));
                        }
                    } else {
                        pos++;
                    }
                }
                if (colors.size() >= 2) { out1 = colors[0]; out2 = colors[1]; }
                else if (colors.size() == 1) { out1 = colors[0]; out2 = colors[0]; }
            };
            extractColors(box.backgroundImage, c1, c2);
            s_gfx_gradient(screenX, screenY, box.width, box.height, c1, c2);
        } else if (box.borderRadius > 0 && s_gfx_rrect) {
            s_gfx_rrect(screenX, screenY, box.width, box.height, box.borderRadius, box.bgColor, alpha);
        } else if (s_gfx_rect) {
            s_gfx_rect(screenX, screenY, box.width, box.height, box.bgColor);
        }
    }
    
    // Draw border
    if ((box.borderTop > 0 || box.borderRight > 0 || box.borderBottom > 0 || box.borderLeft > 0) && s_gfx_border) {
        float thickness = std::max({box.borderTop, box.borderRight, box.borderBottom, box.borderLeft});
        s_gfx_border(screenX, screenY, box.width, box.height, box.borderColor, thickness);
    }
    
    // Draw image
    if (box.isImage) {
        if (!box.svgData.empty() && s_gfx_svg) {
            // SVG: render via NxSVG on main thread
            float svgSize = std::min(box.width, box.height);
            s_gfx_svg(screenX, screenY, svgSize, box.svgData);
        } else if (box.textureId > 0 && s_gfx_texture) {
            s_gfx_texture(screenX, screenY, box.width, box.height, box.textureId);
        } else if (s_gfx_rect && s_gfx_border) {
            s_gfx_rect(screenX, screenY, box.width, box.height, box.bgColor);
            s_gfx_border(screenX, screenY, box.width, box.height, 0x000000, 1.0f);
        }
    }

    // Draw text content
    if ((!box.text.empty() || !box.placeholder.empty()) && 
        (box.type == LayoutType::Text || box.type == LayoutType::Inline || 
         box.type == LayoutType::Block || box.type == LayoutType::InlineBlock ||
         box.type == LayoutType::FlexItem)) {
        
        float textX = screenX + box.paddingLeft + (box.isInput ? 4 : 0);
        float textY = screenY + box.paddingTop + box.fontSize + (box.isInput ? 2 : 0);
        
        std::string drawText = box.text;
        uint32_t drawColor = box.color;
        
        if (box.isInput && drawText.empty() && !box.placeholder.empty()) {
            drawText = box.placeholder;
            drawColor = 0x999999;
        }
        
        // Apply text-align offset
        if (box.textAlign > 0 && s_text_width && !drawText.empty()) {
            float textW = s_text_width(drawText, box.fontSize);
            float availW = box.width - box.paddingLeft - box.paddingRight;
            if (box.textAlign == 1) textX = screenX + (box.width - textW) / 2.0f;
            else if (box.textAlign == 2) textX = screenX + box.width - box.paddingRight - textW;
        }
        
        if (s_text_render && !drawText.empty()) s_text_render(drawText, textX, textY, drawColor, box.fontSize);
        
        // Text decoration
        if (!drawText.empty()) {
            float textW = s_text_width ? s_text_width(drawText, box.fontSize) : (drawText.length() * box.fontSize * 0.5f);
            
            bool underline = box.isLink || (box.textDecoration.find("underline") != std::string::npos);
            bool lineThrough = (box.textDecoration.find("line-through") != std::string::npos);
            
            if (underline) {
                float lineY = textY + 2;
                if (s_gfx_line) s_gfx_line(textX, lineY, textX + textW, lineY, drawColor, 1.0f);
                else if (s_gfx_rect) s_gfx_rect(textX, lineY, textW, 1.0f, drawColor);
            }
            if (lineThrough) {
                float lineY = textY - box.fontSize * 0.35f;
                if (s_gfx_line) s_gfx_line(textX, lineY, textX + textW, lineY, drawColor, 1.0f);
                else if (s_gfx_rect) s_gfx_rect(textX, lineY, textW, 1.0f, drawColor);
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
                 viewportHeight, 0);
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
