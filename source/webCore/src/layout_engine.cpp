/**
 * @file layout_engine.cpp
 * @brief Layout engine implementation
 */

#include "webcore/layout_engine.hpp"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace Zepra::WebCore {

// =============================================================================
// LayoutConstraints
// =============================================================================

LayoutConstraints LayoutConstraints::tight(float width, float height) {
    return {width, width, height, height};
}

LayoutConstraints LayoutConstraints::loose(float maxWidth, float maxHeight) {
    return {0, maxWidth, 0, maxHeight};
}

// =============================================================================
// LayoutEngine
// =============================================================================

void LayoutEngine::layout(RenderNode* root, float viewportWidth, float viewportHeight) {
    viewportWidth_ = viewportWidth;
    viewportHeight_ = viewportHeight;
    
    if (!root) return;
    
    LayoutConstraints constraints;
    constraints.minWidth = 0;
    constraints.maxWidth = viewportWidth;
    constraints.minHeight = 0;
    constraints.maxHeight = viewportHeight;
    
    layoutNode(root, constraints);
}

void LayoutEngine::layoutNode(RenderNode* node, const LayoutConstraints& constraints) {
    if (!node) return;
    
    auto& style = node->style();
    
    // Skip hidden elements
    if (style.display == Display::None) {
        return;
    }
    
    // Handle positioned elements (absolute/fixed)
    if (style.position == ComputedStyle::Position::Absolute ||
        style.position == ComputedStyle::Position::Fixed) {
        layoutPositioned(node, constraints);
        return;
    }
    
    // Normal layout based on display
    switch (style.display) {
        case Display::Block:
            layoutBlock(node, constraints);
            break;
        case Display::Inline:
        case Display::InlineBlock:
            layoutInline(node, constraints);
            break;
        case Display::Flex:
            layoutFlex(node, constraints);
            break;
        case Display::Grid:
            layoutGrid(node, constraints);
            break;
        case Display::None:
            return;
    }
    
    // Apply relative positioning offset
    if (style.position == ComputedStyle::Position::Relative) {
        auto& box = const_cast<BoxModel&>(node->boxModel());
        box.contentBox.x += style.left - style.right;
        box.contentBox.y += style.top - style.bottom;
    }
}

void LayoutEngine::layoutBlock(RenderNode* node, const LayoutConstraints& constraints) {
    auto& style = node->style();
    auto& box = const_cast<BoxModel&>(node->boxModel());
    
    // Resolve width
    float width;
    if (style.autoWidth) {
        width = constraints.maxWidth - style.margin.horizontal();
    } else {
        width = std::clamp(style.width, constraints.minWidth, constraints.maxWidth);
    }
    width -= style.padding.horizontal() + style.borderWidth.horizontal();
    
    box.contentBox.width = std::max(0.0f, width);
    box.margin = style.margin;
    box.padding = style.padding;
    box.border = style.borderWidth;
    
    // Layout children (separate flows for positioned vs normal)
    float childY = 0;
    float maxChildWidth = 0;
    std::vector<RenderNode*> positionedChildren;
    
    for (auto& child : node->children()) {
        auto& childStyle = child->style();
        
        // Collect absolutely positioned children for later
        if (childStyle.position == ComputedStyle::Position::Absolute ||
            childStyle.position == ComputedStyle::Position::Fixed) {
            positionedChildren.push_back(child.get());
            continue;
        }
        
        LayoutConstraints childConstraints;
        childConstraints.minWidth = 0;
        childConstraints.maxWidth = box.contentBox.width;
        childConstraints.minHeight = 0;
        childConstraints.maxHeight = constraints.maxHeight;
        
        layoutNode(child.get(), childConstraints);
        
        // Position child
        auto& childBox = const_cast<BoxModel&>(child->boxModel());
        childBox.contentBox.x = box.contentBox.x + box.padding.left + childBox.margin.left;
        childBox.contentBox.y = box.contentBox.y + box.padding.top + childY + childBox.margin.top;
        
        childY += childBox.marginBox().height;
        maxChildWidth = std::max(maxChildWidth, childBox.marginBox().width);
    }
    
    // Resolve height
    if (style.autoHeight) {
        box.contentBox.height = childY;
    } else {
        box.contentBox.height = std::clamp(style.height, constraints.minHeight, constraints.maxHeight);
        box.contentBox.height -= style.padding.vertical() + style.borderWidth.vertical();
    }
    
    // Now layout positioned children
    for (auto* child : positionedChildren) {
        LayoutConstraints posConstraints;
        posConstraints.minWidth = 0;
        posConstraints.maxWidth = box.contentBox.width;
        posConstraints.minHeight = 0;
        posConstraints.maxHeight = box.contentBox.height;
        layoutPositioned(child, posConstraints);
    }
}

