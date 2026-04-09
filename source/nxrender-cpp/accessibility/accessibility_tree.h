// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file accessibility_tree.h
 * @brief Manages the full accessibility tree and provides query APIs.
 */

#pragma once

#include "accessible.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

namespace NXRender {

class Widget;

/**
 * @brief Event types emitted to screen readers.
 */
enum class AccessibilityEvent : uint8_t {
    Focus,              // Widget gained focus
    Blur,               // Widget lost focus
    ValueChanged,       // Value changed (slider, field)
    StateChanged,       // State flag changed (checked, expanded)
    TextChanged,        // Text content changed
    SelectionChanged,   // Selection changed (list, tree)
    ChildrenChanged,    // Children added/removed
    LiveRegionChanged,  // Live region content changed
    Alert,              // Alert fired
    Scroll              // Scroll position changed
};

/**
 * @brief Callback for accessibility events.
 */
using AccessibilityEventCallback = std::function<void(AccessibilityEvent event,
                                                       AccessibleNode* node)>;

/**
 * @brief Manages the accessibility tree for the entire UI.
 *
 * - Automatically builds the accessible tree from the widget tree
 * - Maps widgets to accessible nodes
 * - Provides focus navigation (next/previous/first/last)
 * - Emits events to registered listeners (screen readers)
 */
class AccessibilityTree {
public:
    static AccessibilityTree& instance();

    /**
     * @brief Build or rebuild the tree from the widget root.
     */
    void build(Widget* root);

    /**
     * @brief Get the root accessible node.
     */
    AccessibleNode* root() const { return root_.get(); }

    /**
     * @brief Find the accessible node for a widget.
     */
    AccessibleNode* nodeForWidget(Widget* widget) const;

    /**
     * @brief Get the currently focused accessible node.
     */
    AccessibleNode* focusedNode() const { return focusedNode_; }

    /**
     * @brief Set focus to a specific node.
     */
    void setFocus(AccessibleNode* node);

    /**
     * @brief Move focus to the next/previous focusable node.
     */
    AccessibleNode* focusNext();
    AccessibleNode* focusPrevious();
    AccessibleNode* focusFirst();
    AccessibleNode* focusLast();

    // ==================================================================
    // Events
    // ==================================================================

    /**
     * @brief Register a listener for accessibility events.
     */
    uint32_t addListener(AccessibilityEventCallback callback);
    void removeListener(uint32_t id);

    /**
     * @brief Fire an accessibility event.
     */
    void fireEvent(AccessibilityEvent event, AccessibleNode* node);

    /**
     * @brief Notify that a widget's value changed.
     */
    void notifyValueChanged(Widget* widget);

    /**
     * @brief Notify that a widget's state changed.
     */
    void notifyStateChanged(Widget* widget);

    /**
     * @brief Announce text to screen reader (live region).
     */
    void announce(const std::string& text, AccessibleNode::LivePriority priority =
                  AccessibleNode::LivePriority::Polite);

    // ==================================================================
    // Queries
    // ==================================================================

    /**
     * @brief Find nodes by role.
     */
    std::vector<AccessibleNode*> findByRole(AccessibleRole role) const;

    /**
     * @brief Find node by accessible name.
     */
    AccessibleNode* findByName(const std::string& name) const;

    /**
     * @brief Hit-test for the accessible element at the given point.
     */
    AccessibleNode* hitTest(float x, float y) const;

    /**
     * @brief Get all focusable nodes in tab order.
     */
    std::vector<AccessibleNode*> focusableNodes() const;

    /**
     * @brief Debug: describe the tree as text.
     */
    std::string describe() const;

private:
    AccessibilityTree() = default;

    void buildNode(Widget* widget, AccessibleNode* parent);
    AccessibleRole inferRole(Widget* widget) const;
    std::string inferName(Widget* widget) const;
    void collectFocusable(AccessibleNode* node, std::vector<AccessibleNode*>& out) const;
    void collectByRole(AccessibleNode* node, AccessibleRole role,
                       std::vector<AccessibleNode*>& out) const;
    AccessibleNode* findByNameRecursive(AccessibleNode* node, const std::string& name) const;
    AccessibleNode* hitTestNode(AccessibleNode* node, float x, float y) const;
    void describeNode(AccessibleNode* node, int depth, std::string& output) const;
    bool isFocusable(AccessibleNode* node) const;

    std::unique_ptr<AccessibleNode> root_;
    std::unordered_map<Widget*, AccessibleNode*> widgetMap_;
    AccessibleNode* focusedNode_ = nullptr;

    struct Listener {
        uint32_t id;
        AccessibilityEventCallback callback;
    };
    std::vector<Listener> listeners_;
    uint32_t nextListenerId_ = 1;

    // For announcements
    std::unique_ptr<AccessibleNode> announceNode_;
};

} // namespace NXRender
