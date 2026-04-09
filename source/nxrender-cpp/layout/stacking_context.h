// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file stacking_context.h
 * @brief CSS stacking context implementation.
 *
 * Manages z-index ordering and paint order per CSS 2.1 Appendix E.
 * Each stacking context is a paint boundary — children are painted
 * in z-index order within their parent context.
 */

#pragma once

#include "../nxgfx/primitives.h"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <memory>

namespace NXRender {

class Widget;

/**
 * @brief Paint layer within a stacking context.
 */
enum class PaintPhase : uint8_t {
    Background,       // Background and borders
    Float,            // Float elements
    Content,          // Normal flow content
    PositionedNeg,    // Positioned elements with negative z-index
    PositionedZero,   // Positioned elements with z-index 0 or auto
    PositionedPos,    // Positioned elements with positive z-index
    Outline           // Outlines
};

/**
 * @brief An entry in the stacking context tree.
 */
struct StackingEntry {
    Widget* widget = nullptr;
    int32_t zIndex = 0;
    bool isStackingContext = false;
    bool isPositioned = false;
    float opacity = 1.0f;
    Rect clipRect;
    bool hasClip = false;

    // Transform
    bool hasTransform = false;

    // Children (sorted by z-index)
    std::vector<std::unique_ptr<StackingEntry>> children;
};

/**
 * @brief Stacking context manager.
 *
 * Builds and maintains the stacking order tree from the widget tree.
 * Provides paint-order iteration for the compositor.
 */
class StackingContext {
public:
    StackingContext();
    ~StackingContext();

    /**
     * @brief Rebuild the stacking tree from the widget root.
     */
    void build(Widget* root);

    /**
     * @brief Visit all entries in paint order (back to front).
     * Callback receives: widget, clip rect, opacity, z-index.
     */
    using PaintVisitor = std::function<void(Widget*, const Rect*, float opacity, int32_t zIndex)>;
    void visitPaintOrder(PaintVisitor visitor) const;

    /**
     * @brief Hit-test in reverse paint order (front to back).
     * Returns the first widget that contains the point.
     */
    Widget* hitTest(float x, float y) const;

    /**
     * @brief Get the number of stacking contexts.
     */
    size_t contextCount() const { return contextCount_; }

    /**
     * @brief Debug dump of the stacking tree.
     */
    std::string debugDump() const;

private:
    void buildEntry(Widget* widget, StackingEntry* parent);
    void sortEntry(StackingEntry* entry);
    void visitEntry(const StackingEntry* entry, PaintVisitor& visitor) const;
    Widget* hitTestEntry(const StackingEntry* entry, float x, float y) const;
    void dumpEntry(const StackingEntry* entry, int depth, std::string& output) const;

    bool createsStackingContext(Widget* w) const;
    int32_t effectiveZIndex(Widget* w) const;

    std::unique_ptr<StackingEntry> root_;
    size_t contextCount_ = 0;
};

} // namespace NXRender
