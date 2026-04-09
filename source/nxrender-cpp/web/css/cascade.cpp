// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "cascade.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace NXRender {
namespace Web {

// ==================================================================
// Declaration ordering
// ==================================================================

bool CSSDeclaration::operator<(const CSSDeclaration& other) const {
    // CSS cascade sorting:
    // 1. Origin + importance
    // 2. Specificity
    // 3. Source order

    int thisOrigin = static_cast<int>(origin);
    int otherOrigin = static_cast<int>(other.origin);

    if (important && !other.important) {
        // Important flips origin order for author vs user
        thisOrigin = static_cast<int>(CascadeOrigin::AuthorImportant);
    }
    if (!important && other.important) {
        otherOrigin = static_cast<int>(CascadeOrigin::AuthorImportant);
    }

    if (thisOrigin != otherOrigin) return thisOrigin < otherOrigin;
    if (specificity != other.specificity) return specificity < other.specificity;
    return sourceOrder < other.sourceOrder;
}

// ==================================================================
// CascadedValues
// ==================================================================

const CSSDeclaration* CascadedValues::get(CSSPropertyID id) const {
    auto it = declarations.find(id);
    if (it == declarations.end()) return nullptr;
    return &it->second;
}

bool CascadedValues::has(CSSPropertyID id) const {
    return declarations.find(id) != declarations.end();
}

void CascadedValues::set(const CSSDeclaration& decl) {
    auto it = declarations.find(decl.property);
    if (it == declarations.end()) {
        declarations[decl.property] = decl;
    } else {
        // Higher cascade priority wins
        if (it->second < decl) {
            it->second = decl;
        }
    }
}

// ==================================================================
// CascadeEngine
// ==================================================================

CascadeEngine::CascadeEngine() {}
CascadeEngine::~CascadeEngine() {}

CascadedValues CascadeEngine::cascade(const std::vector<CSSDeclaration>& declarations) {
    CascadedValues result;

    // Sort by cascade priority
    std::vector<CSSDeclaration> sorted = declarations;
    std::sort(sorted.begin(), sorted.end());

    for (const auto& decl : sorted) {
        // Expand shorthands
        if (isShorthandProperty(decl.property)) {
            auto expanded = expandShorthand(decl);
            for (auto& exp : expanded) {
                result.set(exp);
            }
        } else {
            result.set(decl);
        }
    }

    return result;
}

std::vector<CSSDeclaration> CascadeEngine::expandShorthand(const CSSDeclaration& decl) {
    std::vector<CSSDeclaration> result;

    auto expanded = CSSValueParser::expandShorthand(decl.property, decl.value.stringValue);
    for (auto& pair : expanded) {
        CSSDeclaration d;
        d.property = pair.first;
        d.value = pair.second;
        d.important = decl.important;
        d.origin = decl.origin;
        d.specificity = decl.specificity;
        d.sourceOrder = decl.sourceOrder;
        result.push_back(d);
    }

    return result;
}

// ==================================================================
// Keyword resolvers
// ==================================================================

uint8_t CascadeEngine::resolveDisplayKeyword(const std::string& kw) {
    if (kw == "none") return 0;
    if (kw == "block") return 1;
    if (kw == "inline") return 2;
    if (kw == "inline-block") return 3;
    if (kw == "flex") return 4;
    if (kw == "inline-flex") return 5;
    if (kw == "grid") return 6;
    if (kw == "inline-grid") return 7;
    if (kw == "table") return 8;
    if (kw == "table-row") return 9;
    if (kw == "table-cell") return 10;
    if (kw == "list-item") return 11;
    if (kw == "contents") return 12;
    return 1; // Default: block
}

uint8_t CascadeEngine::resolvePositionKeyword(const std::string& kw) {
    if (kw == "static") return 0;
    if (kw == "relative") return 1;
    if (kw == "absolute") return 2;
    if (kw == "fixed") return 3;
    if (kw == "sticky") return 4;
    return 0;
}

uint8_t CascadeEngine::resolveOverflowKeyword(const std::string& kw) {
    if (kw == "visible") return 0;
    if (kw == "hidden") return 1;
    if (kw == "scroll") return 2;
    if (kw == "auto") return 3;
    if (kw == "clip") return 4;
    return 0;
}

uint8_t CascadeEngine::resolveFlexDirectionKeyword(const std::string& kw) {
    if (kw == "row") return 0;
    if (kw == "row-reverse") return 1;
    if (kw == "column") return 2;
    if (kw == "column-reverse") return 3;
    return 0;
}

uint8_t CascadeEngine::resolveJustifyAlignKeyword(const std::string& kw) {
    if (kw == "flex-start" || kw == "start") return 0;
    if (kw == "flex-end" || kw == "end") return 1;
    if (kw == "center") return 2;
    if (kw == "space-between") return 3;
    if (kw == "space-around") return 4;
    if (kw == "space-evenly") return 5;
    if (kw == "stretch") return 6;
    if (kw == "baseline") return 7;
    return 0;
}

uint8_t CascadeEngine::resolveTextAlignKeyword(const std::string& kw) {
    if (kw == "left" || kw == "start") return 0;
    if (kw == "right" || kw == "end") return 1;
    if (kw == "center") return 2;
    if (kw == "justify") return 3;
    return 0;
}

uint8_t CascadeEngine::resolveVisibilityKeyword(const std::string& kw) {
    if (kw == "visible") return 0;
    if (kw == "hidden") return 1;
    if (kw == "collapse") return 2;
    return 0;
}

uint8_t CascadeEngine::resolveBoxSizingKeyword(const std::string& kw) {
    if (kw == "content-box") return 0;
    if (kw == "border-box") return 1;
    return 0;
}

// ==================================================================
// Length to pixel resolution
// ==================================================================

float CascadeEngine::resolveLengthPx(const CSSValue& value, float fontSize,
                                       float rootFontSize, float containerSize) {
    if (value.type == CSSValueType::Length || value.type == CSSValueType::Percentage) {
        auto resolved = ComputedLength::resolve(
            value.numericValue, value.lengthUnit,
            fontSize, rootFontSize,
            viewport_.width, viewport_.height,
            containerSize);
        return resolved.px;
    }
    if (value.type == CSSValueType::Number) {
        return value.numericValue;
    }
    return 0;
}

// ==================================================================
// Inheritance resolution
// ==================================================================

CSSValue CascadeEngine::resolveInheritance(CSSPropertyID property,
                                             const CSSValue& cascaded,
                                             const ComputedValues* parent) {
    if (cascaded.isInherit()) {
        // Would pull from parent computed value — simplified here
        return cascaded;
    }
    if (cascaded.isInitial()) {
        // Return initial value
        return cascaded;
    }
    if (cascaded.type == CSSValueType::Unset) {
        if (isInheritedProperty(property)) {
            // Inherit
            return cascaded;
        }
        return CSSValue::initial();
    }
    return cascaded;
}

// ==================================================================
// Property application
// ==================================================================

void CascadeEngine::applyProperty(CSSPropertyID id, const CSSValue& value,
                                    ComputedValues& computed,
                                    const ComputedValues* parent,
                                    float fontSize, float rootFontSize) {
    switch (id) {
        // Display/Position
        case CSSPropertyID::Display:
            computed.display = resolveDisplayKeyword(value.stringValue);
            break;
        case CSSPropertyID::Position:
            computed.position = resolvePositionKeyword(value.stringValue);
            break;
        case CSSPropertyID::Visibility:
            computed.visibility = resolveVisibilityKeyword(value.stringValue);
            break;
        case CSSPropertyID::BoxSizing:
            computed.boxSizing = resolveBoxSizingKeyword(value.stringValue);
            break;

        // Dimensions
        case CSSPropertyID::Width:
            if (value.isAuto()) { computed.widthAuto = true; }
            else { computed.widthAuto = false; computed.width = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::Height:
            if (value.isAuto()) { computed.heightAuto = true; }
            else { computed.heightAuto = false; computed.height = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::MinWidth:
            computed.minWidth = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::MinHeight:
            computed.minHeight = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::MaxWidth:
            if (value.isNone()) computed.maxWidth = 1e9f;
            else computed.maxWidth = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::MaxHeight:
            if (value.isNone()) computed.maxHeight = 1e9f;
            else computed.maxHeight = resolveLengthPx(value, fontSize, rootFontSize);
            break;

        // Margin
        case CSSPropertyID::MarginTop:
            if (value.isAuto()) computed.marginTopAuto = true;
            else { computed.marginTopAuto = false; computed.marginTop = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::MarginRight:
            if (value.isAuto()) computed.marginRightAuto = true;
            else { computed.marginRightAuto = false; computed.marginRight = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::MarginBottom:
            if (value.isAuto()) computed.marginBottomAuto = true;
            else { computed.marginBottomAuto = false; computed.marginBottom = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::MarginLeft:
            if (value.isAuto()) computed.marginLeftAuto = true;
            else { computed.marginLeftAuto = false; computed.marginLeft = resolveLengthPx(value, fontSize, rootFontSize); }
            break;

        // Padding
        case CSSPropertyID::PaddingTop:
            computed.paddingTop = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::PaddingRight:
            computed.paddingRight = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::PaddingBottom:
            computed.paddingBottom = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::PaddingLeft:
            computed.paddingLeft = resolveLengthPx(value, fontSize, rootFontSize);
            break;

        // Border
        case CSSPropertyID::BorderTopWidth:
            computed.borderTopWidth = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::BorderRightWidth:
            computed.borderRightWidth = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::BorderBottomWidth:
            computed.borderBottomWidth = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::BorderLeftWidth:
            computed.borderLeftWidth = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::BorderTopColor:
            computed.borderTopColor = value.colorValue;
            break;
        case CSSPropertyID::BorderRightColor:
            computed.borderRightColor = value.colorValue;
            break;
        case CSSPropertyID::BorderBottomColor:
            computed.borderBottomColor = value.colorValue;
            break;
        case CSSPropertyID::BorderLeftColor:
            computed.borderLeftColor = value.colorValue;
            break;
        case CSSPropertyID::BorderTopLeftRadius:
            computed.borderTopLeftRadius = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::BorderTopRightRadius:
            computed.borderTopRightRadius = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::BorderBottomRightRadius:
            computed.borderBottomRightRadius = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::BorderBottomLeftRadius:
            computed.borderBottomLeftRadius = resolveLengthPx(value, fontSize, rootFontSize);
            break;

        // Positioning
        case CSSPropertyID::Top:
            if (value.isAuto()) computed.topAuto = true;
            else { computed.topAuto = false; computed.top = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::Right:
            if (value.isAuto()) computed.rightAuto = true;
            else { computed.rightAuto = false; computed.right = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::Bottom:
            if (value.isAuto()) computed.bottomAuto = true;
            else { computed.bottomAuto = false; computed.bottom = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::Left:
            if (value.isAuto()) computed.leftAuto = true;
            else { computed.leftAuto = false; computed.left = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::ZIndex:
            if (value.isAuto()) computed.zIndexAuto = true;
            else { computed.zIndexAuto = false; computed.zIndex = static_cast<int>(value.numericValue); }
            break;

        // Overflow
        case CSSPropertyID::OverflowX:
            computed.overflowX = resolveOverflowKeyword(value.stringValue);
            break;
        case CSSPropertyID::OverflowY:
            computed.overflowY = resolveOverflowKeyword(value.stringValue);
            break;

        // Flexbox
        case CSSPropertyID::FlexDirection:
            computed.flexDirection = resolveFlexDirectionKeyword(value.stringValue);
            break;
        case CSSPropertyID::FlexWrap:
            computed.flexWrap = (value.stringValue == "wrap" || value.stringValue == "wrap-reverse");
            break;
        case CSSPropertyID::JustifyContent:
            computed.justifyContent = resolveJustifyAlignKeyword(value.stringValue);
            break;
        case CSSPropertyID::AlignItems:
            computed.alignItems = resolveJustifyAlignKeyword(value.stringValue);
            break;
        case CSSPropertyID::AlignContent:
            computed.alignContent = resolveJustifyAlignKeyword(value.stringValue);
            break;
        case CSSPropertyID::AlignSelf:
            computed.alignSelf = resolveJustifyAlignKeyword(value.stringValue);
            break;
        case CSSPropertyID::FlexGrow:
            computed.flexGrow = value.numericValue;
            break;
        case CSSPropertyID::FlexShrink:
            computed.flexShrink = value.numericValue;
            break;
        case CSSPropertyID::FlexBasis:
            if (value.isAuto()) computed.flexBasisAuto = true;
            else { computed.flexBasisAuto = false; computed.flexBasis = resolveLengthPx(value, fontSize, rootFontSize); }
            break;
        case CSSPropertyID::Order:
            computed.order = static_cast<int>(value.numericValue);
            break;
        case CSSPropertyID::RowGap:
            computed.rowGap = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::ColumnGap:
            computed.columnGap = resolveLengthPx(value, fontSize, rootFontSize);
            break;

        // Grid
        case CSSPropertyID::GridTemplateColumns:
            computed.gridTemplateColumns = value.stringValue;
            break;
        case CSSPropertyID::GridTemplateRows:
            computed.gridTemplateRows = value.stringValue;
            break;
        case CSSPropertyID::GridTemplateAreas:
            computed.gridTemplateAreas = value.stringValue;
            break;
        case CSSPropertyID::GridAutoFlow:
            computed.gridAutoFlow = value.stringValue;
            break;

        // Typography
        case CSSPropertyID::Color:
            computed.color = value.colorValue;
            break;
        case CSSPropertyID::FontFamily:
            computed.fontFamily = value.stringValue;
            break;
        case CSSPropertyID::FontSize:
            computed.fontSize = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::FontWeight:
            if (value.type == CSSValueType::Number) {
                computed.fontWeight = static_cast<uint16_t>(value.numericValue);
            } else {
                if (value.stringValue == "bold") computed.fontWeight = 700;
                else if (value.stringValue == "normal") computed.fontWeight = 400;
                else if (value.stringValue == "lighter") computed.fontWeight = 100;
                else if (value.stringValue == "bolder") computed.fontWeight = 900;
            }
            break;
        case CSSPropertyID::LineHeight:
            if (value.stringValue == "normal") {
                computed.lineHeightNormal = true;
                computed.lineHeight = 1.2f;
            } else {
                computed.lineHeightNormal = false;
                if (value.type == CSSValueType::Number) {
                    computed.lineHeight = value.numericValue;
                } else {
                    computed.lineHeight = resolveLengthPx(value, fontSize, rootFontSize);
                }
            }
            break;
        case CSSPropertyID::LetterSpacing:
            if (value.stringValue == "normal") computed.letterSpacing = 0;
            else computed.letterSpacing = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::WordSpacing:
            if (value.stringValue == "normal") computed.wordSpacing = 0;
            else computed.wordSpacing = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::TextAlign:
            computed.textAlign = resolveTextAlignKeyword(value.stringValue);
            break;
        case CSSPropertyID::TextDecoration:
            computed.textDecoration = value.stringValue;
            break;
        case CSSPropertyID::TextTransform:
            computed.textTransform = value.stringValue;
            break;
        case CSSPropertyID::TextIndent:
            computed.textIndent = resolveLengthPx(value, fontSize, rootFontSize);
            break;
        case CSSPropertyID::WhiteSpace:
            computed.whiteSpace = value.stringValue;
            break;

        // Background
        case CSSPropertyID::BackgroundColor:
            computed.backgroundColor = value.colorValue;
            break;
        case CSSPropertyID::BackgroundImage:
            computed.backgroundImage = value.stringValue;
            break;
        case CSSPropertyID::BackgroundSize:
            computed.backgroundSize = value.stringValue;
            break;
        case CSSPropertyID::BackgroundPosition:
            computed.backgroundPosition = value.stringValue;
            break;
        case CSSPropertyID::BackgroundRepeat:
            computed.backgroundRepeat = value.stringValue;
            break;

        // Effects
        case CSSPropertyID::Opacity:
            computed.opacity = std::clamp(value.numericValue, 0.0f, 1.0f);
            break;
        case CSSPropertyID::Transform:
            computed.transform = value.stringValue;
            break;
        case CSSPropertyID::Filter:
            computed.filter = value.stringValue;
            break;
        case CSSPropertyID::BoxShadow:
            computed.boxShadow = value.stringValue;
            break;
        case CSSPropertyID::Cursor:
            computed.cursor = value.stringValue;
            break;
        case CSSPropertyID::PointerEvents:
            computed.pointerEvents = value.stringValue;
            break;
        case CSSPropertyID::WillChange:
            computed.willChange = value.stringValue;
            break;
        case CSSPropertyID::Isolation:
            computed.isolation = value.stringValue;
            break;

        // Sizing
        case CSSPropertyID::ObjectFit:
            computed.objectFit = value.stringValue;
            break;
        case CSSPropertyID::AspectRatio:
            computed.aspectRatio = value.stringValue;
            break;
        case CSSPropertyID::Contain:
            computed.contain = value.stringValue;
            break;
        case CSSPropertyID::ContainerType:
            computed.containerType = value.stringValue;
            break;
        case CSSPropertyID::ContainerName:
            computed.containerName = value.stringValue;
            break;

        // Multi-column
        case CSSPropertyID::ColumnCount:
            if (value.isAuto()) computed.columnCountAuto = true;
            else { computed.columnCountAuto = false; computed.columnCount = static_cast<int>(value.numericValue); }
            break;
        case CSSPropertyID::ColumnWidth:
            if (value.isAuto()) computed.columnWidthAuto = true;
            else { computed.columnWidthAuto = false; computed.columnWidth = resolveLengthPx(value, fontSize, rootFontSize); }
            break;

        // Fragmentation
        case CSSPropertyID::Orphans:
            computed.orphans = static_cast<int>(value.numericValue);
            break;
        case CSSPropertyID::Widows:
            computed.widows = static_cast<int>(value.numericValue);
            break;

        default:
            break;
    }
}

// ==================================================================
// Compute from cascaded values
// ==================================================================

ComputedValues CascadeEngine::compute(const CascadedValues& cascaded,
                                        const ComputedValues* parent,
                                        float rootFontSize) {
    ComputedValues computed;

    // Inherit from parent first for inherited properties
    if (parent) {
        computed.color = parent->color;
        computed.fontFamily = parent->fontFamily;
        computed.fontSize = parent->fontSize;
        computed.fontWeight = parent->fontWeight;
        computed.fontStyle = parent->fontStyle;
        computed.lineHeight = parent->lineHeight;
        computed.lineHeightNormal = parent->lineHeightNormal;
        computed.letterSpacing = parent->letterSpacing;
        computed.wordSpacing = parent->wordSpacing;
        computed.textAlign = parent->textAlign;
        computed.textTransform = parent->textTransform;
        computed.textIndent = parent->textIndent;
        computed.whiteSpace = parent->whiteSpace;
        computed.visibility = parent->visibility;
        computed.cursor = parent->cursor;
        computed.pointerEvents = parent->pointerEvents;
        computed.orphans = parent->orphans;
        computed.widows = parent->widows;
    }

    // Resolve font-size first (other properties depend on it)
    const CSSDeclaration* fsDecl = cascaded.get(CSSPropertyID::FontSize);
    float fontSize = parent ? parent->fontSize : rootFontSize;
    if (fsDecl) {
        if (fsDecl->value.type == CSSValueType::Length ||
            fsDecl->value.type == CSSValueType::Percentage ||
            fsDecl->value.type == CSSValueType::Number) {
            fontSize = resolveLengthPx(fsDecl->value, parent ? parent->fontSize : rootFontSize, rootFontSize);
        }
        computed.fontSize = fontSize;
    }

    // Apply all cascaded declarations
    for (const auto& pair : cascaded.declarations) {
        CSSPropertyID id = pair.first;
        const CSSValue& value = pair.second.value;

        // Handle inheritance keywords
        CSSValue resolved = resolveInheritance(id, value, parent);
        if (resolved.isInherit() || resolved.isInitial()) continue;

        applyProperty(id, resolved, computed, parent, fontSize, rootFontSize);
    }

    return computed;
}

} // namespace Web
} // namespace NXRender
