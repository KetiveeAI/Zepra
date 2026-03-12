/**
 * @file html_slot_element.hpp
 * @brief HTMLSlotElement - Shadow DOM slot
 *
 * Implements the <slot> element per HTML Living Standard.
 * Part of Web Components for Shadow DOM content distribution.
 *
 * @see https://html.spec.whatwg.org/multipage/scripting.html#the-slot-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief HTMLSlotElement - shadow DOM slot
 *
 * The <slot> element is a placeholder inside a web component that you can 
 * fill with your own markup, which lets you create separate DOM trees and 
 * present them together.
 */
class HTMLSlotElement : public HTMLElement {
public:
    HTMLSlotElement();
    ~HTMLSlotElement() override;

    // =========================================================================
    // Core Attribute
    // =========================================================================

    /// Slot name (matches slot attribute on assigned nodes)
    std::string name() const;
    void setName(const std::string& name);

    // =========================================================================
    // Assigned Content
    // =========================================================================

    /// Get assigned nodes
    std::vector<DOMNode*> assignedNodes() const;

    /// Get assigned nodes with flatten option
    struct AssignedNodesOptions {
        bool flatten = false;
    };
    std::vector<DOMNode*> assignedNodes(const AssignedNodesOptions& options) const;

    /// Get assigned elements
    std::vector<HTMLElement*> assignedElements() const;

    /// Get assigned elements with flatten option
    std::vector<HTMLElement*> assignedElements(const AssignedNodesOptions& options) const;

    // =========================================================================
    // Assignment
    // =========================================================================

    /// Manually assign nodes to this slot
    void assign(const std::vector<DOMNode*>& nodes);

    // =========================================================================
    // Events
    // =========================================================================

    using SlotChangeCallback = std::function<void()>;
    
    void setOnSlotChange(SlotChangeCallback callback);
    SlotChangeCallback onSlotChange() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
