// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <cstdint>
#include <string>
#include <array>

namespace NXRender {
namespace Web {

// ==================================================================
// CSS Property ID — exhaustive enumeration
// ==================================================================

enum class CSSPropertyID : uint16_t {
    Invalid = 0,

    // Box Model
    Display,
    Position,
    Float,
    Clear,
    BoxSizing,
    Visibility,
    Opacity,

    // Dimensions
    Width, Height,
    MinWidth, MinHeight,
    MaxWidth, MaxHeight,

    // Margin
    Margin,
    MarginTop, MarginRight, MarginBottom, MarginLeft,
    MarginBlock, MarginBlockStart, MarginBlockEnd,
    MarginInline, MarginInlineStart, MarginInlineEnd,

    // Padding
    Padding,
    PaddingTop, PaddingRight, PaddingBottom, PaddingLeft,
    PaddingBlock, PaddingBlockStart, PaddingBlockEnd,
    PaddingInline, PaddingInlineStart, PaddingInlineEnd,

    // Border Width
    BorderWidth,
    BorderTopWidth, BorderRightWidth, BorderBottomWidth, BorderLeftWidth,

    // Border Style
    BorderStyle,
    BorderTopStyle, BorderRightStyle, BorderBottomStyle, BorderLeftStyle,

    // Border Color
    BorderColor,
    BorderTopColor, BorderRightColor, BorderBottomColor, BorderLeftColor,

    // Border Shorthand
    Border, BorderTop, BorderRight, BorderBottom, BorderLeft,

    // Border Radius
    BorderRadius,
    BorderTopLeftRadius, BorderTopRightRadius,
    BorderBottomRightRadius, BorderBottomLeftRadius,

    // Overflow
    Overflow, OverflowX, OverflowY,
    OverflowWrap,

    // Positioning
    Top, Right, Bottom, Left,
    InsetBlock, InsetBlockStart, InsetBlockEnd,
    InsetInline, InsetInlineStart, InsetInlineEnd,
    Inset,
    ZIndex,

    // Flexbox
    FlexDirection, FlexWrap, FlexFlow,
    JustifyContent, AlignItems, AlignContent, AlignSelf,
    FlexGrow, FlexShrink, FlexBasis, Flex,
    Order,
    Gap, RowGap, ColumnGap,

    // Grid
    GridTemplateColumns, GridTemplateRows, GridTemplateAreas,
    GridTemplate,
    GridColumnStart, GridColumnEnd, GridColumn,
    GridRowStart, GridRowEnd, GridRow,
    GridArea,
    GridAutoColumns, GridAutoRows, GridAutoFlow,
    PlaceContent, PlaceItems, PlaceSelf,

    // Typography
    Color,
    FontFamily, FontSize, FontWeight, FontStyle, FontVariant,
    Font,
    LineHeight,
    LetterSpacing, WordSpacing,
    TextAlign, TextAlignLast,
    TextDecoration, TextDecorationColor, TextDecorationLine, TextDecorationStyle,
    TextTransform,
    TextIndent,
    TextOverflow,
    TextShadow,
    WhiteSpace, WordBreak, WordWrap,
    Hyphens,
    VerticalAlign,
    Direction,
    UnicodeBidi,
    WritingMode,

    // Background
    Background,
    BackgroundColor,
    BackgroundImage,
    BackgroundPosition, BackgroundPositionX, BackgroundPositionY,
    BackgroundSize,
    BackgroundRepeat,
    BackgroundOrigin,
    BackgroundClip,
    BackgroundAttachment,
    BackgroundBlendMode,

    // Effects
    BoxShadow,
    Outline, OutlineWidth, OutlineStyle, OutlineColor, OutlineOffset,
    Filter,
    BackdropFilter,
    MixBlendMode,
    Isolation,

    // Transform
    Transform,
    TransformOrigin,
    TransformStyle,
    Perspective,
    PerspectiveOrigin,
    Backface_visibility,

    // Transition
    Transition,
    TransitionProperty, TransitionDuration,
    TransitionTimingFunction, TransitionDelay,

    // Animation
    Animation,
    AnimationName, AnimationDuration,
    AnimationTimingFunction, AnimationDelay,
    AnimationIterationCount, AnimationDirection,
    AnimationFillMode, AnimationPlayState,

