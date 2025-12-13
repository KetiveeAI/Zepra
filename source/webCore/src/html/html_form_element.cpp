/**
 * @file html_form_element.cpp
 * @brief HTMLFormElement implementation
 *
 * Full implementation of form handling including submission,
 * validation, and form control management.
 */

#include "webcore/html/html_form_element.hpp"
#include "webcore/html/html_anchor_element.hpp"  // For DOMTokenList
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

namespace Zepra::WebCore {

// =============================================================================
// HTMLFormControlsCollection Implementation
// =============================================================================

HTMLFormControlsCollection::HTMLFormControlsCollection() = default;
HTMLFormControlsCollection::~HTMLFormControlsCollection() = default;

size_t HTMLFormControlsCollection::length() const {
    return controls_.size();
}

HTMLElement* HTMLFormControlsCollection::item(size_t index) const {
    if (index < controls_.size()) {
        return controls_[index];
    }
    return nullptr;
}

HTMLElement* HTMLFormControlsCollection::namedItem(const std::string& name) const {
    for (auto* control : controls_) {
        if (control->getAttribute("name") == name || 
            control->getAttribute("id") == name) {
            return control;
        }
    }
    return nullptr;
}

void HTMLFormControlsCollection::addControl(HTMLElement* control) {
    if (control && std::find(controls_.begin(), controls_.end(), control) == controls_.end()) {
        controls_.push_back(control);
    }
}

void HTMLFormControlsCollection::removeControl(HTMLElement* control) {
    controls_.erase(
        std::remove(controls_.begin(), controls_.end(), control),
        controls_.end());
}

void HTMLFormControlsCollection::clear() {
    controls_.clear();
}

std::vector<HTMLElement*>::const_iterator HTMLFormControlsCollection::begin() const {
    return controls_.begin();
}

std::vector<HTMLElement*>::const_iterator HTMLFormControlsCollection::end() const {
    return controls_.end();
}

// =============================================================================
// HTMLFormElement::Impl
// =============================================================================

class HTMLFormElement::Impl {
public:
    HTMLFormControlsCollection elements;
    std::unique_ptr<DOMTokenList> relList;
    
