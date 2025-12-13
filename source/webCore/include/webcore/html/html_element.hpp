/**
 * @file html_element.hpp
 * @brief HTMLElement interface - base class for all HTML elements
 *
 * Implements the HTMLElement interface per HTML Living Standard.
 * All specific HTML element classes inherit from this base.
 *
 * Modern API only - no deprecated/legacy features.
 *
 * @see https://html.spec.whatwg.org/multipage/dom.html#htmlelement
 */

#pragma once

#include "webcore/dom.hpp"
#include "webcore/event.hpp"
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Zepra::WebCore {

// Forward declarations
class CSSStyleDeclaration;
class DOMTokenList;
class DOMStringMap;

/**
 * @brief Direction for text
 */
enum class TextDirection {
    Auto,   ///< Determined from content
    LTR,    ///< Left-to-right
    RTL     ///< Right-to-left
};

/**
 * @brief Content editable state
 */
enum class ContentEditable {
    True,       ///< Element is editable
    False,      ///< Element is not editable
    Inherit,    ///< Inherit from parent
    PlainTextOnly  ///< Only plain text allowed
};

/**
 * @brief Autocapitalize options
 */
enum class AutoCapitalize {
    Off,        ///< No autocapitalization
    None,       ///< Same as Off
    On,         ///< Capitalize first letter of each sentence
    Sentences,  ///< Same as On
    Words,      ///< Capitalize first letter of each word
    Characters  ///< Capitalize all characters
};

/**
 * @brief Enter key hint for virtual keyboards
 */
enum class EnterKeyHint {
    Enter,      ///< Standard enter
    Done,       ///< Done/complete action
    Go,         ///< Navigate to destination
    Next,       ///< Next field
    Previous,   ///< Previous field
    Search,     ///< Submit search
    Send        ///< Send message
};

/**
 * @brief Input mode for virtual keyboards
 */
enum class InputMode {
    None,       ///< No virtual keyboard
    Text,       ///< Standard text keyboard
    Tel,        ///< Telephone keypad
    Url,        ///< URL keyboard (with /)
    Email,      ///< Email keyboard (with @)
    Numeric,    ///< Numeric keypad
    Decimal,    ///< Decimal keypad (with .)
    Search      ///< Search keyboard
};

/**
 * @brief HTMLElement - base interface for all HTML elements
 *
 * Extends DOMElement with HTML-specific properties and methods.
 * This is the base class for all specific HTML element types.
 */
class HTMLElement : public DOMElement {
public:
    explicit HTMLElement(const std::string& tagName);
    ~HTMLElement() override;

    // =========================================================================
    // Metadata Attributes
    // =========================================================================

    /// Element title (tooltip)
    std::string title() const;
    void setTitle(const std::string& title);

    /// Language of element content
    std::string lang() const;
    void setLang(const std::string& lang);

    /// Text direction
    std::string dir() const;
    void setDir(const std::string& dir);
    TextDirection textDirection() const;

    // =========================================================================
    // Interaction
    // =========================================================================

    /// Whether element is hidden
    bool hidden() const;
    void setHidden(bool hidden);

    /// Whether element is inert (non-interactive)
    bool inert() const;
    void setInert(bool inert);

    /// Tab order index (-1 = not focusable via tab)
    int tabIndex() const;
    void setTabIndex(int index);

    /// Access key (keyboard shortcut)
    std::string accessKey() const;
    void setAccessKey(const std::string& key);

    /// Computed access key label
    std::string accessKeyLabel() const;

    /// Whether element is draggable
    bool draggable() const;
    void setDraggable(bool draggable);

    /// Spellcheck enabled
    bool spellcheck() const;
    void setSpellcheck(bool spellcheck);

    /// Writing suggestions enabled
    bool writingSuggestions() const;
    void setWritingSuggestions(bool enabled);

    /// Autocapitalize mode
    std::string autocapitalize() const;
    void setAutocapitalize(const std::string& mode);

    // =========================================================================
    // Content Editing
    // =========================================================================

    /// Content editable state
    std::string contentEditable() const;
    void setContentEditable(const std::string& value);

    /// Effective content editable state
    bool isContentEditable() const;

    /// Input mode hint for virtual keyboards
    std::string inputMode() const;
    void setInputMode(const std::string& mode);

    /// Enter key hint for virtual keyboards
    std::string enterKeyHint() const;
    void setEnterKeyHint(const std::string& hint);

    // =========================================================================
    // Content
    // =========================================================================

    /// Inner text content (rendered text)
    std::string innerText() const;
    void setInnerText(const std::string& text);

    /// Outer text content
    std::string outerText() const;
    void setOuterText(const std::string& text);

    // =========================================================================
    // Focus
    // =========================================================================

    /// Focus this element
    void focus();

    /// Focus with options
    struct FocusOptions {
        bool preventScroll = false;
        bool focusVisible = false;
    };
    void focus(const FocusOptions& options);

    /// Remove focus from this element
    void blur();

    // =========================================================================
    // User Interaction
    // =========================================================================

    /// Simulate a click on this element
    void click();

    /// Attach internals for custom elements
    // ElementInternals* attachInternals();

    // =========================================================================
    // Popover API (Modern)
    // =========================================================================

    /// Popover state
    std::string popover() const;
    void setPopover(const std::string& state);

    /// Show popover
    void showPopover();

    /// Hide popover
    void hidePopover();

    /// Toggle popover
    bool togglePopover(bool force);
    bool togglePopover();

    // =========================================================================
    // Geometry
    // =========================================================================

    /// Offset from positioned parent
    int offsetTop() const;
    int offsetLeft() const;
    int offsetWidth() const;
    int offsetHeight() const;

    /// Positioned parent element
    HTMLElement* offsetParent() const;

    // =========================================================================
    // Style
    // =========================================================================

    /// Inline style declaration
    CSSStyleDeclaration* style();
    const CSSStyleDeclaration* style() const;

    // =========================================================================
    // Dataset (data-* attributes)
    // =========================================================================

    /// Access to data-* attributes as a map
    DOMStringMap* dataset();
    const DOMStringMap* dataset() const;

    // =========================================================================
    // Event Handlers (Common)
    // =========================================================================

    void setOnClick(EventListener callback);
    void setOnDblClick(EventListener callback);
    void setOnMouseDown(EventListener callback);
    void setOnMouseUp(EventListener callback);
    void setOnMouseMove(EventListener callback);
    void setOnMouseEnter(EventListener callback);
    void setOnMouseLeave(EventListener callback);
    void setOnMouseOver(EventListener callback);
    void setOnMouseOut(EventListener callback);

    void setOnKeyDown(EventListener callback);
    void setOnKeyUp(EventListener callback);
    void setOnKeyPress(EventListener callback);

    void setOnFocus(EventListener callback);
    void setOnBlur(EventListener callback);
    void setOnFocusIn(EventListener callback);
    void setOnFocusOut(EventListener callback);

    void setOnInput(EventListener callback);
    void setOnChange(EventListener callback);

    void setOnScroll(EventListener callback);
    void setOnWheel(EventListener callback);

    void setOnDragStart(EventListener callback);
    void setOnDrag(EventListener callback);
    void setOnDragEnd(EventListener callback);
    void setOnDragEnter(EventListener callback);
    void setOnDragOver(EventListener callback);
    void setOnDragLeave(EventListener callback);
    void setOnDrop(EventListener callback);

    void setOnTouchStart(EventListener callback);
    void setOnTouchMove(EventListener callback);
    void setOnTouchEnd(EventListener callback);
    void setOnTouchCancel(EventListener callback);

    void setOnContextMenu(EventListener callback);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

protected:
    /// Copy common HTMLElement properties to clone
    void copyHTMLElementProperties(HTMLElement* target) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief DOMStringMap for data-* attributes
 *
 * Provides access to custom data attributes in a map-like interface.
 */
class DOMStringMap {
public:
    DOMStringMap(HTMLElement* element);
    ~DOMStringMap();

    /// Get value for data-{name}
    std::string get(const std::string& name) const;

    /// Set value for data-{name}
    void set(const std::string& name, const std::string& value);

    /// Delete data-{name}
    void remove(const std::string& name);

    /// Check if data-{name} exists
    bool has(const std::string& name) const;

    /// Get all data attribute names
    std::vector<std::string> keys() const;

private:
    HTMLElement* element_;
};

/**
 * @brief CSSStyleDeclaration for inline styles
 *
 * Provides access to inline CSS styles on an element.
 */
class CSSStyleDeclaration {
public:
    CSSStyleDeclaration();
    ~CSSStyleDeclaration();

    /// Get CSS text
    std::string cssText() const;
    void setCssText(const std::string& text);

    /// Number of properties
    size_t length() const;

    /// Get property value
    std::string getPropertyValue(const std::string& property) const;

    /// Get property priority (e.g., "important")
    std::string getPropertyPriority(const std::string& property) const;

    /// Set property
    void setProperty(const std::string& property, const std::string& value,
                     const std::string& priority = "");

    /// Remove property
    std::string removeProperty(const std::string& property);

    /// Get property name by index
    std::string item(size_t index) const;

    // Common CSS properties as direct accessors
    std::string display() const;
    void setDisplay(const std::string& value);

    std::string visibility() const;
    void setVisibility(const std::string& value);

    std::string position() const;
    void setPosition(const std::string& value);

    std::string width() const;
    void setWidth(const std::string& value);

    std::string height() const;
    void setHeight(const std::string& value);

    std::string backgroundColor() const;
    void setBackgroundColor(const std::string& value);

    std::string color() const;
    void setColor(const std::string& value);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
