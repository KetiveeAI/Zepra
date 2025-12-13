/**
 * @file html_element.cpp
 * @brief HTMLElement implementation
 *
 * Implements HTMLElement interface - base for all HTML elements.
 */

#include "webcore/html/html_element.hpp"
#include "webcore/html/html_dialog_element.hpp"  // For ToggleEvent
#include <algorithm>
#include <cctype>
#include <sstream>

namespace Zepra::WebCore {

// =============================================================================
// HTMLElement::Impl - Private Implementation
// =============================================================================

class HTMLElement::Impl {
public:
    // Computed values
    int tabIndex = -1;
    bool isContentEditable = false;
    
    // Popover state
    std::string popoverState;
    bool popoverShowing = false;
    
    // Style and dataset
    std::unique_ptr<CSSStyleDeclaration> style;
    std::unique_ptr<DOMStringMap> dataset;
    
    // Event handlers
    std::unordered_map<std::string, EventListener> eventHandlers;
    
    Impl() : style(std::make_unique<CSSStyleDeclaration>()) {}
};

// =============================================================================
// HTMLElement - Constructor/Destructor
// =============================================================================

HTMLElement::HTMLElement(const std::string& tagName)
    : DOMElement(tagName),
      impl_(std::make_unique<Impl>()) {
    impl_->dataset = std::make_unique<DOMStringMap>(this);
}

HTMLElement::~HTMLElement() = default;

// =============================================================================
// Metadata Attributes
// =============================================================================

std::string HTMLElement::title() const {
    return getAttribute("title");
}

void HTMLElement::setTitle(const std::string& title) {
    setAttribute("title", title);
}

std::string HTMLElement::lang() const {
    return getAttribute("lang");
}

void HTMLElement::setLang(const std::string& lang) {
    setAttribute("lang", lang);
}

std::string HTMLElement::dir() const {
    return getAttribute("dir");
}

void HTMLElement::setDir(const std::string& dir) {
    setAttribute("dir", dir);
}

TextDirection HTMLElement::textDirection() const {
    std::string d = dir();
    if (d == "ltr") return TextDirection::LTR;
    if (d == "rtl") return TextDirection::RTL;
    return TextDirection::Auto;
}

// =============================================================================
// Interaction
// =============================================================================

bool HTMLElement::hidden() const {
    return hasAttribute("hidden");
}

void HTMLElement::setHidden(bool hidden) {
    if (hidden) {
        setAttribute("hidden", "");
    } else {
        removeAttribute("hidden");
    }
}

bool HTMLElement::inert() const {
    return hasAttribute("inert");
}

void HTMLElement::setInert(bool inert) {
    if (inert) {
        setAttribute("inert", "");
    } else {
        removeAttribute("inert");
    }
}

int HTMLElement::tabIndex() const {
    if (hasAttribute("tabindex")) {
        try {
            return std::stoi(getAttribute("tabindex"));
        } catch (...) {
            return 0;
        }
    }
    return impl_->tabIndex;
}

void HTMLElement::setTabIndex(int index) {
    setAttribute("tabindex", std::to_string(index));
}

std::string HTMLElement::accessKey() const {
    return getAttribute("accesskey");
}

void HTMLElement::setAccessKey(const std::string& key) {
    setAttribute("accesskey", key);
}

std::string HTMLElement::accessKeyLabel() const {
    std::string key = accessKey();
    if (key.empty()) return "";
    // Platform-specific prefix (simplified)
    return "Alt+" + key;
}

bool HTMLElement::draggable() const {
    return getAttribute("draggable") == "true";
}

void HTMLElement::setDraggable(bool draggable) {
    setAttribute("draggable", draggable ? "true" : "false");
}

bool HTMLElement::spellcheck() const {
    std::string val = getAttribute("spellcheck");
    return val.empty() || val == "true";
}

void HTMLElement::setSpellcheck(bool spellcheck) {
    setAttribute("spellcheck", spellcheck ? "true" : "false");
}

bool HTMLElement::writingSuggestions() const {
    std::string val = getAttribute("writingsuggestions");
    return val.empty() || val == "true";
}

void HTMLElement::setWritingSuggestions(bool enabled) {
    setAttribute("writingsuggestions", enabled ? "true" : "false");
}

std::string HTMLElement::autocapitalize() const {
    return getAttribute("autocapitalize");
}

void HTMLElement::setAutocapitalize(const std::string& mode) {
    setAttribute("autocapitalize", mode);
}

// =============================================================================
// Content Editing
// =============================================================================

std::string HTMLElement::contentEditable() const {
    if (!hasAttribute("contenteditable")) {
        return "inherit";
    }
    std::string val = getAttribute("contenteditable");
    if (val.empty() || val == "true") return "true";
    if (val == "false") return "false";
    if (val == "plaintext-only") return "plaintext-only";
    return "inherit";
}

void HTMLElement::setContentEditable(const std::string& value) {
    setAttribute("contenteditable", value);
}

bool HTMLElement::isContentEditable() const {
    std::string ce = contentEditable();
    if (ce == "true" || ce == "plaintext-only") {
        return true;
    }
    if (ce == "false") {
        return false;
    }
    // Inherit from parent
    if (parentNode()) {
        auto* parent = dynamic_cast<HTMLElement*>(parentNode());
        if (parent) {
            return parent->isContentEditable();
        }
    }
    return false;
}

std::string HTMLElement::inputMode() const {
    return getAttribute("inputmode");
}

void HTMLElement::setInputMode(const std::string& mode) {
    setAttribute("inputmode", mode);
}

std::string HTMLElement::enterKeyHint() const {
    return getAttribute("enterkeyhint");
}

void HTMLElement::setEnterKeyHint(const std::string& hint) {
    setAttribute("enterkeyhint", hint);
}

// =============================================================================
// Content
// =============================================================================

std::string HTMLElement::innerText() const {
    // Get rendered text content (simplified - same as textContent for now)
    return textContent();
}

void HTMLElement::setInnerText(const std::string& text) {
    setTextContent(text);
}

std::string HTMLElement::outerText() const {
    return innerText();
}

void HTMLElement::setOuterText(const std::string& text) {
    // Replace this element with text
    setInnerText(text);
}

// =============================================================================
// Focus
// =============================================================================

void HTMLElement::focus() {
    FocusOptions options;
    focus(options);
}

void HTMLElement::focus(const FocusOptions& options) {
    // Fire focus event
    FocusEvent event("focus");
    dispatchEvent(event);
    
    // Fire focusin event (bubbles)
    Event focusInEvent("focusin", true, false);
    dispatchEvent(focusInEvent);
    
    (void)options; // preventScroll/focusVisible handling would go here
}

void HTMLElement::blur() {
    // Fire blur event
    FocusEvent event("blur");
    dispatchEvent(event);
    
    // Fire focusout event (bubbles)
    Event focusOutEvent("focusout", true, false);
    dispatchEvent(focusOutEvent);
}

// =============================================================================
// User Interaction
// =============================================================================

void HTMLElement::click() {
    // Fire click event
    MouseEvent event("click", 0, 0, 0);
    dispatchEvent(event);
}

// =============================================================================
// Popover API
// =============================================================================

std::string HTMLElement::popover() const {
    if (!hasAttribute("popover")) {
        return "";
    }
    std::string val = getAttribute("popover");
    if (val.empty() || val == "auto") return "auto";
    if (val == "manual") return "manual";
    return "auto";
}

void HTMLElement::setPopover(const std::string& state) {
    if (state.empty()) {
        removeAttribute("popover");
    } else {
        setAttribute("popover", state);
    }
}

void HTMLElement::showPopover() {
    if (impl_->popoverShowing) return;
    
    // Fire beforetoggle event
    ToggleEvent beforeEvent("beforetoggle", "closed", "open");
    dispatchEvent(beforeEvent);
    
    if (beforeEvent.defaultPrevented()) return;
    
    impl_->popoverShowing = true;
    
    // Fire toggle event
    ToggleEvent toggleEvent("toggle", "closed", "open");
    dispatchEvent(toggleEvent);
}

void HTMLElement::hidePopover() {
    if (!impl_->popoverShowing) return;
    
    // Fire beforetoggle event
    ToggleEvent beforeEvent("beforetoggle", "open", "closed");
    dispatchEvent(beforeEvent);
    
    impl_->popoverShowing = false;
    
    // Fire toggle event
    ToggleEvent toggleEvent("toggle", "open", "closed");
    dispatchEvent(toggleEvent);
}

bool HTMLElement::togglePopover(bool force) {
    if (force) {
        showPopover();
    } else {
        hidePopover();
    }
    return impl_->popoverShowing;
}

bool HTMLElement::togglePopover() {
    return togglePopover(!impl_->popoverShowing);
}

// =============================================================================
// Geometry
// =============================================================================

int HTMLElement::offsetTop() const {
    // Would be computed from layout
    return 0;
}

int HTMLElement::offsetLeft() const {
    return 0;
}

int HTMLElement::offsetWidth() const {
    return 0;
}

int HTMLElement::offsetHeight() const {
    return 0;
}

HTMLElement* HTMLElement::offsetParent() const {
    // Find positioned ancestor
    return nullptr;
}

// =============================================================================
// Style
// =============================================================================

CSSStyleDeclaration* HTMLElement::style() {
    return impl_->style.get();
}

const CSSStyleDeclaration* HTMLElement::style() const {
    return impl_->style.get();
}

// =============================================================================
// Dataset
// =============================================================================

DOMStringMap* HTMLElement::dataset() {
    return impl_->dataset.get();
}

const DOMStringMap* HTMLElement::dataset() const {
    return impl_->dataset.get();
}

// =============================================================================
// Event Handlers
// =============================================================================

#define DEFINE_EVENT_HANDLER_SETTER(name, eventType) \
    void HTMLElement::setOn##name(EventListener callback) { \
        impl_->eventHandlers[eventType] = std::move(callback); \
        addEventListener(eventType, impl_->eventHandlers[eventType]); \
    }

