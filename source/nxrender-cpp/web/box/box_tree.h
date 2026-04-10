// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "../css/cascade.h"
#include "nxgfx/primitives.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace NXRender {
namespace Web {

using NXRender::Rect;
using NXRender::Size;
using NXRender::Point;

// ==================================================================
// Box types (CSS 2.1 §9.2)
// ==================================================================

enum class BoxType : uint8_t {
    Block,
    Inline,
    InlineBlock,
    Flex,
    InlineFlex,
    Grid,
    InlineGrid,
    Table,
    TableRow,
    TableCell,
    ListItem,
    Anonymous,      // Anonymous block/inline boxes
    None,           // display:none
    Contents,       // display:contents (no box generated)
};

// ==================================================================
// Formatting context type
// ==================================================================

enum class FormattingContext : uint8_t {
    Block,      // BFC
    Inline,     // IFC
    Flex,       // FFC
    Grid,       // GFC
    Table,      // TFC
};

// ==================================================================
// Box node — the render tree node
// ==================================================================

class BoxNode {
public:
    BoxNode();
    ~BoxNode();

    // Identity
    void setTag(const std::string& tag) { tag_ = tag; }
    const std::string& tag() const { return tag_; }
    void setBoxType(BoxType type) { boxType_ = type; }
    BoxType boxType() const { return boxType_; }

    // DOM linkage (opaque pointer — avoids webCore dependency)
    void setDomNode(void* node) { domNode_ = node; }
    void* domNode() const { return domNode_; }

    // Computed style
    void setComputedValues(const ComputedValues& cv) { computed_ = cv; }
    ComputedValues& computed() { return computed_; }
    const ComputedValues& computed() const { return computed_; }

    // Tree structure
    BoxNode* parent() const { return parent_; }
    const std::vector<std::unique_ptr<BoxNode>>& children() const { return children_; }
    void appendChild(std::unique_ptr<BoxNode> child);
    void insertChild(size_t index, std::unique_ptr<BoxNode> child);
    std::unique_ptr<BoxNode> removeChild(BoxNode* child);
    BoxNode* firstChild() const;
    BoxNode* lastChild() const;
    BoxNode* nextSibling() const;
    BoxNode* previousSibling() const;
    size_t childCount() const { return children_.size(); }
    size_t childIndex() const;

    // Layout results (set by layout engine)
    struct LayoutBox {
        float x = 0, y = 0;
        float width = 0, height = 0;
        float contentX = 0, contentY = 0;
        float contentWidth = 0, contentHeight = 0;

        float paddingTop = 0, paddingRight = 0;
        float paddingBottom = 0, paddingLeft = 0;
        float borderTop = 0, borderRight = 0;
        float borderBottom = 0, borderLeft = 0;
        float marginTop = 0, marginRight = 0;
        float marginBottom = 0, marginLeft = 0;

        float borderBoxWidth() const { return width; }
        float borderBoxHeight() const { return height; }
        float marginBoxWidth() const { return marginLeft + width + marginRight; }
        float marginBoxHeight() const { return marginTop + height + marginBottom; }

        bool contains(float px, float py) const {
            return px >= x && px < x + width && py >= y && py < y + height;
        }
    };

    LayoutBox& layoutBox() { return layout_; }
    const LayoutBox& layoutBox() const { return layout_; }

    // Text content (for text nodes)
    void setText(const std::string& text) { text_ = text; isTextNode_ = true; }
    const std::string& text() const { return text_; }
    bool isTextNode() const { return isTextNode_; }

    // Formatting context
    FormattingContext establishedContext() const;
    bool establishesNewContext() const;

    // Stacking context
    bool createsStackingContext() const;
    int stackingOrder() const;

    // Query
    bool isBlock() const;
    bool isInline() const;
    bool isFlexContainer() const;
    bool isGridContainer() const;
    bool isPositioned() const;
    bool isFloating() const;
    bool isReplaced() const;  // img, video, canvas, etc

    // Rect accessors (convenience over LayoutBox)
    struct EdgeSizes { float top = 0, right = 0, bottom = 0, left = 0; };
    struct BorderRadii { float topLeft = 0, topRight = 0, bottomRight = 0, bottomLeft = 0; };

