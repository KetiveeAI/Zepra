// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_media_extras.cpp
 * @brief Implementation of media/interactive elements: audio, source, picture, details, template, slot, map
 */

#include "html/html_audio_element.hpp"
#include "html/html_source_element.hpp"
#include "html/html_picture_element.hpp"
#include "html/html_details_element.hpp"
#include "html/html_template_element.hpp"
#include "html/html_slot_element.hpp"
#include "html/html_map_element.hpp"

namespace Zepra::WebCore {

// =============================================================================
// HTMLAudioElement
// =============================================================================

class HTMLAudioElement::Impl {
    // Audio inherits everything from HTMLMediaElement
};

HTMLAudioElement::HTMLAudioElement()
    : HTMLMediaElement("audio")
{
}

HTMLAudioElement::~HTMLAudioElement() = default;

std::unique_ptr<DOMNode> HTMLAudioElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLAudioElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

HTMLAudioElement* createAudio(const std::string& src) {
    auto* audio = new HTMLAudioElement();
    if (!src.empty()) {
        audio->setSrc(src);
    }
    return audio;
}

// =============================================================================
// HTMLSourceElement
// =============================================================================

class HTMLSourceElement::Impl {
public:
    std::string src_;
    std::string type_;
    std::string srcset_;
    std::string sizes_;
    std::string media_;
    int width_ = 0;
    int height_ = 0;
};

HTMLSourceElement::HTMLSourceElement()
    : HTMLElement("source")
    , impl_(std::make_unique<Impl>())
{
}

HTMLSourceElement::~HTMLSourceElement() = default;

std::string HTMLSourceElement::src() const { return impl_->src_; }
void HTMLSourceElement::setSrc(const std::string& src) {
    impl_->src_ = src;
    setAttribute("src", src);
}

std::string HTMLSourceElement::type() const { return impl_->type_; }
void HTMLSourceElement::setType(const std::string& type) {
    impl_->type_ = type;
    setAttribute("type", type);
}

std::string HTMLSourceElement::srcset() const { return impl_->srcset_; }
void HTMLSourceElement::setSrcset(const std::string& srcset) {
    impl_->srcset_ = srcset;
    setAttribute("srcset", srcset);
}

std::string HTMLSourceElement::sizes() const { return impl_->sizes_; }
void HTMLSourceElement::setSizes(const std::string& sizes) {
    impl_->sizes_ = sizes;
    setAttribute("sizes", sizes);
}

std::string HTMLSourceElement::media() const { return impl_->media_; }
void HTMLSourceElement::setMedia(const std::string& media) {
    impl_->media_ = media;
    setAttribute("media", media);
}

int HTMLSourceElement::width() const { return impl_->width_; }
void HTMLSourceElement::setWidth(int width) {
    impl_->width_ = width;
    setAttribute("width", std::to_string(width));
}

int HTMLSourceElement::height() const { return impl_->height_; }
void HTMLSourceElement::setHeight(int height) {
    impl_->height_ = height;
    setAttribute("height", std::to_string(height));
}

bool HTMLSourceElement::isInPicture() const {
    // TODO: DOM traversal not implemented
    HTMLElement* parent = nullptr;
    return parent && parent->tagName() == "picture";
}

bool HTMLSourceElement::isInMedia() const {
    // TODO: DOM traversal not implemented
    HTMLElement* parent = nullptr;
    if (!parent) return false;
    std::string tag = parent->tagName();
    return tag == "audio" || tag == "video";
}

bool HTMLSourceElement::mediaMatches() const {
    if (impl_->media_.empty()) return true;
    // Media query matching would be done by CSS engine
    return true;
}

bool HTMLSourceElement::typeSupported() const {
    // MIME type checking would be done by media engine
    return true;
}

std::unique_ptr<DOMNode> HTMLSourceElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSourceElement>();
    clone->setSrc(impl_->src_);
    clone->setType(impl_->type_);
    clone->setSrcset(impl_->srcset_);
    clone->setSizes(impl_->sizes_);
    clone->setMedia(impl_->media_);
    clone->setWidth(impl_->width_);
    clone->setHeight(impl_->height_);
    return clone;
}

// =============================================================================
// HTMLPictureElement
// =============================================================================

class HTMLPictureElement::Impl {};

