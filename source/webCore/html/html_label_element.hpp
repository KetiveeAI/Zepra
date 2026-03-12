/**
 * @file html_label_element.hpp  
 * @brief HTML Label Element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>

namespace Zepra::WebCore {

class HTMLFormControlElement;

/**
 * @brief HTML Label Element (<label>)
 */
class HTMLLabelElement : public HTMLElement {
public:
    HTMLLabelElement();
    explicit HTMLLabelElement(const std::string& forId);
    ~HTMLLabelElement() override = default;
    
    // For attribute (ID of labeled control)
    std::string htmlFor() const { return getAttribute("for"); }
    void setHtmlFor(const std::string& id) { setAttribute("for", id); }
    
    // Get the labeled control
    HTMLElement* control() const;
    
    // Form association
    HTMLElement* form() const;
    
    std::string tagName() const { return "LABEL"; }
};

/**
 * @brief HTML FieldSet Element (<fieldset>)
 */
class HTMLFieldSetElement : public HTMLElement {
public:
    HTMLFieldSetElement();
    ~HTMLFieldSetElement() override = default;
    
    bool disabled() const { return hasAttribute("disabled"); }
    void setDisabled(bool d) { if (d) setAttribute("disabled", ""); else removeAttribute("disabled"); }
    
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    // Get the first legend child
    HTMLElement* legend() const;
    
    // Form association
    HTMLElement* form() const;
    
    // Validation
    bool checkValidity() const;
    
    std::string tagName() const { return "FIELDSET"; }
    bool isFormControl() const { return true; }
};

/**
 * @brief HTML Legend Element (<legend>)
 */
class HTMLLegendElement : public HTMLElement {
public:
    HTMLLegendElement();
    ~HTMLLegendElement() override = default;
    
    // Get parent fieldset's form
    HTMLElement* form() const;
    
    std::string tagName() const { return "LEGEND"; }
};

} // namespace Zepra::WebCore