DEFINE_EVENT_HANDLER_SETTER(Click, "click")
DEFINE_EVENT_HANDLER_SETTER(DblClick, "dblclick")
DEFINE_EVENT_HANDLER_SETTER(MouseDown, "mousedown")
DEFINE_EVENT_HANDLER_SETTER(MouseUp, "mouseup")
DEFINE_EVENT_HANDLER_SETTER(MouseMove, "mousemove")
DEFINE_EVENT_HANDLER_SETTER(MouseEnter, "mouseenter")
DEFINE_EVENT_HANDLER_SETTER(MouseLeave, "mouseleave")
DEFINE_EVENT_HANDLER_SETTER(MouseOver, "mouseover")
DEFINE_EVENT_HANDLER_SETTER(MouseOut, "mouseout")

DEFINE_EVENT_HANDLER_SETTER(KeyDown, "keydown")
DEFINE_EVENT_HANDLER_SETTER(KeyUp, "keyup")
DEFINE_EVENT_HANDLER_SETTER(KeyPress, "keypress")

DEFINE_EVENT_HANDLER_SETTER(Focus, "focus")
DEFINE_EVENT_HANDLER_SETTER(Blur, "blur")
DEFINE_EVENT_HANDLER_SETTER(FocusIn, "focusin")
DEFINE_EVENT_HANDLER_SETTER(FocusOut, "focusout")

