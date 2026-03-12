// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_form_extras.cpp
 * @brief Implementation of additional form elements: option, fieldset, progress, meter, datalist, output
 */

#include "html/html_option_element.hpp"
#include "html/html_fieldset_element.hpp"
#include "html/html_progress_element.hpp"
#include "html/html_meter_element.hpp"
#include "html/html_datalist_element.hpp"
#include "html/html_output_element.hpp"

namespace Zepra::WebCore {

// =============================================================================
// HTMLOptionElement
// =============================================================================

class HTMLOptionElement::Impl {
public:
    bool disabled_ = false;
    std::string label_;
    bool defaultSelected_ = false;
    bool selected_ = false;
    std::string value_;
};

HTMLOptionElement::HTMLOptionElement()
    : HTMLElement("option")
    , impl_(std::make_unique<Impl>())
{
}

HTMLOptionElement::~HTMLOptionElement() = default;

bool HTMLOptionElement::disabled() const { return impl_->disabled_; }
void HTMLOptionElement::setDisabled(bool disabled) {
    impl_->disabled_ = disabled;
    if (disabled) setAttribute("disabled", "");
    else removeAttribute("disabled");
}

std::string HTMLOptionElement::label() const { return impl_->label_; }
void HTMLOptionElement::setLabel(const std::string& label) {
    impl_->label_ = label;
    setAttribute("label", label);
}

bool HTMLOptionElement::defaultSelected() const { return impl_->defaultSelected_; }
void HTMLOptionElement::setDefaultSelected(bool selected) {
    impl_->defaultSelected_ = selected;
    if (selected) setAttribute("selected", "");
    else removeAttribute("selected");
}

bool HTMLOptionElement::selected() const { return impl_->selected_; }
void HTMLOptionElement::setSelected(bool selected) {
    impl_->selected_ = selected;
}

std::string HTMLOptionElement::value() const {
    if (!impl_->value_.empty()) return impl_->value_;
    return textContent();
}

void HTMLOptionElement::setValue(const std::string& value) {
    impl_->value_ = value;
    setAttribute("value", value);
}

std::string HTMLOptionElement::text() const {
    if (!impl_->label_.empty()) return impl_->label_;
    return textContent();
}

void HTMLOptionElement::setText(const std::string& text) {
    setTextContent(text);
}

int HTMLOptionElement::index() const {
    // Would be computed from parent select/datalist
    return 0;
}

HTMLElement* HTMLOptionElement::form() const {
    // Would traverse up to find form
    return nullptr;
}

std::unique_ptr<DOMNode> HTMLOptionElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLOptionElement>();
    clone->setDisabled(impl_->disabled_);
    clone->setLabel(impl_->label_);
    clone->setDefaultSelected(impl_->defaultSelected_);
    clone->setValue(impl_->value_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLOptGroupElement
// =============================================================================

class HTMLOptGroupElement::Impl {
public:
    bool disabled_ = false;
    std::string label_;
};

HTMLOptGroupElement::HTMLOptGroupElement()
    : HTMLElement("optgroup")
    , impl_(std::make_unique<Impl>())
{
}

HTMLOptGroupElement::~HTMLOptGroupElement() = default;

bool HTMLOptGroupElement::disabled() const { return impl_->disabled_; }
void HTMLOptGroupElement::setDisabled(bool disabled) {
    impl_->disabled_ = disabled;
    if (disabled) setAttribute("disabled", "");
    else removeAttribute("disabled");
}

std::string HTMLOptGroupElement::label() const { return impl_->label_; }
void HTMLOptGroupElement::setLabel(const std::string& label) {
    impl_->label_ = label;
    setAttribute("label", label);
}

std::unique_ptr<DOMNode> HTMLOptGroupElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLOptGroupElement>();
    clone->setDisabled(impl_->disabled_);
    clone->setLabel(impl_->label_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

HTMLOptionElement* createOption(const std::string& text, const std::string& value,
                                 bool defaultSelected, bool selected) {
    auto* option = new HTMLOptionElement();
    option->setText(text);
    if (!value.empty()) option->setValue(value);
    option->setDefaultSelected(defaultSelected);
    option->setSelected(selected);
    return option;
}

// =============================================================================
// HTMLFieldSetElement
// =============================================================================

class HTMLFieldSetElement::Impl {
public:
    bool disabled_ = false;
    std::string name_;
    std::string formId_;
    std::string customValidity_;
};

HTMLFieldSetElement::HTMLFieldSetElement()
    : HTMLElement("fieldset")
    , impl_(std::make_unique<Impl>())
{
}

HTMLFieldSetElement::~HTMLFieldSetElement() = default;

bool HTMLFieldSetElement::disabled() const { return impl_->disabled_; }
void HTMLFieldSetElement::setDisabled(bool disabled) {
    impl_->disabled_ = disabled;
    if (disabled) setAttribute("disabled", "");
    else removeAttribute("disabled");
}

std::string HTMLFieldSetElement::name() const { return impl_->name_; }
void HTMLFieldSetElement::setName(const std::string& name) {
    impl_->name_ = name;
    setAttribute("name", name);
}

std::string HTMLFieldSetElement::formId() const { return impl_->formId_; }
void HTMLFieldSetElement::setFormId(const std::string& formId) {
    impl_->formId_ = formId;
    setAttribute("form", formId);
}

HTMLFormElement* HTMLFieldSetElement::form() const {
    return nullptr; // Would be resolved at runtime
}

HTMLLegendElement* HTMLFieldSetElement::legend() const {
    // TODO: DOM traversal not implemented - would find first legend child
    return nullptr;
}

std::vector<HTMLElement*> HTMLFieldSetElement::elements() const {
    std::vector<HTMLElement*> result;
    // Would collect all form elements within
    return result;
}

std::string HTMLFieldSetElement::type() const { return "fieldset"; }

bool HTMLFieldSetElement::checkValidity() {
    return impl_->customValidity_.empty();
}

bool HTMLFieldSetElement::reportValidity() {
    return checkValidity();
}

void HTMLFieldSetElement::setCustomValidity(const std::string& message) {
    impl_->customValidity_ = message;
}

std::string HTMLFieldSetElement::validationMessage() const {
    return impl_->customValidity_;
}

std::unique_ptr<DOMNode> HTMLFieldSetElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLFieldSetElement>();
    clone->setDisabled(impl_->disabled_);
    clone->setName(impl_->name_);
    clone->setFormId(impl_->formId_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLLegendElement
// =============================================================================

class HTMLLegendElement::Impl {
public:
    std::string align_;
};

HTMLLegendElement::HTMLLegendElement()
    : HTMLElement("legend")
    , impl_(std::make_unique<Impl>())
{
}

HTMLLegendElement::~HTMLLegendElement() = default;

HTMLFieldSetElement* HTMLLegendElement::form() const {
    // TODO: DOM traversal not implemented
    HTMLElement* parent = nullptr;
    if (auto* fieldset = dynamic_cast<HTMLFieldSetElement*>(parent)) {
        return fieldset;
    }
    return nullptr;
}

std::string HTMLLegendElement::align() const { return impl_->align_; }
void HTMLLegendElement::setAlign(const std::string& align) {
    impl_->align_ = align;
    setAttribute("align", align);
}

std::unique_ptr<DOMNode> HTMLLegendElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLLegendElement>();
    clone->setAlign(impl_->align_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLProgressElement
// =============================================================================

class HTMLProgressElement::Impl {
public:
    double value_ = -1; // -1 means indeterminate
    double max_ = 1.0;
};

HTMLProgressElement::HTMLProgressElement()
    : HTMLElement("progress")
    , impl_(std::make_unique<Impl>())
{
}

HTMLProgressElement::~HTMLProgressElement() = default;

double HTMLProgressElement::value() const { return impl_->value_; }
void HTMLProgressElement::setValue(double value) {
    impl_->value_ = value;
    setAttribute("value", std::to_string(value));
}

double HTMLProgressElement::max() const { return impl_->max_; }
void HTMLProgressElement::setMax(double max) {
    impl_->max_ = max;
    setAttribute("max", std::to_string(max));
}

double HTMLProgressElement::position() const {
    if (impl_->value_ < 0) return -1;
    return impl_->value_ / impl_->max_;
}

bool HTMLProgressElement::isIndeterminate() const {
    return impl_->value_ < 0;
}

std::vector<HTMLElement*> HTMLProgressElement::labels() const {
    return {}; // Would be computed from DOM
}

std::unique_ptr<DOMNode> HTMLProgressElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLProgressElement>();
    clone->setValue(impl_->value_);
    clone->setMax(impl_->max_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLMeterElement
// =============================================================================

class HTMLMeterElement::Impl {
public:
    double value_ = 0;
    double min_ = 0;
    double max_ = 1;
    double low_ = 0;
    double high_ = 1;
    double optimum_ = 0.5;
};

HTMLMeterElement::HTMLMeterElement()
    : HTMLElement("meter")
    , impl_(std::make_unique<Impl>())
{
}

HTMLMeterElement::~HTMLMeterElement() = default;

double HTMLMeterElement::value() const { return impl_->value_; }
void HTMLMeterElement::setValue(double value) {
    impl_->value_ = value;
    setAttribute("value", std::to_string(value));
}

double HTMLMeterElement::min() const { return impl_->min_; }
void HTMLMeterElement::setMin(double min) {
    impl_->min_ = min;
    setAttribute("min", std::to_string(min));
}

double HTMLMeterElement::max() const { return impl_->max_; }
void HTMLMeterElement::setMax(double max) {
    impl_->max_ = max;
    setAttribute("max", std::to_string(max));
}

double HTMLMeterElement::low() const { return impl_->low_; }
void HTMLMeterElement::setLow(double low) {
    impl_->low_ = low;
    setAttribute("low", std::to_string(low));
}

double HTMLMeterElement::high() const { return impl_->high_; }
void HTMLMeterElement::setHigh(double high) {
    impl_->high_ = high;
    setAttribute("high", std::to_string(high));
}

double HTMLMeterElement::optimum() const { return impl_->optimum_; }
void HTMLMeterElement::setOptimum(double optimum) {
    impl_->optimum_ = optimum;
    setAttribute("optimum", std::to_string(optimum));
}

std::vector<HTMLElement*> HTMLMeterElement::labels() const {
    return {};
}

bool HTMLMeterElement::isLow() const {
    return impl_->value_ < impl_->low_;
}

bool HTMLMeterElement::isHigh() const {
    return impl_->value_ > impl_->high_;
}

bool HTMLMeterElement::isOptimum() const {
    double range = impl_->max_ - impl_->min_;
    double threshold = range * 0.1;
    return std::abs(impl_->value_ - impl_->optimum_) < threshold;
}

std::unique_ptr<DOMNode> HTMLMeterElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLMeterElement>();
    clone->setValue(impl_->value_);
    clone->setMin(impl_->min_);
    clone->setMax(impl_->max_);
    clone->setLow(impl_->low_);
    clone->setHigh(impl_->high_);
    clone->setOptimum(impl_->optimum_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLDataListElement
// =============================================================================

class HTMLDataListElement::Impl {
public:
    // Options are child elements
};

HTMLDataListElement::HTMLDataListElement()
    : HTMLElement("datalist")
    , impl_(std::make_unique<Impl>())
{
}

HTMLDataListElement::~HTMLDataListElement() = default;

HTMLCollection* HTMLDataListElement::options() const {
    return nullptr; // Would return live collection
}

std::vector<HTMLOptionElement*> HTMLDataListElement::optionElements() const {
    // TODO: DOM traversal not implemented - would collect option children
    return {};
}

size_t HTMLDataListElement::length() const {
    return optionElements().size();
}

HTMLOptionElement* HTMLDataListElement::item(size_t index) const {
    auto options = optionElements();
    if (index < options.size()) return options[index];
    return nullptr;
}

HTMLOptionElement* HTMLDataListElement::namedItem(const std::string& name) const {
    for (auto* option : optionElements()) {
        if (option->id() == name || option->getAttribute("name") == name) {
            return option;
        }
    }
    return nullptr;
}

std::vector<std::string> HTMLDataListElement::values() const {
    std::vector<std::string> vals;
    for (auto* option : optionElements()) {
        vals.push_back(option->value());
    }
    return vals;
}

bool HTMLDataListElement::hasValue(const std::string& value) const {
    for (auto* option : optionElements()) {
        if (option->value() == value) return true;
    }
    return false;
}

std::unique_ptr<DOMNode> HTMLDataListElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLDataListElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLOutputElement
// =============================================================================

class HTMLOutputElement::Impl {
public:
    std::string htmlFor_;
    std::string name_;
    std::string formId_;
    std::string defaultValue_;
    std::string customValidity_;
};

HTMLOutputElement::HTMLOutputElement()
    : HTMLElement("output")
    , impl_(std::make_unique<Impl>())
{
}

HTMLOutputElement::~HTMLOutputElement() = default;

std::string HTMLOutputElement::htmlFor() const { return impl_->htmlFor_; }
void HTMLOutputElement::setHtmlFor(const std::string& forValue) {
    impl_->htmlFor_ = forValue;
    setAttribute("for", forValue);
}

std::string HTMLOutputElement::name() const { return impl_->name_; }
void HTMLOutputElement::setName(const std::string& name) {
    impl_->name_ = name;
    setAttribute("name", name);
}

std::string HTMLOutputElement::formId() const { return impl_->formId_; }
void HTMLOutputElement::setFormId(const std::string& formId) {
    impl_->formId_ = formId;
    setAttribute("form", formId);
}

std::string HTMLOutputElement::value() const {
    return textContent();
}

void HTMLOutputElement::setValue(const std::string& value) {
    setTextContent(value);
}

std::string HTMLOutputElement::defaultValue() const { return impl_->defaultValue_; }
void HTMLOutputElement::setDefaultValue(const std::string& value) {
    impl_->defaultValue_ = value;
}

DOMTokenList* HTMLOutputElement::htmlForElements() const {
    return nullptr;
}

HTMLFormElement* HTMLOutputElement::form() const {
    return nullptr;
}

std::vector<HTMLElement*> HTMLOutputElement::labels() const {
    return {};
}

std::string HTMLOutputElement::type() const { return "output"; }
bool HTMLOutputElement::checkValidity() { return true; }
bool HTMLOutputElement::reportValidity() { return true; }
void HTMLOutputElement::setCustomValidity(const std::string& message) {
    impl_->customValidity_ = message;
}
std::string HTMLOutputElement::validationMessage() const { return impl_->customValidity_; }
bool HTMLOutputElement::willValidate() const { return false; }

std::unique_ptr<DOMNode> HTMLOutputElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLOutputElement>();
    clone->setHtmlFor(impl_->htmlFor_);
    clone->setName(impl_->name_);
    clone->setFormId(impl_->formId_);
    clone->setDefaultValue(impl_->defaultValue_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

} // namespace Zepra::WebCore
