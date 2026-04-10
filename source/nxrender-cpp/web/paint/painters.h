// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "../box/box_tree.h"
#include "../css/value_parser.h"
#include "paint_ops.h"
#include <vector>
#include <string>

namespace NXRender {
namespace Web {

// ==================================================================
// Background painter
// CSS Backgrounds & Borders Level 3 specification
// ==================================================================

enum class BackgroundClip : uint8_t {
    BorderBox, PaddingBox, ContentBox, Text
};

enum class BackgroundOrigin : uint8_t {
    BorderBox, PaddingBox, ContentBox
};

enum class BackgroundSize : uint8_t {
    Auto, Cover, Contain, Explicit
};

enum class BackgroundRepeat : uint8_t {
    Repeat, RepeatX, RepeatY, NoRepeat, Space, Round
};

enum class BackgroundAttachment : uint8_t {
    Scroll, Fixed, Local
};

struct BackgroundImage {
    enum class Type { None, URL, LinearGradient, RadialGradient, ConicGradient } type = Type::None;
    std::string url;
    Gradient gradient;
};

struct BackgroundLayer {
    BackgroundImage image;
    BackgroundClip clip = BackgroundClip::BorderBox;
    BackgroundOrigin origin = BackgroundOrigin::PaddingBox;
    BackgroundSize sizeMode = BackgroundSize::Auto;
    float sizeW = 0, sizeH = 0;       // explicit dimensions (if sizeMode == Explicit)
    bool sizeWAuto = true, sizeHAuto = true;
    BackgroundRepeat repeatX = BackgroundRepeat::Repeat;
    BackgroundRepeat repeatY = BackgroundRepeat::Repeat;
    BackgroundAttachment attachment = BackgroundAttachment::Scroll;
    float positionX = 0, positionY = 0;
    bool positionXPercent = false, positionYPercent = false;
};

struct BackgroundStyle {
    uint32_t color = 0x00000000;
    std::vector<BackgroundLayer> layers;
};

class BackgroundPainter {
public:
    void paint(const BoxNode* node, const BackgroundStyle& style, PaintList& list);

private:
    Rect computePaintingArea(const BoxNode* node, BackgroundClip clip);
    Rect computePositioningArea(const BoxNode* node, BackgroundOrigin origin);
    void paintColor(const BoxNode* node, const BackgroundStyle& style, PaintList& list);
    void paintLayer(const BoxNode* node, const BackgroundLayer& layer,
                    const Rect& paintArea, const Rect& posArea, PaintList& list);
    void paintGradientLayer(const BoxNode* node, const BackgroundLayer& layer,
                            const Rect& paintArea, const Rect& posArea, PaintList& list);
    void paintImageLayer(const BoxNode* node, const BackgroundLayer& layer,
                         const Rect& paintArea, const Rect& posArea, PaintList& list);
    void computeImageSize(const BackgroundLayer& layer, float intrinsicW, float intrinsicH,
                          const Rect& posArea, float& outW, float& outH);
    void computeTilePositions(const BackgroundLayer& layer, float imgW, float imgH,
                              const Rect& posArea, const Rect& paintArea,
                              std::vector<Rect>& tiles);
};

// ==================================================================
// Border painter
// CSS Borders — all styles, separate sides, radius
// ==================================================================

enum class BorderStyle : uint8_t {
    None, Hidden, Solid, Dashed, Dotted, Double,
    Groove, Ridge, Inset, Outset
};

struct BorderSide {
    float width = 0;
    uint32_t color = 0x000000FF;
    BorderStyle style = BorderStyle::None;
};

struct BorderSpec {
    BorderSide top, right, bottom, left;
    float radiusTL = 0, radiusTR = 0;
    float radiusBR = 0, radiusBL = 0;