DEFINE_EVENT_HANDLER_SETTER(Input, "input")
DEFINE_EVENT_HANDLER_SETTER(Change, "change")

DEFINE_EVENT_HANDLER_SETTER(Scroll, "scroll")
DEFINE_EVENT_HANDLER_SETTER(Wheel, "wheel")

DEFINE_EVENT_HANDLER_SETTER(DragStart, "dragstart")
DEFINE_EVENT_HANDLER_SETTER(Drag, "drag")
DEFINE_EVENT_HANDLER_SETTER(DragEnd, "dragend")
DEFINE_EVENT_HANDLER_SETTER(DragEnter, "dragenter")
DEFINE_EVENT_HANDLER_SETTER(DragOver, "dragover")
DEFINE_EVENT_HANDLER_SETTER(DragLeave, "dragleave")
DEFINE_EVENT_HANDLER_SETTER(Drop, "drop")

DEFINE_EVENT_HANDLER_SETTER(TouchStart, "touchstart")
DEFINE_EVENT_HANDLER_SETTER(TouchMove, "touchmove")
DEFINE_EVENT_HANDLER_SETTER(TouchEnd, "touchend")
DEFINE_EVENT_HANDLER_SETTER(TouchCancel, "touchcancel")

DEFINE_EVENT_HANDLER_SETTER(ContextMenu, "contextmenu")

