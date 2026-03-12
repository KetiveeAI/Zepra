/**
 * @file html_details_element.hpp
 * @brief HTMLDetailsElement and HTMLSummaryElement
 *
 * Implements <details> and <summary> elements per HTML Living Standard.
 * Expandable disclosure widgets.
 *
 * @see https://html.spec.whatwg.org/multipage/interactive-elements.html#the-details-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <functional>

namespace Zepra::WebCore {

// Forward declarations
class HTMLSummaryElement;

/**
 * @brief HTMLDetailsElement - disclosure widget
 *
 * The <details> element represents a disclosure widget from which the user
 * can obtain additional information or controls.
 */
class HTMLDetailsElement : public HTMLElement {
public:
    HTMLDetailsElement();
    ~HTMLDetailsElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Whether the details are visible
    bool open() const;
    void setOpen(bool open);

    /// Name for exclusive accordion grouping
    std::string name() const;
    void setName(const std::string& name);

    // =========================================================================
    // Child Access
    // =========================================================================

    /// Get the summary element (first child summary)
    HTMLSummaryElement* summary() const;

    // =========================================================================
    // Actions
    // =========================================================================

    /// Toggle open state
    void toggle();

    /// Expand the details
    void expand();

    /// Collapse the details
    void collapse();

    // =========================================================================
    // Events
    // =========================================================================

    using ToggleCallback = std::function<void(bool /* newState */)>;
    
    void setOnToggle(ToggleCallback callback);
    ToggleCallback onToggle() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLSummaryElement - disclosure summary
 *
 * The <summary> element represents the summary, caption, or legend for
 * the rest of the contents of its parent <details> element.
 */
class HTMLSummaryElement : public HTMLElement {
public:
    HTMLSummaryElement();
    ~HTMLSummaryElement() override;

    // =========================================================================
    // Parent Access
    // =========================================================================

    /// Get the parent details element
    HTMLDetailsElement* details() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
