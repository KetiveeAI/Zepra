/**
 * @file style_resolver_adapter.hpp
 * @brief Connects CSS engine to render tree/layout
 */

#pragma once

#include "css/css_engine.hpp"
#include "css/css_computed_style.hpp"
#include "rendering/render_tree.hpp"
#include "webcore/browser/dom.hpp"

namespace Zepra::WebCore {

/**
 * @brief Converts CSS ComputedStyle to render tree ComputedStyle
 * 
 * Bridges the CSS engine's type system to the render tree's
 */
class StyleResolverAdapter {
public:
    /**
     * @brief Convert CSS computed style to render style
     */
    static void applyToRenderNode(RenderNode* renderNode, 
                                   const css::ComputedStyle& cssStyle) {
        if (!renderNode) return;
        
        auto& style = renderNode->style();
        
        // Display
        switch (cssStyle.display) {
            case css::DisplayValue::None: 
                style.display = Display::None; break;
            case css::DisplayValue::Block: 
                style.display = Display::Block; break;
            case css::DisplayValue::Inline: 
                style.display = Display::Inline; break;
            case css::DisplayValue::InlineBlock: 
                style.display = Display::InlineBlock; break;
            case css::DisplayValue::Flex:
            case css::DisplayValue::InlineFlex:
                style.display = Display::Flex; break;
            case css::DisplayValue::Grid:
            case css::DisplayValue::InlineGrid:
                style.display = Display::Grid; break;
            default:
                style.display = Display::Block; break;
        }
        
        // Position
        switch (cssStyle.position) {
            case css::PositionValue::Static:
                style.position = ComputedStyle::Position::Static; break;
            case css::PositionValue::Relative:
                style.position = ComputedStyle::Position::Relative; break;
            case css::PositionValue::Absolute:
                style.position = ComputedStyle::Position::Absolute; break;
            case css::PositionValue::Fixed:
                style.position = ComputedStyle::Position::Fixed; break;
            case css::PositionValue::Sticky:
                style.position = ComputedStyle::Position::Relative; break;  // Fallback
        }
        
        // Dimensions
        style.width = cssStyle.width.value;
        style.height = cssStyle.height.value;
        style.autoWidth = cssStyle.width.isAuto();
        style.autoHeight = cssStyle.height.isAuto();
        
        // Margin
        style.margin.top = cssStyle.marginTop.value;
        style.margin.right = cssStyle.marginRight.value;
        style.margin.bottom = cssStyle.marginBottom.value;
        style.margin.left = cssStyle.marginLeft.value;
        
        // Padding
        style.padding.top = cssStyle.paddingTop.value;
        style.padding.right = cssStyle.paddingRight.value;
        style.padding.bottom = cssStyle.paddingBottom.value;
        style.padding.left = cssStyle.paddingLeft.value;
        
        // Border widths
        style.borderWidth.top = cssStyle.borderTopWidth;
        style.borderWidth.right = cssStyle.borderRightWidth;
        style.borderWidth.bottom = cssStyle.borderBottomWidth;
        style.borderWidth.left = cssStyle.borderLeftWidth;
        
        // Colors
        style.color = Color::fromRGBA(
            cssStyle.color.r, cssStyle.color.g, 
            cssStyle.color.b, cssStyle.color.a);
        style.backgroundColor = Color::fromRGBA(
            cssStyle.backgroundColor.r, cssStyle.backgroundColor.g,
            cssStyle.backgroundColor.b, cssStyle.backgroundColor.a);
        style.borderColor = Color::fromRGBA(
            cssStyle.borderTopColor.r, cssStyle.borderTopColor.g,
            cssStyle.borderTopColor.b, cssStyle.borderTopColor.a);
        
        // Font
        style.fontFamily = cssStyle.fontFamily;
        style.fontSize = cssStyle.fontSize;
        style.fontBold = (static_cast<int>(cssStyle.fontWeight) >= 700);
        style.fontItalic = (cssStyle.fontStyle == css::FontStyle::Italic);
        
        // Positioning offsets
        style.top = cssStyle.top.value;
        style.right = cssStyle.right.value;
        style.bottom = cssStyle.bottom.value;
        style.left = cssStyle.left.value;
        
        // Z-index
        if (!cssStyle.zIndexAuto) {
            style.zIndex = cssStyle.zIndex;
        }
        
        // Overflow
        switch (cssStyle.overflowX) {
            case css::OverflowValue::Visible:
                style.overflow = ComputedStyle::Overflow::Visible; break;
            case css::OverflowValue::Hidden:
                style.overflow = ComputedStyle::Overflow::Hidden; break;
            case css::OverflowValue::Scroll:
                style.overflow = ComputedStyle::Overflow::Scroll; break;
            case css::OverflowValue::Auto:
                style.overflow = ComputedStyle::Overflow::Auto; break;
        }
    }
    
    /**
     * @brief Apply styles from CSS engine to entire render tree
     */
    static void applyStylesRecursive(RenderNode* node, css::CSSEngine& engine) {
        if (!node) return;
        
        DOMNode* domNode = node->domNode();
        if (domNode && domNode->nodeType() == NodeType::Element) {
            auto* element = static_cast<DOMElement*>(domNode);
            if (const auto* cssStyle = engine.getComputedStyle(element)) {
                applyToRenderNode(node, *cssStyle);
            }
        }
        
        for (auto& child : node->children()) {
            applyStylesRecursive(child.get(), engine);
        }
    }
};

} // namespace Zepra::WebCore