    Rect borderRect() const {
        return Rect(layout_.x, layout_.y, layout_.width, layout_.height);
    }
    Rect contentRect() const {
        return Rect(layout_.contentX, layout_.contentY,
                    layout_.contentWidth, layout_.contentHeight);
    }
    Rect paddingRect() const {
        return Rect(layout_.x + layout_.borderLeft, layout_.y + layout_.borderTop,
                    layout_.width - layout_.borderLeft - layout_.borderRight,
                    layout_.height - layout_.borderTop - layout_.borderBottom);
    }
    EdgeSizes padding() const {
        return {layout_.paddingTop, layout_.paddingRight,
                layout_.paddingBottom, layout_.paddingLeft};
    }
    EdgeSizes borderWidths() const {
        return {layout_.borderTop, layout_.borderRight,
                layout_.borderBottom, layout_.borderLeft};
    }
    EdgeSizes margins() const {
        return {layout_.marginTop, layout_.marginRight,
                layout_.marginBottom, layout_.marginLeft};
    }

    // Border radius
    const BorderRadii& borderRadii() const { return radii_; }
    void setBorderRadii(const BorderRadii& r) { radii_ = r; }
    bool hasBorderRadius() const {
        return radii_.topLeft > 0 || radii_.topRight > 0 ||
               radii_.bottomRight > 0 || radii_.bottomLeft > 0;
    }

    // Visual state
    float zIndex() const { return zIndex_; }
    void setZIndex(float z) { zIndex_ = z; }
    bool isVisible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }
    bool pointerEventsNone() const { return pointerEventsNone_; }
    void setPointerEventsNone(bool pe) { pointerEventsNone_ = pe; }

    // Overflow clip
    bool hasOverflowClip() const { return overflowClip_; }
    void setOverflowClip(bool clip) { overflowClip_ = clip; }

    // Transform
    bool hasTransform() const { return hasTransform_; }
    const float* transformMatrix() const { return transform_; }
    void setTransform(const float m[6]) {
        hasTransform_ = true;
        for (int i = 0; i < 6; i++) transform_[i] = m[i];
    }

    // Child access
    const BoxNode* childAt(size_t index) const {
        return (index < children_.size()) ? children_[index].get() : nullptr;
    }

    // Debug
    void dump(int indent = 0) const;

private:
    std::string tag_;
    BoxType boxType_ = BoxType::Block;
    void* domNode_ = nullptr;
    ComputedValues computed_;
    LayoutBox layout_;
    std::string text_;
    bool isTextNode_ = false;

    BoxNode* parent_ = nullptr;
    std::vector<std::unique_ptr<BoxNode>> children_;
    int siblingIndex_ = -1;

    BorderRadii radii_;
    float zIndex_ = 0;
    bool visible_ = true;
    bool pointerEventsNone_ = false;
    bool overflowClip_ = false;
    bool hasTransform_ = false;
    float transform_[6] = {1,0,0,1,0,0};
};

// ==================================================================
// Box tree builder
// ==================================================================

class BoxTreeBuilder {
public:
    BoxTreeBuilder();
    ~BoxTreeBuilder();

    void setCascadeEngine(CascadeEngine* engine) { cascade_ = engine; }

    // Build box tree from DOM-like structure
    // The callback provides: tag, computedValues, text, children
    using DOMVisitor = std::function<void(
        BoxNode* node,
        const std::string& tag,
        const ComputedValues& computed,
        const std::string& text,
        const std::vector<void*>& childDOMNodes
    )>;

    std::unique_ptr<BoxNode> buildTree(
        void* rootDOMNode,
        std::function<void(void* domNode, DOMVisitor visitor)> domWalker
    );

    // Direct box creation (for testing/manual construction)
    static std::unique_ptr<BoxNode> createBlockBox(const std::string& tag, const ComputedValues& cv);
    static std::unique_ptr<BoxNode> createInlineBox(const std::string& tag, const ComputedValues& cv);
    static std::unique_ptr<BoxNode> createTextBox(const std::string& text, const ComputedValues& cv);
    static std::unique_ptr<BoxNode> createAnonymousBlock();
    static std::unique_ptr<BoxNode> createAnonymousInline();

private:
    CascadeEngine* cascade_ = nullptr;

    void insertAnonymousBlocks(BoxNode* parent);
    void insertAnonymousInlines(BoxNode* parent);
    BoxType resolveBoxType(uint8_t displayValue);
};

} // namespace Web
} // namespace NXRender
