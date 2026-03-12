// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_text_elements.cpp
 * @brief Implementation of text content elements: paragraph, break, pre, quote, time
 */

#include "html/html_paragraph_element.hpp"
#include "html/html_break_element.hpp"
#include "html/html_preformatted_element.hpp"
#include "html/html_quote_element.hpp"
#include "html/html_time_element.hpp"

namespace Zepra::WebCore {

// =============================================================================
// HTMLParagraphElement
// =============================================================================

class HTMLParagraphElement::Impl {
public:
    std::string align_;
};

HTMLParagraphElement::HTMLParagraphElement()
    : HTMLElement("p")
    , impl_(std::make_unique<Impl>())
{
}

HTMLParagraphElement::~HTMLParagraphElement() = default;

std::string HTMLParagraphElement::align() const {
    return impl_->align_;
}

void HTMLParagraphElement::setAlign(const std::string& align) {
    impl_->align_ = align;
    setAttribute("align", align);
}

std::unique_ptr<DOMNode> HTMLParagraphElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLParagraphElement>();
    clone->setAlign(impl_->align_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLBRElement
// =============================================================================

class HTMLBRElement::Impl {
public:
    std::string clear_;
};

HTMLBRElement::HTMLBRElement()
    : HTMLElement("br")
    , impl_(std::make_unique<Impl>())
{
}

HTMLBRElement::~HTMLBRElement() = default;

std::string HTMLBRElement::clear() const {
    return impl_->clear_;
}

void HTMLBRElement::setClear(const std::string& clear) {
    impl_->clear_ = clear;
    setAttribute("clear", clear);
}

std::unique_ptr<DOMNode> HTMLBRElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLBRElement>();
    clone->setClear(impl_->clear_);
    return clone;
}

// =============================================================================
// HTMLHRElement
// =============================================================================

class HTMLHRElement::Impl {
public:
    std::string align_;
    std::string color_;
    bool noShade_ = false;
    std::string size_;
    std::string width_;
};

HTMLHRElement::HTMLHRElement()
    : HTMLElement("hr")
    , impl_(std::make_unique<Impl>())
{
}

HTMLHRElement::~HTMLHRElement() = default;

std::string HTMLHRElement::align() const { return impl_->align_; }
void HTMLHRElement::setAlign(const std::string& align) {
    impl_->align_ = align;
    setAttribute("align", align);
}

std::string HTMLHRElement::color() const { return impl_->color_; }
void HTMLHRElement::setColor(const std::string& color) {
    impl_->color_ = color;
    setAttribute("color", color);
}

bool HTMLHRElement::noShade() const { return impl_->noShade_; }
void HTMLHRElement::setNoShade(bool noShade) {
    impl_->noShade_ = noShade;
    if (noShade) {
        setAttribute("noshade", "");
    } else {
        removeAttribute("noshade");
    }
}

std::string HTMLHRElement::size() const { return impl_->size_; }
void HTMLHRElement::setSize(const std::string& size) {
    impl_->size_ = size;
    setAttribute("size", size);
}

std::string HTMLHRElement::width() const { return impl_->width_; }
void HTMLHRElement::setWidth(const std::string& width) {
    impl_->width_ = width;
    setAttribute("width", width);
}

std::unique_ptr<DOMNode> HTMLHRElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLHRElement>();
    clone->setAlign(impl_->align_);
    clone->setColor(impl_->color_);
    clone->setNoShade(impl_->noShade_);
    clone->setSize(impl_->size_);
    clone->setWidth(impl_->width_);
    return clone;
}

// =============================================================================
// HTMLWBRElement
// =============================================================================

class HTMLWBRElement::Impl {
public:
    // No additional state
};

HTMLWBRElement::HTMLWBRElement()
    : HTMLElement("wbr")
    , impl_(std::make_unique<Impl>())
{
}

HTMLWBRElement::~HTMLWBRElement() = default;

std::unique_ptr<DOMNode> HTMLWBRElement::cloneNode(bool deep) const {
    return std::make_unique<HTMLWBRElement>();
}

// =============================================================================
// HTMLPreElement
// =============================================================================

class HTMLPreElement::Impl {
public:
    int width_ = 0;
};

HTMLPreElement::HTMLPreElement()
    : HTMLElement("pre")
    , impl_(std::make_unique<Impl>())
{
}

HTMLPreElement::~HTMLPreElement() = default;

int HTMLPreElement::width() const { return impl_->width_; }
void HTMLPreElement::setWidth(int width) {
    impl_->width_ = width;
    setAttribute("width", std::to_string(width));
}