HTMLPictureElement::HTMLPictureElement()
    : HTMLElement("picture")
    , impl_(std::make_unique<Impl>())
{
}

HTMLPictureElement::~HTMLPictureElement() = default;

std::vector<HTMLSourceElement*> HTMLPictureElement::sources() const {
    // TODO: DOM traversal not implemented - would collect source children
    return {};
}

HTMLImageElement* HTMLPictureElement::image() const {
    // TODO: DOM traversal not implemented - would find img child
    return nullptr;
}

HTMLSourceElement* HTMLPictureElement::selectedSource() const {
    for (auto* source : sources()) {
        if (source->mediaMatches() && source->typeSupported()) {
            return source;
        }
    }
    return nullptr;
}

std::string HTMLPictureElement::effectiveSrc() const {
    // Would select source based on media queries
    return "";
}

std::unique_ptr<DOMNode> HTMLPictureElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLPictureElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLDetailsElement
// =============================================================================

class HTMLDetailsElement::Impl {
public:
    bool open_ = false;
    std::string name_;
    ToggleCallback onToggle_;
};

HTMLDetailsElement::HTMLDetailsElement()
    : HTMLElement("details")
    , impl_(std::make_unique<Impl>())
{
}

HTMLDetailsElement::~HTMLDetailsElement() = default;

bool HTMLDetailsElement::open() const { return impl_->open_; }
void HTMLDetailsElement::setOpen(bool open) {
    bool wasOpen = impl_->open_;
    impl_->open_ = open;
    if (open) setAttribute("open", "");
    else removeAttribute("open");
    
    if (wasOpen != open && impl_->onToggle_) {
        impl_->onToggle_(open);
    }
}

std::string HTMLDetailsElement::name() const { return impl_->name_; }
void HTMLDetailsElement::setName(const std::string& name) {
    impl_->name_ = name;
    setAttribute("name", name);
}

HTMLSummaryElement* HTMLDetailsElement::summary() const {
    // TODO: DOM traversal not implemented - would find summary child
    return nullptr;
}

void HTMLDetailsElement::toggle() {
    setOpen(!impl_->open_);
}

void HTMLDetailsElement::expand() {
    setOpen(true);
}

void HTMLDetailsElement::collapse() {
    setOpen(false);
}

void HTMLDetailsElement::setOnToggle(ToggleCallback callback) {
    impl_->onToggle_ = std::move(callback);
}

HTMLDetailsElement::ToggleCallback HTMLDetailsElement::onToggle() const {
    return impl_->onToggle_;
}

std::unique_ptr<DOMNode> HTMLDetailsElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLDetailsElement>();
    clone->setOpen(impl_->open_);
    clone->setName(impl_->name_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLSummaryElement
// =============================================================================

class HTMLSummaryElement::Impl {};

HTMLSummaryElement::HTMLSummaryElement()
    : HTMLElement("summary")
    , impl_(std::make_unique<Impl>())
{
}

HTMLSummaryElement::~HTMLSummaryElement() = default;

HTMLDetailsElement* HTMLSummaryElement::details() const {
    // TODO: DOM traversal not implemented
    HTMLElement* parent = nullptr;
    if (auto* details = dynamic_cast<HTMLDetailsElement*>(parent)) {
        return details;
    }
    return nullptr;
}

std::unique_ptr<DOMNode> HTMLSummaryElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSummaryElement>();
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLTemplateElement
// =============================================================================

class HTMLTemplateElement::Impl {
public:
    DocumentFragment* content_ = nullptr;
    std::string shadowRootMode_;
    bool shadowRootDelegatesFocus_ = false;
    bool shadowRootClonable_ = false;
};

HTMLTemplateElement::HTMLTemplateElement()
    : HTMLElement("template")
    , impl_(std::make_unique<Impl>())
{
}

HTMLTemplateElement::~HTMLTemplateElement() = default;

DocumentFragment* HTMLTemplateElement::content() const {
    return impl_->content_;
}

std::string HTMLTemplateElement::shadowRootMode() const { return impl_->shadowRootMode_; }
void HTMLTemplateElement::setShadowRootMode(const std::string& mode) {
    impl_->shadowRootMode_ = mode;
    setAttribute("shadowrootmode", mode);
}

bool HTMLTemplateElement::shadowRootDelegatesFocus() const { return impl_->shadowRootDelegatesFocus_; }
void HTMLTemplateElement::setShadowRootDelegatesFocus(bool delegates) {
    impl_->shadowRootDelegatesFocus_ = delegates;
}

bool HTMLTemplateElement::shadowRootClonable() const { return impl_->shadowRootClonable_; }
void HTMLTemplateElement::setShadowRootClonable(bool clonable) {
    impl_->shadowRootClonable_ = clonable;
}

std::unique_ptr<DOMNode> HTMLTemplateElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLTemplateElement>();
    clone->setShadowRootMode(impl_->shadowRootMode_);
    clone->setShadowRootDelegatesFocus(impl_->shadowRootDelegatesFocus_);
    clone->setShadowRootClonable(impl_->shadowRootClonable_);
    // Note: template content cloning is special
    return clone;
}

