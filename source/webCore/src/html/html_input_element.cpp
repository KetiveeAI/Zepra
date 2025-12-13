/**
 * @file html_input_element.cpp
 * @brief HTMLInputElement implementation
 *
 * Full implementation of input element with validation.
 */

#include "webcore/html/html_input_element.hpp"
#include "webcore/html/html_form_element.hpp"
#include <algorithm>
#include <cmath>
#include <regex>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace Zepra::WebCore {

// =============================================================================
// ValidityState Implementation
// =============================================================================

ValidityState::ValidityState() = default;

// =============================================================================
// HTMLInputElement::Impl
// =============================================================================

class HTMLInputElement::Impl {
public:
    ValidityState validity;
    std::string customValidity;
    
    unsigned int selectionStart = 0;
    unsigned int selectionEnd = 0;
    std::string selectionDirection = "none";
    
    HTMLElement* popoverTarget = nullptr;
    
    std::vector<std::string> files;
    
    EventListener onInput;
    EventListener onChange;
    EventListener onInvalid;
    EventListener onSearch;
    EventListener onSelect;
};

// =============================================================================
// HTMLInputElement
// =============================================================================

HTMLInputElement::HTMLInputElement()
    : HTMLElement("input"),
      impl_(std::make_unique<Impl>()) {}

HTMLInputElement::~HTMLInputElement() = default;

// Type

std::string HTMLInputElement::type() const {
    std::string t = getAttribute("type");
    if (t.empty()) return "text";
    
    // Normalize to lowercase
    std::string lower;
    lower.reserve(t.size());
    for (char c : t) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    
    // Validate known types
    static const std::vector<std::string> validTypes = {
        "text", "password", "email", "url", "tel", "number", "range",
        "date", "datetime-local", "month", "week", "time", "color",
        "checkbox", "radio", "file", "hidden", "submit", "reset",
        "button", "image", "search"
    };
    
    for (const auto& valid : validTypes) {
        if (lower == valid) return lower;
    }
    
    return "text";  // Default for unknown types
}

void HTMLInputElement::setType(const std::string& type) {
    setAttribute("type", type);
    updateValidity();
}

// Value

std::string HTMLInputElement::value() const {
    return getAttribute("value");
}

void HTMLInputElement::setValue(const std::string& value) {
    setAttribute("value", value);
    updateValidity();
    
    // Fire input event
    InputEvent event("input", value, "insertText");
    dispatchEvent(event);
}

std::string HTMLInputElement::defaultValue() const {
    return getAttribute("defaultValue");
}

void HTMLInputElement::setDefaultValue(const std::string& value) {
    setAttribute("defaultValue", value);
}

// Name

std::string HTMLInputElement::name() const {
    return getAttribute("name");
}

void HTMLInputElement::setName(const std::string& name) {
    setAttribute("name", name);
}

// States

bool HTMLInputElement::disabled() const {
    return hasAttribute("disabled");
}

void HTMLInputElement::setDisabled(bool disabled) {
    if (disabled) {
        setAttribute("disabled", "");
    } else {
        removeAttribute("disabled");
    }
}

bool HTMLInputElement::required() const {
    return hasAttribute("required");
}

void HTMLInputElement::setRequired(bool required) {
    if (required) {
        setAttribute("required", "");
    } else {
        removeAttribute("required");
    }
    updateValidity();
}

bool HTMLInputElement::readOnly() const {
    return hasAttribute("readonly");
}

void HTMLInputElement::setReadOnly(bool readOnly) {
    if (readOnly) {
        setAttribute("readonly", "");
    } else {
        removeAttribute("readonly");
    }
}

std::string HTMLInputElement::placeholder() const {
    return getAttribute("placeholder");
}

void HTMLInputElement::setPlaceholder(const std::string& placeholder) {
    setAttribute("placeholder", placeholder);
}

std::string HTMLInputElement::autocomplete() const {
    return getAttribute("autocomplete");
}

void HTMLInputElement::setAutocomplete(const std::string& autocomplete) {
    setAttribute("autocomplete", autocomplete);
}

bool HTMLInputElement::autofocus() const {
    return hasAttribute("autofocus");
}

