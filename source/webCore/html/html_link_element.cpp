// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_link_element.cpp
 * @brief HTMLLinkElement implementation
 */

#include "html/html_link_element.hpp"
#include <sstream>
#include <algorithm>

namespace Zepra::WebCore {

// =============================================================================
// Implementation class
// =============================================================================

class HTMLLinkElement::Impl {
public:
    std::string href_;
    std::string rel_;
    std::string type_;
    std::string media_;
    std::string sizes_;
    std::string imagesrcset_;
    std::string imagesizes_;
    std::string crossOrigin_;
    std::string as_;
    std::string referrerPolicy_;
    std::string integrity_;
    std::string hreflang_;
    std::string fetchpriority_;
    std::string blocking_;
    bool disabled_ = false;
    
    LinkLoadState loadState_ = LinkLoadState::Idle;
    CSSStyleSheet* sheet_ = nullptr;
    
    LoadCallback onLoad_;
    ErrorCallback onError_;
    
    std::vector<std::string> relTokens_;
    
    void parseRelTokens() {
        relTokens_.clear();
        std::istringstream iss(rel_);
        std::string token;
        while (iss >> token) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            relTokens_.push_back(token);
        }
    }
    
    bool relContains(const std::string& token) const {
        std::string lowerToken = token;
        std::transform(lowerToken.begin(), lowerToken.end(), lowerToken.begin(), ::tolower);
        return std::find(relTokens_.begin(), relTokens_.end(), lowerToken) != relTokens_.end();
    }
};

// =============================================================================
// Constructor / Destructor
// =============================================================================

HTMLLinkElement::HTMLLinkElement()
    : HTMLElement("link")
    , impl_(std::make_unique<Impl>())
{
}

HTMLLinkElement::~HTMLLinkElement() = default;

// =============================================================================
// Core Attributes
// =============================================================================

std::string HTMLLinkElement::href() const {
    return impl_->href_;
}

void HTMLLinkElement::setHref(const std::string& href) {
    impl_->href_ = href;
    setAttribute("href", href);
}

std::string HTMLLinkElement::rel() const {
    return impl_->rel_;
}

void HTMLLinkElement::setRel(const std::string& rel) {
    impl_->rel_ = rel;
    impl_->parseRelTokens();
    setAttribute("rel", rel);
}

std::string HTMLLinkElement::type() const {
    return impl_->type_;
}

void HTMLLinkElement::setType(const std::string& type) {
    impl_->type_ = type;
    setAttribute("type", type);
}

std::string HTMLLinkElement::media() const {
    return impl_->media_;
}

void HTMLLinkElement::setMedia(const std::string& media) {
    impl_->media_ = media;
    setAttribute("media", media);
}

std::string HTMLLinkElement::sizes() const {
    return impl_->sizes_;
}

void HTMLLinkElement::setSizes(const std::string& sizes) {
    impl_->sizes_ = sizes;
    setAttribute("sizes", sizes);
}

std::string HTMLLinkElement::imagesrcset() const {
    return impl_->imagesrcset_;
}

void HTMLLinkElement::setImagesrcset(const std::string& srcset) {
    impl_->imagesrcset_ = srcset;
    setAttribute("imagesrcset", srcset);
}

std::string HTMLLinkElement::imagesizes() const {
    return impl_->imagesizes_;
}

void HTMLLinkElement::setImagesizes(const std::string& sizes) {
    impl_->imagesizes_ = sizes;
    setAttribute("imagesizes", sizes);
}

// =============================================================================
// Loading Attributes
// =============================================================================

std::string HTMLLinkElement::crossOrigin() const {
    return impl_->crossOrigin_;
}

void HTMLLinkElement::setCrossOrigin(const std::string& value) {
    impl_->crossOrigin_ = value;
    setAttribute("crossorigin", value);
}

std::string HTMLLinkElement::as() const {
    return impl_->as_;
}

void HTMLLinkElement::setAs(const std::string& value) {
    impl_->as_ = value;
    setAttribute("as", value);
}

std::string HTMLLinkElement::referrerPolicy() const {
    return impl_->referrerPolicy_;
}

void HTMLLinkElement::setReferrerPolicy(const std::string& policy) {
    impl_->referrerPolicy_ = policy;
    setAttribute("referrerpolicy", policy);
}

std::string HTMLLinkElement::integrity() const {
    return impl_->integrity_;
}

void HTMLLinkElement::setIntegrity(const std::string& hash) {
    impl_->integrity_ = hash;
    setAttribute("integrity", hash);
}

std::string HTMLLinkElement::hreflang() const {
    return impl_->hreflang_;
}

void HTMLLinkElement::setHreflang(const std::string& lang) {
    impl_->hreflang_ = lang;
    setAttribute("hreflang", lang);
}

std::string HTMLLinkElement::fetchpriority() const {
    return impl_->fetchpriority_;
}

void HTMLLinkElement::setFetchpriority(const std::string& priority) {
    impl_->fetchpriority_ = priority;
    setAttribute("fetchpriority", priority);
}

std::string HTMLLinkElement::blocking() const {
    return impl_->blocking_;
}

void HTMLLinkElement::setBlocking(const std::string& value) {
    impl_->blocking_ = value;
    setAttribute("blocking", value);
}