// =============================================================================
// HTMLSlotElement
// =============================================================================

class HTMLSlotElement::Impl {
public:
    std::string name_;
    SlotChangeCallback onSlotChange_;
};

HTMLSlotElement::HTMLSlotElement()
    : HTMLElement("slot")
    , impl_(std::make_unique<Impl>())
{
}

HTMLSlotElement::~HTMLSlotElement() = default;

std::string HTMLSlotElement::name() const { return impl_->name_; }
void HTMLSlotElement::setName(const std::string& name) {
    impl_->name_ = name;
    setAttribute("name", name);
}

std::vector<DOMNode*> HTMLSlotElement::assignedNodes() const {
    return {};
}

std::vector<DOMNode*> HTMLSlotElement::assignedNodes(const AssignedNodesOptions& options) const {
    return {};
}

std::vector<HTMLElement*> HTMLSlotElement::assignedElements() const {
    return {};
}

std::vector<HTMLElement*> HTMLSlotElement::assignedElements(const AssignedNodesOptions& options) const {
    return {};
}

void HTMLSlotElement::assign(const std::vector<DOMNode*>& nodes) {
    // Manual slot assignment
}

void HTMLSlotElement::setOnSlotChange(SlotChangeCallback callback) {
    impl_->onSlotChange_ = std::move(callback);
}

HTMLSlotElement::SlotChangeCallback HTMLSlotElement::onSlotChange() const {
    return impl_->onSlotChange_;
}

std::unique_ptr<DOMNode> HTMLSlotElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSlotElement>();
    clone->setName(impl_->name_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLMapElement
// =============================================================================

class HTMLMapElement::Impl {
public:
    std::string name_;
};

HTMLMapElement::HTMLMapElement()
    : HTMLElement("map")
    , impl_(std::make_unique<Impl>())
{
}

HTMLMapElement::~HTMLMapElement() = default;

std::string HTMLMapElement::name() const { return impl_->name_; }
void HTMLMapElement::setName(const std::string& name) {
    impl_->name_ = name;
    setAttribute("name", name);
}

HTMLCollection* HTMLMapElement::areas() const {
    return nullptr;
}

std::vector<HTMLAreaElement*> HTMLMapElement::areaElements() const {
    // TODO: DOM traversal not implemented - would collect area children
    return {};
}

std::vector<HTMLImageElement*> HTMLMapElement::images() const {
    return {};
}

std::unique_ptr<DOMNode> HTMLMapElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLMapElement>();
    clone->setName(impl_->name_);
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    return clone;
}

// =============================================================================
// HTMLAreaElement
// =============================================================================

class HTMLAreaElement::Impl {
public:
    std::string alt_;
    std::string coords_;
    std::string href_;
    std::string shape_ = "default";
    std::string target_;
    std::string download_;
    std::string ping_;
    std::string rel_;
    std::string referrerPolicy_;
};

HTMLAreaElement::HTMLAreaElement()
    : HTMLElement("area")
    , impl_(std::make_unique<Impl>())
{
}

HTMLAreaElement::~HTMLAreaElement() = default;

std::string HTMLAreaElement::alt() const { return impl_->alt_; }
void HTMLAreaElement::setAlt(const std::string& alt) {
    impl_->alt_ = alt;
    setAttribute("alt", alt);
}

