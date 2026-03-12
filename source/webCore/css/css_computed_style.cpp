// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file css_computed_style.cpp
 * @brief CSS Computed Style implementation stub
 */

#include "css/css_computed_style.hpp"

namespace Zepra::WebCore {

// Explicit default constructor required since member-default init doesn't generate export
CSSComputedStyle::CSSComputedStyle() = default;

CSSComputedStyle CSSComputedStyle::inherit(const CSSComputedStyle& parent) {
    CSSComputedStyle style;
    
    // Inherit text properties
    style.color = parent.color;
    style.fontFamily = parent.fontFamily;
    style.fontSize = parent.fontSize;
    style.fontWeight = parent.fontWeight;
    style.fontStyle = parent.fontStyle;
    style.lineHeight = parent.lineHeight;
    style.textAlign = parent.textAlign;
    style.visibility = parent.visibility;
    
    return style;
}

} // namespace Zepra::WebCore
