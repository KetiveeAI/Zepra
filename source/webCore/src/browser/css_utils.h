/**
 * css_utils.h - CSS Processing Utilities for ZepraBrowser
 * 
 * Contains helpers for:
 * - Expanding CSS variables (var(--name))
 * - Polyfilling shorthands (background -> background-color)
 * - Extracting styles from raw HTML (parser workaround)
 * - Color conversion
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <css/css_computed_style.hpp>

namespace ZepraBrowser {

/**
 * @brief Convert WebCore CSSColor to uint32_t RGB (0xRRGGBB)
 */
uint32_t cssColorToRGB(const Zepra::WebCore::CSSColor& c);

/**
 * @brief Preprocess CSS to expand CSS variables (var(--name))
 * LibWebCore doesn't support CSS variables natively yet.
 */
std::string expandCSSVariables(const std::string& css);

/**
 * @brief Polyfill CSS shorthands (background -> background-color, etc.)
 */
std::string polyfillCSSShorthands(const std::string& css);

/**
 * @brief Extract CSS from raw HTML <style> tags
 * Workaround for HTMLParser issue where styles aren't added to head.
 * Includes UTF-8 safe processing and auto-calls expand/polyfill.
 */
std::vector<std::string> extractStylesFromRawHTML(const std::string& html);

std::string extractBodyInlineStyle(const std::string& html);
std::string stripOuterTags(const std::string& html);

} // namespace ZepraBrowser