#undef DEFINE_EVENT_HANDLER_SETTER

// =============================================================================
// Clone
// =============================================================================

std::unique_ptr<DOMNode> HTMLElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLElement>(tagName());
    copyHTMLElementProperties(clone.get());
    
    if (deep) {
        for (const auto& child : childNodes()) {
            auto childClone = child->cloneNode(true);
            clone->appendChild(std::move(childClone));
        }
    }
    
    return clone;
}

void HTMLElement::copyHTMLElementProperties(HTMLElement* target) const {
    // Copy all attributes
    for (const auto& [name, value] : attributes()) {
        target->setAttribute(name, value);
    }
    
    // Copy style
    target->style()->setCssText(style()->cssText());
}

// =============================================================================
// DOMStringMap Implementation
// =============================================================================

DOMStringMap::DOMStringMap(HTMLElement* element) : element_(element) {}
DOMStringMap::~DOMStringMap() = default;

static std::string camelToKebab(const std::string& camel) {
    std::string result;
    for (char c : camel) {
        if (std::isupper(c)) {
            result += '-';
            result += static_cast<char>(std::tolower(c));
        } else {
            result += c;
        }
    }
    return result;
}

static std::string kebabToCamel(const std::string& kebab) {
    std::string result;
    bool nextUpper = false;
    for (char c : kebab) {
        if (c == '-') {
            nextUpper = true;
        } else if (nextUpper) {
            result += static_cast<char>(std::toupper(c));
            nextUpper = false;
        } else {
            result += c;
        }
    }
    return result;
}

std::string DOMStringMap::get(const std::string& name) const {
    std::string attrName = "data-" + camelToKebab(name);
    return element_->getAttribute(attrName);
}

void DOMStringMap::set(const std::string& name, const std::string& value) {
    std::string attrName = "data-" + camelToKebab(name);
    element_->setAttribute(attrName, value);
}

void DOMStringMap::remove(const std::string& name) {
    std::string attrName = "data-" + camelToKebab(name);
    element_->removeAttribute(attrName);
}

bool DOMStringMap::has(const std::string& name) const {
    std::string attrName = "data-" + camelToKebab(name);
    return element_->hasAttribute(attrName);
}

std::vector<std::string> DOMStringMap::keys() const {
    std::vector<std::string> result;
    for (const auto& [name, value] : element_->attributes()) {
        if (name.find("data-") == 0) {
            // Remove "data-" prefix and convert to camelCase
            std::string dataName = name.substr(5);
            result.push_back(kebabToCamel(dataName));
        }
    }
    return result;
}

// =============================================================================
// CSSStyleDeclaration Implementation
// =============================================================================

class CSSStyleDeclaration::Impl {
public:
    std::unordered_map<std::string, std::string> properties;
    std::unordered_map<std::string, std::string> priorities;
    std::vector<std::string> propertyOrder;
};

CSSStyleDeclaration::CSSStyleDeclaration()
    : impl_(std::make_unique<Impl>()) {}

CSSStyleDeclaration::~CSSStyleDeclaration() = default;

std::string CSSStyleDeclaration::cssText() const {
    std::ostringstream oss;
    for (const auto& prop : impl_->propertyOrder) {
        oss << prop << ": " << impl_->properties.at(prop);
        auto it = impl_->priorities.find(prop);
        if (it != impl_->priorities.end() && !it->second.empty()) {
            oss << " !" << it->second;
        }
        oss << "; ";
    }
    return oss.str();
}

