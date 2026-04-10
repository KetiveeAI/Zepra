// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "painters.h"
#include <algorithm>
#include <cmath>
#include <cctype>

namespace NXRender {
namespace Web {

// ==================================================================
// BackgroundPainter
// ==================================================================

Rect BackgroundPainter::computePaintingArea(const BoxNode* node, BackgroundClip clip) {
    Rect b = node->contentRect();
    auto pad = node->padding();
    auto border = node->borderWidths();

    switch (clip) {
        case BackgroundClip::ContentBox:
            return b;
        case BackgroundClip::PaddingBox:
            return Rect(b.x - pad.left, b.y - pad.top,
                        b.width + pad.left + pad.right,
                        b.height + pad.top + pad.bottom);
        case BackgroundClip::BorderBox:
        default:
            return Rect(b.x - pad.left - border.left,
                        b.y - pad.top - border.top,
                        b.width + pad.left + pad.right + border.left + border.right,
                        b.height + pad.top + pad.bottom + border.top + border.bottom);
        case BackgroundClip::Text:
            return b; // Text clip handled separately
    }
}

Rect BackgroundPainter::computePositioningArea(const BoxNode* node, BackgroundOrigin origin) {
    Rect b = node->contentRect();
    auto pad = node->padding();
    auto border = node->borderWidths();

    switch (origin) {
        case BackgroundOrigin::ContentBox:
            return b;
        case BackgroundOrigin::PaddingBox:
            return Rect(b.x - pad.left, b.y - pad.top,
                        b.width + pad.left + pad.right,
                        b.height + pad.top + pad.bottom);
        case BackgroundOrigin::BorderBox:
        default:
            return Rect(b.x - pad.left - border.left,
                        b.y - pad.top - border.top,
                        b.width + pad.left + pad.right + border.left + border.right,
                        b.height + pad.top + pad.bottom + border.top + border.bottom);
    }
}

void BackgroundPainter::paint(const BoxNode* node, const BackgroundStyle& style, PaintList& list) {
    // Paint in reverse order — bottom layer first
    paintColor(node, style, list);

    for (int i = static_cast<int>(style.layers.size()) - 1; i >= 0; i--) {
        const auto& layer = style.layers[i];
        if (layer.image.type == BackgroundImage::Type::None) continue;

        Rect paintArea = computePaintingArea(node, layer.clip);
        Rect posArea = computePositioningArea(node, layer.origin);

        list.addOp(PaintOp::pushClip(paintArea.x, paintArea.y, paintArea.width, paintArea.height));
        paintLayer(node, layer, paintArea, posArea, list);
        list.addOp(PaintOp::popClip());
    }
}

void BackgroundPainter::paintColor(const BoxNode* node, const BackgroundStyle& style, PaintList& list) {
    if ((style.color & 0xFF) == 0) return; // transparent

    BackgroundClip clip = BackgroundClip::BorderBox;
    if (!style.layers.empty()) clip = style.layers.back().clip;

    Rect area = computePaintingArea(node, clip);
    list.addOp(PaintOp::fillRect(area.x, area.y, area.width, area.height, style.color));
}

void BackgroundPainter::paintLayer(const BoxNode* node, const BackgroundLayer& layer,
                                    const Rect& paintArea, const Rect& posArea, PaintList& list) {
    if (layer.image.type == BackgroundImage::Type::LinearGradient ||
        layer.image.type == BackgroundImage::Type::RadialGradient ||
        layer.image.type == BackgroundImage::Type::ConicGradient) {
        paintGradientLayer(node, layer, paintArea, posArea, list);
    } else if (layer.image.type == BackgroundImage::Type::URL) {
        paintImageLayer(node, layer, paintArea, posArea, list);
    }
}

void BackgroundPainter::paintGradientLayer(const BoxNode* /*node*/, const BackgroundLayer& layer,
                                            const Rect& paintArea, const Rect& /*posArea*/,
                                            PaintList& list) {
    PaintOp op;
    op.type = PaintOpType::FillGradient;
    op.x = paintArea.x;
    op.y = paintArea.y;
    op.width = paintArea.width;
    op.height = paintArea.height;
    op.gradient = layer.image.gradient;
    list.addOp(std::move(op));
}

void BackgroundPainter::computeImageSize(const BackgroundLayer& layer,
                                          float intrinsicW, float intrinsicH,
                                          const Rect& posArea,
                                          float& outW, float& outH) {
    switch (layer.sizeMode) {
        case BackgroundSize::Cover: {
            float scaleX = posArea.width / intrinsicW;
            float scaleY = posArea.height / intrinsicH;
            float scale = std::max(scaleX, scaleY);
            outW = intrinsicW * scale;
            outH = intrinsicH * scale;
            break;
        }
        case BackgroundSize::Contain: {
            float scaleX = posArea.width / intrinsicW;
            float scaleY = posArea.height / intrinsicH;
            float scale = std::min(scaleX, scaleY);
            outW = intrinsicW * scale;
            outH = intrinsicH * scale;
            break;
        }
        case BackgroundSize::Explicit: {
            outW = layer.sizeWAuto ? intrinsicW : layer.sizeW;
            outH = layer.sizeHAuto ? intrinsicH : layer.sizeH;
            // Maintain aspect if one dimension is auto
            if (layer.sizeWAuto && !layer.sizeHAuto && intrinsicH > 0) {
                outW = intrinsicW * (outH / intrinsicH);
            } else if (!layer.sizeWAuto && layer.sizeHAuto && intrinsicW > 0) {
                outH = intrinsicH * (outW / intrinsicW);
            }
            break;
        }
        case BackgroundSize::Auto:
        default:
            outW = intrinsicW;
            outH = intrinsicH;
            break;
    }
}

void BackgroundPainter::computeTilePositions(const BackgroundLayer& layer,
                                              float imgW, float imgH,
                                              const Rect& posArea, const Rect& paintArea,
                                              std::vector<Rect>& tiles) {
    // Compute origin position
    float originX = posArea.x;
    float originY = posArea.y;

    if (layer.positionXPercent) {
        originX += (posArea.width - imgW) * (layer.positionX / 100.0f);
    } else {
        originX += layer.positionX;
    }

    if (layer.positionYPercent) {
        originY += (posArea.height - imgH) * (layer.positionY / 100.0f);
    } else {
        originY += layer.positionY;
    }

    bool repeatH = (layer.repeatX == BackgroundRepeat::Repeat ||
                    layer.repeatX == BackgroundRepeat::Space ||
                    layer.repeatX == BackgroundRepeat::Round);
    bool repeatV = (layer.repeatY == BackgroundRepeat::Repeat ||
                    layer.repeatY == BackgroundRepeat::Space ||
                    layer.repeatY == BackgroundRepeat::Round);

    float startX = repeatH ? paintArea.x - std::fmod(originX - paintArea.x, imgW) - imgW : originX;
    float startY = repeatV ? paintArea.y - std::fmod(originY - paintArea.y, imgH) - imgH : originY;
    float endX = repeatH ? paintArea.x + paintArea.width + imgW : originX + imgW;
    float endY = repeatV ? paintArea.y + paintArea.height + imgH : originY + imgH;

    float spacingX = 0, spacingY = 0;
    if (layer.repeatX == BackgroundRepeat::Space && imgW > 0) {
        int count = static_cast<int>(posArea.width / imgW);
        if (count > 1) spacingX = (posArea.width - count * imgW) / (count - 1);
    }
    if (layer.repeatY == BackgroundRepeat::Space && imgH > 0) {
        int count = static_cast<int>(posArea.height / imgH);
        if (count > 1) spacingY = (posArea.height - count * imgH) / (count - 1);
    }

    for (float ty = startY; ty < endY; ty += imgH + spacingY) {
        for (float tx = startX; tx < endX; tx += imgW + spacingX) {
            tiles.push_back(Rect(tx, ty, imgW, imgH));
            if (!repeatH) break;
        }
        if (!repeatV) break;
    }
}

void BackgroundPainter::paintImageLayer(const BoxNode* /*node*/, const BackgroundLayer& layer,
                                         const Rect& paintArea, const Rect& posArea,
                                         PaintList& list) {
    float imgW = 0, imgH = 0;
    // Intrinsic size would come from image decode — use posArea for now
    float intrinsicW = posArea.width;
    float intrinsicH = posArea.height;
    computeImageSize(layer, intrinsicW, intrinsicH, posArea, imgW, imgH);

    if (imgW <= 0 || imgH <= 0) return;

    std::vector<Rect> tiles;
    computeTilePositions(layer, imgW, imgH, posArea, paintArea, tiles);

    for (const auto& tile : tiles) {
        list.addOp(PaintOp::drawImage(tile.x, tile.y, tile.width, tile.height, layer.image.url));
    }
}

// ==================================================================
// BorderPainter
// ==================================================================

bool BorderSpec::hasVisibleBorder() const {
    return (top.width > 0 && top.style != BorderStyle::None && top.style != BorderStyle::Hidden) ||
           (right.width > 0 && right.style != BorderStyle::None && right.style != BorderStyle::Hidden) ||
           (bottom.width > 0 && bottom.style != BorderStyle::None && bottom.style != BorderStyle::Hidden) ||
           (left.width > 0 && left.style != BorderStyle::None && left.style != BorderStyle::Hidden);
}

bool BorderSpec::isUniform() const {
    return top.width == right.width && right.width == bottom.width && bottom.width == left.width &&
           top.color == right.color && right.color == bottom.color && bottom.color == left.color &&
           top.style == right.style && right.style == bottom.style && bottom.style == left.style;
}

void BorderPainter::paint(const BoxNode* node, const BorderSpec& border, PaintList& list) {
    if (!border.hasVisibleBorder()) return;

    Rect b = node->borderRect();

    if (border.isUniform() && border.top.style == BorderStyle::Solid) {
        paintUniformBorder(node, border, list);
        return;
    }

    // Paint each side independently
    const BorderSide* sides[4] = { &border.top, &border.right, &border.bottom, &border.left };
    for (int i = 0; i < 4; i++) {
        if (sides[i]->width > 0 && sides[i]->style != BorderStyle::None &&
            sides[i]->style != BorderStyle::Hidden) {
            paintSideBorder(node, *sides[i], i, border, list);
        }
    }
    (void)b;
}

void BorderPainter::paintUniformBorder(const BoxNode* node, const BorderSpec& border,
                                         PaintList& list) {
    Rect b = node->borderRect();
    float bw = border.top.width;

    if (border.hasRadius()) {
        // Outer rounded rect
        list.addOp(PaintOp::fillRoundedRect(b.x, b.y, b.width, b.height,
                                             border.radiusTL, border.radiusTR,
                                             border.radiusBR, border.radiusBL,
                                             border.top.color));
        // Inner rect (subtract border)
        float innerTL = std::max(0.0f, border.radiusTL - bw);
        float innerTR = std::max(0.0f, border.radiusTR - bw);
        float innerBR = std::max(0.0f, border.radiusBR - bw);
        float innerBL = std::max(0.0f, border.radiusBL - bw);
        list.addOp(PaintOp::fillRoundedRect(b.x + bw, b.y + bw,
                                             b.width - 2 * bw, b.height - 2 * bw,
                                             innerTL, innerTR, innerBR, innerBL,
                                             0xFFFFFF00)); // Transparent inner
    } else {
        // Top
        list.addOp(PaintOp::fillRect(b.x, b.y, b.width, bw, border.top.color));
        // Bottom
        list.addOp(PaintOp::fillRect(b.x, b.y + b.height - bw, b.width, bw, border.top.color));
        // Left
        list.addOp(PaintOp::fillRect(b.x, b.y + bw, bw, b.height - 2 * bw, border.top.color));
        // Right
        list.addOp(PaintOp::fillRect(b.x + b.width - bw, b.y + bw, bw,
                                      b.height - 2 * bw, border.top.color));
    }
}

void BorderPainter::paintSideBorder(const BoxNode* node, const BorderSide& side,
                                      uint8_t sideIdx, const BorderSpec& border,
                                      PaintList& list) {
    Rect b = node->borderRect();

    float x = b.x, y = b.y, w = b.width, h = b.height;

    switch (side.style) {
        case BorderStyle::Solid:
            paintSolidSide(x, y, w, h, sideIdx, side.width, side.color, list);
            break;
        case BorderStyle::Dashed:
            paintDashedSide(x, y, w, h, sideIdx, side.width, side.color, list);
            break;
        case BorderStyle::Dotted:
            paintDottedSide(x, y, w, h, sideIdx, side.width, side.color, list);
            break;
        case BorderStyle::Double:
            paintDoubleSide(x, y, w, h, sideIdx, side.width, side.color, list);
            break;
        case BorderStyle::Groove:
            paintGrooveSide(x, y, w, h, sideIdx, side.width, side.color, false, list);
            break;
        case BorderStyle::Ridge:
            paintGrooveSide(x, y, w, h, sideIdx, side.width, side.color, true, list);
            break;
        case BorderStyle::Inset: {
            bool dark = (sideIdx == 0 || sideIdx == 3); // top, left
            uint32_t c = dark ? darkenColor(side.color, 0.6f) : lightenColor(side.color, 0.4f);
            paintSolidSide(x, y, w, h, sideIdx, side.width, c, list);
            break;
        }
        case BorderStyle::Outset: {
            bool dark = (sideIdx == 2 || sideIdx == 1); // bottom, right
            uint32_t c = dark ? darkenColor(side.color, 0.6f) : lightenColor(side.color, 0.4f);
            paintSolidSide(x, y, w, h, sideIdx, side.width, c, list);
            break;
        }
        default:
            break;
    }
    (void)border;
}

void BorderPainter::paintSolidSide(float x, float y, float w, float h,
                                     uint8_t side, float bw, uint32_t color, PaintList& list) {
    list.addOp(PaintOp::drawBorderSide(x, y, w, h, side, bw, color, 1));
}

void BorderPainter::paintDashedSide(float x, float y, float w, float h,
                                      uint8_t side, float bw, uint32_t color, PaintList& list) {
    float dashLen = bw * 3;
    float gapLen = dashLen;
    bool horiz = (side == 0 || side == 2);
    float length = horiz ? w : h;
    float pos = 0;

    while (pos < length) {
        float segLen = std::min(dashLen, length - pos);
        if (horiz) {
            float sy = (side == 0) ? y : y + h - bw;
            list.addOp(PaintOp::fillRect(x + pos, sy, segLen, bw, color));
        } else {
            float sx = (side == 3) ? x : x + w - bw;
            list.addOp(PaintOp::fillRect(sx, y + pos, bw, segLen, color));
        }
        pos += dashLen + gapLen;
    }
}

void BorderPainter::paintDottedSide(float x, float y, float w, float h,
                                      uint8_t side, float bw, uint32_t color, PaintList& list) {
    float dotSize = bw;
    float gap = dotSize;
    bool horiz = (side == 0 || side == 2);
    float length = horiz ? w : h;
    float pos = 0;

    while (pos < length) {
        if (horiz) {
            float sy = (side == 0) ? y : y + h - bw;
            list.addOp(PaintOp::fillRect(x + pos, sy, dotSize, dotSize, color));
        } else {
            float sx = (side == 3) ? x : x + w - bw;
            list.addOp(PaintOp::fillRect(sx, y + pos, dotSize, dotSize, color));
        }
        pos += dotSize + gap;
    }
}

void BorderPainter::paintDoubleSide(float x, float y, float w, float h,
                                      uint8_t side, float bw, uint32_t color, PaintList& list) {
    float lineW = std::max(1.0f, bw / 3.0f);
    // Outer line
    paintSolidSide(x, y, w, h, side, lineW, color, list);
    // Inner line (offset by 2*lineW)
    float offset = bw - lineW;
    switch (side) {
        case 0: // top
            list.addOp(PaintOp::fillRect(x, y + offset, w, lineW, color)); break;
        case 1: // right
            list.addOp(PaintOp::fillRect(x + w - bw + (bw - lineW - offset), y, lineW, h, color)); break;
        case 2: // bottom
            list.addOp(PaintOp::fillRect(x, y + h - bw + (bw - lineW - offset), w, lineW, color)); break;
        case 3: // left
            list.addOp(PaintOp::fillRect(x + offset, y, lineW, h, color)); break;
    }
}

void BorderPainter::paintGrooveSide(float x, float y, float w, float h,
                                      uint8_t side, float bw, uint32_t color,
                                      bool ridge, PaintList& list) {
    float halfBW = bw / 2;
    uint32_t dark = darkenColor(color, 0.5f);
    uint32_t light = lightenColor(color, 0.5f);

    uint32_t outer = ridge ? light : dark;
    uint32_t inner = ridge ? dark : light;

    // Outer half
    switch (side) {
        case 0: list.addOp(PaintOp::fillRect(x, y, w, halfBW, outer)); break;
        case 1: list.addOp(PaintOp::fillRect(x + w - halfBW, y, halfBW, h, outer)); break;
        case 2: list.addOp(PaintOp::fillRect(x, y + h - halfBW, w, halfBW, outer)); break;
        case 3: list.addOp(PaintOp::fillRect(x, y, halfBW, h, outer)); break;
    }
    // Inner half
    switch (side) {
        case 0: list.addOp(PaintOp::fillRect(x, y + halfBW, w, halfBW, inner)); break;
        case 1: list.addOp(PaintOp::fillRect(x + w - bw, y, halfBW, h, inner)); break;
        case 2: list.addOp(PaintOp::fillRect(x, y + h - bw, w, halfBW, inner)); break;
        case 3: list.addOp(PaintOp::fillRect(x + halfBW, y, halfBW, h, inner)); break;
    }
}

uint32_t BorderPainter::lightenColor(uint32_t color, float factor) {
    uint8_t r = (color >> 24) & 0xFF;
    uint8_t g = (color >> 16) & 0xFF;
    uint8_t b = (color >> 8) & 0xFF;
    uint8_t a = color & 0xFF;
    r = static_cast<uint8_t>(std::min(255.0f, r + (255 - r) * factor));
    g = static_cast<uint8_t>(std::min(255.0f, g + (255 - g) * factor));
    b = static_cast<uint8_t>(std::min(255.0f, b + (255 - b) * factor));
    return (r << 24) | (g << 16) | (b << 8) | a;
}

uint32_t BorderPainter::darkenColor(uint32_t color, float factor) {
    uint8_t r = (color >> 24) & 0xFF;
    uint8_t g = (color >> 16) & 0xFF;
    uint8_t b = (color >> 8) & 0xFF;
    uint8_t a = color & 0xFF;
    r = static_cast<uint8_t>(r * (1 - factor));
    g = static_cast<uint8_t>(g * (1 - factor));
    b = static_cast<uint8_t>(b * (1 - factor));
    return (r << 24) | (g << 16) | (b << 8) | a;
}

// ==================================================================
// OutlinePainter
// ==================================================================

void OutlinePainter::paint(const BoxNode* node, const OutlineSpec& outline, PaintList& list) {
    if (outline.width <= 0 || outline.style == BorderStyle::None) return;

    Rect b = node->borderRect();
    float offset = outline.offset;
    float ow = outline.width;

    Rect outer(b.x - offset - ow, b.y - offset - ow,
               b.width + 2 * (offset + ow), b.height + 2 * (offset + ow));

    // Top
    list.addOp(PaintOp::fillRect(outer.x, outer.y, outer.width, ow, outline.color));
    // Bottom
    list.addOp(PaintOp::fillRect(outer.x, outer.y + outer.height - ow, outer.width, ow, outline.color));
    // Left
    list.addOp(PaintOp::fillRect(outer.x, outer.y + ow, ow, outer.height - 2 * ow, outline.color));
    // Right
    list.addOp(PaintOp::fillRect(outer.x + outer.width - ow, outer.y + ow,
                                  ow, outer.height - 2 * ow, outline.color));
}

// ==================================================================
// TextPainter
// ==================================================================

std::string TextPainter::applyTextTransform(const std::string& text, const std::string& transform) {
    if (transform == "uppercase") {
        std::string result = text;
        for (auto& c : result) c = std::toupper(c);
        return result;
    }
    if (transform == "lowercase") {
        std::string result = text;
        for (auto& c : result) c = std::tolower(c);
        return result;
    }
    if (transform == "capitalize") {
        std::string result = text;
        bool nextUpper = true;
        for (auto& c : result) {
            if (std::isspace(c)) { nextUpper = true; }
            else if (nextUpper) { c = std::toupper(c); nextUpper = false; }
        }
        return result;
    }
    return text;
}

void TextPainter::paint(const BoxNode* /*node*/, const TextPaintStyle& style,
                         float x, float y, PaintList& list) {
    std::string text = applyTextTransform(style.text, style.textTransform);
    if (text.empty()) return;

    float charWidth = style.fontSize * 0.6f;

    // Shadows (painted behind text)
    paintShadows(style, x, y, list);

    // Selection highlight
    if (style.selection.startOffset >= 0) {
        paintSelection(style, x, y, charWidth, list);
    }

    // Main text
    list.addOp(PaintOp::drawText(x, y, text, style.fontFamily, style.fontSize, style.color));

    // Text decorations
    float textWidth = text.length() * charWidth;
    float ascent = style.fontSize * 0.8f;
    float descent = style.fontSize * 0.2f;

    if (style.decoration.lines & static_cast<uint8_t>(TextDecorationLine::Underline)) {
        float lineY = y + descent + 2;
        paintDecorationLine(x, lineY, textWidth, style.decoration.thickness,
                            style.decoration.style, style.decoration.color, list);
    }
    if (style.decoration.lines & static_cast<uint8_t>(TextDecorationLine::Overline)) {
        float lineY = y - ascent;
        paintDecorationLine(x, lineY, textWidth, style.decoration.thickness,
                            style.decoration.style, style.decoration.color, list);
    }
    if (style.decoration.lines & static_cast<uint8_t>(TextDecorationLine::LineThrough)) {
        float lineY = y - ascent * 0.35f;
        paintDecorationLine(x, lineY, textWidth, style.decoration.thickness,
                            style.decoration.style, style.decoration.color, list);
    }

    // Caret
    if (style.caret.visible) {
        paintCaret(style, x, y, charWidth, list);
    }
}

void TextPainter::paintShadows(const TextPaintStyle& style, float x, float y, PaintList& list) {
    for (const auto& shadow : style.shadows) {
        float sx = x + shadow.offsetX;
        float sy = y + shadow.offsetY;
        list.addOp(PaintOp::drawText(sx, sy, style.text, style.fontFamily,
                                      style.fontSize, shadow.color));
    }
}

void TextPainter::paintSelection(const TextPaintStyle& style, float x, float y,
                                   float charWidth, PaintList& list) {
    int start = std::max(0, style.selection.startOffset);
    int end = std::min(static_cast<int>(style.text.size()), style.selection.endOffset);
    if (start >= end) return;

    float selX = x + start * charWidth;
    float selW = (end - start) * charWidth;
    float selY = y - style.fontSize * 0.8f;
    float selH = style.fontSize * 1.2f;

    list.addOp(PaintOp::fillRect(selX, selY, selW, selH, style.selection.bgColor));
    // Re-draw selected text with selection color
    std::string selText = style.text.substr(start, end - start);
    list.addOp(PaintOp::drawText(selX, y, selText, style.fontFamily,
                                  style.fontSize, style.selection.textColor));
}

void TextPainter::paintDecorationLine(float x, float y, float width, float thickness,
                                        TextDecorationStyle style, uint32_t color,
                                        PaintList& list) {
    switch (style) {
        case TextDecorationStyle::Solid:
            list.addOp(PaintOp::fillRect(x, y, width, thickness, color));
            break;
        case TextDecorationStyle::Double:
            list.addOp(PaintOp::fillRect(x, y, width, thickness, color));
            list.addOp(PaintOp::fillRect(x, y + thickness * 3, width, thickness, color));
            break;
        case TextDecorationStyle::Dotted: {
            float pos = 0;
            while (pos < width) {
                list.addOp(PaintOp::fillRect(x + pos, y, thickness, thickness, color));
                pos += thickness * 2;
            }
            break;
        }
        case TextDecorationStyle::Dashed: {
            float dashLen = thickness * 4;
            float pos = 0;
            while (pos < width) {
                float len = std::min(dashLen, width - pos);
                list.addOp(PaintOp::fillRect(x + pos, y, len, thickness, color));
                pos += dashLen * 2;
            }
            break;
        }
        case TextDecorationStyle::Wavy: {
            // Approximate wavy with short segments at alternating heights
            float waveLength = thickness * 4;
            float amplitude = thickness;
            for (float pos = 0; pos < width; pos += waveLength / 2) {
                float segLen = std::min(waveLength / 2, width - pos);
                int phase = static_cast<int>(pos / (waveLength / 2)) % 2;
                float segY = y + (phase ? amplitude : -amplitude);
                list.addOp(PaintOp::fillRect(x + pos, segY, segLen, thickness, color));
            }
            break;
        }
    }
}

void TextPainter::paintCaret(const TextPaintStyle& style, float x, float y,
                               float charWidth, PaintList& list) {
    // Blink: visible on even phases
    if (std::fmod(style.caret.blinkPhase, 1.0f) > 0.5f) return;

    float caretX = x + style.caret.offset * charWidth;
    float caretY = y - style.fontSize * 0.8f;
    list.addOp(PaintOp::fillRect(caretX, caretY, 1.5f, style.caret.height, style.caret.color));
}

// ==================================================================
// BoxShadowPainter
// ==================================================================

void BoxShadowPainter::paint(const BoxNode* node, const std::vector<BoxShadowSpec>& shadows,
                               const BorderSpec& border, PaintList& list) {
    // Paint outset shadows first (behind the element), then inset
    for (auto it = shadows.rbegin(); it != shadows.rend(); ++it) {
        if (!it->inset) paintOutsetShadow(node, *it, border, list);
    }
    for (const auto& shadow : shadows) {
        if (shadow.inset) paintInsetShadow(node, shadow, border, list);
    }
}

void BoxShadowPainter::paintOutsetShadow(const BoxNode* node, const BoxShadowSpec& shadow,
                                            const BorderSpec& border, PaintList& list) {
    Rect b = node->borderRect();

    float sx = b.x + shadow.offsetX - shadow.spreadRadius;
    float sy = b.y + shadow.offsetY - shadow.spreadRadius;
    float sw = b.width + 2 * shadow.spreadRadius;
    float sh = b.height + 2 * shadow.spreadRadius;

    if (border.hasRadius()) {
        float spreadTL = border.radiusTL + shadow.spreadRadius;
        float spreadTR = border.radiusTR + shadow.spreadRadius;
        float spreadBR = border.radiusBR + shadow.spreadRadius;
        float spreadBL = border.radiusBL + shadow.spreadRadius;
        list.addOp(PaintOp::drawShadow(sx, sy, sw, sh,
                                        shadow.offsetX, shadow.offsetY,
                                        shadow.blurRadius, shadow.spreadRadius,
                                        shadow.color, false));
        (void)spreadTL; (void)spreadTR; (void)spreadBR; (void)spreadBL;
    } else {
        list.addOp(PaintOp::drawShadow(sx, sy, sw, sh,
                                        shadow.offsetX, shadow.offsetY,
                                        shadow.blurRadius, shadow.spreadRadius,
                                        shadow.color, false));
    }
}

void BoxShadowPainter::paintInsetShadow(const BoxNode* node, const BoxShadowSpec& shadow,
                                           const BorderSpec& /*border*/, PaintList& list) {
    Rect b = node->borderRect();
    list.addOp(PaintOp::drawShadow(b.x, b.y, b.width, b.height,
                                    shadow.offsetX, shadow.offsetY,
                                    shadow.blurRadius, shadow.spreadRadius,
                                    shadow.color, true));
}

} // namespace Web
} // namespace NXRender