std::unique_ptr<DOMNode> HTMLPreElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLPreElement>();
    clone->setWidth(impl_->width_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLCodeElement
// =============================================================================

class HTMLCodeElement::Impl {};

HTMLCodeElement::HTMLCodeElement() : HTMLElement("code"), impl_(std::make_unique<Impl>()) {}
HTMLCodeElement::~HTMLCodeElement() = default;

std::unique_ptr<DOMNode> HTMLCodeElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLCodeElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLKbdElement
// =============================================================================

class HTMLKbdElement::Impl {};

HTMLKbdElement::HTMLKbdElement() : HTMLElement("kbd"), impl_(std::make_unique<Impl>()) {}
HTMLKbdElement::~HTMLKbdElement() = default;

std::unique_ptr<DOMNode> HTMLKbdElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLKbdElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLSampElement
// =============================================================================

class HTMLSampElement::Impl {};

HTMLSampElement::HTMLSampElement() : HTMLElement("samp"), impl_(std::make_unique<Impl>()) {}
HTMLSampElement::~HTMLSampElement() = default;

std::unique_ptr<DOMNode> HTMLSampElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSampElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLVarElement
// =============================================================================

class HTMLVarElement::Impl {};

HTMLVarElement::HTMLVarElement() : HTMLElement("var"), impl_(std::make_unique<Impl>()) {}
HTMLVarElement::~HTMLVarElement() = default;

std::unique_ptr<DOMNode> HTMLVarElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLVarElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLQuoteElement
// =============================================================================

class HTMLQuoteElement::Impl {
public:
    std::string cite_;
};

HTMLQuoteElement::HTMLQuoteElement(const std::string& tagName)
    : HTMLElement(tagName)
    , impl_(std::make_unique<Impl>())
{
}

HTMLQuoteElement::~HTMLQuoteElement() = default;

std::string HTMLQuoteElement::cite() const { return impl_->cite_; }
void HTMLQuoteElement::setCite(const std::string& cite) {
    impl_->cite_ = cite;
    setAttribute("cite", cite);
}

std::unique_ptr<DOMNode> HTMLQuoteElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLQuoteElement>(tagName());
    clone->setCite(impl_->cite_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

HTMLBlockQuoteElement::HTMLBlockQuoteElement() : HTMLQuoteElement("blockquote") {}
HTMLQElement::HTMLQElement() : HTMLQuoteElement("q") {}

// =============================================================================
// HTMLCiteElement
// =============================================================================

class HTMLCiteElement::Impl {};

HTMLCiteElement::HTMLCiteElement() : HTMLElement("cite"), impl_(std::make_unique<Impl>()) {}
HTMLCiteElement::~HTMLCiteElement() = default;

std::unique_ptr<DOMNode> HTMLCiteElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLCiteElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLTimeElement
// =============================================================================

class HTMLTimeElement::Impl {
public:
    std::string dateTime_;
};

HTMLTimeElement::HTMLTimeElement()
    : HTMLElement("time")
    , impl_(std::make_unique<Impl>())
{
}

HTMLTimeElement::~HTMLTimeElement() = default;

std::string HTMLTimeElement::dateTime() const { return impl_->dateTime_; }
void HTMLTimeElement::setDateTime(const std::string& datetime) {
    impl_->dateTime_ = datetime;
    setAttribute("datetime", datetime);
}

std::string HTMLTimeElement::effectiveDateTime() const {
    if (!impl_->dateTime_.empty()) {
        return impl_->dateTime_;
    }
    return textContent();
}

std::optional<std::chrono::system_clock::time_point> HTMLTimeElement::asDate() const {
    // Date parsing would be implemented here
    return std::nullopt;
}

std::optional<std::chrono::seconds> HTMLTimeElement::asTime() const {
    // Time parsing would be implemented here
    return std::nullopt;
}

std::optional<std::chrono::system_clock::time_point> HTMLTimeElement::asDateTime() const {
    // DateTime parsing would be implemented here
    return std::nullopt;
}

std::optional<std::chrono::seconds> HTMLTimeElement::asDuration() const {
    // ISO 8601 duration parsing would be implemented here
    return std::nullopt;
}

bool HTMLTimeElement::isDate() const {
    std::string dt = effectiveDateTime();
    // Simple check: YYYY-MM-DD format
    return dt.length() == 10 && dt[4] == '-' && dt[7] == '-';
}

bool HTMLTimeElement::isTime() const {
    std::string dt = effectiveDateTime();
    // Simple check: HH:MM or HH:MM:SS
    return (dt.length() == 5 || dt.length() == 8) && dt[2] == ':';
}

bool HTMLTimeElement::isDateTime() const {
    std::string dt = effectiveDateTime();
    // Contains both date and time
    return dt.find('T') != std::string::npos;
}

bool HTMLTimeElement::isDuration() const {
    std::string dt = effectiveDateTime();
    // ISO 8601 duration starts with P
    return !dt.empty() && dt[0] == 'P';
}

std::unique_ptr<DOMNode> HTMLTimeElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLTimeElement>();
    clone->setDateTime(impl_->dateTime_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

} // namespace Zepra::WebCore
