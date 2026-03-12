// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_meta_element.cpp
 * @brief HTMLMetaElement implementation
 */

#include "html/html_meta_element.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

namespace Zepra::WebCore {

// =============================================================================
// Implementation class
// =============================================================================

class HTMLMetaElement::Impl {
public:
    std::string name_;
    std::string content_;
    std::string httpEquiv_;
    std::string charset_;
    std::string media_;
    
    MetaName parseNameType() const {
        std::string lowerName = name_;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName == "application-name") return MetaName::Application;
        if (lowerName == "author") return MetaName::Author;
        if (lowerName == "description") return MetaName::Description;
        if (lowerName == "generator") return MetaName::Generator;
        if (lowerName == "keywords") return MetaName::Keywords;
        if (lowerName == "referrer") return MetaName::Referrer;
        if (lowerName == "theme-color") return MetaName::ThemeColor;
        if (lowerName == "color-scheme") return MetaName::ColorScheme;
        if (lowerName == "viewport") return MetaName::Viewport;
        if (lowerName == "robots") return MetaName::Robots;
        return MetaName::Other;
    }
};

// =============================================================================
// Constructor / Destructor
// =============================================================================

HTMLMetaElement::HTMLMetaElement()
    : HTMLElement("meta")
    , impl_(std::make_unique<Impl>())
{
}

HTMLMetaElement::~HTMLMetaElement() = default;

// =============================================================================
// Core Attributes
// =============================================================================

std::string HTMLMetaElement::name() const {
    return impl_->name_;
}

void HTMLMetaElement::setName(const std::string& name) {
    impl_->name_ = name;
    setAttribute("name", name);
}

std::string HTMLMetaElement::content() const {
    return impl_->content_;
}

void HTMLMetaElement::setContent(const std::string& content) {
    impl_->content_ = content;
    setAttribute("content", content);
}

std::string HTMLMetaElement::httpEquiv() const {
    return impl_->httpEquiv_;
}

void HTMLMetaElement::setHttpEquiv(const std::string& equiv) {
    impl_->httpEquiv_ = equiv;
    setAttribute("http-equiv", equiv);
}

std::string HTMLMetaElement::charset() const {
    return impl_->charset_;
}

void HTMLMetaElement::setCharset(const std::string& charset) {
    impl_->charset_ = charset;
    setAttribute("charset", charset);
}

std::string HTMLMetaElement::media() const {
    return impl_->media_;
}

void HTMLMetaElement::setMedia(const std::string& media) {
    impl_->media_ = media;
    setAttribute("media", media);
}

// =============================================================================
// Convenience Methods
// =============================================================================

MetaName HTMLMetaElement::nameType() const {
    return impl_->parseNameType();
}

bool HTMLMetaElement::isCharset() const {
    return !impl_->charset_.empty();
}

bool HTMLMetaElement::isViewport() const {
    return nameType() == MetaName::Viewport;
}

bool HTMLMetaElement::isHttpEquiv() const {
    return !impl_->httpEquiv_.empty();
}

ViewportSettings HTMLMetaElement::parseViewport() const {
    ViewportSettings settings;
    
    if (!isViewport()) {
        return settings;
    }
    
    std::string content = impl_->content_;
    
    // Parse key=value pairs separated by comma or semicolon
    std::regex pairRegex(R"((\w+)\s*=\s*([^,;]+))");
    std::sregex_iterator it(content.begin(), content.end(), pairRegex);
    std::sregex_iterator end;
    
    while (it != end) {
        std::string key = (*it)[1].str();
        std::string value = (*it)[2].str();
        
        // Trim whitespace
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        
        if (key == "width") {
            if (value == "device-width") {
                settings.widthDeviceWidth = true;
            } else {
                try {
                    settings.width = std::stof(value);
                } catch (...) {}
            }
        } else if (key == "height") {
            if (value == "device-height") {
                settings.heightDeviceHeight = true;
            } else {
                try {
                    settings.height = std::stof(value);
                } catch (...) {}
            }
        } else if (key == "initial-scale") {
            try {
                settings.initialScale = std::stof(value);
            } catch (...) {}
        } else if (key == "minimum-scale") {
            try {
                settings.minimumScale = std::stof(value);
            } catch (...) {}
        } else if (key == "maximum-scale") {
            try {
                settings.maximumScale = std::stof(value);
            } catch (...) {}
        } else if (key == "user-scalable") {
            settings.userScalable = (value != "no" && value != "0");
        }
        
        ++it;
    }
    
    return settings;
}

std::string HTMLMetaElement::themeColor() const {
    if (nameType() == MetaName::ThemeColor) {
        return impl_->content_;
    }
    return "";
}

std::string HTMLMetaElement::colorScheme() const {
    if (nameType() == MetaName::ColorScheme) {
        return impl_->content_;
    }
    return "";
}

// =============================================================================
// HTTP-Equiv Actions
// =============================================================================

bool HTMLMetaElement::isRefresh() const {
    std::string equiv = impl_->httpEquiv_;
    std::transform(equiv.begin(), equiv.end(), equiv.begin(), ::tolower);
    return equiv == "refresh";
}

int HTMLMetaElement::refreshTimeout() const {
    if (!isRefresh()) {
        return -1;
    }
    
    std::string content = impl_->content_;
    size_t pos = content.find_first_of(";,");
    std::string timeStr = (pos != std::string::npos) ? content.substr(0, pos) : content;
    
    try {
        return std::stoi(timeStr);
    } catch (...) {
        return 0;
    }
}

std::string HTMLMetaElement::refreshUrl() const {
    if (!isRefresh()) {
        return "";
    }
    
    std::string content = impl_->content_;
    size_t pos = content.find_first_of(";,");
    if (pos == std::string::npos) {
        return "";
    }
    
    std::string rest = content.substr(pos + 1);
    
    // Look for url=
    std::regex urlRegex(R"(url\s*=\s*['\"]?([^'\"]+)['\"]?)", std::regex::icase);
    std::smatch match;
    if (std::regex_search(rest, match, urlRegex)) {
        return match[1].str();
    }
    
    // Trim and return the rest
    rest.erase(0, rest.find_first_not_of(" \t"));
    return rest;
}

bool HTMLMetaElement::isCSP() const {
    std::string equiv = impl_->httpEquiv_;
    std::transform(equiv.begin(), equiv.end(), equiv.begin(), ::tolower);
    return equiv == "content-security-policy" || equiv == "content-security-policy-report-only";
}

std::string HTMLMetaElement::cspPolicy() const {
    if (isCSP()) {
        return impl_->content_;
    }
    return "";
}

// =============================================================================
// Clone
// =============================================================================

std::unique_ptr<DOMNode> HTMLMetaElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLMetaElement>();
    clone->setName(impl_->name_);
    clone->setContent(impl_->content_);
    clone->setHttpEquiv(impl_->httpEquiv_);
    clone->setCharset(impl_->charset_);
    clone->setMedia(impl_->media_);
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

} // namespace Zepra::WebCore
