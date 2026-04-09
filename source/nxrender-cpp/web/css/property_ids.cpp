// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "property_ids.h"
#include <unordered_map>
#include <cstring>
#include <algorithm>

namespace NXRender {
namespace Web {

// ==================================================================
// CSSValue factory methods
// ==================================================================

CSSValue CSSValue::keyword(const std::string& kw) {
    CSSValue v;
    v.type = CSSValueType::Keyword;
    v.stringValue = kw;
    return v;
}

CSSValue CSSValue::length(float val, CSSLengthUnit unit) {
    CSSValue v;
    v.type = CSSValueType::Length;
    v.numericValue = val;
    v.lengthUnit = unit;
    return v;
}

CSSValue CSSValue::percentage(float val) {
    CSSValue v;
    v.type = CSSValueType::Percentage;
    v.numericValue = val;
    v.lengthUnit = CSSLengthUnit::Percent;
    return v;
}

CSSValue CSSValue::color(uint32_t rgba) {
    CSSValue v;
    v.type = CSSValueType::Color;
    v.colorValue = rgba;
    return v;
}

CSSValue CSSValue::number(float val) {
    CSSValue v;
    v.type = CSSValueType::Number;
    v.numericValue = val;
    return v;
}

CSSValue CSSValue::string(const std::string& val) {
    CSSValue v;
    v.type = CSSValueType::String;
    v.stringValue = val;
    return v;
}

CSSValue CSSValue::initial() {
    CSSValue v;
    v.type = CSSValueType::Initial;
    return v;
}

CSSValue CSSValue::inherit() {
    CSSValue v;
    v.type = CSSValueType::Inherit;
    return v;
}

// ==================================================================
// Property metadata table
// ==================================================================

struct PropertyEntry {
    CSSPropertyID id;
    const char* name;
    bool inherited;
    bool shorthand;
};

static const PropertyEntry s_propertyTable[] = {
    {CSSPropertyID::Invalid, "", false, false},

    // Box Model
    {CSSPropertyID::Display, "display", false, false},
    {CSSPropertyID::Position, "position", false, false},
    {CSSPropertyID::Float, "float", false, false},
    {CSSPropertyID::Clear, "clear", false, false},
    {CSSPropertyID::BoxSizing, "box-sizing", false, false},
    {CSSPropertyID::Visibility, "visibility", true, false},
    {CSSPropertyID::Opacity, "opacity", false, false},

    // Dimensions
    {CSSPropertyID::Width, "width", false, false},
    {CSSPropertyID::Height, "height", false, false},
    {CSSPropertyID::MinWidth, "min-width", false, false},
    {CSSPropertyID::MinHeight, "min-height", false, false},
    {CSSPropertyID::MaxWidth, "max-width", false, false},
    {CSSPropertyID::MaxHeight, "max-height", false, false},

    // Margin
    {CSSPropertyID::Margin, "margin", false, true},
    {CSSPropertyID::MarginTop, "margin-top", false, false},
    {CSSPropertyID::MarginRight, "margin-right", false, false},
    {CSSPropertyID::MarginBottom, "margin-bottom", false, false},
    {CSSPropertyID::MarginLeft, "margin-left", false, false},
    {CSSPropertyID::MarginBlock, "margin-block", false, true},
    {CSSPropertyID::MarginBlockStart, "margin-block-start", false, false},
    {CSSPropertyID::MarginBlockEnd, "margin-block-end", false, false},
    {CSSPropertyID::MarginInline, "margin-inline", false, true},
    {CSSPropertyID::MarginInlineStart, "margin-inline-start", false, false},
    {CSSPropertyID::MarginInlineEnd, "margin-inline-end", false, false},

    // Padding
    {CSSPropertyID::Padding, "padding", false, true},
    {CSSPropertyID::PaddingTop, "padding-top", false, false},
    {CSSPropertyID::PaddingRight, "padding-right", false, false},
    {CSSPropertyID::PaddingBottom, "padding-bottom", false, false},
    {CSSPropertyID::PaddingLeft, "padding-left", false, false},
    {CSSPropertyID::PaddingBlock, "padding-block", false, true},
    {CSSPropertyID::PaddingBlockStart, "padding-block-start", false, false},
    {CSSPropertyID::PaddingBlockEnd, "padding-block-end", false, false},
    {CSSPropertyID::PaddingInline, "padding-inline", false, true},
    {CSSPropertyID::PaddingInlineStart, "padding-inline-start", false, false},
    {CSSPropertyID::PaddingInlineEnd, "padding-inline-end", false, false},

    // Border Width
    {CSSPropertyID::BorderWidth, "border-width", false, true},
    {CSSPropertyID::BorderTopWidth, "border-top-width", false, false},
    {CSSPropertyID::BorderRightWidth, "border-right-width", false, false},
    {CSSPropertyID::BorderBottomWidth, "border-bottom-width", false, false},
    {CSSPropertyID::BorderLeftWidth, "border-left-width", false, false},

    // Border Style
    {CSSPropertyID::BorderStyle, "border-style", false, true},
    {CSSPropertyID::BorderTopStyle, "border-top-style", false, false},
    {CSSPropertyID::BorderRightStyle, "border-right-style", false, false},
    {CSSPropertyID::BorderBottomStyle, "border-bottom-style", false, false},
    {CSSPropertyID::BorderLeftStyle, "border-left-style", false, false},

    // Border Color
    {CSSPropertyID::BorderColor, "border-color", false, true},
    {CSSPropertyID::BorderTopColor, "border-top-color", false, false},
    {CSSPropertyID::BorderRightColor, "border-right-color", false, false},
    {CSSPropertyID::BorderBottomColor, "border-bottom-color", false, false},
    {CSSPropertyID::BorderLeftColor, "border-left-color", false, false},

    // Border Shorthand
    {CSSPropertyID::Border, "border", false, true},
    {CSSPropertyID::BorderTop, "border-top", false, true},
    {CSSPropertyID::BorderRight, "border-right", false, true},
    {CSSPropertyID::BorderBottom, "border-bottom", false, true},
    {CSSPropertyID::BorderLeft, "border-left", false, true},

    // Border Radius
    {CSSPropertyID::BorderRadius, "border-radius", false, true},
    {CSSPropertyID::BorderTopLeftRadius, "border-top-left-radius", false, false},
    {CSSPropertyID::BorderTopRightRadius, "border-top-right-radius", false, false},
    {CSSPropertyID::BorderBottomRightRadius, "border-bottom-right-radius", false, false},
    {CSSPropertyID::BorderBottomLeftRadius, "border-bottom-left-radius", false, false},

    // Overflow
    {CSSPropertyID::Overflow, "overflow", false, true},
    {CSSPropertyID::OverflowX, "overflow-x", false, false},
    {CSSPropertyID::OverflowY, "overflow-y", false, false},
    {CSSPropertyID::OverflowWrap, "overflow-wrap", true, false},

    // Positioning
    {CSSPropertyID::Top, "top", false, false},
    {CSSPropertyID::Right, "right", false, false},
    {CSSPropertyID::Bottom, "bottom", false, false},
    {CSSPropertyID::Left, "left", false, false},
    {CSSPropertyID::Inset, "inset", false, true},
    {CSSPropertyID::ZIndex, "z-index", false, false},

    // Flexbox
    {CSSPropertyID::FlexDirection, "flex-direction", false, false},
    {CSSPropertyID::FlexWrap, "flex-wrap", false, false},
    {CSSPropertyID::FlexFlow, "flex-flow", false, true},
    {CSSPropertyID::JustifyContent, "justify-content", false, false},
    {CSSPropertyID::AlignItems, "align-items", false, false},
    {CSSPropertyID::AlignContent, "align-content", false, false},
    {CSSPropertyID::AlignSelf, "align-self", false, false},
    {CSSPropertyID::FlexGrow, "flex-grow", false, false},
    {CSSPropertyID::FlexShrink, "flex-shrink", false, false},
    {CSSPropertyID::FlexBasis, "flex-basis", false, false},
    {CSSPropertyID::Flex, "flex", false, true},
    {CSSPropertyID::Order, "order", false, false},
    {CSSPropertyID::Gap, "gap", false, true},
    {CSSPropertyID::RowGap, "row-gap", false, false},
    {CSSPropertyID::ColumnGap, "column-gap", false, false},

    // Grid
    {CSSPropertyID::GridTemplateColumns, "grid-template-columns", false, false},
    {CSSPropertyID::GridTemplateRows, "grid-template-rows", false, false},
    {CSSPropertyID::GridTemplateAreas, "grid-template-areas", false, false},
    {CSSPropertyID::GridTemplate, "grid-template", false, true},
    {CSSPropertyID::GridColumnStart, "grid-column-start", false, false},
    {CSSPropertyID::GridColumnEnd, "grid-column-end", false, false},
    {CSSPropertyID::GridColumn, "grid-column", false, true},
    {CSSPropertyID::GridRowStart, "grid-row-start", false, false},
    {CSSPropertyID::GridRowEnd, "grid-row-end", false, false},
    {CSSPropertyID::GridRow, "grid-row", false, true},
    {CSSPropertyID::GridArea, "grid-area", false, true},
    {CSSPropertyID::GridAutoColumns, "grid-auto-columns", false, false},
    {CSSPropertyID::GridAutoRows, "grid-auto-rows", false, false},
    {CSSPropertyID::GridAutoFlow, "grid-auto-flow", false, false},
    {CSSPropertyID::PlaceContent, "place-content", false, true},
    {CSSPropertyID::PlaceItems, "place-items", false, true},
    {CSSPropertyID::PlaceSelf, "place-self", false, true},

    // Typography (inherited)
    {CSSPropertyID::Color, "color", true, false},
    {CSSPropertyID::FontFamily, "font-family", true, false},
    {CSSPropertyID::FontSize, "font-size", true, false},
    {CSSPropertyID::FontWeight, "font-weight", true, false},
    {CSSPropertyID::FontStyle, "font-style", true, false},
    {CSSPropertyID::FontVariant, "font-variant", true, false},
    {CSSPropertyID::Font, "font", true, true},
    {CSSPropertyID::LineHeight, "line-height", true, false},
    {CSSPropertyID::LetterSpacing, "letter-spacing", true, false},
    {CSSPropertyID::WordSpacing, "word-spacing", true, false},
    {CSSPropertyID::TextAlign, "text-align", true, false},
    {CSSPropertyID::TextAlignLast, "text-align-last", true, false},
    {CSSPropertyID::TextDecoration, "text-decoration", false, true},
    {CSSPropertyID::TextDecorationColor, "text-decoration-color", false, false},
    {CSSPropertyID::TextDecorationLine, "text-decoration-line", false, false},
    {CSSPropertyID::TextDecorationStyle, "text-decoration-style", false, false},
    {CSSPropertyID::TextTransform, "text-transform", true, false},
    {CSSPropertyID::TextIndent, "text-indent", true, false},
    {CSSPropertyID::TextOverflow, "text-overflow", false, false},
    {CSSPropertyID::TextShadow, "text-shadow", true, false},
    {CSSPropertyID::WhiteSpace, "white-space", true, false},
    {CSSPropertyID::WordBreak, "word-break", true, false},
    {CSSPropertyID::WordWrap, "word-wrap", true, false},
    {CSSPropertyID::Hyphens, "hyphens", true, false},
    {CSSPropertyID::VerticalAlign, "vertical-align", false, false},
    {CSSPropertyID::Direction, "direction", true, false},
    {CSSPropertyID::UnicodeBidi, "unicode-bidi", false, false},
    {CSSPropertyID::WritingMode, "writing-mode", true, false},

    // Background
    {CSSPropertyID::Background, "background", false, true},
    {CSSPropertyID::BackgroundColor, "background-color", false, false},
    {CSSPropertyID::BackgroundImage, "background-image", false, false},
    {CSSPropertyID::BackgroundPosition, "background-position", false, true},
    {CSSPropertyID::BackgroundPositionX, "background-position-x", false, false},
    {CSSPropertyID::BackgroundPositionY, "background-position-y", false, false},
    {CSSPropertyID::BackgroundSize, "background-size", false, false},
    {CSSPropertyID::BackgroundRepeat, "background-repeat", false, false},
    {CSSPropertyID::BackgroundOrigin, "background-origin", false, false},
    {CSSPropertyID::BackgroundClip, "background-clip", false, false},
    {CSSPropertyID::BackgroundAttachment, "background-attachment", false, false},
    {CSSPropertyID::BackgroundBlendMode, "background-blend-mode", false, false},

    // Effects
    {CSSPropertyID::BoxShadow, "box-shadow", false, false},
    {CSSPropertyID::Outline, "outline", false, true},
    {CSSPropertyID::OutlineWidth, "outline-width", false, false},
    {CSSPropertyID::OutlineStyle, "outline-style", false, false},
    {CSSPropertyID::OutlineColor, "outline-color", false, false},
    {CSSPropertyID::OutlineOffset, "outline-offset", false, false},
    {CSSPropertyID::Filter, "filter", false, false},
    {CSSPropertyID::BackdropFilter, "backdrop-filter", false, false},
    {CSSPropertyID::MixBlendMode, "mix-blend-mode", false, false},
    {CSSPropertyID::Isolation, "isolation", false, false},

    // Transform
    {CSSPropertyID::Transform, "transform", false, false},
    {CSSPropertyID::TransformOrigin, "transform-origin", false, false},
    {CSSPropertyID::TransformStyle, "transform-style", false, false},
    {CSSPropertyID::Perspective, "perspective", false, false},
    {CSSPropertyID::PerspectiveOrigin, "perspective-origin", false, false},
    {CSSPropertyID::Backface_visibility, "backface-visibility", false, false},

    // Transition
    {CSSPropertyID::Transition, "transition", false, true},
    {CSSPropertyID::TransitionProperty, "transition-property", false, false},
    {CSSPropertyID::TransitionDuration, "transition-duration", false, false},
    {CSSPropertyID::TransitionTimingFunction, "transition-timing-function", false, false},
    {CSSPropertyID::TransitionDelay, "transition-delay", false, false},

    // Animation
    {CSSPropertyID::Animation, "animation", false, true},
    {CSSPropertyID::AnimationName, "animation-name", false, false},
    {CSSPropertyID::AnimationDuration, "animation-duration", false, false},
    {CSSPropertyID::AnimationTimingFunction, "animation-timing-function", false, false},
    {CSSPropertyID::AnimationDelay, "animation-delay", false, false},
    {CSSPropertyID::AnimationIterationCount, "animation-iteration-count", false, false},
    {CSSPropertyID::AnimationDirection, "animation-direction", false, false},
    {CSSPropertyID::AnimationFillMode, "animation-fill-mode", false, false},
    {CSSPropertyID::AnimationPlayState, "animation-play-state", false, false},

    // Lists
    {CSSPropertyID::ListStyle, "list-style", true, true},
    {CSSPropertyID::ListStyleType, "list-style-type", true, false},
    {CSSPropertyID::ListStylePosition, "list-style-position", true, false},
    {CSSPropertyID::ListStyleImage, "list-style-image", true, false},

    // Table
    {CSSPropertyID::TableLayout, "table-layout", false, false},
    {CSSPropertyID::BorderCollapse, "border-collapse", true, false},
    {CSSPropertyID::BorderSpacing, "border-spacing", true, false},
    {CSSPropertyID::CaptionSide, "caption-side", true, false},
    {CSSPropertyID::EmptyCells, "empty-cells", true, false},

    // Cursor & Interaction
    {CSSPropertyID::Cursor, "cursor", true, false},
    {CSSPropertyID::PointerEvents, "pointer-events", true, false},
    {CSSPropertyID::UserSelect, "user-select", false, false},
    {CSSPropertyID::TouchAction, "touch-action", false, false},
    {CSSPropertyID::Resize, "resize", false, false},
    {CSSPropertyID::ScrollBehavior, "scroll-behavior", false, false},
    {CSSPropertyID::ScrollSnapType, "scroll-snap-type", false, false},
    {CSSPropertyID::ScrollSnapAlign, "scroll-snap-align", false, false},

    // Sizing
    {CSSPropertyID::AspectRatio, "aspect-ratio", false, false},
    {CSSPropertyID::ObjectFit, "object-fit", false, false},
    {CSSPropertyID::ObjectPosition, "object-position", false, false},
    {CSSPropertyID::Contain, "contain", false, false},
    {CSSPropertyID::ContainerType, "container-type", false, false},
    {CSSPropertyID::ContainerName, "container-name", false, false},
    {CSSPropertyID::ContentVisibility, "content-visibility", false, false},

    // Misc
    {CSSPropertyID::WillChange, "will-change", false, false},
    {CSSPropertyID::Appearance, "appearance", false, false},
    {CSSPropertyID::Content, "content", false, false},
    {CSSPropertyID::Quotes, "quotes", true, false},
    {CSSPropertyID::CounterIncrement, "counter-increment", false, false},
    {CSSPropertyID::CounterReset, "counter-reset", false, false},
    {CSSPropertyID::CounterSet, "counter-set", false, false},
    {CSSPropertyID::ClipPath, "clip-path", false, false},
    {CSSPropertyID::Mask, "mask", false, true},
    {CSSPropertyID::MaskImage, "mask-image", false, false},
    {CSSPropertyID::MaskSize, "mask-size", false, false},
    {CSSPropertyID::MaskRepeat, "mask-repeat", false, false},
    {CSSPropertyID::MaskPosition, "mask-position", false, false},
    {CSSPropertyID::ShapeOutside, "shape-outside", false, false},
    {CSSPropertyID::ShapeMargin, "shape-margin", false, false},
    {CSSPropertyID::ShapeImageThreshold, "shape-image-threshold", false, false},

    // Multi-Column
    {CSSPropertyID::ColumnCount, "column-count", false, false},
    {CSSPropertyID::ColumnWidth, "column-width", false, false},
    {CSSPropertyID::ColumnGap_MC, "column-gap", false, false},
    {CSSPropertyID::ColumnRule, "column-rule", false, true},
    {CSSPropertyID::ColumnRuleWidth, "column-rule-width", false, false},
    {CSSPropertyID::ColumnRuleStyle, "column-rule-style", false, false},
    {CSSPropertyID::ColumnRuleColor, "column-rule-color", false, false},
    {CSSPropertyID::ColumnSpan, "column-span", false, false},
    {CSSPropertyID::ColumnFill, "column-fill", false, false},
    {CSSPropertyID::Columns, "columns", false, true},

    // Fragmentation
    {CSSPropertyID::PageBreakBefore, "page-break-before", false, false},
    {CSSPropertyID::PageBreakAfter, "page-break-after", false, false},
    {CSSPropertyID::PageBreakInside, "page-break-inside", false, false},
    {CSSPropertyID::BreakBefore, "break-before", false, false},
    {CSSPropertyID::BreakAfter, "break-after", false, false},
    {CSSPropertyID::BreakInside, "break-inside", false, false},
    {CSSPropertyID::Orphans, "orphans", true, false},
    {CSSPropertyID::Widows, "widows", true, false},

    // Modern
    {CSSPropertyID::AccentColor, "accent-color", true, false},
    {CSSPropertyID::CaretColor, "caret-color", true, false},
    {CSSPropertyID::ColorScheme, "color-scheme", true, false},
    {CSSPropertyID::ForcedColorAdjust, "forced-color-adjust", true, false},
    {CSSPropertyID::PrintColorAdjust, "print-color-adjust", true, false},
    {CSSPropertyID::All, "all", false, true},
};

static constexpr size_t PROPERTY_TABLE_SIZE = sizeof(s_propertyTable) / sizeof(s_propertyTable[0]);

// ==================================================================
// Name → ID lookup (built lazily)
// ==================================================================

static std::unordered_map<std::string, CSSPropertyID>& getNameMap() {
    static std::unordered_map<std::string, CSSPropertyID> map;
    if (map.empty()) {
        for (size_t i = 0; i < PROPERTY_TABLE_SIZE; i++) {
            if (s_propertyTable[i].name[0] != '\0') {
                map[s_propertyTable[i].name] = s_propertyTable[i].id;
            }
        }
    }
    return map;
}

// ==================================================================
// Public API
// ==================================================================

CSSPropertyID propertyIDFromString(const std::string& name) {
    // Normalize: lowercase, strip vendor prefix for lookup
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

    auto& map = getNameMap();
    auto it = map.find(normalized);
    if (it != map.end()) return it->second;

    // Try with -webkit- stripped
    if (normalized.substr(0, 8) == "-webkit-") {
        auto it2 = map.find(normalized.substr(8));
        if (it2 != map.end()) return it2->second;
    }

    // Try with -moz- stripped
    if (normalized.substr(0, 5) == "-moz-") {
        auto it2 = map.find(normalized.substr(5));
        if (it2 != map.end()) return it2->second;
    }

    return CSSPropertyID::Invalid;
}

const char* propertyIDToString(CSSPropertyID id) {
    uint16_t idx = static_cast<uint16_t>(id);
    for (size_t i = 0; i < PROPERTY_TABLE_SIZE; i++) {
        if (s_propertyTable[i].id == id) return s_propertyTable[i].name;
    }
    return "";
}

bool isInheritedProperty(CSSPropertyID id) {
    for (size_t i = 0; i < PROPERTY_TABLE_SIZE; i++) {
        if (s_propertyTable[i].id == id) return s_propertyTable[i].inherited;
    }
    return false;
}

bool isShorthandProperty(CSSPropertyID id) {
    for (size_t i = 0; i < PROPERTY_TABLE_SIZE; i++) {
        if (s_propertyTable[i].id == id) return s_propertyTable[i].shorthand;
    }
    return false;
}

const CSSPropertyMeta& getPropertyMeta(CSSPropertyID id) {
    static CSSPropertyMeta fallback{CSSPropertyID::Invalid, "", false, false, CSSPropertyID::Invalid, 0};

    for (size_t i = 0; i < PROPERTY_TABLE_SIZE; i++) {
        if (s_propertyTable[i].id == id) {
            static CSSPropertyMeta meta;
            meta.id = s_propertyTable[i].id;
            meta.name = s_propertyTable[i].name;
            meta.inherited = s_propertyTable[i].inherited;
            meta.isShorthand = s_propertyTable[i].shorthand;
            meta.longhandFirst = CSSPropertyID::Invalid;
            meta.longhandCount = 0;
            return meta;
        }
    }
    return fallback;
}

} // namespace Web
} // namespace NXRender