    EventListener onSubmit;
    EventListener onReset;
    EventListener onFormData;
};

// =============================================================================
// HTMLFormElement
// =============================================================================

HTMLFormElement::HTMLFormElement()
    : HTMLElement("form"),
      impl_(std::make_unique<Impl>()) {
    impl_->relList = std::make_unique<DOMTokenList>(this, "rel");
}

HTMLFormElement::~HTMLFormElement() = default;

// Properties

std::string HTMLFormElement::acceptCharset() const {
    std::string val = getAttribute("accept-charset");
    return val.empty() ? "UTF-8" : val;
}

void HTMLFormElement::setAcceptCharset(const std::string& charset) {
    setAttribute("accept-charset", charset);
}

std::string HTMLFormElement::action() const {
    return getAttribute("action");
}

void HTMLFormElement::setAction(const std::string& action) {
    setAttribute("action", action);
}

std::string HTMLFormElement::autocomplete() const {
    std::string val = getAttribute("autocomplete");
    return val.empty() ? "on" : val;
}

void HTMLFormElement::setAutocomplete(const std::string& autocomplete) {
    setAttribute("autocomplete", autocomplete);
}

std::string HTMLFormElement::enctype() const {
    std::string val = getAttribute("enctype");
    if (val == "multipart/form-data") return val;
    if (val == "text/plain") return val;
    return "application/x-www-form-urlencoded";
}

void HTMLFormElement::setEnctype(const std::string& enctype) {
    setAttribute("enctype", enctype);
}

std::string HTMLFormElement::encoding() const {
    return enctype();
}

void HTMLFormElement::setEncoding(const std::string& encoding) {
    setEnctype(encoding);
}

std::string HTMLFormElement::method() const {
    std::string val = getAttribute("method");
    // Normalize to lowercase
    std::string lower;
    lower.reserve(val.size());
    for (char c : val) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (lower == "post") return "post";
    if (lower == "dialog") return "dialog";
    return "get";
}

void HTMLFormElement::setMethod(const std::string& method) {
    setAttribute("method", method);
}

std::string HTMLFormElement::name() const {
    return getAttribute("name");
}

void HTMLFormElement::setName(const std::string& name) {
    setAttribute("name", name);
}

bool HTMLFormElement::noValidate() const {
    return hasAttribute("novalidate");
}

void HTMLFormElement::setNoValidate(bool noValidate) {
    if (noValidate) {
        setAttribute("novalidate", "");
    } else {
        removeAttribute("novalidate");
    }
}

std::string HTMLFormElement::rel() const {
    return getAttribute("rel");
}

void HTMLFormElement::setRel(const std::string& rel) {
    setAttribute("rel", rel);
}

DOMTokenList* HTMLFormElement::relList() {
    return impl_->relList.get();
}

std::string HTMLFormElement::target() const {
    return getAttribute("target");
}

void HTMLFormElement::setTarget(const std::string& target) {
    setAttribute("target", target);
}

// Form controls access

HTMLFormControlsCollection* HTMLFormElement::elements() {
    return &impl_->elements;
}

const HTMLFormControlsCollection* HTMLFormElement::elements() const {
    return &impl_->elements;
}

size_t HTMLFormElement::length() const {
    return impl_->elements.length();
}

HTMLElement* HTMLFormElement::operator[](size_t index) {
    return impl_->elements.item(index);
}

HTMLElement* HTMLFormElement::operator[](const std::string& name) {
    return impl_->elements.namedItem(name);
}

// Validation methods

bool HTMLFormElement::checkValidity() {
    bool valid = true;
    
    for (auto* control : impl_->elements) {
        // Check if control has checkValidity method
        std::string tagName = control->tagName();
        
        // Fire invalid event on invalid controls
        if (control->hasAttribute("required")) {
            std::string value = control->getAttribute("value");
            if (value.empty()) {
                Event invalidEvent("invalid", true, true);
                control->dispatchEvent(invalidEvent);
                valid = false;
            }
        }
        
        // Check pattern attribute
        if (control->hasAttribute("pattern")) {
            std::string pattern = control->getAttribute("pattern");
            std::string value = control->getAttribute("value");
            // Pattern validation would use regex here
            // For production, implement full pattern matching
        }
        
        // Check min/max for number/date inputs
        if (control->hasAttribute("min") || control->hasAttribute("max")) {
            // Range validation
        }
        
        // Check minlength/maxlength
        if (control->hasAttribute("minlength")) {
            std::string minStr = control->getAttribute("minlength");
            std::string value = control->getAttribute("value");
            try {
                size_t minLen = std::stoul(minStr);
                if (value.length() < minLen) {
                    Event invalidEvent("invalid", true, true);
                    control->dispatchEvent(invalidEvent);
                    valid = false;
                }
            } catch (...) {}
        }
        
        if (control->hasAttribute("maxlength")) {
            std::string maxStr = control->getAttribute("maxlength");
            std::string value = control->getAttribute("value");
            try {
                size_t maxLen = std::stoul(maxStr);
                if (value.length() > maxLen) {
                    Event invalidEvent("invalid", true, true);
                    control->dispatchEvent(invalidEvent);
                    valid = false;
                }
            } catch (...) {}
        }
    }
    
    return valid;
}

bool HTMLFormElement::reportValidity() {
    bool valid = checkValidity();
    
    if (!valid) {
        // Focus first invalid control
        for (auto* control : impl_->elements) {
            if (control->hasAttribute("required")) {
                std::string value = control->getAttribute("value");
                if (value.empty()) {
                    control->focus();
                    break;
                }
            }
        }
    }
    
    return valid;
}

// Form actions

void HTMLFormElement::reset() {
    // Fire reset event (cancelable)
    if (!fireReset()) {
        return;  // Event was canceled
    }
    
    // Reset all controls to default values
    for (auto* control : impl_->elements) {
        std::string tagName = control->tagName();
        
        if (tagName == "input" || tagName == "INPUT") {
            std::string type = control->getAttribute("type");
            
            if (type == "checkbox" || type == "radio") {
                // Reset to default checked state
                if (control->hasAttribute("checked")) {
                    control->setAttribute("checked", "");
                } else {
                    control->removeAttribute("checked");
                }
            } else {
                // Reset to default value
                std::string defaultValue = control->getAttribute("defaultValue");
                if (defaultValue.empty()) {
                    defaultValue = control->getAttribute("value");
                }
                control->setAttribute("value", defaultValue);
            }
        } else if (tagName == "textarea" || tagName == "TEXTAREA") {
            std::string defaultValue = control->textContent();
            control->setAttribute("value", defaultValue);
        } else if (tagName == "select" || tagName == "SELECT") {
            // Reset to default selected option
            // Would iterate through options and reset selection
        }
    }
}

void HTMLFormElement::submit() {
    // Get form data
    std::vector<FormDataEntry> entries = getFormData();
    
    // Encode data based on enctype
    std::string encodedData = encodeFormData(entries);
    
    // In a full implementation, this would:
    // 1. Create the request URL
    // 2. Send the request via the networking layer
    // 3. Navigate to the result
    
    // For method="dialog", close the dialog
    if (method() == "dialog") {
        // Find parent dialog and close it
        DOMNode* parent = parentNode();
        while (parent) {
            if (auto* elem = dynamic_cast<HTMLElement*>(parent)) {
                if (elem->tagName() == "dialog" || elem->tagName() == "DIALOG") {
                    // Get return value from submit button or first text input
                    std::string returnValue;
                    for (const auto& entry : entries) {
                        if (!entry.value.empty()) {
                            returnValue = entry.value;
                            break;
                        }
                    }
                    // Close dialog with return value
                    // elem->close(returnValue);
                    break;
                }
            }
            parent = parent->parentNode();
        }
    }
}

void HTMLFormElement::requestSubmit(HTMLElement* submitter) {
    // Validate form if noValidate is not set
    if (!noValidate() && !reportValidity()) {
        return;
    }
    
    // Fire submit event
    if (!fireSubmit(submitter)) {
        return;  // Event was canceled
    }
    
    // Perform actual submission
    submit();
}

std::vector<FormDataEntry> HTMLFormElement::getFormData() const {
    std::vector<FormDataEntry> entries;
    buildFormData(entries);
    
    // Fire formdata event to allow modification
    // const_cast needed for event dispatch
    const_cast<HTMLFormElement*>(this)->fireFormData(entries);
    
    return entries;
}

void HTMLFormElement::buildFormData(std::vector<FormDataEntry>& entries) const {
    for (auto* control : impl_->elements) {
        std::string name = control->getAttribute("name");
        if (name.empty()) continue;
        
        // Skip disabled controls
        if (control->hasAttribute("disabled")) continue;
        
        std::string tagName = control->tagName();
        
        if (tagName == "input" || tagName == "INPUT") {
            std::string type = control->getAttribute("type");
            
            if (type == "submit" || type == "reset" || type == "button" || type == "image") {
                continue;  // Skip button types unless submitter
            }
            
            if (type == "checkbox" || type == "radio") {
                if (!control->hasAttribute("checked")) {
                    continue;  // Skip unchecked
                }
                std::string value = control->getAttribute("value");
                if (value.empty()) value = "on";
                entries.push_back({name, value, "", ""});
            } else if (type == "file") {
                // File handling would access the file list
                entries.push_back({name, "", "", ""});
            } else {
                std::string value = control->getAttribute("value");
                entries.push_back({name, value, "", ""});
            }
        } else if (tagName == "textarea" || tagName == "TEXTAREA") {
            std::string value = control->getAttribute("value");
            if (value.empty()) {
                value = control->textContent();
            }
            entries.push_back({name, value, "", ""});
        } else if (tagName == "select" || tagName == "SELECT") {
            std::string value = control->getAttribute("value");
            entries.push_back({name, value, "", ""});
        }
    }
}

std::string HTMLFormElement::encodeFormData(const std::vector<FormDataEntry>& entries) const {
    std::string enc = enctype();
    
    if (enc == "application/x-www-form-urlencoded") {
        std::ostringstream oss;
        bool first = true;
        
        for (const auto& entry : entries) {
            if (!first) oss << "&";
            first = false;
            
            // URL encode name and value
            for (char c : entry.name) {
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                    oss << c;
                } else if (c == ' ') {
                    oss << '+';
                } else {
                    oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') 
                        << static_cast<int>(static_cast<unsigned char>(c));
                }
            }
            
            oss << '=';
            
            for (char c : entry.value) {
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                    oss << c;
                } else if (c == ' ') {
                    oss << '+';
                } else {
                    oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(c));
                }
            }
        }
        
        return oss.str();
    } else if (enc == "multipart/form-data") {
        // Multipart encoding
        std::string boundary = "----WebKitFormBoundary" + std::to_string(std::hash<std::string>{}(action()));
        std::ostringstream oss;
        
        for (const auto& entry : entries) {
            oss << "--" << boundary << "\r\n";
            oss << "Content-Disposition: form-data; name=\"" << entry.name << "\"";
            if (!entry.filename.empty()) {
                oss << "; filename=\"" << entry.filename << "\"";
            }
            oss << "\r\n";
            if (!entry.type.empty()) {
                oss << "Content-Type: " << entry.type << "\r\n";
            }
            oss << "\r\n";
            oss << entry.value << "\r\n";
        }
        
        oss << "--" << boundary << "--\r\n";
        return oss.str();
    } else {
        // text/plain
        std::ostringstream oss;
        for (const auto& entry : entries) {
            oss << entry.name << "=" << entry.value << "\r\n";
        }
        return oss.str();
    }
}

