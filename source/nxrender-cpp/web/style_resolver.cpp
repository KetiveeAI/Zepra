/**
 * @file style_resolver.cpp
 * @brief NXRender Web Layer - CSS Style Resolution
 * 
 * @copyright 2024 KetiveeAI
 */

#include "web/style_resolver.h"
#include "nxgfx/color.h"

#ifdef USE_WEBCORE
#include "webcore/css/css_types.hpp"
#endif

namespace NXRender::Web {

// =============================================================================
// Color Resolution
// =============================================================================

Color cssToNXColor(uint32_t rgb) {
    return Color{
        static_cast<uint8_t>((rgb >> 16) & 0xFF),
        static_cast<uint8_t>((rgb >> 8) & 0xFF),
        static_cast<uint8_t>(rgb & 0xFF),
        255
    };
}

#ifdef USE_WEBCORE
Color cssToNXColor(const Zepra::WebCore::CSSColor& css) {
    return Color{css.r, css.g, css.b, css.a};
}
#endif

uint32_t nxColorToCSS(const Color& color) {
    return (color.r << 16) | (color.g << 8) | color.b;
}

// =============================================================================
// Font Resolution
// =============================================================================

#ifdef USE_WEBCORE
FontStyle resolveFontStyle(const Zepra::WebCore::CSSComputedStyle* style) {
    FontStyle font;
    
    if (style) {
        font.size = style->fontSize;
        font.weight = static_cast<int>(style->fontWeight);
        font.italic = (style->fontStyle == Zepra::WebCore::FontStyle::Italic);
        font.lineHeight = style->lineHeight > 0 ? style->lineHeight : 1.5f;
        
        // Font family resolution
        if (!style->fontFamily.empty()) {
            font.family = style->fontFamily;
        }
    }
    
    return font;
}
#else
FontStyle resolveFontStyle(const void*) {
    return FontStyle{};
}
#endif

// =============================================================================
// Box Model Resolution
// =============================================================================

#ifdef USE_WEBCORE
BoxModel resolveBoxModel(const Zepra::WebCore::CSSComputedStyle* style) {
    BoxModel box;
    
    if (style) {
        // Margins
        box.marginTop = style->marginTop.value;
        box.marginRight = style->marginRight.value;
        box.marginBottom = style->marginBottom.value;
        box.marginLeft = style->marginLeft.value;
        
        // Padding
        box.paddingTop = style->paddingTop.value;
        box.paddingRight = style->paddingRight.value;
        box.paddingBottom = style->paddingBottom.value;
        box.paddingLeft = style->paddingLeft.value;
        
        // Border
        box.borderWidth = style->borderWidth;
        
        // Size
        if (style->width.unit != Zepra::WebCore::CSSUnit::Auto) {
            box.width = style->width.value;
        }
        if (style->height.unit != Zepra::WebCore::CSSUnit::Auto) {
            box.height = style->height.value;
        }
    }
    
    return box;
}
#else
BoxModel resolveBoxModel(const void*) {
    return BoxModel{};
}
#endif

// =============================================================================
// Display Resolution
// =============================================================================

#ifdef USE_WEBCORE
DisplayType resolveDisplay(const Zepra::WebCore::CSSComputedStyle* style) {
    if (!style) return DisplayType::Block;
    
    switch (style->display) {
        case Zepra::WebCore::DisplayValue::None:
            return DisplayType::None;
        case Zepra::WebCore::DisplayValue::Inline:
            return DisplayType::Inline;
        case Zepra::WebCore::DisplayValue::InlineBlock:
            return DisplayType::InlineBlock;
        case Zepra::WebCore::DisplayValue::Flex:
            return DisplayType::Flex;
        case Zepra::WebCore::DisplayValue::Grid:
            return DisplayType::Grid;
        case Zepra::WebCore::DisplayValue::Block:
        default:
            return DisplayType::Block;
    }
}
#else
DisplayType resolveDisplay(const void*) {
    return DisplayType::Block;
}
#endif

// =============================================================================
// Full Style Resolution
// =============================================================================

#ifdef USE_WEBCORE
ResolvedStyle resolveAllStyles(const Zepra::WebCore::CSSComputedStyle* style) {
    ResolvedStyle resolved;
    
    if (style) {
        // Typography
        resolved.font = resolveFontStyle(style);
        resolved.color = (style->color.r << 16) | 
                         (style->color.g << 8) | 
                         style->color.b;
        
        // Background
        if (!style->backgroundColor.isTransparent()) {
            resolved.hasBackground = true;
            resolved.backgroundColor = (style->backgroundColor.r << 16) | 
                                        (style->backgroundColor.g << 8) | 
                                        style->backgroundColor.b;
        }
        
        // Box model
        resolved.box = resolveBoxModel(style);
        
        // Layout
        resolved.display = resolveDisplay(style);
        
        // Visibility
        resolved.visible = (style->display != Zepra::WebCore::DisplayValue::None);
    }
    
    return resolved;
}
#else
ResolvedStyle resolveAllStyles(const void*) {
    return ResolvedStyle{};
}
#endif

} // namespace NXRender::Web
