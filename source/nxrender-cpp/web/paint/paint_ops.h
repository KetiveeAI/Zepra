// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "../box/box_tree.h"
#include "../css/value_parser.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace NXRender {
namespace Web {

// ==================================================================
// Paint operation types
// ==================================================================

enum class PaintOpType : uint8_t {
    FillRect,
    StrokeRect,
    FillRoundedRect,
    DrawBorder,
    DrawBorderSide,
    FillGradient,
    DrawImage,
    DrawText,
    DrawTextDecoration,
    DrawShadow,
    PushClip,
    PopClip,
    PushTransform,
    PopTransform,
    PushOpacity,
    PopOpacity,
    PushFilter,
    PopFilter,
};

// ==================================================================
// Paint operation
// ==================================================================

struct PaintOp {
    PaintOpType type;

    // Geometry
    float x = 0, y = 0;
    float width = 0, height = 0;

    // Corners (for rounded rect)
    float radiusTL = 0, radiusTR = 0;
    float radiusBR = 0, radiusBL = 0;

    // Color
    uint32_t color = 0;
    float opacity = 1.0f;

    // Border
    float borderWidth = 0;
    uint8_t borderStyle = 0;  // 0=none, 1=solid, 2=dashed, 3=dotted, 4=double
    uint8_t side = 0;         // 0=top, 1=right, 2=bottom, 3=left

    // Text
    std::string text;
    std::string fontFamily;
    float fontSize = 16;
    uint16_t fontWeight = 400;
    bool fontItalic = false;

    // Image
    std::string imageURL;
    uint32_t textureId = 0;

    // Transform
    float matrix[6] = {1,0,0,1,0,0}; // 2D affine

    // Shadow
    float shadowOffsetX = 0, shadowOffsetY = 0;
    float shadowBlur = 0, shadowSpread = 0;
    bool shadowInset = false;

    // Gradient
    Gradient gradient;

    // Filter
    std::string filterStr;

    // Factory methods
    static PaintOp fillRect(float x, float y, float w, float h, uint32_t color);
    static PaintOp strokeRect(float x, float y, float w, float h, uint32_t color, float lineWidth);
    static PaintOp fillRoundedRect(float x, float y, float w, float h,
                                    float tl, float tr, float br, float bl, uint32_t color);
    static PaintOp drawBorderSide(float x, float y, float w, float h,
                                   uint8_t side, float width, uint32_t color, uint8_t style);
    static PaintOp drawText(float x, float y, const std::string& text,
                            const std::string& font, float size, uint32_t color);
    static PaintOp pushClip(float x, float y, float w, float h);
    static PaintOp popClip();
    static PaintOp pushOpacity(float opacity);
    static PaintOp popOpacity();
    static PaintOp drawShadow(float x, float y, float w, float h,
                               float ox, float oy, float blur, float spread,
                               uint32_t color, bool inset);
    static PaintOp drawImage(float x, float y, float w, float h,
                             const std::string& url, uint32_t texId = 0);
};

// ==================================================================
// Paint list — ordered list of operations for a frame
// ==================================================================

class PaintList {
public:
    PaintList();
    ~PaintList();

    void addOp(const PaintOp& op);
    void addOp(PaintOp&& op);
    const std::vector<PaintOp>& ops() const { return ops_; }
    size_t opCount() const { return ops_.size(); }
    void clear();

    // Merge another list
    void append(const PaintList& other);

private:
    std::vector<PaintOp> ops_;
};

// ==================================================================
// Paint tree builder — generates PaintList from BoxNode tree
// ==================================================================

class PaintTreeBuilder {
public:
    PaintTreeBuilder();
    ~PaintTreeBuilder();

    PaintList build(const BoxNode* root);

private:
    void paintNode(const BoxNode* node, PaintList& list);
    void paintBackground(const BoxNode* node, PaintList& list);
    void paintBorders(const BoxNode* node, PaintList& list);
    void paintText(const BoxNode* node, PaintList& list);
    void paintOutline(const BoxNode* node, PaintList& list);
    void paintBoxShadow(const BoxNode* node, PaintList& list);
    void paintChildren(const BoxNode* node, PaintList& list);

    // Stacking context aware painting (CSS Appendix E)
    void paintStackingContext(const BoxNode* node, PaintList& list);
    void sortByZOrder(std::vector<const BoxNode*>& nodes);
};

// ==================================================================
// Hit testing
// ==================================================================

struct HitTestResult {
    const BoxNode* node = nullptr;
    float localX = 0, localY = 0;
    int depth = 0;
};

class HitTester {
public:
    HitTestResult hitTest(const BoxNode* root, float x, float y);

private:
    bool hitTestNode(const BoxNode* node, float x, float y,
                     int depth, HitTestResult& result);
};

} // namespace Web
} // namespace NXRender
