// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * layout_engine.cpp - CSS Layout Engine Implementation
 */

#include "layout_engine.h"
#include <algorithm>
#include <iostream>
#include <cfloat>

#include "../css/css_computed_style.hpp"

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
static void (*s_gfx_svg)(float, float, float, float, const std::string&) = nullptr;

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
    void (*gfx_svg)(float, float, float, float, const std::string&)
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
        if (box.boxSizing == 0) {
            box.width += box.paddingLeft + box.paddingRight + box.borderLeft + box.borderRight;
        }
    } else if (box.type == LayoutType::Block || box.type == LayoutType::Flex) {
        // Block and Flex containers fill their containing block width (CSS2 §10.3.3)
        float w = containingWidth - box.marginLeft - box.marginRight;
        box.width = w > 0 ? w : containingWidth;
    }
    
    // Resolve deferred CSS height (container height is 0 for now — auto)
    if (box.cssHeight.isSet() && !box.cssHeight.isAuto()) {
        box.height = box.cssHeight.resolve(0, box.fontSize, vpW, vpH);
        if (box.boxSizing == 0) {
            box.height += box.paddingTop + box.paddingBottom + box.borderTop + box.borderBottom;
        }
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
        
        struct FlexChild {
            LayoutBox* ptr;
            float mainSize;
            float crossSize;
        };
        
        struct FlexLine {
            std::vector<FlexChild> items;
            float mainSize = 0;
            float crossSize = 0;
            float freeSpace = 0;
        };
        std::vector<FlexLine> lines;
        lines.push_back(FlexLine());
        
        float containerMainSize = (box.flexDirection == 1)
            ? (box.height - box.paddingTop - box.paddingBottom - box.borderTop - box.borderBottom)
            : contentWidth;
        if (containerMainSize < 0) containerMainSize = 0;
        
        // Pass 1: Measure all flex children and partition into lines
        for (auto& child : box.children) {
            if (child.type == LayoutType::None) continue;
            
            float childAvailableWidth = contentWidth > 0 ? contentWidth : 200;
            
            // Flex items in a row normally shrink-to-fit unless width is set
            LayoutType oldType = child.type;
            if (box.flexDirection == 0 && (child.type == LayoutType::Block || child.type == LayoutType::Flex) && (!child.cssWidth.isSet() || child.cssWidth.isAuto())) {
                 child.type = LayoutType::InlineBlock; // Force shrink-to-fit measurement
            }
            
            layoutBlock(child, childAvailableWidth, 0);
            child.type = oldType;
            
            if (child.width == 0 && !child.text.empty())
                child.width = measureTextWidth(child.text, child.fontSize) + 8;
            if (child.height == 0) child.height = child.fontSize + 8;
            
            float mainSize = (box.flexDirection == 1)
                ? (child.height + child.marginTop + child.marginBottom)
                : (child.width + child.marginLeft + child.marginRight);
            float crossSize = (box.flexDirection == 1)
                ? (child.width + child.marginLeft + child.marginRight)
                : (child.height + child.marginTop + child.marginBottom);
            
            FlexLine& currentLine = lines.back();
            float gapRequired = currentLine.items.empty() ? 0 : flexGap;
            
            // Check if we need to wrap
            if (box.flexWrap && !currentLine.items.empty() && 
                currentLine.mainSize + gapRequired + mainSize > containerMainSize) {
                lines.push_back(FlexLine());
                FlexLine& newLine = lines.back();
                newLine.items.push_back({&child, mainSize, crossSize});
                newLine.mainSize = mainSize;
                newLine.crossSize = crossSize;
            } else {
                currentLine.items.push_back({&child, mainSize, crossSize});
                currentLine.mainSize += gapRequired + mainSize;
                currentLine.crossSize = std::max(currentLine.crossSize, crossSize);
            }
        }
        
        // Pass 2 & 3: Iterate over lines, distribute main-axis space, position items
        float crossCursor = 0; // Tracks vertical progression of wrapped lines (or horizontal if column)
        
        for (auto& line : lines) {
            if (line.items.empty()) continue;
            
            line.freeSpace = containerMainSize - line.mainSize;
            if (line.freeSpace < 0) line.freeSpace = 0;
            
            float mainOffset = 0;
            float itemSpacing = flexGap;
            int childCount = line.items.size();
            
            switch (box.justifyContent) {
                case 1: mainOffset = line.freeSpace; break; // flex-end
                case 2: mainOffset = line.freeSpace / 2.0f; break; // center
                case 3: // space-between
                    if (childCount > 1) itemSpacing = flexGap + line.freeSpace / (childCount - 1);
                    break;
                case 4: // space-around
                    if (childCount > 0) {
                        float pad = line.freeSpace / (childCount * 2);
                        mainOffset = pad;
                        itemSpacing = flexGap + pad * 2;
                    }
                    break;
                case 5: // space-evenly
                    if (childCount > 0) {
                        float pad = line.freeSpace / (childCount + 1);
                        mainOffset = pad;
                        itemSpacing = flexGap + pad;
                    }
                    break;
                default: break;
            }
            
            float cursor = mainOffset;
            for (size_t i = 0; i < line.items.size(); i++) {
                auto& fc = line.items[i];
                LayoutBox& child = *fc.ptr;
                
                float crossOffset = 0;
                float crossSpace = (box.flexDirection == 1) ? contentWidth : line.crossSize;
                switch (box.alignItems) {
                    case 1: crossOffset = 0; break;
                    case 2: crossOffset = crossSpace - fc.crossSize; break;
                    case 3: crossOffset = (crossSpace - fc.crossSize) / 2.0f; break;
                    default:
                        if (box.flexDirection == 1)
                            child.width = contentWidth - child.marginLeft - child.marginRight;
                        else
                            child.height = line.crossSize - child.marginTop - child.marginBottom;
                        break;
                }
                
                if (box.flexDirection == 1) {
                    child.x = childX + child.marginLeft + crossOffset + crossCursor;
                    child.y = box.paddingTop + box.borderTop + cursor + child.marginTop;
                    cursor += fc.mainSize;
                    if (i < line.items.size() - 1) cursor += itemSpacing;
                } else {
                    child.x = childX + cursor + child.marginLeft;
                    child.y = childY + child.marginTop + crossOffset + crossCursor;
                    cursor += fc.mainSize;
                    if (i < line.items.size() - 1) cursor += itemSpacing;
                }
                child.type = LayoutType::FlexItem;
            }
            
            // Advance cross cursor for next line
            crossCursor += line.crossSize + flexGap;
        }
        
        if (box.flexDirection == 0) {
            lineHeight = std::max(lineHeight, crossCursor > 0 ? crossCursor - flexGap : 0);
        }
    } else {
    // Block/Inline flow layout
    float prevBlockMarginBottom = 0; // For margin collapsing (CSS2 §8.3.1)
    for (auto& child : box.children) {
        if (child.type == LayoutType::None) continue;
        
        if (child.type == LayoutType::Block || child.type == LayoutType::Flex) {
            // Flush any inline content first
            if (lineHeight > 0) {
                childY += lineHeight;
                lineHeight = 0;
                lineWidth = 0;
                prevBlockMarginBottom = 0;
            }
            
            // Margin collapsing: adjacent block margins collapse to max(prev_bottom, cur_top)
            // instead of summing (CSS2 §8.3.1, WebKit RenderBlockFlow::MarginValues)
            float effectiveMarginTop = child.marginTop;
            if (prevBlockMarginBottom > 0 && effectiveMarginTop > 0) {
                float collapsed = std::max(prevBlockMarginBottom, effectiveMarginTop);
                childY -= prevBlockMarginBottom; // undo prev bottom margin
                child.marginTop = collapsed;     // apply collapsed margin
            } else if (prevBlockMarginBottom < 0 && effectiveMarginTop < 0) {
                float collapsed = std::min(prevBlockMarginBottom, effectiveMarginTop);
                childY -= prevBlockMarginBottom;
                child.marginTop = collapsed;
            }
            
            layoutBlock(child, contentWidth, childY);
            child.x += box.paddingLeft + box.borderLeft;
            
            childY = child.y + child.height + child.marginBottom;
            prevBlockMarginBottom = child.marginBottom;
            
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
            
            child.x = childX + lineWidth + child.marginLeft;
            child.y = childY + child.marginTop;
            
            lineWidth += child.width + child.marginLeft + child.marginRight + 4; // Spacing
            lineHeight = std::max(lineHeight, child.height + child.marginTop + child.marginBottom);
        }
    }
    
    // Add height of the last inline line
    if (lineHeight > 0) {
        childY += lineHeight;
        lineHeight = 0;
    }
    } // end else (block/inline flow)
    
    // Shrink-to-fit for InlineBlock wrappers (MUST be before text-align so we don't include huge X shifts)
    if (box.type == LayoutType::InlineBlock || box.type == LayoutType::Inline) {
        float maxRight = 0;
        for (const auto& child : box.children) {
            maxRight = std::max(maxRight, child.x + child.width + child.marginRight);
        }
        float tightWidth = maxRight + box.paddingRight + box.borderRight;
        if (tightWidth > 0 && tightWidth < box.width) {
            box.width = tightWidth;
            // Also need to update contentWidth so text-align uses the new smaller width!
            contentWidth = box.width - box.paddingLeft - box.paddingRight - box.borderLeft - box.borderRight;
        }
    }
    
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
    

    if (box.type == LayoutType::Inline || box.type == LayoutType::InlineBlock) {
        printf("[Layout Debug] Inline element id=%d width=%f height=%f x=%f y=%f\n", 
            (int)box.type, box.width, box.height, (double)box.x, (double)box.y);
    }
}