void HTMLInputElement::setAutofocus(bool autofocus) {
    if (autofocus) {
        setAttribute("autofocus", "");
    } else {
        removeAttribute("autofocus");
    }
}

// Form association

HTMLFormElement* HTMLInputElement::form() const {
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

std::string HTMLInputElement::formAction() const {
    return getAttribute("formaction");
}

void HTMLInputElement::setFormAction(const std::string& action) {
    setAttribute("formaction", action);
}

std::string HTMLInputElement::formEnctype() const {
    return getAttribute("formenctype");
}

void HTMLInputElement::setFormEnctype(const std::string& enctype) {
    setAttribute("formenctype", enctype);
}

std::string HTMLInputElement::formMethod() const {
    return getAttribute("formmethod");
}

void HTMLInputElement::setFormMethod(const std::string& method) {
    setAttribute("formmethod", method);
}

bool HTMLInputElement::formNoValidate() const {
    return hasAttribute("formnovalidate");
}

void HTMLInputElement::setFormNoValidate(bool noValidate) {
    if (noValidate) {
        setAttribute("formnovalidate", "");
    } else {
        removeAttribute("formnovalidate");
    }
}

std::string HTMLInputElement::formTarget() const {
    return getAttribute("formtarget");
}

void HTMLInputElement::setFormTarget(const std::string& target) {
    setAttribute("formtarget", target);
}

// Checkbox/Radio

bool HTMLInputElement::checked() const {
    return hasAttribute("checked");
}

void HTMLInputElement::setChecked(bool checked) {
    if (checked) {
        setAttribute("checked", "");
        
        // For radio buttons, uncheck others in group
        if (type() == "radio") {
            std::string n = name();
            if (!n.empty() && form()) {
                for (size_t i = 0; i < form()->length(); ++i) {
                    auto* other = dynamic_cast<HTMLInputElement*>((*form())[i]);
                    if (other && other != this && other->type() == "radio" && other->name() == n) {
                        other->removeAttribute("checked");
                    }
                }
            }
        }
    } else {
        removeAttribute("checked");
    }
    
    // Fire change event
    Event event("change", true, false);
    dispatchEvent(event);
}

bool HTMLInputElement::defaultChecked() const {
    return hasAttribute("checked");
}

void HTMLInputElement::setDefaultChecked(bool checked) {
    if (checked) {
        setAttribute("checked", "");
    } else {
        removeAttribute("checked");
    }
}

bool HTMLInputElement::indeterminate() const {
    return hasAttribute("indeterminate");
}

void HTMLInputElement::setIndeterminate(bool indeterminate) {
    if (indeterminate) {
        setAttribute("indeterminate", "");
    } else {
        removeAttribute("indeterminate");
    }
}

// Text constraints

int HTMLInputElement::maxLength() const {
    std::string val = getAttribute("maxlength");
    if (val.empty()) return -1;
    try {
        return std::stoi(val);
    } catch (...) {
        return -1;
    }
}

void HTMLInputElement::setMaxLength(int length) {
    if (length < 0) {
        removeAttribute("maxlength");
    } else {
        setAttribute("maxlength", std::to_string(length));
    }
    updateValidity();
}

int HTMLInputElement::minLength() const {
    std::string val = getAttribute("minlength");
    if (val.empty()) return 0;
    try {
        return std::stoi(val);
    } catch (...) {
        return 0;
    }
}

void HTMLInputElement::setMinLength(int length) {
    if (length <= 0) {
        removeAttribute("minlength");
    } else {
        setAttribute("minlength", std::to_string(length));
    }
    updateValidity();
}

std::string HTMLInputElement::pattern() const {
    return getAttribute("pattern");
}

void HTMLInputElement::setPattern(const std::string& pattern) {
    setAttribute("pattern", pattern);
    updateValidity();
}

unsigned int HTMLInputElement::size() const {
    std::string val = getAttribute("size");
    if (val.empty()) return 20;
    try {
        return static_cast<unsigned int>(std::stoul(val));
    } catch (...) {
        return 20;
    }
}

void HTMLInputElement::setSize(unsigned int size) {
    setAttribute("size", std::to_string(size));
}

// Number/Range

std::string HTMLInputElement::min() const {
    return getAttribute("min");
}

void HTMLInputElement::setMin(const std::string& min) {
    setAttribute("min", min);
    updateValidity();
}

std::string HTMLInputElement::max() const {
    return getAttribute("max");
}

void HTMLInputElement::setMax(const std::string& max) {
    setAttribute("max", max);
    updateValidity();
}

std::string HTMLInputElement::step() const {
    std::string val = getAttribute("step");
    return val.empty() ? "1" : val;
}

void HTMLInputElement::setStep(const std::string& step) {
    setAttribute("step", step);
    updateValidity();
}

double HTMLInputElement::valueAsNumber() const {
    std::string t = type();
    if (t == "number" || t == "range") {
        try {
            return std::stod(value());
        } catch (...) {
            return std::nan("");
        }
    }
    return std::nan("");
}

void HTMLInputElement::setValueAsNumber(double val) {
    if (std::isnan(val)) {
        setValue("");
    } else {
        std::ostringstream oss;
        oss << val;
        setValue(oss.str());
    }
}

// Date

long long HTMLInputElement::valueAsDate() const {
    std::string t = type();
    std::string v = value();
    
    if (t == "date" && !v.empty()) {
        // Parse YYYY-MM-DD
        std::tm tm = {};
        std::istringstream ss(v);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (!ss.fail()) {
            return static_cast<long long>(std::mktime(&tm)) * 1000;
        }
    }
    
    return 0;
}

void HTMLInputElement::setValueAsDate(long long date) {
    std::time_t time = static_cast<std::time_t>(date / 1000);
    std::tm* tm = std::localtime(&time);
    if (tm) {
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d");
        setValue(oss.str());
    }
}

// File

std::string HTMLInputElement::accept() const {
    return getAttribute("accept");
}

void HTMLInputElement::setAccept(const std::string& accept) {
    setAttribute("accept", accept);
}

bool HTMLInputElement::multiple() const {
    return hasAttribute("multiple");
}

void HTMLInputElement::setMultiple(bool multiple) {
    if (multiple) {
        setAttribute("multiple", "");
    } else {
        removeAttribute("multiple");
    }
}

std::vector<std::string> HTMLInputElement::files() const {
    return impl_->files;
}

// Selection

unsigned int HTMLInputElement::selectionStart() const {
    return impl_->selectionStart;
}

void HTMLInputElement::setSelectionStart(unsigned int start) {
    impl_->selectionStart = start;
}

unsigned int HTMLInputElement::selectionEnd() const {
    return impl_->selectionEnd;
}

void HTMLInputElement::setSelectionEnd(unsigned int end) {
    impl_->selectionEnd = end;
}

std::string HTMLInputElement::selectionDirection() const {
    return impl_->selectionDirection;
}

void HTMLInputElement::setSelectionDirection(const std::string& direction) {
    impl_->selectionDirection = direction;
}

// Validation

ValidityState* HTMLInputElement::validity() {
    return &impl_->validity;
}

const ValidityState* HTMLInputElement::validity() const {
    return &impl_->validity;
}

std::string HTMLInputElement::validationMessage() const {
    if (!impl_->customValidity.empty()) {
        return impl_->customValidity;
    }
    
    if (impl_->validity.valueMissing()) {
        return "Please fill out this field.";
    }
    if (impl_->validity.typeMismatch()) {
        std::string t = type();
        if (t == "email") return "Please enter a valid email address.";
        if (t == "url") return "Please enter a valid URL.";
        return "Please enter a valid value.";
    }
    if (impl_->validity.patternMismatch()) {
        return "Please match the requested format.";
    }
    if (impl_->validity.tooShort()) {
        return "Please lengthen this text.";
    }
    if (impl_->validity.tooLong()) {
        return "Please shorten this text.";
    }
    if (impl_->validity.rangeUnderflow()) {
        return "Value must be greater than or equal to " + min() + ".";
    }
    if (impl_->validity.rangeOverflow()) {
        return "Value must be less than or equal to " + max() + ".";
    }
    if (impl_->validity.stepMismatch()) {
        return "Please enter a valid value.";
    }
    
    return "";
}

bool HTMLInputElement::willValidate() const {
    if (disabled() || readOnly()) return false;
    std::string t = type();
    if (t == "hidden" || t == "reset" || t == "button") return false;
    return true;
}

bool HTMLInputElement::checkValidity() {
    if (!willValidate()) return true;
    
    updateValidity();
    
    if (!impl_->validity.valid()) {
        Event event("invalid", true, true);
        dispatchEvent(event);
        return false;
    }
    
    return true;
}

bool HTMLInputElement::reportValidity() {
    bool valid = checkValidity();
    
    if (!valid) {
        focus();
        // Would show validation UI in full implementation
    }
    
    return valid;
}

void HTMLInputElement::setCustomValidity(const std::string& message) {
    impl_->customValidity = message;
    impl_->validity.setCustomError(!message.empty());
}

void HTMLInputElement::updateValidity() {
    ValidityState& v = impl_->validity;
    std::string val = value();
    std::string t = type();
    
    // Reset all flags
    v.setBadInput(false);
    v.setPatternMismatch(false);
    v.setRangeOverflow(false);
    v.setRangeUnderflow(false);
    v.setStepMismatch(false);
    v.setTooLong(false);
    v.setTooShort(false);
    v.setTypeMismatch(false);
    v.setValueMissing(false);
    
    // Check required
    if (required() && val.empty()) {
        v.setValueMissing(true);
    }
    
    // Skip further checks if empty
    if (val.empty()) return;
    
    // Type validation
    if (t == "email") {
        // Simple email regex
        std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        if (!std::regex_match(val, emailRegex)) {
            v.setTypeMismatch(true);
        }
    } else if (t == "url") {
        // Simple URL regex
        std::regex urlRegex(R"(https?://[^\s]+)");
        if (!std::regex_match(val, urlRegex)) {
            v.setTypeMismatch(true);
        }
    } else if (t == "number" || t == "range") {
        try {
            double num = std::stod(val);
            
            std::string minStr = min();
            if (!minStr.empty()) {
                double minVal = std::stod(minStr);
                if (num < minVal) v.setRangeUnderflow(true);
            }
            
            std::string maxStr = max();
            if (!maxStr.empty()) {
                double maxVal = std::stod(maxStr);
                if (num > maxVal) v.setRangeOverflow(true);
            }
            
            std::string stepStr = step();
            if (stepStr != "any") {
                double stepVal = std::stod(stepStr);
                double base = minStr.empty() ? 0 : std::stod(minStr);
                double remainder = std::fmod(num - base, stepVal);
                if (std::abs(remainder) > 1e-10 && std::abs(remainder - stepVal) > 1e-10) {
                    v.setStepMismatch(true);
                }
            }
        } catch (...) {
            v.setBadInput(true);
        }
    }
    
    // Pattern validation
    std::string pat = pattern();
    if (!pat.empty()) {
        try {
            std::regex patternRegex("^(" + pat + ")$");
            if (!std::regex_match(val, patternRegex)) {
                v.setPatternMismatch(true);
            }
        } catch (...) {
            // Invalid pattern - ignore
        }
    }
    
    // Length validation
    int minLen = minLength();
    if (minLen > 0 && static_cast<int>(val.length()) < minLen) {
        v.setTooShort(true);
    }
    
    int maxLen = maxLength();
    if (maxLen >= 0 && static_cast<int>(val.length()) > maxLen) {
        v.setTooLong(true);
    }
}

// Methods

void HTMLInputElement::select() {
    impl_->selectionStart = 0;
    impl_->selectionEnd = static_cast<unsigned int>(value().length());
    impl_->selectionDirection = "none";
    
    // Fire select event
    Event event("select", true, false);
    dispatchEvent(event);
}

void HTMLInputElement::setSelectionRange(unsigned int start, unsigned int end, 
                                          const std::string& direction) {
    unsigned int len = static_cast<unsigned int>(value().length());
    impl_->selectionStart = std::min(start, len);
    impl_->selectionEnd = std::min(end, len);
    impl_->selectionDirection = direction;
}

void HTMLInputElement::setRangeText(const std::string& replacement) {
    setRangeText(replacement, impl_->selectionStart, impl_->selectionEnd, "preserve");
}

void HTMLInputElement::setRangeText(const std::string& replacement, 
                                     unsigned int start, unsigned int end,
                                     const std::string& selectMode) {
    std::string val = value();
    unsigned int len = static_cast<unsigned int>(val.length());
    
    start = std::min(start, len);
    end = std::min(end, len);
    if (start > end) std::swap(start, end);
    
    std::string newValue = val.substr(0, start) + replacement + val.substr(end);
    setValue(newValue);
    
    if (selectMode == "select") {
        impl_->selectionStart = start;
        impl_->selectionEnd = start + static_cast<unsigned int>(replacement.length());
    } else if (selectMode == "start") {
        impl_->selectionStart = impl_->selectionEnd = start;
    } else if (selectMode == "end") {
        impl_->selectionStart = impl_->selectionEnd = start + static_cast<unsigned int>(replacement.length());
    }
    // "preserve" keeps current selection
}

void HTMLInputElement::stepUp(int n) {
    double num = valueAsNumber();
    if (std::isnan(num)) num = 0;
    
    double stepVal = 1;
    std::string stepStr = step();
    if (stepStr != "any") {
        try {
            stepVal = std::stod(stepStr);
        } catch (...) {}
    }
    
    setValueAsNumber(num + stepVal * n);
}

void HTMLInputElement::stepDown(int n) {
    stepUp(-n);
}

void HTMLInputElement::showPicker() {
    // Would trigger native picker UI for date/color/file
    // This is platform-specific
}

// Popover

HTMLElement* HTMLInputElement::popoverTargetElement() const {
    return impl_->popoverTarget;
}

void HTMLInputElement::setPopoverTargetElement(HTMLElement* element) {
    impl_->popoverTarget = element;
}

std::string HTMLInputElement::popoverTargetAction() const {
    return getAttribute("popovertargetaction");
}

void HTMLInputElement::setPopoverTargetAction(const std::string& action) {
    setAttribute("popovertargetaction", action);
}

// Event handlers

void HTMLInputElement::setOnInput(EventListener callback) {
    impl_->onInput = std::move(callback);
    addEventListener("input", impl_->onInput);
}

void HTMLInputElement::setOnChange(EventListener callback) {
    impl_->onChange = std::move(callback);
    addEventListener("change", impl_->onChange);
}

void HTMLInputElement::setOnInvalid(EventListener callback) {
    impl_->onInvalid = std::move(callback);
    addEventListener("invalid", impl_->onInvalid);
}

void HTMLInputElement::setOnSearch(EventListener callback) {
    impl_->onSearch = std::move(callback);
    addEventListener("search", impl_->onSearch);
}

void HTMLInputElement::setOnSelect(EventListener callback) {
    impl_->onSelect = std::move(callback);
    addEventListener("select", impl_->onSelect);
}

// Clone

std::unique_ptr<DOMNode> HTMLInputElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLInputElement>();
    copyHTMLElementProperties(clone.get());
    (void)deep;  // Input has no children
    return clone;
}

InputType HTMLInputElement::parseType(const std::string& t) const {
    if (t == "password") return InputType::Password;
    if (t == "email") return InputType::Email;
    if (t == "url") return InputType::Url;
    if (t == "tel") return InputType::Tel;
    if (t == "number") return InputType::Number;
    if (t == "range") return InputType::Range;
    if (t == "date") return InputType::Date;
    if (t == "datetime-local") return InputType::DateTimeLocal;
    if (t == "month") return InputType::Month;
    if (t == "week") return InputType::Week;
    if (t == "time") return InputType::Time;
    if (t == "color") return InputType::Color;
    if (t == "checkbox") return InputType::Checkbox;
    if (t == "radio") return InputType::Radio;
    if (t == "file") return InputType::File;
    if (t == "hidden") return InputType::Hidden;
    if (t == "submit") return InputType::Submit;
    if (t == "reset") return InputType::Reset;
    if (t == "button") return InputType::Button;
    if (t == "image") return InputType::Image;
    if (t == "search") return InputType::Search;
    return InputType::Text;
}

} // namespace Zepra::WebCore