void LayoutEngine::layoutInline(RenderNode* node, const LayoutConstraints& constraints) {
    auto& style = node->style();
    auto& box = const_cast<BoxModel&>(node->boxModel());
    
    box.margin = style.margin;
    box.padding = style.padding;
    box.border = style.borderWidth;
    
    // Inline elements shrink-wrap content
    float contentWidth = 0;
    float contentHeight = 0;
    float x = 0;
    float lineHeight = style.fontSize * 1.2f;
    
    for (auto& child : node->children()) {
        LayoutConstraints childConstraints;
        childConstraints.minWidth = 0;
        childConstraints.maxWidth = constraints.maxWidth - x;
        childConstraints.minHeight = 0;
        childConstraints.maxHeight = constraints.maxHeight;
        
        layoutNode(child.get(), childConstraints);
        
        auto& childBox = const_cast<BoxModel&>(child->boxModel());
        
        // Check for line wrap
        if (x + childBox.marginBox().width > constraints.maxWidth && x > 0) {
            x = 0;
            contentHeight += lineHeight;
        }
        
        childBox.contentBox.x = box.contentBox.x + box.padding.left + x;
        childBox.contentBox.y = box.contentBox.y + box.padding.top + contentHeight;
        
        x += childBox.marginBox().width;
        contentWidth = std::max(contentWidth, x);
    }
    
    contentHeight += lineHeight;
    
    box.contentBox.width = style.autoWidth ? contentWidth : style.width;
    box.contentBox.height = style.autoHeight ? contentHeight : style.height;
}

void LayoutEngine::layoutFlex(RenderNode* node, const LayoutConstraints& constraints) {
    auto& style = node->style();
    auto& box = const_cast<BoxModel&>(node->boxModel());
    
    box.margin = style.margin;
    box.padding = style.padding;
    box.border = style.borderWidth;
    
    float width = style.autoWidth ? constraints.maxWidth : style.width;
    width -= box.padding.horizontal() + box.border.horizontal();
    box.contentBox.width = std::max(0.0f, width);
    
    // Calculate total flex-grow and fixed sizes
    float totalFlexGrow = 0;
    float totalFixedWidth = 0;
    size_t flexItemCount = 0;
    
    for (auto& child : node->children()) {
        auto& childStyle = child->style();
        if (childStyle.position == ComputedStyle::Position::Absolute ||
            childStyle.position == ComputedStyle::Position::Fixed) {
            continue;
        }
        
        flexItemCount++;
        // Simple: assume flex-grow = 1 for now
        totalFlexGrow += 1.0f;
    }
    
    // Distribute space
    float remainingSpace = box.contentBox.width - totalFixedWidth;
    float flexUnit = (totalFlexGrow > 0) ? remainingSpace / totalFlexGrow : 0;
    
    float x = 0;
    float maxHeight = 0;
    
    for (auto& child : node->children()) {
        auto& childStyle = child->style();
        if (childStyle.position == ComputedStyle::Position::Absolute ||
            childStyle.position == ComputedStyle::Position::Fixed) {
            continue;
        }
        
        float childWidth = flexUnit;  // flex-grow = 1
        
        LayoutConstraints childConstraints;
        childConstraints.minWidth = 0;
        childConstraints.maxWidth = childWidth;
        childConstraints.minHeight = 0;
        childConstraints.maxHeight = constraints.maxHeight;
        
        layoutNode(child.get(), childConstraints);
        
        auto& childBox = const_cast<BoxModel&>(child->boxModel());
        childBox.contentBox.x = box.contentBox.x + box.padding.left + x;
        childBox.contentBox.y = box.contentBox.y + box.padding.top;
        
        x += childBox.marginBox().width;
        maxHeight = std::max(maxHeight, childBox.marginBox().height);
    }
    
    box.contentBox.height = style.autoHeight ? maxHeight : style.height;
}