// =============================================================================
// PAINTING
// =============================================================================

static void dumpLayoutTree(const LayoutBox& box, int depth = 0) {
    static FILE* fp = nullptr;
    if (depth == 0) fp = fopen("/tmp/layout_dump.txt", "w");
    if (fp) {
        for (int i=0; i<depth; i++) fprintf(fp, "  ");
        fprintf(fp, "Type:%d xy=(%.1f, %.1f) wh=(%.1f, %.1f) text='%s'\n", 
            (int)box.type, box.x, box.y, box.width, box.height, box.text.c_str());
        for (const auto& c : box.children) dumpLayoutTree(c, depth + 1);
        if (depth == 0) { fclose(fp); fp = nullptr; }
    }
}

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
            paintBox(child, screenX2,
                     screenY2,
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
            std::string grad = box.backgroundImage;
            bool isConic = grad.find("conic-gradient") != std::string::npos;
            
            std::vector<uint32_t> colors;
            
            // Simple robust linear parser allowing any CSS color
            size_t startPos = grad.find('(');
            size_t endPos = grad.rfind(')');
            if (startPos != std::string::npos && endPos != std::string::npos && endPos > startPos) {
                std::string inner = grad.substr(startPos + 1, endPos - startPos - 1);
                
                size_t p = 0;
                while (p < inner.length()) {
                    while (p < inner.length() && std::isspace(inner[p])) p++;
                    size_t nextComma = inner.find(',', p);
                    
                    // Skip commas inside rgb() or hsl()
                    size_t scope = 0;
                    for (size_t i = p; i < inner.length(); i++) {
                        if (inner[i] == '(') scope++;
                        else if (inner[i] == ')') scope--;
                        else if (inner[i] == ',' && scope == 0) {
                            nextComma = i;
                            break;
                        }
                    }
                    if (scope != 0 || nextComma == std::string::npos) nextComma = inner.length();
                    
                    std::string token = inner.substr(p, nextComma - p);
                    
                    // Trim token
                    while (!token.empty() && std::isspace(token.back())) token.pop_back();
                    while (!token.empty() && std::isspace(token.front())) token.erase(token.begin());
                    
                    if (!token.empty() && token.find("to ") == std::string::npos && token.find("deg") == std::string::npos) {
                        Zepra::WebCore::CSSColor col = Zepra::WebCore::CSSColor::parse(token);
                        // Actually Zepra colors are RGB in UI but s_gfx uses 0xRRGGBB
                        colors.push_back((col.r << 16) | (col.g << 8) | col.b);
                    }
                    p = nextComma + 1;
                }
            }
            
            if (isConic) {
                // Fallback for conic gradients (not natively supported by s_gfx)
                s_gfx_rect(screenX, screenY, box.width, box.height, colors.empty() ? box.bgColor : colors[0]);
            } else {
                if (colors.size() >= 2) {
                    float chunkW = box.width / (colors.size() - 1);
                    float curX = screenX;
                    for (size_t i = 0; i < colors.size() - 1; i++) {
                         s_gfx_gradient(curX, screenY, chunkW + 1, box.height, colors[i], colors[i+1]);
                         curX += chunkW;
                    }
                } else if (colors.size() == 1) {
                    s_gfx_rect(screenX, screenY, box.width, box.height, colors[0]);
                } else {
                    s_gfx_rect(screenX, screenY, box.width, box.height, box.bgColor);
                }
            }
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
            s_gfx_svg(screenX, screenY, box.width, box.height, box.svgData);
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
        
        if (box.textAlign > 0 && s_text_width && !drawText.empty()) {
            float textW = s_text_width(drawText, box.fontSize);
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
        paintBox(child, screenX,
                 screenY,
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
