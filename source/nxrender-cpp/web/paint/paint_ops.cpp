// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "paint_ops.h"
#include <algorithm>
#include <cmath>

namespace NXRender {
namespace Web {

// ==================================================================
// PaintOp factories
// ==================================================================

PaintOp PaintOp::fillRect(float x, float y, float w, float h, uint32_t color) {
    PaintOp op;
    op.type = PaintOpType::FillRect;
    op.x = x; op.y = y; op.width = w; op.height = h;
    op.color = color;
    return op;
}

PaintOp PaintOp::strokeRect(float x, float y, float w, float h, uint32_t color, float lineWidth) {
    PaintOp op;
    op.type = PaintOpType::StrokeRect;
    op.x = x; op.y = y; op.width = w; op.height = h;
    op.color = color;
    op.borderWidth = lineWidth;
    return op;
}

PaintOp PaintOp::fillRoundedRect(float x, float y, float w, float h,
                                   float tl, float tr, float br, float bl, uint32_t color) {
    PaintOp op;
    op.type = PaintOpType::FillRoundedRect;
    op.x = x; op.y = y; op.width = w; op.height = h;
    op.radiusTL = tl; op.radiusTR = tr;
    op.radiusBR = br; op.radiusBL = bl;
    op.color = color;
    return op;
}

PaintOp PaintOp::drawBorderSide(float x, float y, float w, float h,
                                  uint8_t side, float width, uint32_t color, uint8_t style) {
    PaintOp op;
    op.type = PaintOpType::DrawBorderSide;
    op.x = x; op.y = y; op.width = w; op.height = h;
    op.side = side;
    op.borderWidth = width;
    op.color = color;
    op.borderStyle = style;
    return op;
}

PaintOp PaintOp::drawText(float x, float y, const std::string& text,
                            const std::string& font, float size, uint32_t color) {
    PaintOp op;
    op.type = PaintOpType::DrawText;
    op.x = x; op.y = y;
    op.text = text;
    op.fontFamily = font;
    op.fontSize = size;
    op.color = color;
    return op;
}

PaintOp PaintOp::pushClip(float x, float y, float w, float h) {
    PaintOp op;
    op.type = PaintOpType::PushClip;
    op.x = x; op.y = y; op.width = w; op.height = h;
    return op;
}

PaintOp PaintOp::popClip() {
    PaintOp op;
    op.type = PaintOpType::PopClip;
    return op;
}

PaintOp PaintOp::pushOpacity(float opacity) {
    PaintOp op;
    op.type = PaintOpType::PushOpacity;
    op.opacity = opacity;
    return op;
}

PaintOp PaintOp::popOpacity() {
    PaintOp op;
    op.type = PaintOpType::PopOpacity;
    return op;
}

PaintOp PaintOp::drawShadow(float x, float y, float w, float h,
                              float ox, float oy, float blur, float spread,
                              uint32_t color, bool inset) {
    PaintOp op;
    op.type = PaintOpType::DrawShadow;
    op.x = x; op.y = y; op.width = w; op.height = h;
    op.shadowOffsetX = ox; op.shadowOffsetY = oy;
    op.shadowBlur = blur; op.shadowSpread = spread;
    op.color = color;
    op.shadowInset = inset;
    return op;
}

PaintOp PaintOp::drawImage(float x, float y, float w, float h,
                             const std::string& url, uint32_t texId) {
    PaintOp op;
    op.type = PaintOpType::DrawImage;
    op.x = x; op.y = y; op.width = w; op.height = h;
    op.imageURL = url;
    op.textureId = texId;
    return op;
}

// ==================================================================
// PaintList
// ==================================================================

PaintList::PaintList() {}
PaintList::~PaintList() {}

void PaintList::addOp(const PaintOp& op) {
    ops_.push_back(op);
}

void PaintList::addOp(PaintOp&& op) {
    ops_.push_back(std::move(op));
}

void PaintList::clear() {
    ops_.clear();
}

void PaintList::append(const PaintList& other) {
    ops_.insert(ops_.end(), other.ops_.begin(), other.ops_.end());
}

// ==================================================================
// PaintTreeBuilder
// ==================================================================

PaintTreeBuilder::PaintTreeBuilder() {}
PaintTreeBuilder::~PaintTreeBuilder() {}

PaintList PaintTreeBuilder::build(const BoxNode* root) {
    PaintList list;
    if (root) {
        paintStackingContext(root, list);
    }
    return list;
}

// ==================================================================
// Stacking context painting (CSS 2.1 Appendix E)
// ==================================================================

void PaintTreeBuilder::paintStackingContext(const BoxNode* node, PaintList& list) {
    // Step 1: Background and borders of the element
    paintBoxShadow(node, list);
    paintBackground(node, list);
    paintBorders(node, list);

    // Step 2: Negative z-index stacking contexts
    std::vector<const BoxNode*> negativeZ;
    std::vector<const BoxNode*> normalFlow;
    std::vector<const BoxNode*> positiveZ;
    std::vector<const BoxNode*> nonPositioned;

    for (const auto& child : node->children()) {
        if (child->boxType() == BoxType::None) continue;
        if (child->computed().display == 0) continue; // display:none

        if (child->createsStackingContext()) {
            int z = child->stackingOrder();
            if (z < 0) negativeZ.push_back(child.get());
            else if (z > 0) positiveZ.push_back(child.get());
            else normalFlow.push_back(child.get());
        } else if (child->isPositioned()) {
            normalFlow.push_back(child.get());
        } else {
            nonPositioned.push_back(child.get());
        }
    }

    sortByZOrder(negativeZ);

    // Paint negative z-index children
    for (const auto* child : negativeZ) {
        paintStackingContext(child, list);
    }

    // Step 3: In-flow non-positioned block children
    for (const auto* child : nonPositioned) {
        paintNode(child, list);
    }

    // Step 4: Non-positioned floats
    // (Simplified: floats treated as normal flow)

    // Step 5: In-flow inline content (text)
    paintText(node, list);

    // Step 6: Positioned children + z-index 0
    for (const auto* child : normalFlow) {
        if (child->createsStackingContext()) {
            paintStackingContext(child, list);
        } else {
            paintNode(child, list);
        }
    }

    // Step 7: Positive z-index stacking contexts
    sortByZOrder(positiveZ);
    for (const auto* child : positiveZ) {
        paintStackingContext(child, list);
    }

    // Outline (painted after everything)
    paintOutline(node, list);
}

void PaintTreeBuilder::sortByZOrder(std::vector<const BoxNode*>& nodes) {
    std::stable_sort(nodes.begin(), nodes.end(),
        [](const BoxNode* a, const BoxNode* b) {
            return a->stackingOrder() < b->stackingOrder();
        });
}

// ==================================================================
// Background painting
// ==================================================================

void PaintTreeBuilder::paintBackground(const BoxNode* node, PaintList& list) {
    const auto& cv = node->computed();
    const auto& lb = node->layoutBox();

    // Skip if transparent
    uint8_t alpha = cv.backgroundColor & 0xFF;
    if (alpha == 0 && cv.backgroundImage.empty()) return;

    bool hasRadius = cv.borderTopLeftRadius > 0 || cv.borderTopRightRadius > 0 ||
                     cv.borderBottomRightRadius > 0 || cv.borderBottomLeftRadius > 0;

    if (hasRadius) {
        list.addOp(PaintOp::fillRoundedRect(
            lb.x, lb.y, lb.width, lb.height,
            cv.borderTopLeftRadius, cv.borderTopRightRadius,
            cv.borderBottomRightRadius, cv.borderBottomLeftRadius,
            cv.backgroundColor));
    } else {
        list.addOp(PaintOp::fillRect(lb.x, lb.y, lb.width, lb.height, cv.backgroundColor));
    }

    // Background image
    if (!cv.backgroundImage.empty() && cv.backgroundImage != "none") {
        // Check if it's a gradient
        std::string lower = cv.backgroundImage;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("gradient") != std::string::npos) {
            PaintOp op;
            op.type = PaintOpType::FillGradient;
            op.x = lb.x; op.y = lb.y;
            op.width = lb.width; op.height = lb.height;
            auto grad = CSSValueParser::parseGradient(cv.backgroundImage);
            if (grad) op.gradient = *grad;
            list.addOp(std::move(op));
        } else {
            // URL image
            std::string url = cv.backgroundImage;
            // Strip url()
            if (url.substr(0, 4) == "url(") {
                url = url.substr(4);
                if (!url.empty() && url.back() == ')') url.pop_back();
                // Strip quotes
                if (url.size() >= 2 && (url.front() == '"' || url.front() == '\'')) {
                    url = url.substr(1, url.size() - 2);
                }
            }
            list.addOp(PaintOp::drawImage(lb.x, lb.y, lb.width, lb.height, url));
        }
    }
}

// ==================================================================
// Border painting
// ==================================================================

void PaintTreeBuilder::paintBorders(const BoxNode* node, PaintList& list) {
    const auto& cv = node->computed();
    const auto& lb = node->layoutBox();

    // Top border
    if (cv.borderTopWidth > 0 && cv.borderTopStyle > 0) {
        list.addOp(PaintOp::drawBorderSide(
            lb.x, lb.y, lb.width, cv.borderTopWidth,
            0, cv.borderTopWidth, cv.borderTopColor, cv.borderTopStyle));
    }

    // Right border
    if (cv.borderRightWidth > 0 && cv.borderRightStyle > 0) {
        list.addOp(PaintOp::drawBorderSide(
            lb.x + lb.width - cv.borderRightWidth, lb.y,
            cv.borderRightWidth, lb.height,
            1, cv.borderRightWidth, cv.borderRightColor, cv.borderRightStyle));
    }

    // Bottom border
    if (cv.borderBottomWidth > 0 && cv.borderBottomStyle > 0) {
        list.addOp(PaintOp::drawBorderSide(
            lb.x, lb.y + lb.height - cv.borderBottomWidth,
            lb.width, cv.borderBottomWidth,
            2, cv.borderBottomWidth, cv.borderBottomColor, cv.borderBottomStyle));
    }

    // Left border
    if (cv.borderLeftWidth > 0 && cv.borderLeftStyle > 0) {
        list.addOp(PaintOp::drawBorderSide(
            lb.x, lb.y, cv.borderLeftWidth, lb.height,
            3, cv.borderLeftWidth, cv.borderLeftColor, cv.borderLeftStyle));
    }
}

// ==================================================================
// Text painting
// ==================================================================

void PaintTreeBuilder::paintText(const BoxNode* node, PaintList& list) {
    if (!node->isTextNode()) return;
    if (node->text().empty()) return;

    const auto& cv = node->computed();
    const auto& lb = node->layoutBox();

    list.addOp(PaintOp::drawText(
        lb.contentX, lb.contentY + cv.fontSize, // Baseline offset
        node->text(),
        cv.fontFamily,
        cv.fontSize,
        cv.color));

    // Text decoration
    if (!cv.textDecoration.empty() && cv.textDecoration != "none") {
        PaintOp decorOp;
        decorOp.type = PaintOpType::DrawTextDecoration;
        decorOp.x = lb.contentX;
        decorOp.y = lb.contentY;
        decorOp.width = lb.contentWidth;
        decorOp.fontSize = cv.fontSize;
        decorOp.color = cv.color;
        decorOp.text = cv.textDecoration; // "underline", "line-through", etc
        list.addOp(std::move(decorOp));
    }
}

// ==================================================================
// Box shadow painting
// ==================================================================

void PaintTreeBuilder::paintBoxShadow(const BoxNode* node, PaintList& list) {
    const auto& cv = node->computed();
    const auto& lb = node->layoutBox();

    if (cv.boxShadow.empty() || cv.boxShadow == "none") return;

    auto shadows = CSSValueParser::parseBoxShadow(cv.boxShadow);

    // Outer shadows first (painted before background)
    for (const auto& shadow : shadows) {
        if (shadow.inset) continue; // Inset painted after background

        list.addOp(PaintOp::drawShadow(
            lb.x, lb.y, lb.width, lb.height,
            shadow.offsetX, shadow.offsetY,
            shadow.blurRadius, shadow.spreadRadius,
            shadow.color.toRGBA(), false));
    }
}

// ==================================================================
// Outline painting
// ==================================================================

void PaintTreeBuilder::paintOutline(const BoxNode* node, PaintList& list) {
    // Outline is painted after all content, outside the border box
    // Simplified: not yet implemented in NXRender
}

// ==================================================================
// Child painting
// ==================================================================

void PaintTreeBuilder::paintChildren(const BoxNode* node, PaintList& list) {
    for (const auto& child : node->children()) {
        if (child->boxType() == BoxType::None) continue;
        if (child->computed().display == 0) continue;

        paintNode(child.get(), list);
    }
}

// ==================================================================
// Single node painting
// ==================================================================

void PaintTreeBuilder::paintNode(const BoxNode* node, PaintList& list) {
    const auto& cv = node->computed();
    const auto& lb = node->layoutBox();

    // Visibility check
    if (cv.visibility == 1) return; // hidden

    // Opacity grouping
    bool hasOpacity = cv.opacity < 1.0f;
    if (hasOpacity) list.addOp(PaintOp::pushOpacity(cv.opacity));

    // Overflow clipping
    bool needsClip = (cv.overflowX == 1 || cv.overflowX == 2 || cv.overflowX == 4);
    if (needsClip) {
        list.addOp(PaintOp::pushClip(lb.contentX, lb.contentY,
                                       lb.contentWidth, lb.contentHeight));
    }

    // Paint this node
    if (node->createsStackingContext()) {
        paintStackingContext(node, list);
    } else {
        paintBoxShadow(node, list);
        paintBackground(node, list);
        paintBorders(node, list);
        paintText(node, list);
        paintChildren(node, list);
        paintOutline(node, list);
    }

    if (needsClip) list.addOp(PaintOp::popClip());
    if (hasOpacity) list.addOp(PaintOp::popOpacity());
}

// ==================================================================
// Hit testing
// ==================================================================

HitTestResult HitTester::hitTest(const BoxNode* root, float x, float y) {
    HitTestResult result;
    if (root) {
        hitTestNode(root, x, y, 0, result);
    }
    return result;
}

bool HitTester::hitTestNode(const BoxNode* node, float x, float y,
                              int depth, HitTestResult& result) {
    const auto& cv = node->computed();
    const auto& lb = node->layoutBox();

    // Skip invisible/none
    if (cv.display == 0) return false;
    if (cv.visibility == 1) return false;
    if (cv.pointerEvents == "none") return false;

    // Check bounds
    if (!lb.contains(x, y)) return false;

    // Test children in reverse order (front-to-back)
    for (int i = static_cast<int>(node->children().size()) - 1; i >= 0; i--) {
        if (hitTestNode(node->children()[i].get(), x, y, depth + 1, result)) {
            return true;
        }
    }

    // This node is the hit
    result.node = node;
    result.localX = x - lb.x;
    result.localY = y - lb.y;
    result.depth = depth;
    return true;
}

} // namespace Web
} // namespace NXRender