void LayoutEngine::layoutGrid(RenderNode* node, const LayoutConstraints& constraints) {
    auto& style = node->style();
    auto& box = const_cast<BoxModel&>(node->boxModel());
    
    box.margin = style.margin;
    box.padding = style.padding;
    box.border = style.borderWidth;
    
    float width = style.autoWidth ? constraints.maxWidth : style.width;
    width -= box.padding.horizontal() + box.border.horizontal();
    box.contentBox.width = std::max(0.0f, width);
    
    // Simple grid: auto-fit columns based on child count
    size_t childCount = 0;
    for (auto& child : node->children()) {
        auto& childStyle = child->style();
        if (childStyle.position != ComputedStyle::Position::Absolute &&
            childStyle.position != ComputedStyle::Position::Fixed) {
            childCount++;
        }
    }
    
    if (childCount == 0) {
        box.contentBox.height = 0;
        return;
    }
    
    // Calculate columns (simple: square-ish grid)
    int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(childCount))));
    columns = std::max(1, columns);
    
    float cellWidth = box.contentBox.width / columns;
    float gap = 8.0f;  // Default gap
    
    int col = 0;
    int row = 0;
    float maxRowHeight = 0;
    float totalHeight = 0;
    
    for (auto& child : node->children()) {
        auto& childStyle = child->style();
        if (childStyle.position == ComputedStyle::Position::Absolute ||
            childStyle.position == ComputedStyle::Position::Fixed) {
            continue;
        }
        
        LayoutConstraints childConstraints;
        childConstraints.minWidth = 0;
        childConstraints.maxWidth = cellWidth - gap;
        childConstraints.minHeight = 0;
        childConstraints.maxHeight = constraints.maxHeight;
        
        layoutNode(child.get(), childConstraints);
        
        auto& childBox = const_cast<BoxModel&>(child->boxModel());
        childBox.contentBox.x = box.contentBox.x + box.padding.left + col * cellWidth;
        childBox.contentBox.y = box.contentBox.y + box.padding.top + totalHeight;
        
        maxRowHeight = std::max(maxRowHeight, childBox.marginBox().height);
        
        col++;
        if (col >= columns) {
            col = 0;
            row++;
            totalHeight += maxRowHeight + gap;
            maxRowHeight = 0;
        }
    }
    
    if (col > 0) {
        totalHeight += maxRowHeight;
    }
    
    box.contentBox.height = style.autoHeight ? totalHeight : style.height;
}

void LayoutEngine::layoutPositioned(RenderNode* node, const LayoutConstraints& constraints) {
    auto& style = node->style();
    auto& box = const_cast<BoxModel&>(node->boxModel());
    
    box.margin = style.margin;
    box.padding = style.padding;
    box.border = style.borderWidth;
    
    // Calculate position based on top/right/bottom/left
    float containerWidth = constraints.maxWidth;
    float containerHeight = constraints.maxHeight;
    
    // For fixed positioning, use viewport
    if (style.position == ComputedStyle::Position::Fixed) {
        containerWidth = viewportWidth_;
        containerHeight = viewportHeight_;
    }
    
    // Resolve width
    if (style.autoWidth) {
        // Width is determined by left/right or content
        if (style.left != 0 && style.right != 0) {
            box.contentBox.width = containerWidth - style.left - style.right 
                                   - box.padding.horizontal() - box.border.horizontal();
        } else {
            box.contentBox.width = computeIntrinsicWidth(node);
        }
    } else {
        box.contentBox.width = style.width - box.padding.horizontal() - box.border.horizontal();
    }
    box.contentBox.width = std::max(0.0f, box.contentBox.width);
    
    // Resolve height
    if (style.autoHeight) {
        if (style.top != 0 && style.bottom != 0) {
            box.contentBox.height = containerHeight - style.top - style.bottom
                                    - box.padding.vertical() - box.border.vertical();
        } else {
            box.contentBox.height = computeIntrinsicHeight(node, box.contentBox.width);
        }
    } else {
        box.contentBox.height = style.height - box.padding.vertical() - box.border.vertical();
    }
    box.contentBox.height = std::max(0.0f, box.contentBox.height);
    
    // Calculate position
    if (style.left != 0) {
        box.contentBox.x = style.left + box.margin.left;
    } else if (style.right != 0) {
        box.contentBox.x = containerWidth - style.right - box.marginBox().width;
    } else {
        box.contentBox.x = box.margin.left;
    }
    
    if (style.top != 0) {
        box.contentBox.y = style.top + box.margin.top;
    } else if (style.bottom != 0) {
        box.contentBox.y = containerHeight - style.bottom - box.marginBox().height;
    } else {
        box.contentBox.y = box.margin.top;
    }
    
    // Layout children within this positioned element
    for (auto& child : node->children()) {
        LayoutConstraints childConstraints;
        childConstraints.minWidth = 0;
        childConstraints.maxWidth = box.contentBox.width;
        childConstraints.minHeight = 0;
        childConstraints.maxHeight = box.contentBox.height;
        
        layoutNode(child.get(), childConstraints);
    }
}

