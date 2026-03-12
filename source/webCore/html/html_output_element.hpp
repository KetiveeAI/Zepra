/**
 * @file html_output_element.hpp
 * @brief HTMLOutputElement - Calculation output
 *
 * Implements the <output> element per HTML Living Standard.
 * Represents the result of a calculation or user action.
 *
 * @see https://html.spec.whatwg.org/multipage/form-elements.html#the-output-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Zepra::WebCore {

// Forward declarations
class HTMLFormElement;
class DOMTokenList;

/**
 * @brief HTMLOutputElement - calculation result
 *
 * The <output> element represents the result of a calculation
 * or user action.
 */
class HTMLOutputElement : public HTMLElement {
public:
    HTMLOutputElement();
    ~HTMLOutputElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// IDs of elements that contributed to the output
    std::string htmlFor() const;
    void setHtmlFor(const std::string& forValue);

    /// Named of the output (for form submission)
    std::string name() const;
    void setName(const std::string& name);

    /// ID of associated form
    std::string formId() const;
    void setFormId(const std::string& formId);

    // =========================================================================
    // Value
    // =========================================================================

    /// Current value (text content)
    std::string value() const;
    void setValue(const std::string& value);

    /// Default value
    std::string defaultValue() const;
    void setDefaultValue(const std::string& value);

    // =========================================================================
    // Associations
    // =========================================================================

    /// The for attribute as a DOMTokenList
    DOMTokenList* htmlForElements() const;

    /// The associated form
    HTMLFormElement* form() const;

    /// Labels associated with this output
    std::vector<HTMLElement*> labels() const;

    // =========================================================================
    // Validation
    // =========================================================================

    /// Type of form control (always "output")
    std::string type() const;

    /// Whether the output is valid (always true)
    bool checkValidity();

    /// Report validity with UI feedback
    bool reportValidity();

    /// Set custom validity message
    void setCustomValidity(const std::string& message);

    /// Validation message
    std::string validationMessage() const;

    /// Validity state
    bool willValidate() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
