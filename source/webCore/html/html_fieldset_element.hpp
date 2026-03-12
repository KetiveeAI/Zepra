/**
 * @file html_fieldset_element.hpp
 * @brief HTMLFieldSetElement and HTMLLegendElement
 *
 * Implements <fieldset> and <legend> elements per HTML Living Standard.
 * Used to group form controls with a caption.
 *
 * @see https://html.spec.whatwg.org/multipage/form-elements.html#the-fieldset-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Zepra::WebCore {

// Forward declarations
class HTMLFormElement;
class HTMLLegendElement;

/**
 * @brief HTMLFieldSetElement - form control group
 *
 * The <fieldset> element represents a set of form controls
 * grouped under a common name.
 */
class HTMLFieldSetElement : public HTMLElement {
public:
    HTMLFieldSetElement();
    ~HTMLFieldSetElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Whether the fieldset is disabled (all controls within are disabled)
    bool disabled() const;
    void setDisabled(bool disabled);

    /// Name for the fieldset
    std::string name() const;
    void setName(const std::string& name);

    /// ID of associated form (if not nested within form)
    std::string formId() const;
    void setFormId(const std::string& formId);

    // =========================================================================
    // Associations
    // =========================================================================

    /// The containing or associated form
    HTMLFormElement* form() const;

    /// The legend element (if any)
    HTMLLegendElement* legend() const;

    /// All form elements within this fieldset
    std::vector<HTMLElement*> elements() const;

    // =========================================================================
    // Validation
    // =========================================================================

    /// Type of form control (always "fieldset")
    std::string type() const;

    /// Whether the fieldset is valid
    bool checkValidity();

    /// Report validity with UI feedback
    bool reportValidity();

    /// Custom validity message
    void setCustomValidity(const std::string& message);

    /// Validation message
    std::string validationMessage() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLLegendElement - fieldset caption
 *
 * The <legend> element represents a caption for the contents of its
 * parent <fieldset>.
 */
class HTMLLegendElement : public HTMLElement {
public:
    HTMLLegendElement();
    ~HTMLLegendElement() override;

    // =========================================================================
    // Associations
    // =========================================================================

    /// The fieldset this legend belongs to (if directly within fieldset)
    HTMLFieldSetElement* form() const;

    // =========================================================================
    // Deprecated Attributes (for compatibility)
    // =========================================================================

    /// Alignment (deprecated, use CSS)
    std::string align() const;
    void setAlign(const std::string& align);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
