// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "layout/stacking_context.h"
#include "widgets/widget.h"
#include <sstream>
#include <cmath>

namespace NXRender {

StackingContext::StackingContext() {}
StackingContext::~StackingContext() {}

bool StackingContext::createsStackingContext(Widget* w) const {
    if (!w) return false;

    // A widget creates a stacking context if:
    // 1. It has a non-auto z-index and is positioned
    // 2. It has opacity < 1
    // 3. It has a transform
    // 4. It has a filter
    // 5. It's the root element

    if (w->opacity() < 1.0f) return true;
    if (w->hasTransform()) return true;

    // Positioned with explicit z-index
    auto pos = w->positionType();
    if (pos != PositionType::Static && w->zIndex() != 0) return true;

    return false;
}

int32_t StackingContext::effectiveZIndex(Widget* w) const {
    if (!w) return 0;
    auto pos = w->positionType();
    if (pos == PositionType::Static) return 0;
    return w->zIndex();
}

void StackingContext::build(Widget* root) {
    root_ = std::make_unique<StackingEntry>();
    root_->widget = root;
    root_->zIndex = 0;
    root_->isStackingContext = true;
    root_->opacity = root ? root->opacity() : 1.0f;
    contextCount_ = 1;

    if (root) {
        for (auto* child : root->children()) {
            buildEntry(child, root_.get());
        }
        sortEntry(root_.get());
    }
}

void StackingContext::buildEntry(Widget* widget, StackingEntry* parent) {
    if (!widget || !widget->isVisible()) return;

    auto entry = std::make_unique<StackingEntry>();
    entry->widget = widget;
    entry->zIndex = effectiveZIndex(widget);
    entry->isStackingContext = createsStackingContext(widget);
    entry->isPositioned = (widget->positionType() != PositionType::Static);
    entry->opacity = widget->opacity();
    entry->hasTransform = widget->hasTransform();

    if (entry->isStackingContext) {
        contextCount_++;
    }

    // Recurse into children
    for (auto* child : widget->children()) {
        buildEntry(child, entry.get());
    }

    sortEntry(entry.get());
    parent->children.push_back(std::move(entry));
}

void StackingContext::sortEntry(StackingEntry* entry) {
    if (!entry) return;

    // Sort children by z-index (stable sort to preserve DOM order for same z-index)
    std::stable_sort(entry->children.begin(), entry->children.end(),
        [](const std::unique_ptr<StackingEntry>& a, const std::unique_ptr<StackingEntry>& b) {
            return a->zIndex < b->zIndex;
        });
}

void StackingContext::visitPaintOrder(PaintVisitor visitor) const {
    if (!root_) return;
    visitEntry(root_.get(), visitor);
}

void StackingContext::visitEntry(const StackingEntry* entry, PaintVisitor& visitor) const {
    if (!entry || !entry->widget) return;

    // CSS paint order within a stacking context:
    // 1. Negative z-index children
    // 2. This element's background/content
    // 3. z-index 0 / auto children (in DOM order)
    // 4. Positive z-index children

    // Phase 1: Negative z-index children
    for (const auto& child : entry->children) {
        if (child->zIndex < 0) {
            visitEntry(child.get(), visitor);
        }
    }

    // Phase 2: This element
    const Rect* clipPtr = entry->hasClip ? &entry->clipRect : nullptr;
    visitor(entry->widget, clipPtr, entry->opacity, entry->zIndex);

    // Phase 3: Non-positioned children and z-index 0 children
    for (const auto& child : entry->children) {
        if (child->zIndex == 0 && !child->isPositioned) {
            visitEntry(child.get(), visitor);
        }
    }
    for (const auto& child : entry->children) {
        if (child->zIndex == 0 && child->isPositioned) {
            visitEntry(child.get(), visitor);
        }
    }

    // Phase 4: Positive z-index children
    for (const auto& child : entry->children) {
        if (child->zIndex > 0) {
            visitEntry(child.get(), visitor);
        }
    }
}

Widget* StackingContext::hitTest(float x, float y) const {
    if (!root_) return nullptr;
    return hitTestEntry(root_.get(), x, y);
}

Widget* StackingContext::hitTestEntry(const StackingEntry* entry, float x, float y) const {
    if (!entry || !entry->widget) return nullptr;

    // Reverse paint order: positive z-index first (front-most)
    for (auto it = entry->children.rbegin(); it != entry->children.rend(); ++it) {
        if ((*it)->zIndex > 0) {
            Widget* hit = hitTestEntry(it->get(), x, y);
            if (hit) return hit;
        }
    }

    // z-index 0 positioned
    for (auto it = entry->children.rbegin(); it != entry->children.rend(); ++it) {
        if ((*it)->zIndex == 0 && (*it)->isPositioned) {
            Widget* hit = hitTestEntry(it->get(), x, y);
            if (hit) return hit;
        }
    }

    // z-index 0 non-positioned
    for (auto it = entry->children.rbegin(); it != entry->children.rend(); ++it) {
        if ((*it)->zIndex == 0 && !(*it)->isPositioned) {
            Widget* hit = hitTestEntry(it->get(), x, y);
            if (hit) return hit;
        }
    }

    // This element
    if (entry->widget->bounds().contains(x, y) && entry->widget->isEnabled()) {
        return entry->widget;
    }

    // Negative z-index children
    for (auto it = entry->children.rbegin(); it != entry->children.rend(); ++it) {
        if ((*it)->zIndex < 0) {
            Widget* hit = hitTestEntry(it->get(), x, y);
            if (hit) return hit;
        }
    }

    return nullptr;
}

std::string StackingContext::debugDump() const {
    std::string output;
    output += "=== Stacking Context Tree ===\n";
    output += "Context count: " + std::to_string(contextCount_) + "\n";
    if (root_) {
        dumpEntry(root_.get(), 0, output);
    }
    return output;
}

void StackingContext::dumpEntry(const StackingEntry* entry, int depth, std::string& output) const {
    if (!entry) return;

    std::string indent(static_cast<size_t>(depth) * 2, ' ');
    std::string name = entry->widget ? entry->widget->id() : "(null)";

    output += indent + "- " + name;
    output += " z=" + std::to_string(entry->zIndex);
    if (entry->isStackingContext) output += " [SC]";
    if (entry->isPositioned) output += " [pos]";
    if (entry->opacity < 1.0f) output += " opacity=" + std::to_string(entry->opacity);
    output += "\n";

    for (const auto& child : entry->children) {
        dumpEntry(child.get(), depth + 1, output);
    }
}

} // namespace NXRender
