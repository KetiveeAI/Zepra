// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_style_element.cpp
 * @brief HTMLStyleElement implementation
 */

#include "html/html_style_element.hpp"

namespace Zepra::WebCore {

// =============================================================================
// Implementation class
// =============================================================================

class HTMLStyleElement::Impl {
public:
    std::string media_;
    std::string type_ = "text/css";
    std::string blocking_;
    bool disabled_ = false;
    
    CSSStyleSheet* sheet_ = nullptr;
    bool ready_ = false;
    std::vector<std::string> parseErrors_;
    
    std::string cssText_;
};

// =============================================================================
// Constructor / Destructor
// =============================================================================

HTMLStyleElement::HTMLStyleElement()
    : HTMLElement("style")
    , impl_(std::make_unique<Impl>())
{
}

HTMLStyleElement::~HTMLStyleElement() = default;

// =============================================================================
// Core Attributes
// =============================================================================

std::string HTMLStyleElement::media() const {
    return impl_->media_;
}

void HTMLStyleElement::setMedia(const std::string& media) {
    impl_->media_ = media;
    setAttribute("media", media);
}

std::string HTMLStyleElement::type() const {
    return impl_->type_;
}

void HTMLStyleElement::setType(const std::string& type) {
    impl_->type_ = type;
    setAttribute("type", type);
}

std::string HTMLStyleElement::blocking() const {
    return impl_->blocking_;
}

void HTMLStyleElement::setBlocking(const std::string& value) {
    impl_->blocking_ = value;
    setAttribute("blocking", value);
}

bool HTMLStyleElement::disabled() const {
    return impl_->disabled_;
}

void HTMLStyleElement::setDisabled(bool disabled) {
    impl_->disabled_ = disabled;
    if (disabled) {
        setAttribute("disabled", "");
    } else {
        removeAttribute("disabled");
    }
}

// =============================================================================
// Stylesheet Access
// =============================================================================

CSSStyleSheet* HTMLStyleElement::sheet() const {
    return impl_->sheet_;
}

std::string HTMLStyleElement::cssText() const {
    return textContent();
}

void HTMLStyleElement::setCssText(const std::string& css) {
    setTextContent(css);
    reparse();
}

// =============================================================================
// State
// =============================================================================

bool HTMLStyleElement::isReady() const {
    return impl_->ready_;
}

bool HTMLStyleElement::isApplicable() const {
    if (impl_->disabled_) {
        return false;
    }
    
    // Check type
    if (!impl_->type_.empty() && impl_->type_ != "text/css") {
        return false;
    }
    
    // Check media query (simplified - always applicable if no media or "all")
    if (impl_->media_.empty() || impl_->media_ == "all") {
        return true;
    }
    
    return mediaMatches();
}

bool HTMLStyleElement::mediaMatches() const {
    // Media query matching would be handled by the CSS engine
    // For now, return true for common values
    if (impl_->media_.empty() || impl_->media_ == "all" || impl_->media_ == "screen") {
        return true;
    }
    return false;
}

// =============================================================================
// Parsing
// =============================================================================

void HTMLStyleElement::reparse() {
    impl_->parseErrors_.clear();
    impl_->ready_ = false;
    
    std::string css = textContent();
    
    // CSS parsing would be handled by the CSS parser
    // For now, mark as ready
    impl_->ready_ = true;
}

std::vector<std::string> HTMLStyleElement::parseErrors() const {
    return impl_->parseErrors_;
}

// =============================================================================
// Clone
// =============================================================================

std::unique_ptr<DOMNode> HTMLStyleElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLStyleElement>();
    clone->setMedia(impl_->media_);
    clone->setType(impl_->type_);
    clone->setBlocking(impl_->blocking_);
    clone->setDisabled(impl_->disabled_);
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

} // namespace Zepra::WebCore