void LayoutEngine::layoutText(RenderText* text, float maxWidth) {
    if (!text) return;
    text->layout(maxWidth);
}

float LayoutEngine::computeIntrinsicWidth(RenderNode* node) {
    if (!node) return 0;
    
    if (node->isText()) {
        auto* text = static_cast<RenderText*>(node);
        return text->text().length() * node->style().fontSize * 0.6f;
    }
    
    float maxWidth = 0;
    for (auto& child : node->children()) {
        maxWidth = std::max(maxWidth, computeIntrinsicWidth(child.get()));
    }
    
    return maxWidth + node->style().padding.horizontal() + node->style().borderWidth.horizontal();
}

float LayoutEngine::computeIntrinsicHeight(RenderNode* node, float width) {
    if (!node) return 0;
    
    if (node->isText()) {
        auto* text = static_cast<RenderText*>(node);
        float charWidth = node->style().fontSize * 0.6f;
        float lineHeight = node->style().fontSize * 1.2f;
        float chars = text->text().length() * charWidth;
        int lines = static_cast<int>(std::ceil(chars / width));
        return lines * lineHeight;
    }
    
    float totalHeight = 0;
    for (auto& child : node->children()) {
        totalHeight += computeIntrinsicHeight(child.get(), width);
    }
    
    return totalHeight + node->style().padding.vertical() + node->style().borderWidth.vertical();
}

float LayoutEngine::resolveLength(float value, float percentBase, bool isAuto) {
    if (isAuto) return 0;
    return value;
}

// =============================================================================
// SimpleTextMeasurer
// =============================================================================

float SimpleTextMeasurer::measureWidth(const std::string& text, 
                                        const std::string& fontFamily,
                                        float fontSize, bool bold) {
    (void)fontFamily;
    float charWidth = fontSize * (bold ? 0.65f : 0.6f);
    return text.length() * charWidth;
}

FontMetrics SimpleTextMeasurer::getFontMetrics(const std::string& fontFamily,
                                                float fontSize, bool bold) {
    (void)fontFamily;
    (void)bold;
    FontMetrics m;
    m.ascent = fontSize * 0.8f;
    m.descent = fontSize * 0.2f;
    m.lineHeight = fontSize * 1.2f;
    m.xHeight = fontSize * 0.5f;
    m.capHeight = fontSize * 0.7f;
    return m;
}

std::vector<std::string> SimpleTextMeasurer::breakIntoLines(const std::string& text,
                                                             const std::string& fontFamily,
                                                             float fontSize, bool bold,
                                                             float maxWidth) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word;
    std::string currentLine;
    
    while (stream >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        float testWidth = measureWidth(testLine, fontFamily, fontSize, bold);
        
        if (testWidth <= maxWidth || currentLine.empty()) {
            currentLine = testLine;
        } else {
            lines.push_back(currentLine);
            currentLine = word;
        }
    }
    
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    return lines;
}

} // namespace Zepra::WebCore