bool HTMLLinkElement::disabled() const {
    return impl_->disabled_;
}

void HTMLLinkElement::setDisabled(bool disabled) {
    impl_->disabled_ = disabled;
    if (disabled) {
        setAttribute("disabled", "");
    } else {
        removeAttribute("disabled");
    }
}

// =============================================================================
// Resource Loading
// =============================================================================

LinkLoadState HTMLLinkElement::loadState() const {
    return impl_->loadState_;
}

bool HTMLLinkElement::isLoaded() const {
    return impl_->loadState_ == LinkLoadState::Loaded;
}

CSSStyleSheet* HTMLLinkElement::sheet() const {
    return impl_->sheet_;
}

void HTMLLinkElement::load() {
    if (impl_->href_.empty()) {
        return;
    }
    
    impl_->loadState_ = LinkLoadState::Loading;
    
    // Loading would be handled by the document's resource loader
    // For now, mark as loaded
    impl_->loadState_ = LinkLoadState::Loaded;
    
    if (impl_->onLoad_) {
        impl_->onLoad_();
    }
}

void HTMLLinkElement::abort() {
    if (impl_->loadState_ == LinkLoadState::Loading) {
        impl_->loadState_ = LinkLoadState::Idle;
    }
}

// =============================================================================
// Rel List
// =============================================================================

bool HTMLLinkElement::relContains(const std::string& token) const {
    return impl_->relContains(token);
}

void HTMLLinkElement::relAdd(const std::string& token) {
    if (!relContains(token)) {
        std::string newRel = impl_->rel_;
        if (!newRel.empty()) {
            newRel += " ";
        }
        newRel += token;
        setRel(newRel);
    }
}

void HTMLLinkElement::relRemove(const std::string& token) {
    std::string lowerToken = token;
    std::transform(lowerToken.begin(), lowerToken.end(), lowerToken.begin(), ::tolower);
    
    auto it = std::find(impl_->relTokens_.begin(), impl_->relTokens_.end(), lowerToken);
    if (it != impl_->relTokens_.end()) {
        impl_->relTokens_.erase(it);
        
        std::string newRel;
        for (const auto& t : impl_->relTokens_) {
            if (!newRel.empty()) {
                newRel += " ";
            }
            newRel += t;
        }
        impl_->rel_ = newRel;
        setAttribute("rel", newRel);
    }
}

bool HTMLLinkElement::relToggle(const std::string& token) {
    if (relContains(token)) {
        relRemove(token);
        return false;
    } else {
        relAdd(token);
        return true;
    }
}

// =============================================================================
// Convenience Methods
// =============================================================================

bool HTMLLinkElement::isStylesheet() const {
    return impl_->relContains("stylesheet");
}

bool HTMLLinkElement::isIcon() const {
    return impl_->relContains("icon");
}

bool HTMLLinkElement::isPreload() const {
    return impl_->relContains("preload") || impl_->relContains("prefetch");
}

LinkRelType HTMLLinkElement::relType() const {
    if (impl_->relContains("stylesheet")) return LinkRelType::Stylesheet;
    if (impl_->relContains("icon")) return LinkRelType::Icon;
    if (impl_->relContains("preload")) return LinkRelType::Preload;
    if (impl_->relContains("prefetch")) return LinkRelType::Prefetch;
    if (impl_->relContains("preconnect")) return LinkRelType::Preconnect;
    if (impl_->relContains("dns-prefetch")) return LinkRelType::DnsPrefetch;
    if (impl_->relContains("modulepreload")) return LinkRelType::Modulepreload;
    if (impl_->relContains("manifest")) return LinkRelType::Manifest;
    if (impl_->relContains("alternate")) return LinkRelType::Alternate;
    if (impl_->relContains("canonical")) return LinkRelType::Canonical;
    if (impl_->relContains("author")) return LinkRelType::Author;
    if (impl_->relContains("help")) return LinkRelType::Help;
    if (impl_->relContains("license")) return LinkRelType::License;
    if (impl_->relContains("next")) return LinkRelType::Next;
    if (impl_->relContains("prev")) return LinkRelType::Prev;
    if (impl_->relContains("search")) return LinkRelType::Search;
    return LinkRelType::Other;
}

// =============================================================================
// Event Handlers
// =============================================================================

void HTMLLinkElement::setOnLoad(LoadCallback callback) {
    impl_->onLoad_ = std::move(callback);
}

void HTMLLinkElement::setOnError(ErrorCallback callback) {
    impl_->onError_ = std::move(callback);
}

// =============================================================================
// Clone
// =============================================================================

std::unique_ptr<DOMNode> HTMLLinkElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLLinkElement>();
    clone->setHref(impl_->href_);
    clone->setRel(impl_->rel_);
    clone->setType(impl_->type_);
    clone->setMedia(impl_->media_);
    clone->setSizes(impl_->sizes_);
    clone->setCrossOrigin(impl_->crossOrigin_);
    clone->setAs(impl_->as_);
    clone->setReferrerPolicy(impl_->referrerPolicy_);
    clone->setIntegrity(impl_->integrity_);
    clone->setHreflang(impl_->hreflang_);
    clone->setFetchpriority(impl_->fetchpriority_);
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
