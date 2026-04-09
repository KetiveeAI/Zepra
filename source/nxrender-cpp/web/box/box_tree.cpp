// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "box_tree.h"
#include <algorithm>
#include <cstdio>

namespace NXRender {
namespace Web {

// ==================================================================
// BoxNode
// ==================================================================

BoxNode::BoxNode() {}
BoxNode::~BoxNode() {}

void BoxNode::appendChild(std::unique_ptr<BoxNode> child) {
    child->parent_ = this;
    child->siblingIndex_ = static_cast<int>(children_.size());
    children_.push_back(std::move(child));
}

void BoxNode::insertChild(size_t index, std::unique_ptr<BoxNode> child) {
    child->parent_ = this;
    if (index >= children_.size()) {
        child->siblingIndex_ = static_cast<int>(children_.size());
        children_.push_back(std::move(child));
    } else {
        children_.insert(children_.begin() + index, std::move(child));
        // Reindex
        for (size_t i = index; i < children_.size(); i++) {
            children_[i]->siblingIndex_ = static_cast<int>(i);
        }
    }
}

std::unique_ptr<BoxNode> BoxNode::removeChild(BoxNode* child) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == child) {
            child->parent_ = nullptr;
            auto removed = std::move(*it);
            children_.erase(it);
            // Reindex
            for (size_t i = 0; i < children_.size(); i++) {
                children_[i]->siblingIndex_ = static_cast<int>(i);
            }
            return removed;
        }
    }
    return nullptr;
}

BoxNode* BoxNode::firstChild() const {
    return children_.empty() ? nullptr : children_.front().get();
}

BoxNode* BoxNode::lastChild() const {
    return children_.empty() ? nullptr : children_.back().get();
}

BoxNode* BoxNode::nextSibling() const {
    if (!parent_ || siblingIndex_ < 0) return nullptr;
    size_t next = static_cast<size_t>(siblingIndex_) + 1;
    if (next >= parent_->children_.size()) return nullptr;
    return parent_->children_[next].get();
}

BoxNode* BoxNode::previousSibling() const {
    if (!parent_ || siblingIndex_ <= 0) return nullptr;
    return parent_->children_[siblingIndex_ - 1].get();
}

size_t BoxNode::childIndex() const {
    if (siblingIndex_ >= 0) return static_cast<size_t>(siblingIndex_);
    return 0;
}

// ==================================================================
// Box type queries
// ==================================================================

bool BoxNode::isBlock() const {
    return boxType_ == BoxType::Block || boxType_ == BoxType::ListItem ||
           boxType_ == BoxType::Anonymous;
}

bool BoxNode::isInline() const {
    return boxType_ == BoxType::Inline || boxType_ == BoxType::InlineBlock;
}

bool BoxNode::isFlexContainer() const {
    return boxType_ == BoxType::Flex || boxType_ == BoxType::InlineFlex;
}

bool BoxNode::isGridContainer() const {
    return boxType_ == BoxType::Grid || boxType_ == BoxType::InlineGrid;
}

bool BoxNode::isPositioned() const {
    return computed_.position != 0; // Non-static
}

bool BoxNode::isFloating() const {
    return computed_.floatVal != 0;
}

bool BoxNode::isReplaced() const {
    return tag_ == "img" || tag_ == "video" || tag_ == "canvas" ||
           tag_ == "iframe" || tag_ == "object" || tag_ == "embed" ||
           tag_ == "svg" || tag_ == "audio" || tag_ == "input" ||
           tag_ == "select" || tag_ == "textarea";
}

// ==================================================================
// Formatting context
// ==================================================================

FormattingContext BoxNode::establishedContext() const {
    if (isFlexContainer()) return FormattingContext::Flex;
    if (isGridContainer()) return FormattingContext::Grid;
    if (boxType_ == BoxType::Table) return FormattingContext::Table;
    // Check if BFC is established
    if (establishesNewContext()) return FormattingContext::Block;
    return FormattingContext::Block;
}

bool BoxNode::establishesNewContext() const {
    // CSS 2.1: elements that establish a new BFC
    if (isFlexContainer() || isGridContainer()) return true;
    if (boxType_ == BoxType::InlineBlock) return true;
    if (computed_.overflowX != 0 || computed_.overflowY != 0) return true; // Not visible
    if (computed_.position == 2 || computed_.position == 3) return true; // absolute/fixed
    if (computed_.floatVal != 0) return true;
    if (computed_.columnCount > 0 && !computed_.columnCountAuto) return true;
    if (computed_.display == 12) return true; // Contents
    return false;
}

// ==================================================================
// Stacking context
// ==================================================================

bool BoxNode::createsStackingContext() const {
    if (computed_.position != 0 && !computed_.zIndexAuto) return true;
    if (computed_.opacity < 1.0f) return true;
    if (!computed_.transform.empty() && computed_.transform != "none") return true;
    if (!computed_.filter.empty() && computed_.filter != "none") return true;
    if (!computed_.willChange.empty() && computed_.willChange != "auto") return true;
    if (computed_.isolation == "isolate") return true;
    if (isFlexContainer() || isGridContainer()) {
        // Flex/grid children with z-index
        if (!computed_.zIndexAuto) return true;
    }
    return false;
}

int BoxNode::stackingOrder() const {
    if (computed_.zIndexAuto) return 0;
    return computed_.zIndex;
}

// ==================================================================
// Debug dump
// ==================================================================