    // Lists
    ListStyle, ListStyleType, ListStylePosition, ListStyleImage,

    // Table
    TableLayout, BorderCollapse, BorderSpacing,
    CaptionSide, EmptyCells,

    // Cursor & Interaction
    Cursor,
    PointerEvents,
    UserSelect,
    TouchAction,
    Resize,
    ScrollBehavior,
    ScrollSnapType, ScrollSnapAlign,

    // Sizing
    AspectRatio,
    ObjectFit, ObjectPosition,
    Contain,
    ContainerType, ContainerName,
    ContentVisibility,

    // Misc
    WillChange,
    Appearance,
    Content,
    Quotes,
    CounterIncrement, CounterReset, CounterSet,
    ClipPath,
    Mask, MaskImage, MaskSize, MaskRepeat, MaskPosition,
    ShapeOutside, ShapeMargin, ShapeImageThreshold,
    ColumnCount, ColumnWidth, ColumnGap_MC, ColumnRule,
    ColumnRuleWidth, ColumnRuleStyle, ColumnRuleColor,
    ColumnSpan, ColumnFill, Columns,
    PageBreakBefore, PageBreakAfter, PageBreakInside,
    BreakBefore, BreakAfter, BreakInside,
    Orphans, Widows,
    AccentColor,
    CaretColor,
    ColorScheme,
    ForcedColorAdjust,
    PrintColorAdjust,
    All,

    _Count
};

static constexpr uint16_t CSS_PROPERTY_COUNT = static_cast<uint16_t>(CSSPropertyID::_Count);

// ==================================================================
// Property metadata
// ==================================================================

struct CSSPropertyMeta {
    CSSPropertyID id;
    const char* name;
    bool inherited;
    bool isShorthand;
    CSSPropertyID longhandFirst;  // First longhand if shorthand
    uint8_t longhandCount;        // Number of longhands
};

// ==================================================================
// Value types
// ==================================================================

enum class CSSValueType : uint8_t {
    Keyword,
    Length,
    Percentage,
    Color,
    Number,
    Integer,
    String,
    URL,
    Function,     // calc(), var(), etc
    Gradient,
    Image,
    Time,
    Angle,
    Resolution,
    Flex,         // fr unit
    Custom,       // var(--x)
    Initial,
    Inherit,
    Unset,
    Revert,
};

// ==================================================================
// Length units
// ==================================================================

enum class CSSLengthUnit : uint8_t {
    Px, Em, Rem, Ex, Ch, Cap, Ic, Lh, Rlh,
    Vw, Vh, Vmin, Vmax, Vi, Vb,
    Svw, Svh, Lvw, Lvh, Dvw, Dvh,
    Cm, Mm, In, Pt, Pc, Q,
    Percent,
    Fr,
    Auto,
};

// ==================================================================
// Parsed CSS value
// ==================================================================

struct CSSValue {
    CSSValueType type = CSSValueType::Keyword;
    float numericValue = 0;
    CSSLengthUnit lengthUnit = CSSLengthUnit::Px;
    uint32_t colorValue = 0;      // RGBA packed
    std::string stringValue;
    std::string functionName;     // For function values

    bool isAuto() const { return type == CSSValueType::Keyword && stringValue == "auto"; }
    bool isNone() const { return type == CSSValueType::Keyword && stringValue == "none"; }
    bool isInherit() const { return type == CSSValueType::Inherit; }
    bool isInitial() const { return type == CSSValueType::Initial; }

    static CSSValue keyword(const std::string& kw);
    static CSSValue length(float val, CSSLengthUnit unit);
    static CSSValue percentage(float val);
    static CSSValue color(uint32_t rgba);
    static CSSValue number(float val);
    static CSSValue string(const std::string& val);
    static CSSValue initial();
    static CSSValue inherit();
};

// ==================================================================
// Property table access
// ==================================================================

const CSSPropertyMeta& getPropertyMeta(CSSPropertyID id);
CSSPropertyID propertyIDFromString(const std::string& name);
const char* propertyIDToString(CSSPropertyID id);
bool isInheritedProperty(CSSPropertyID id);
bool isShorthandProperty(CSSPropertyID id);

} // namespace Web
} // namespace NXRender
