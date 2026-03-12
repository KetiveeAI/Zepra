/**
 * @file html_option_element.hpp
 * @brief HTMLOptionElement and HTMLOptGroupElement
 *
 * Implements <option> and <optgroup> elements per HTML Living Standard.
 * Used within <select>, <datalist>, and listbox controls.
 *
 * @see https://html.spec.whatwg.org/multipage/form-elements.html#the-option-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

// Forward declarations
class HTMLSelectElement;
class HTMLDataListElement;

/**
 * @brief HTMLOptionElement - select option
 *
 * The <option> element represents an option in a select control or datalist.
 */
class HTMLOptionElement : public HTMLElement {
public:
    HTMLOptionElement();
    ~HTMLOptionElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Whether this option is disabled
    bool disabled() const;
    void setDisabled(bool disabled);

    /// Label for the option (if different from text content)
    std::string label() const;
    void setLabel(const std::string& label);

    /// Whether this is the default selected option
    bool defaultSelected() const;
    void setDefaultSelected(bool selected);

    /// Whether this option is currently selected
    bool selected() const;
    void setSelected(bool selected);

    /// Value submitted with the form
    std::string value() const;
    void setValue(const std::string& value);

    // =========================================================================
    // Computed Properties
    // =========================================================================

    /// Effective label (label attribute or text content)
    std::string text() const;
    void setText(const std::string& text);

    /// Index within the containing select/datalist
    int index() const;

    /// The containing form element (if any)
    HTMLElement* form() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLOptGroupElement - option group
 *
 * The <optgroup> element represents a group of options with a common label.
 */
class HTMLOptGroupElement : public HTMLElement {
public:
    HTMLOptGroupElement();
    ~HTMLOptGroupElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Whether this group is disabled (all options within are disabled)
    bool disabled() const;
    void setDisabled(bool disabled);

    /// Group label displayed to user
    std::string label() const;
    void setLabel(const std::string& label);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Option constructor helper
 *
 * Creates an HTMLOptionElement with specified properties.
 */
HTMLOptionElement* createOption(
    const std::string& text,
    const std::string& value = "",
    bool defaultSelected = false,
    bool selected = false
);

} // namespace Zepra::WebCore