void BoxNode::dump(int indent) const {
    for (int i = 0; i < indent; i++) printf("  ");

    const char* typeNames[] = {
        "block", "inline", "inline-block", "flex", "inline-flex",
        "grid", "inline-grid", "table", "table-row", "table-cell",
        "list-item", "anonymous", "none", "contents"
    };

    const char* typeName = "unknown";
    int typeIdx = static_cast<int>(boxType_);
    if (typeIdx >= 0 && typeIdx < 14) typeName = typeNames[typeIdx];

    if (isTextNode_) {
        printf("[text] \"%s\" (%.0f,%.0f %.0fx%.0f)\n",
               text_.substr(0, 30).c_str(),
               layout_.x, layout_.y, layout_.width, layout_.height);
    } else {
        printf("<%s> [%s] (%.0f,%.0f %.0fx%.0f)\n",
               tag_.c_str(), typeName,
               layout_.x, layout_.y, layout_.width, layout_.height);
    }

    for (const auto& child : children_) {
        child->dump(indent + 1);
    }
}

// ==================================================================
// BoxTreeBuilder
// ==================================================================

BoxTreeBuilder::BoxTreeBuilder() {}
BoxTreeBuilder::~BoxTreeBuilder() {}

BoxType BoxTreeBuilder::resolveBoxType(uint8_t displayValue) {
    switch (displayValue) {
        case 0: return BoxType::None;
        case 1: return BoxType::Block;
        case 2: return BoxType::Inline;
        case 3: return BoxType::InlineBlock;
        case 4: return BoxType::Flex;
        case 5: return BoxType::InlineFlex;
        case 6: return BoxType::Grid;
        case 7: return BoxType::InlineGrid;
        case 8: return BoxType::Table;
        case 9: return BoxType::TableRow;
        case 10: return BoxType::TableCell;
        case 11: return BoxType::ListItem;
        case 12: return BoxType::Contents;
        default: return BoxType::Block;
    }
}

std::unique_ptr<BoxNode> BoxTreeBuilder::createBlockBox(const std::string& tag,
                                                          const ComputedValues& cv) {
    auto node = std::make_unique<BoxNode>();
    node->setTag(tag);
    node->setBoxType(BoxType::Block);
    node->setComputedValues(cv);
    return node;
}

std::unique_ptr<BoxNode> BoxTreeBuilder::createInlineBox(const std::string& tag,
                                                           const ComputedValues& cv) {
    auto node = std::make_unique<BoxNode>();
    node->setTag(tag);
    node->setBoxType(BoxType::Inline);
    node->setComputedValues(cv);
    return node;
}

std::unique_ptr<BoxNode> BoxTreeBuilder::createTextBox(const std::string& text,
                                                         const ComputedValues& cv) {
    auto node = std::make_unique<BoxNode>();
    node->setTag("#text");
    node->setBoxType(BoxType::Inline);
    node->setText(text);
    node->setComputedValues(cv);
    return node;
}

std::unique_ptr<BoxNode> BoxTreeBuilder::createAnonymousBlock() {
    auto node = std::make_unique<BoxNode>();
    node->setTag("#anon-block");
    node->setBoxType(BoxType::Anonymous);
    return node;
}

std::unique_ptr<BoxNode> BoxTreeBuilder::createAnonymousInline() {
    auto node = std::make_unique<BoxNode>();
    node->setTag("#anon-inline");
    node->setBoxType(BoxType::Inline);
    return node;
}

// ==================================================================
// Anonymous box insertion (CSS 2.1 §9.2.1.1)
// ==================================================================

void BoxTreeBuilder::insertAnonymousBlocks(BoxNode* parent) {
    if (!parent || parent->childCount() == 0) return;

    // Check if parent has mixed inline/block children
    bool hasBlock = false;
    bool hasInline = false;

    for (const auto& child : parent->children()) {
        if (child->boxType() == BoxType::None) continue;
        if (child->isBlock()) hasBlock = true;
        else hasInline = true;
    }

    // If mixed, wrap inline runs in anonymous blocks
    if (hasBlock && hasInline) {
        std::vector<std::unique_ptr<BoxNode>> newChildren;
        std::unique_ptr<BoxNode> currentAnon;

        for (size_t i = 0; i < parent->children().size(); i++) {
            // We need to extract without removing indices
            // This is a simplification — real implementation would be more careful
            BoxNode* child = parent->children()[i].get();

            if (child->boxType() == BoxType::None) continue;

            if (child->isInline() || child->isTextNode()) {
                if (!currentAnon) {
                    currentAnon = createAnonymousBlock();
                }
                // Clone reference (can't move from const vector)
                // In production, we'd restructure the tree in-place
            } else {
                if (currentAnon) {
                    newChildren.push_back(std::move(currentAnon));
                    currentAnon = nullptr;
                }
            }
        }
        if (currentAnon) {
            newChildren.push_back(std::move(currentAnon));
        }
    }
}

void BoxTreeBuilder::insertAnonymousInlines(BoxNode* parent) {
    // For inline formatting contexts: wrap bare text in anonymous inline boxes
    // This is called when a block element directly contains text and inline children
}

std::unique_ptr<BoxNode> BoxTreeBuilder::buildTree(
    void* rootDOMNode,
    std::function<void(void* domNode, DOMVisitor visitor)> domWalker) {

    auto root = std::make_unique<BoxNode>();
    root->setTag("body");
    root->setBoxType(BoxType::Block);
    root->setDomNode(rootDOMNode);

    if (domWalker) {
        domWalker(rootDOMNode, [&](BoxNode* node,
                                    const std::string& tag,
                                    const ComputedValues& computed,
                                    const std::string& text,
                                    const std::vector<void*>& childDOMNodes) {
            node->setTag(tag);
            node->setComputedValues(computed);
            node->setBoxType(resolveBoxType(computed.display));

            if (!text.empty()) {
                node->setText(text);
            }
        });
    }

    // Post-process: insert anonymous boxes
    insertAnonymousBlocks(root.get());

    return root;
}

} // namespace Web
} // namespace NXRender
