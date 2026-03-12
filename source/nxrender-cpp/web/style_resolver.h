/**
 * @file style_resolver.h
 * @brief NXRender Web Layer - CSS to Theme Style Resolution
 * 
 * Maps CSS computed styles to NXRender theme properties.
 * 
 * @copyright 2024 KetiveeAI
 */

#pragma once

#include <cstdint>
#include <string>

// Forward declarations
namespace Zepra::WebCore {
    class CSSComputedStyle;
    struct CSSColor;
}

namespace NXRender {
    struct Color;
    class Theme;
}

namespace NXRender::Web {

// =============================================================================
// Color Resolution
// =============================================================================

/**
 * @brief Convert CSS color (RGB packed) to NXRender Color
 */
Color cssToNXColor(uint32_t rgb);

/**
 * @brief Convert CSS color struct to NXRender Color
 */
Color cssToNXColor(const Zepra::WebCore::CSSColor& css);

/**
 * @brief Convert NXRender Color to CSS RGB
 */
uint32_t nxColorToCSS(const Color& color);

// =============================================================================
// Font Resolution
// =============================================================================

/**
 * @brief Resolved font properties
 */
struct FontStyle {
    std::string family = "sans-serif";
    float size = 16.0f;
    int weight = 400;       // 400=normal, 700=bold
    bool italic = false;
    float lineHeight = 1.5f;
};

/**
 * @brief Resolve font from CSS computed style
 */
FontStyle resolveFontStyle(const Zepra::WebCore::CSSComputedStyle* style);

// =============================================================================
// Box Model Resolution
// =============================================================================

/**
 * @brief Resolved box model
 */
struct BoxModel {
    // Margin
    float marginTop = 0;
    float marginRight = 0;
    float marginBottom = 0;
    float marginLeft = 0;
    
    // Padding
    float paddingTop = 0;
    float paddingRight = 0;
    float paddingBottom = 0;
    float paddingLeft = 0;
    
    // Border
    float borderWidth = 0;
    uint32_t borderColor = 0;
    float borderRadius = 0;
    
    // Size
    float width = -1;  // -1 = auto
    float height = -1;
};

/**
 * @brief Resolve box model from CSS
 */
BoxModel resolveBoxModel(const Zepra::WebCore::CSSComputedStyle* style);

// =============================================================================
// Display/Layout Resolution
// =============================================================================

enum class DisplayType {
    Block,
    Inline,
    InlineBlock,
    Flex,
    Grid,
    None
};

/**
 * @brief Resolve display type from CSS
 */
DisplayType resolveDisplay(const Zepra::WebCore::CSSComputedStyle* style);

// =============================================================================
// Full Style Resolution
// =============================================================================

/**
 * @brief Complete resolved style
 */
struct ResolvedStyle {
    // Typography
    FontStyle font;
    uint32_t color = 0x000000;
    
    // Background
    uint32_t backgroundColor = 0xFFFFFF;
    bool hasBackground = false;
    
    // Box model
    BoxModel box;
    
    // Layout
    DisplayType display = DisplayType::Block;
    
    // Visibility
    bool visible = true;
};

/**
 * @brief Resolve all styles from CSS
 */
ResolvedStyle resolveAllStyles(const Zepra::WebCore::CSSComputedStyle* style);

} // namespace NXRender::Web