void CSSStyleDeclaration::setCssText(const std::string& text) {
    impl_->properties.clear();
    impl_->priorities.clear();
    impl_->propertyOrder.clear();
    
    // Simple CSS parsing
    std::istringstream iss(text);
    std::string declaration;
    while (std::getline(iss, declaration, ';')) {
        size_t colonPos = declaration.find(':');
        if (colonPos != std::string::npos) {
            std::string prop = declaration.substr(0, colonPos);
            std::string value = declaration.substr(colonPos + 1);
            
            // Trim whitespace
            auto trimStart = prop.find_first_not_of(" \t");
            auto trimEnd = prop.find_last_not_of(" \t");
            if (trimStart != std::string::npos) {
                prop = prop.substr(trimStart, trimEnd - trimStart + 1);
            }
            
            trimStart = value.find_first_not_of(" \t");
            trimEnd = value.find_last_not_of(" \t");
            if (trimStart != std::string::npos) {
                value = value.substr(trimStart, trimEnd - trimStart + 1);
            }
            
            // Check for !important
            std::string priority;
            size_t importantPos = value.find("!important");
            if (importantPos != std::string::npos) {
                priority = "important";
                value = value.substr(0, importantPos);
                trimEnd = value.find_last_not_of(" \t");
                if (trimEnd != std::string::npos) {
                    value = value.substr(0, trimEnd + 1);
                }
            }
            
            setProperty(prop, value, priority);
        }
    }
}

size_t CSSStyleDeclaration::length() const {
    return impl_->propertyOrder.size();
}

std::string CSSStyleDeclaration::getPropertyValue(const std::string& property) const {
    auto it = impl_->properties.find(property);
    return it != impl_->properties.end() ? it->second : "";
}

std::string CSSStyleDeclaration::getPropertyPriority(const std::string& property) const {
    auto it = impl_->priorities.find(property);
    return it != impl_->priorities.end() ? it->second : "";
}

void CSSStyleDeclaration::setProperty(const std::string& property, 
                                       const std::string& value,
                                       const std::string& priority) {
    if (impl_->properties.find(property) == impl_->properties.end()) {
        impl_->propertyOrder.push_back(property);
    }
    impl_->properties[property] = value;
    impl_->priorities[property] = priority;
}

std::string CSSStyleDeclaration::removeProperty(const std::string& property) {
    auto it = impl_->properties.find(property);
    if (it != impl_->properties.end()) {
        std::string value = it->second;
        impl_->properties.erase(it);
        impl_->priorities.erase(property);
        impl_->propertyOrder.erase(
            std::remove(impl_->propertyOrder.begin(), 
                       impl_->propertyOrder.end(), property),
            impl_->propertyOrder.end());
        return value;
    }
    return "";
}

std::string CSSStyleDeclaration::item(size_t index) const {
    if (index < impl_->propertyOrder.size()) {
        return impl_->propertyOrder[index];
    }
    return "";
}

// CSS property implementations
std::string CSSStyleDeclaration::display() const {
    return getPropertyValue("display");
}

void CSSStyleDeclaration::setDisplay(const std::string& value) {
    setProperty("display", value);
}

std::string CSSStyleDeclaration::visibility() const {
    return getPropertyValue("visibility");
}

void CSSStyleDeclaration::setVisibility(const std::string& value) {
    setProperty("visibility", value);
}

std::string CSSStyleDeclaration::position() const {
    return getPropertyValue("position");
}

void CSSStyleDeclaration::setPosition(const std::string& value) {
    setProperty("position", value);
}

std::string CSSStyleDeclaration::width() const {
    return getPropertyValue("width");
}

void CSSStyleDeclaration::setWidth(const std::string& value) {
    setProperty("width", value);
}

std::string CSSStyleDeclaration::height() const {
    return getPropertyValue("height");
}

void CSSStyleDeclaration::setHeight(const std::string& value) {
    setProperty("height", value);
}

std::string CSSStyleDeclaration::backgroundColor() const {
    return getPropertyValue("background-color");
}

void CSSStyleDeclaration::setBackgroundColor(const std::string& value) {
    setProperty("background-color", value);
}

std::string CSSStyleDeclaration::color() const {
    return getPropertyValue("color");
}

void CSSStyleDeclaration::setColor(const std::string& value) {
    setProperty("color", value);
}

} // namespace Zepra::WebCore