    bool hasVisibleBorder() const;
    bool isUniform() const;
    bool hasRadius() const { return radiusTL > 0 || radiusTR > 0 || radiusBR > 0 || radiusBL > 0; }
};

class BorderPainter {
public:
    void paint(const BoxNode* node, const BorderSpec& border, PaintList& list);

private:
    void paintUniformBorder(const BoxNode* node, const BorderSpec& border, PaintList& list);
    void paintSideBorder(const BoxNode* node, const BorderSide& side,
                         uint8_t sideIdx, const BorderSpec& border, PaintList& list);
    void paintSolidSide(float x, float y, float w, float h,
                        uint8_t side, float bw, uint32_t color, PaintList& list);
    void paintDashedSide(float x, float y, float w, float h,
                         uint8_t side, float bw, uint32_t color, PaintList& list);
    void paintDottedSide(float x, float y, float w, float h,
                         uint8_t side, float bw, uint32_t color, PaintList& list);
    void paintDoubleSide(float x, float y, float w, float h,
                         uint8_t side, float bw, uint32_t color, PaintList& list);
    void paintGrooveSide(float x, float y, float w, float h,
                         uint8_t side, float bw, uint32_t color, bool ridge, PaintList& list);
    uint32_t lightenColor(uint32_t color, float factor);
    uint32_t darkenColor(uint32_t color, float factor);
};

// ==================================================================
// Outline painter
// ==================================================================

struct OutlineSpec {
    float width = 0;
    float offset = 0;
    uint32_t color = 0x000000FF;
    BorderStyle style = BorderStyle::None;
};

class OutlinePainter {
public:
    void paint(const BoxNode* node, const OutlineSpec& outline, PaintList& list);
};

// ==================================================================
// Text painter
// Text decoration, shadow, selection highlight, caret
// ==================================================================

enum class TextDecorationLine : uint8_t {
    None = 0,
    Underline = 1,
    Overline = 2,
    LineThrough = 4,
};

enum class TextDecorationStyle : uint8_t {
    Solid, Double, Dotted, Dashed, Wavy
};

struct TextDecoration {
    uint8_t lines = 0;
    TextDecorationStyle style = TextDecorationStyle::Solid;
    uint32_t color = 0x000000FF;
    float thickness = 1.0f;
};

struct TextShadow {
    float offsetX = 0, offsetY = 0;
    float blur = 0;
    uint32_t color = 0x00000080;
};

struct SelectionRange {
    int startOffset = -1, endOffset = -1;
    uint32_t bgColor = 0x3399FFAA;
    uint32_t textColor = 0xFFFFFFFF;
};

struct CaretState {
    bool visible = false;
    int offset = 0;
    float height = 16;
    uint32_t color = 0x000000FF;
    float blinkPhase = 0;
};

struct TextPaintStyle {
    std::string text;
    std::string fontFamily = "sans-serif";
    float fontSize = 16;
    uint16_t fontWeight = 400;
    bool italic = false;
    uint32_t color = 0x000000FF;
    float lineHeight = 1.2f;
    float letterSpacing = 0;
    float wordSpacing = 0;
    std::string textTransform; // none, uppercase, lowercase, capitalize
    TextDecoration decoration;
    std::vector<TextShadow> shadows;
    SelectionRange selection;
    CaretState caret;
    float textIndent = 0;
    bool whiteSpaceCollapse = true;
    bool wordBreak = false;
    bool overflowWrap = false;
};

class TextPainter {
public:
    void paint(const BoxNode* node, const TextPaintStyle& style,
               float x, float y, PaintList& list);

private:
    std::string applyTextTransform(const std::string& text, const std::string& transform);
    void paintShadows(const TextPaintStyle& style, float x, float y, PaintList& list);
    void paintSelection(const TextPaintStyle& style, float x, float y,
                        float charWidth, PaintList& list);
    void paintDecorationLine(float x, float y, float width, float thickness,
                             TextDecorationStyle style, uint32_t color, PaintList& list);
    void paintCaret(const TextPaintStyle& style, float x, float y,
                    float charWidth, PaintList& list);
};

// ==================================================================
// Box shadow painter
// ==================================================================

struct BoxShadowSpec {
    float offsetX = 0, offsetY = 0;
    float blurRadius = 0;
    float spreadRadius = 0;
    uint32_t color = 0x00000040;
    bool inset = false;
};

class BoxShadowPainter {
public:
    void paint(const BoxNode* node, const std::vector<BoxShadowSpec>& shadows,
               const BorderSpec& border, PaintList& list);

private:
    void paintOutsetShadow(const BoxNode* node, const BoxShadowSpec& shadow,
                           const BorderSpec& border, PaintList& list);
    void paintInsetShadow(const BoxNode* node, const BoxShadowSpec& shadow,
                          const BorderSpec& border, PaintList& list);
};

} // namespace Web
} // namespace NXRender