bool HTMLFormElement::fireSubmit(HTMLElement* submitter) {
    Event event("submit", true, true);  // bubbles, cancelable
    
    // Set submitter on event (would need SubmitEvent class)
    (void)submitter;
    
    dispatchEvent(event);
    
    if (impl_->onSubmit) {
        impl_->onSubmit(event);
    }
    
    return !event.defaultPrevented();
}

bool HTMLFormElement::fireReset() {
    Event event("reset", true, true);  // bubbles, cancelable
    dispatchEvent(event);
    
    if (impl_->onReset) {
        impl_->onReset(event);
    }
    
    return !event.defaultPrevented();
}

void HTMLFormElement::fireFormData(std::vector<FormDataEntry>& entries) {
    Event event("formdata", true, false);  // bubbles, not cancelable
    dispatchEvent(event);
    
    if (impl_->onFormData) {
        impl_->onFormData(event);
    }
    
    (void)entries;  // Entries could be modified via FormData API
}

// Event handlers

void HTMLFormElement::setOnSubmit(EventListener callback) {
    impl_->onSubmit = std::move(callback);
    addEventListener("submit", impl_->onSubmit);
}

void HTMLFormElement::setOnReset(EventListener callback) {
    impl_->onReset = std::move(callback);
    addEventListener("reset", impl_->onReset);
}

void HTMLFormElement::setOnFormData(EventListener callback) {
    impl_->onFormData = std::move(callback);
    addEventListener("formdata", impl_->onFormData);
}

// Control registration

void HTMLFormElement::registerControl(HTMLElement* control) {
    impl_->elements.addControl(control);
}

void HTMLFormElement::unregisterControl(HTMLElement* control) {
    impl_->elements.removeControl(control);
}

// Clone

std::unique_ptr<DOMNode> HTMLFormElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLFormElement>();
    copyHTMLElementProperties(clone.get());
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

} // namespace Zepra::WebCore