std::string HTMLAreaElement::coords() const { return impl_->coords_; }
void HTMLAreaElement::setCoords(const std::string& coords) {
    impl_->coords_ = coords;
    setAttribute("coords", coords);
}

std::string HTMLAreaElement::href() const { return impl_->href_; }
void HTMLAreaElement::setHref(const std::string& href) {
    impl_->href_ = href;
    setAttribute("href", href);
}

std::string HTMLAreaElement::shape() const { return impl_->shape_; }
void HTMLAreaElement::setShape(const std::string& shape) {
    impl_->shape_ = shape;
    setAttribute("shape", shape);
}

std::string HTMLAreaElement::target() const { return impl_->target_; }
void HTMLAreaElement::setTarget(const std::string& target) {
    impl_->target_ = target;
    setAttribute("target", target);
}

std::string HTMLAreaElement::download() const { return impl_->download_; }
void HTMLAreaElement::setDownload(const std::string& download) {
    impl_->download_ = download;
    setAttribute("download", download);
}

std::string HTMLAreaElement::ping() const { return impl_->ping_; }
void HTMLAreaElement::setPing(const std::string& ping) {
    impl_->ping_ = ping;
    setAttribute("ping", ping);
}

std::string HTMLAreaElement::rel() const { return impl_->rel_; }
void HTMLAreaElement::setRel(const std::string& rel) {
    impl_->rel_ = rel;
    setAttribute("rel", rel);
}

std::string HTMLAreaElement::referrerPolicy() const { return impl_->referrerPolicy_; }
void HTMLAreaElement::setReferrerPolicy(const std::string& policy) {
    impl_->referrerPolicy_ = policy;
    setAttribute("referrerpolicy", policy);
}

AreaShape HTMLAreaElement::shapeType() const {
    if (impl_->shape_ == "rect") return AreaShape::Rect;
    if (impl_->shape_ == "circle") return AreaShape::Circle;
    if (impl_->shape_ == "poly") return AreaShape::Poly;
    return AreaShape::Default;
}

std::vector<int> HTMLAreaElement::coordsArray() const {
    std::vector<int> result;
    std::string coords = impl_->coords_;
    size_t pos = 0;
    while (pos < coords.length()) {
        size_t end = coords.find(',', pos);
        if (end == std::string::npos) end = coords.length();
        try {
            result.push_back(std::stoi(coords.substr(pos, end - pos)));
        } catch (...) {}
        pos = end + 1;
    }
    return result;
}

std::string HTMLAreaElement::resolvedHref() const {
    return impl_->href_;
}

bool HTMLAreaElement::containsPoint(int x, int y) const {
    auto coords = coordsArray();
    
    switch (shapeType()) {
        case AreaShape::Rect:
            if (coords.size() >= 4) {
                return x >= coords[0] && x <= coords[2] &&
                       y >= coords[1] && y <= coords[3];
            }
            break;
        case AreaShape::Circle:
            if (coords.size() >= 3) {
                int dx = x - coords[0];
                int dy = y - coords[1];
                return (dx * dx + dy * dy) <= (coords[2] * coords[2]);
            }
            break;
        case AreaShape::Poly:
            // Point-in-polygon test
            if (coords.size() >= 6) {
                bool inside = false;
                size_t n = coords.size() / 2;
                for (size_t i = 0, j = n - 1; i < n; j = i++) {
                    int xi = coords[i * 2], yi = coords[i * 2 + 1];
                    int xj = coords[j * 2], yj = coords[j * 2 + 1];
                    if (((yi > y) != (yj > y)) &&
                        (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) {
                        inside = !inside;
                    }
                }
                return inside;
            }
            break;
        case AreaShape::Default:
            return true;
    }
    return false;
}

std::unique_ptr<DOMNode> HTMLAreaElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLAreaElement>();
    clone->setAlt(impl_->alt_);
    clone->setCoords(impl_->coords_);
    clone->setHref(impl_->href_);
    clone->setShape(impl_->shape_);
    clone->setTarget(impl_->target_);
    clone->setDownload(impl_->download_);
    clone->setPing(impl_->ping_);
    clone->setRel(impl_->rel_);
    clone->setReferrerPolicy(impl_->referrerPolicy_);
    return clone;
}

} // namespace Zepra::WebCore
