/**
 * @file html_button_element.hpp
 * @brief HTMLButtonElement interface for <button> elements
 *
 * Implements button functionality per HTML Living Standard.
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/HTMLButtonElement
 * @see https://html.spec.whatwg.org/multipage/form-elements.html#the-button-element
 */

#pragma once

#include "html/html_element.hpp"

namespace Zepra::WebCore {

// Forward declarations
class HTMLFormElement;
class ValidityState;

/**
 * @brief Button type
 */
enum class ButtonType {
    Submit,     ///< Submit form (default)
    Reset,      ///< Reset form
    Button      ///< Plain button (no default action)
};

/**
 * @brief HTMLButtonElement represents a <button> element
 *
 * Provides properties and methods for buttons in forms.
 */
class HTMLButtonElement : public HTMLElement {
public:
    HTMLButtonElement();
    ~HTMLButtonElement() override;

    // =========================================================================
    // Properties
    // =========================================================================

    /// Button type (submit, reset, button)
    std::string type() const;
    void setType(const std::string& type);

    /// Button name for form submission
    std::string name() const;
    void setName(const std::string& name);

    /// Button value for form submission
    std::string value() const;
    void setValue(const std::string& value);

    /// Whether button is disabled
    bool disabled() const;
    void setDisabled(bool disabled);

    /// Autofocus on page load
    bool autofocus() const;
    void setAutofocus(bool autofocus);

    // =========================================================================
    // Form Association
    // =========================================================================

    /// Associated form element
    HTMLFormElement* form() const;

    /// Form action override
    std::string formAction() const;
    void setFormAction(const std::string& action);

    /// Form enctype override
    std::string formEnctype() const;
    void setFormEnctype(const std::string& enctype);

    /// Form method override
    std::string formMethod() const;
    void setFormMethod(const std::string& method);

    /// Form novalidate override
    bool formNoValidate() const;
    void setFormNoValidate(bool noValidate);

    /// Form target override
    std::string formTarget() const;
    void setFormTarget(const std::string& target);

    // =========================================================================
    // Validation
    // =========================================================================

    /// Validation state
    ValidityState* validity();
    const ValidityState* validity() const;

    /// Validation message
    std::string validationMessage() const;

    /// Whether button will be validated
    bool willValidate() const;

    /// Check validity
    bool checkValidity();

    /// Report validity to user
    bool reportValidity();

    /// Set custom validation message
    void setCustomValidity(const std::string& message);

    // =========================================================================
    // Command API (Modern)
    // =========================================================================

    /// Command action
    std::string command() const;
    void setCommand(const std::string& command);

    /// Command target element
    HTMLElement* commandForElement() const;
    void setCommandForElement(HTMLElement* element);

    // =========================================================================
    // Popover Control
    // =========================================================================

    /// Popover target element
    HTMLElement* popoverTargetElement() const;
    void setPopoverTargetElement(HTMLElement* element);

    /// Popover target action
    std::string popoverTargetAction() const;
    void setPopoverTargetAction(const std::string& action);

    // =========================================================================
    // Labels
    // =========================================================================

    /// Associated labels
    std::vector<HTMLElement*> labels() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
