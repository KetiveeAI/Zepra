// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_structural_elements.cpp
 * @brief Implementation of structural HTML elements: head, body, title, base
 */

#include "html/html_head_element.hpp"
#include "html/html_body_element.hpp"
#include "html/html_title_element.hpp"
#include "html/html_base_element.hpp"
#include "html/html_link_element.hpp"
#include "html/html_meta_element.hpp"
#include "html/html_style_element.hpp"
#include "html/html_script_element.hpp"

namespace Zepra::WebCore {

// =============================================================================
// HTMLHeadElement
// =============================================================================

class HTMLHeadElement::Impl {
public:
    // Head element properties
};

HTMLHeadElement::HTMLHeadElement()
    : HTMLElement("head")
    , impl_(std::make_unique<Impl>())
{
}

HTMLHeadElement::~HTMLHeadElement() = default;

HTMLTitleElement* HTMLHeadElement::titleElement() const {
    // Would traverse children when DOM is fully implemented
    return nullptr;
}

HTMLBaseElement* HTMLHeadElement::baseElement() const {
    return nullptr;
}

std::vector<HTMLMetaElement*> HTMLHeadElement::metaElements() const {
    return {};
}

std::vector<HTMLLinkElement*> HTMLHeadElement::linkElements() const {
    return {};
}

std::vector<HTMLStyleElement*> HTMLHeadElement::styleElements() const {
    return {};
}

std::vector<HTMLScriptElement*> HTMLHeadElement::scriptElements() const {
    return {};
}

HTMLMetaElement* HTMLHeadElement::getMetaByName(const std::string& name) const {
    for (auto* meta : metaElements()) {
        if (meta->name() == name) {
            return meta;
        }
    }
    return nullptr;
}

HTMLMetaElement* HTMLHeadElement::getMetaByEquiv(const std::string& equiv) const {
    for (auto* meta : metaElements()) {
        if (meta->httpEquiv() == equiv) {
            return meta;
        }
    }
    return nullptr;
}

std::vector<HTMLLinkElement*> HTMLHeadElement::stylesheets() const {
    std::vector<HTMLLinkElement*> sheets;
    for (auto* link : linkElements()) {
        if (link->relContains("stylesheet")) {
            sheets.push_back(link);
        }
    }
    return sheets;
}

std::string HTMLHeadElement::charset() const {
    for (auto* meta : metaElements()) {
        std::string cs = meta->charset();
        if (!cs.empty()) {
            return cs;
        }
    }
    return "";
}

std::string HTMLHeadElement::viewport() const {
    if (auto* meta = getMetaByName("viewport")) {
        return meta->content();
    }
    return "";
}

std::string HTMLHeadElement::titleText() const {
    if (auto* title = titleElement()) {
        return title->text();
    }
    return "";
}

std::unique_ptr<DOMNode> HTMLHeadElement::cloneNode(bool deep) const {
    (void)deep;
    auto clone = std::make_unique<HTMLHeadElement>();
    return clone;
}

// =============================================================================
// HTMLBodyElement
// =============================================================================

class HTMLBodyElement::Impl {
public:
    // Deprecated presentational attributes
    std::string bgColor_;
    std::string text_;
    std::string link_;
    std::string vLink_;
    std::string aLink_;
    std::string background_;
    
