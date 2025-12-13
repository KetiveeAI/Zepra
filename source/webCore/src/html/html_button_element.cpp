/**
 * @file html_button_element.cpp
 * @brief HTMLButtonElement implementation
 *
 * Full implementation of button element.
 */

#include "webcore/html/html_button_element.hpp"
#include "webcore/html/html_form_element.hpp"
#include "webcore/html/html_input_element.hpp"  // For ValidityState

namespace Zepra::WebCore {

// =============================================================================
// HTMLButtonElement::Impl
// =============================================================================

class HTMLButtonElement::Impl {
public:
    ValidityState validity;
    std::string customValidity;
    
    HTMLElement* commandTarget = nullptr;
    HTMLElement* popoverTarget = nullptr;
};

// =============================================================================
// HTMLButtonElement
// =============================================================================

HTMLButtonElement::HTMLButtonElement()
    : HTMLElement("button"),
      impl_(std::make_unique<Impl>()) {
    // Default type is submit
    setAttribute("type", "submit");
}

HTMLButtonElement::~HTMLButtonElement() = default;

// Type

std::string HTMLButtonElement::type() const {
    std::string t = getAttribute("type");
    
    // Normalize to lowercase
    std::string lower;
    lower.reserve(t.size());
    for (char c : t) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    
    if (lower == "reset") return "reset";
    if (lower == "button") return "button";
    return "submit";  // Default
}

void HTMLButtonElement::setType(const std::string& type) {
    setAttribute("type", type);
}

// Name/Value

std::string HTMLButtonElement::name() const {
    return getAttribute("name");
}

void HTMLButtonElement::setName(const std::string& name) {
    setAttribute("name", name);
}

std::string HTMLButtonElement::value() const {
    return getAttribute("value");
}

void HTMLButtonElement::setValue(const std::string& value) {
    setAttribute("value", value);
}

// Disabled

bool HTMLButtonElement::disabled() const {
    return hasAttribute("disabled");
}

void HTMLButtonElement::setDisabled(bool disabled) {
    if (disabled) {
        setAttribute("disabled", "");
    } else {
        removeAttribute("disabled");
    }
}

// Autofocus

bool HTMLButtonElement::autofocus() const {
    return hasAttribute("autofocus");
}

void HTMLButtonElement::setAutofocus(bool autofocus) {
    if (autofocus) {
        setAttribute("autofocus", "");
    } else {
        removeAttribute("autofocus");
    }
}

// Form association

HTMLFormElement* HTMLButtonElement::form() const {
    // Walk up tree to find form
    DOMNode* parent = parentNode();
    while (parent) {
        if (auto* form = dynamic_cast<HTMLFormElement*>(parent)) {
            return form;
        }
        parent = parent->parentNode();
    }
    return nullptr;
}

std::string HTMLButtonElement::formAction() const {
    return getAttribute("formaction");
}

void HTMLButtonElement::setFormAction(const std::string& action) {
    setAttribute("formaction", action);
}

std::string HTMLButtonElement::formEnctype() const {
    return getAttribute("formenctype");
}

void HTMLButtonElement::setFormEnctype(const std::string& enctype) {
    setAttribute("formenctype", enctype);
}

std::string HTMLButtonElement::formMethod() const {
    return getAttribute("formmethod");
}

void HTMLButtonElement::setFormMethod(const std::string& method) {
    setAttribute("formmethod", method);
}

bool HTMLButtonElement::formNoValidate() const {
    return hasAttribute("formnovalidate");
}

void HTMLButtonElement::setFormNoValidate(bool noValidate) {
    if (noValidate) {
        setAttribute("formnovalidate", "");
    } else {
        removeAttribute("formnovalidate");
    }
}

std::string HTMLButtonElement::formTarget() const {
    return getAttribute("formtarget");
}

void HTMLButtonElement::setFormTarget(const std::string& target) {
    setAttribute("formtarget", target);
}

// Validation

ValidityState* HTMLButtonElement::validity() {
    return &impl_->validity;
}

const ValidityState* HTMLButtonElement::validity() const {
    return &impl_->validity;
}

std::string HTMLButtonElement::validationMessage() const {
    return impl_->customValidity;
}

bool HTMLButtonElement::willValidate() const {
    // Buttons don't participate in constraint validation
    return false;
}

bool HTMLButtonElement::checkValidity() {
    // Buttons are always valid
    return true;
}

bool HTMLButtonElement::reportValidity() {
    return true;
}

void HTMLButtonElement::setCustomValidity(const std::string& message) {
    impl_->customValidity = message;
    impl_->validity.setCustomError(!message.empty());
}

// Command API

std::string HTMLButtonElement::command() const {
    return getAttribute("command");
}

void HTMLButtonElement::setCommand(const std::string& command) {
    setAttribute("command", command);
}

HTMLElement* HTMLButtonElement::commandForElement() const {
    return impl_->commandTarget;
}

void HTMLButtonElement::setCommandForElement(HTMLElement* element) {
    impl_->commandTarget = element;
}

// Popover

HTMLElement* HTMLButtonElement::popoverTargetElement() const {
    return impl_->popoverTarget;
}

void HTMLButtonElement::setPopoverTargetElement(HTMLElement* element) {
    impl_->popoverTarget = element;
}

std::string HTMLButtonElement::popoverTargetAction() const {
    return getAttribute("popovertargetaction");
}

void HTMLButtonElement::setPopoverTargetAction(const std::string& action) {
    setAttribute("popovertargetaction", action);
}

// Labels

std::vector<HTMLElement*> HTMLButtonElement::labels() const {
    std::vector<HTMLElement*> result;
    
    std::string id = getAttribute("id");
    if (id.empty()) return result;
    
    // Would search document for labels with for=id
    // This requires document traversal
    
    return result;
}

// Clone

std::unique_ptr<DOMNode> HTMLButtonElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLButtonElement>();
    copyHTMLElementProperties(clone.get());
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

} // namespace Zepra::WebCore
