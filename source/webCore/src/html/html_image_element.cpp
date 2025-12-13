/**
 * @file html_image_element.cpp
 * @brief HTMLImageElement implementation
 */

#include "webcore/html/html_image_element.hpp"

namespace Zepra::WebCore {

// =============================================================================
// HTMLImageElement::Impl
// =============================================================================

class HTMLImageElement::Impl {
public:
    ImageLoadingState loadingState = ImageLoadingState::Unavailable;
    unsigned int naturalWidth = 0;
    unsigned int naturalHeight = 0;
    std::string currentSrc;
    
    EventListener onLoad;
    EventListener onError;
    EventListener onAbort;
};

// =============================================================================
// HTMLImageElement
// =============================================================================

HTMLImageElement::HTMLImageElement()
    : HTMLElement("img"),
      impl_(std::make_unique<Impl>()) {}

HTMLImageElement::~HTMLImageElement() = default;

// Source properties

std::string HTMLImageElement::src() const {
    return getAttribute("src");
}

void HTMLImageElement::setSrc(const std::string& src) {
    setAttribute("src", src);
    impl_->currentSrc = src;
    impl_->loadingState = ImageLoadingState::Loading;
    // In production, would trigger image load
}

std::string HTMLImageElement::srcset() const {
    return getAttribute("srcset");
}

void HTMLImageElement::setSrcset(const std::string& srcset) {
    setAttribute("srcset", srcset);
}

std::string HTMLImageElement::sizes() const {
    return getAttribute("sizes");
}

void HTMLImageElement::setSizes(const std::string& sizes) {
    setAttribute("sizes", sizes);
}

std::string HTMLImageElement::currentSrc() const {
    return impl_->currentSrc.empty() ? src() : impl_->currentSrc;
}

std::string HTMLImageElement::alt() const {
    return getAttribute("alt");
}

void HTMLImageElement::setAlt(const std::string& alt) {
    setAttribute("alt", alt);
}

// Dimension properties

unsigned int HTMLImageElement::width() const {
    std::string w = getAttribute("width");
    return w.empty() ? 0 : static_cast<unsigned int>(std::stoul(w));
}

void HTMLImageElement::setWidth(unsigned int width) {
    setAttribute("width", std::to_string(width));
}

unsigned int HTMLImageElement::height() const {
    std::string h = getAttribute("height");
    return h.empty() ? 0 : static_cast<unsigned int>(std::stoul(h));
}

void HTMLImageElement::setHeight(unsigned int height) {
    setAttribute("height", std::to_string(height));
}

unsigned int HTMLImageElement::naturalWidth() const {
    return impl_->naturalWidth;
}

unsigned int HTMLImageElement::naturalHeight() const {
    return impl_->naturalHeight;
}

// Loading properties

std::string HTMLImageElement::loading() const {
    std::string val = getAttribute("loading");
    if (val == "lazy") return "lazy";
    return "eager";
}

void HTMLImageElement::setLoading(const std::string& loading) {
    setAttribute("loading", loading);
}

std::string HTMLImageElement::decoding() const {
    return getAttribute("decoding");
}

void HTMLImageElement::setDecoding(const std::string& decoding) {
    setAttribute("decoding", decoding);
}

std::string HTMLImageElement::fetchPriority() const {
    return getAttribute("fetchpriority");
}

void HTMLImageElement::setFetchPriority(const std::string& priority) {
    setAttribute("fetchpriority", priority);
}

bool HTMLImageElement::complete() const {
    return impl_->loadingState == ImageLoadingState::Complete ||
           impl_->loadingState == ImageLoadingState::Broken;
}

int HTMLImageElement::x() const {
    return offsetLeft();
}

int HTMLImageElement::y() const {
    return offsetTop();
}

// Cross-origin

std::string HTMLImageElement::crossOrigin() const {
    if (!hasAttribute("crossorigin")) return "";
    std::string val = getAttribute("crossorigin");
    if (val == "use-credentials") return "use-credentials";
    return "anonymous";
}

void HTMLImageElement::setCrossOrigin(const std::string& mode) {
    if (mode.empty()) {
        removeAttribute("crossorigin");
    } else {
        setAttribute("crossorigin", mode);
    }
}

std::string HTMLImageElement::referrerPolicy() const {
    return getAttribute("referrerpolicy");
}

void HTMLImageElement::setReferrerPolicy(const std::string& policy) {
    setAttribute("referrerpolicy", policy);
}

std::string HTMLImageElement::useMap() const {
    return getAttribute("usemap");
}

void HTMLImageElement::setUseMap(const std::string& map) {
    setAttribute("usemap", map);
}

bool HTMLImageElement::isMap() const {
    return hasAttribute("ismap");
}

void HTMLImageElement::setIsMap(bool isMap) {
    if (isMap) {
        setAttribute("ismap", "");
    } else {
        removeAttribute("ismap");
    }
}

// Methods

void HTMLImageElement::decode(std::function<void(bool success)> callback) {
    // In production, would async decode and call callback
    // For now, immediately call with success if complete
    if (callback) {
        callback(impl_->loadingState == ImageLoadingState::Complete);
    }
}

std::unique_ptr<DOMNode> HTMLImageElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLImageElement>();
    copyHTMLElementProperties(clone.get());
    
    // Note: deep cloning doesn't apply to img (no children)
    (void)deep;
    
    return clone;
}

// Event handlers

void HTMLImageElement::setOnLoad(EventListener callback) {
    impl_->onLoad = std::move(callback);
    addEventListener("load", impl_->onLoad);
}

void HTMLImageElement::setOnError(EventListener callback) {
    impl_->onError = std::move(callback);
    addEventListener("error", impl_->onError);
}

void HTMLImageElement::setOnAbort(EventListener callback) {
    impl_->onAbort = std::move(callback);
    addEventListener("abort", impl_->onAbort);
}

} // namespace Zepra::WebCore