    // Event handlers
    HTMLBodyElement::EventCallback onLoad_;
    HTMLBodyElement::EventCallback onBeforeUnload_;
    HTMLBodyElement::EventCallback onUnload_;
    HTMLBodyElement::EventCallback onOffline_;
    HTMLBodyElement::EventCallback onOnline_;
    HTMLBodyElement::EventCallback onPageHide_;
    HTMLBodyElement::EventCallback onPageShow_;
    HTMLBodyElement::EventCallback onPopState_;
    HTMLBodyElement::EventCallback onStorage_;
    HTMLBodyElement::EventCallback onHashChange_;
    HTMLBodyElement::EventCallback onMessage_;
    HTMLBodyElement::EventCallback onError_;
    HTMLBodyElement::EventCallback onResize_;
    HTMLBodyElement::EventCallback onScroll_;
};

HTMLBodyElement::HTMLBodyElement()
    : HTMLElement("body")
    , impl_(std::make_unique<Impl>())
{
}

HTMLBodyElement::~HTMLBodyElement() = default;

// Event handlers
void HTMLBodyElement::setOnLoad(EventCallback callback) { impl_->onLoad_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onLoad() const { return impl_->onLoad_; }

void HTMLBodyElement::setOnBeforeUnload(EventCallback callback) { impl_->onBeforeUnload_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onBeforeUnload() const { return impl_->onBeforeUnload_; }

void HTMLBodyElement::setOnUnload(EventCallback callback) { impl_->onUnload_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onUnload() const { return impl_->onUnload_; }

void HTMLBodyElement::setOnOffline(EventCallback callback) { impl_->onOffline_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onOffline() const { return impl_->onOffline_; }

void HTMLBodyElement::setOnOnline(EventCallback callback) { impl_->onOnline_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onOnline() const { return impl_->onOnline_; }

void HTMLBodyElement::setOnPageHide(EventCallback callback) { impl_->onPageHide_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onPageHide() const { return impl_->onPageHide_; }

void HTMLBodyElement::setOnPageShow(EventCallback callback) { impl_->onPageShow_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onPageShow() const { return impl_->onPageShow_; }

void HTMLBodyElement::setOnPopState(EventCallback callback) { impl_->onPopState_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onPopState() const { return impl_->onPopState_; }

void HTMLBodyElement::setOnStorage(EventCallback callback) { impl_->onStorage_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onStorage() const { return impl_->onStorage_; }

void HTMLBodyElement::setOnHashChange(EventCallback callback) { impl_->onHashChange_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onHashChange() const { return impl_->onHashChange_; }

void HTMLBodyElement::setOnMessage(EventCallback callback) { impl_->onMessage_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onMessage() const { return impl_->onMessage_; }

void HTMLBodyElement::setOnError(EventCallback callback) { impl_->onError_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onError() const { return impl_->onError_; }

void HTMLBodyElement::setOnResize(EventCallback callback) { impl_->onResize_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onResize() const { return impl_->onResize_; }

void HTMLBodyElement::setOnScroll(EventCallback callback) { impl_->onScroll_ = std::move(callback); }
HTMLBodyElement::EventCallback HTMLBodyElement::onScroll() const { return impl_->onScroll_; }

// Deprecated attributes
std::string HTMLBodyElement::bgColor() const { return impl_->bgColor_; }
void HTMLBodyElement::setBgColor(const std::string& color) {
    impl_->bgColor_ = color;
    setAttribute("bgcolor", color);
}

std::string HTMLBodyElement::text() const { return impl_->text_; }
void HTMLBodyElement::setText(const std::string& color) {
    impl_->text_ = color;
    setAttribute("text", color);
}

std::string HTMLBodyElement::link() const { return impl_->link_; }
void HTMLBodyElement::setLink(const std::string& color) {
    impl_->link_ = color;
    setAttribute("link", color);
}

std::string HTMLBodyElement::vLink() const { return impl_->vLink_; }
void HTMLBodyElement::setVLink(const std::string& color) {
    impl_->vLink_ = color;
    setAttribute("vlink", color);
}

std::string HTMLBodyElement::aLink() const { return impl_->aLink_; }
void HTMLBodyElement::setALink(const std::string& color) {
    impl_->aLink_ = color;
    setAttribute("alink", color);
}

std::string HTMLBodyElement::background() const { return impl_->background_; }
void HTMLBodyElement::setBackground(const std::string& url) {
    impl_->background_ = url;
    setAttribute("background", url);
}

std::unique_ptr<DOMNode> HTMLBodyElement::cloneNode(bool deep) const {
    (void)deep;
    auto clone = std::make_unique<HTMLBodyElement>();
    clone->setBgColor(impl_->bgColor_);
    clone->setText(impl_->text_);
    clone->setLink(impl_->link_);
    clone->setVLink(impl_->vLink_);
    clone->setALink(impl_->aLink_);
    clone->setBackground(impl_->background_);
    return clone;
}

// =============================================================================
// HTMLTitleElement
// =============================================================================

class HTMLTitleElement::Impl {
public:
    std::string text_;
};

HTMLTitleElement::HTMLTitleElement()
    : HTMLElement("title")
    , impl_(std::make_unique<Impl>())
{
}

HTMLTitleElement::~HTMLTitleElement() = default;

std::string HTMLTitleElement::text() const {
    return impl_->text_;
}

void HTMLTitleElement::setText(const std::string& text) {
    impl_->text_ = text;
}

std::unique_ptr<DOMNode> HTMLTitleElement::cloneNode(bool deep) const {
    (void)deep;
    auto clone = std::make_unique<HTMLTitleElement>();
    clone->setText(impl_->text_);
    return clone;
}

// =============================================================================
// HTMLBaseElement
// =============================================================================

class HTMLBaseElement::Impl {
public:
    std::string href_;
    std::string target_;
};

HTMLBaseElement::HTMLBaseElement()
    : HTMLElement("base")
    , impl_(std::make_unique<Impl>())
{
}

HTMLBaseElement::~HTMLBaseElement() = default;

std::string HTMLBaseElement::href() const {
    return impl_->href_;
}

void HTMLBaseElement::setHref(const std::string& href) {
    impl_->href_ = href;
    setAttribute("href", href);
}

std::string HTMLBaseElement::target() const {
    return impl_->target_;
}

void HTMLBaseElement::setTarget(const std::string& target) {
    impl_->target_ = target;
    setAttribute("target", target);
}

std::string HTMLBaseElement::resolvedHref() const {
    // Would resolve against document URL
    return impl_->href_;
}

std::unique_ptr<DOMNode> HTMLBaseElement::cloneNode(bool deep) const {
    (void)deep;
    auto clone = std::make_unique<HTMLBaseElement>();
    clone->setHref(impl_->href_);
    clone->setTarget(impl_->target_);
    return clone;
}

} // namespace Zepra::WebCore
