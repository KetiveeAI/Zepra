// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_text_formatting.cpp
 * @brief Implementation of text formatting elements
 */

#include "html/html_text_formatting.hpp"

namespace Zepra::WebCore {

// =============================================================================
// HTMLStrongElement
// =============================================================================

class HTMLStrongElement::Impl {};

HTMLStrongElement::HTMLStrongElement() : HTMLElement("strong"), impl_(std::make_unique<Impl>()) {}
HTMLStrongElement::~HTMLStrongElement() = default;

std::unique_ptr<DOMNode> HTMLStrongElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLStrongElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLEmElement
// =============================================================================

class HTMLEmElement::Impl {};

HTMLEmElement::HTMLEmElement() : HTMLElement("em"), impl_(std::make_unique<Impl>()) {}
HTMLEmElement::~HTMLEmElement() = default;

std::unique_ptr<DOMNode> HTMLEmElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLEmElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLBElement
// =============================================================================

class HTMLBElement::Impl {};

HTMLBElement::HTMLBElement() : HTMLElement("b"), impl_(std::make_unique<Impl>()) {}
HTMLBElement::~HTMLBElement() = default;

std::unique_ptr<DOMNode> HTMLBElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLBElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLIElement
// =============================================================================

class HTMLIElement::Impl {};

HTMLIElement::HTMLIElement() : HTMLElement("i"), impl_(std::make_unique<Impl>()) {}
HTMLIElement::~HTMLIElement() = default;

std::unique_ptr<DOMNode> HTMLIElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLIElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLUElement
// =============================================================================

class HTMLUElement::Impl {};

HTMLUElement::HTMLUElement() : HTMLElement("u"), impl_(std::make_unique<Impl>()) {}
HTMLUElement::~HTMLUElement() = default;

std::unique_ptr<DOMNode> HTMLUElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLUElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLSElement
// =============================================================================

class HTMLSElement::Impl {};

HTMLSElement::HTMLSElement() : HTMLElement("s"), impl_(std::make_unique<Impl>()) {}
HTMLSElement::~HTMLSElement() = default;

std::unique_ptr<DOMNode> HTMLSElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLMarkElement
// =============================================================================

class HTMLMarkElement::Impl {};

HTMLMarkElement::HTMLMarkElement() : HTMLElement("mark"), impl_(std::make_unique<Impl>()) {}
HTMLMarkElement::~HTMLMarkElement() = default;

std::unique_ptr<DOMNode> HTMLMarkElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLMarkElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLSmallElement
// =============================================================================

class HTMLSmallElement::Impl {};

HTMLSmallElement::HTMLSmallElement() : HTMLElement("small"), impl_(std::make_unique<Impl>()) {}
HTMLSmallElement::~HTMLSmallElement() = default;

std::unique_ptr<DOMNode> HTMLSmallElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSmallElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLSubElement
// =============================================================================

class HTMLSubElement::Impl {};

HTMLSubElement::HTMLSubElement() : HTMLElement("sub"), impl_(std::make_unique<Impl>()) {}
HTMLSubElement::~HTMLSubElement() = default;

std::unique_ptr<DOMNode> HTMLSubElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSubElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLSupElement
// =============================================================================

class HTMLSupElement::Impl {};

HTMLSupElement::HTMLSupElement() : HTMLElement("sup"), impl_(std::make_unique<Impl>()) {}
HTMLSupElement::~HTMLSupElement() = default;

std::unique_ptr<DOMNode> HTMLSupElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSupElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLAbbrElement
// =============================================================================

class HTMLAbbrElement::Impl {};

HTMLAbbrElement::HTMLAbbrElement() : HTMLElement("abbr"), impl_(std::make_unique<Impl>()) {}
HTMLAbbrElement::~HTMLAbbrElement() = default;

std::unique_ptr<DOMNode> HTMLAbbrElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLAbbrElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLDfnElement
// =============================================================================

class HTMLDfnElement::Impl {};

HTMLDfnElement::HTMLDfnElement() : HTMLElement("dfn"), impl_(std::make_unique<Impl>()) {}
HTMLDfnElement::~HTMLDfnElement() = default;

std::unique_ptr<DOMNode> HTMLDfnElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLDfnElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLAddressElement
// =============================================================================

class HTMLAddressElement::Impl {};

HTMLAddressElement::HTMLAddressElement() : HTMLElement("address"), impl_(std::make_unique<Impl>()) {}
HTMLAddressElement::~HTMLAddressElement() = default;

std::unique_ptr<DOMNode> HTMLAddressElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLAddressElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLDelElement
// =============================================================================

class HTMLDelElement::Impl {
public:
    std::string cite_;
    std::string dateTime_;
};

HTMLDelElement::HTMLDelElement() : HTMLElement("del"), impl_(std::make_unique<Impl>()) {}
HTMLDelElement::~HTMLDelElement() = default;

std::string HTMLDelElement::cite() const { return impl_->cite_; }
void HTMLDelElement::setCite(const std::string& cite) {
    impl_->cite_ = cite;
    setAttribute("cite", cite);
}

std::string HTMLDelElement::dateTime() const { return impl_->dateTime_; }
void HTMLDelElement::setDateTime(const std::string& datetime) {
    impl_->dateTime_ = datetime;
    setAttribute("datetime", datetime);
}

std::unique_ptr<DOMNode> HTMLDelElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLDelElement>();
    clone->setCite(impl_->cite_);
    clone->setDateTime(impl_->dateTime_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLInsElement
// =============================================================================

class HTMLInsElement::Impl {
public:
    std::string cite_;
    std::string dateTime_;
};

HTMLInsElement::HTMLInsElement() : HTMLElement("ins"), impl_(std::make_unique<Impl>()) {}
HTMLInsElement::~HTMLInsElement() = default;

std::string HTMLInsElement::cite() const { return impl_->cite_; }
void HTMLInsElement::setCite(const std::string& cite) {
    impl_->cite_ = cite;
    setAttribute("cite", cite);
}

std::string HTMLInsElement::dateTime() const { return impl_->dateTime_; }
void HTMLInsElement::setDateTime(const std::string& datetime) {
    impl_->dateTime_ = datetime;
    setAttribute("datetime", datetime);
}

std::unique_ptr<DOMNode> HTMLInsElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLInsElement>();
    clone->setCite(impl_->cite_);
    clone->setDateTime(impl_->dateTime_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

} // namespace Zepra::WebCore
