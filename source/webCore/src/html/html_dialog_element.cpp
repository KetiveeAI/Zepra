/**
 * @file html_dialog_element.cpp
 * @brief HTMLDialogElement implementation
 *
 * Implements the HTMLDialogElement interface for <dialog> elements.
 * Handles modal/non-modal display, event dispatching, and accessibility.
 */

#include "webcore/html/html_dialog_element.hpp"
#include <algorithm>

namespace Zepra::WebCore {

// =============================================================================
// ToggleEvent Implementation
// =============================================================================

ToggleEvent::ToggleEvent(const std::string& type,
                         const std::string& oldState,
                         const std::string& newState)
    : Event(type, false, type == "beforetoggle"), // beforetoggle is cancelable
      oldState_(oldState),
      newState_(newState) {}

// =============================================================================
// HTMLDialogElement::Impl - Private Implementation
// =============================================================================

class HTMLDialogElement::Impl {
public:
    // Dialog state
    bool isModal = false;
    std::string returnValue;
    DialogClosedBy closedByMode = DialogClosedBy::Any;

    // Event callbacks
    EventListener onCancel;
    EventListener onClose;
    EventListener onBeforeToggle;
    EventListener onToggle;

    // Focus management
    DOMElement* previouslyFocusedElement = nullptr;

    // Backdrop handling
    bool backdropVisible = false;

    // Convert closedBy enum to string
    static std::string closedByToString(DialogClosedBy mode) {
        switch (mode) {
            case DialogClosedBy::Any:
                return "any";
            case DialogClosedBy::CloseRequest:
                return "closerequest";
            case DialogClosedBy::None:
                return "none";
        }
        return "any";
    }

    // Parse closedBy string to enum
    static DialogClosedBy parseClosedBy(const std::string& value) {
        if (value == "closerequest") {
            return DialogClosedBy::CloseRequest;
        } else if (value == "none") {
            return DialogClosedBy::None;
        }
        return DialogClosedBy::Any;
    }
};

// =============================================================================
// HTMLDialogElement - Constructor/Destructor
// =============================================================================

HTMLDialogElement::HTMLDialogElement()
    : DOMElement("dialog"),
      impl_(std::make_unique<Impl>()) {
    // Set default ARIA role for accessibility
    setAttribute("role", "dialog");
}

HTMLDialogElement::~HTMLDialogElement() = default;

// =============================================================================
// Instance Properties
// =============================================================================

bool HTMLDialogElement::open() const {
    return hasAttribute("open");
}

void HTMLDialogElement::setOpen(bool isOpen) {
    if (isOpen) {
        setAttribute("open", "");
    } else {
        removeAttribute("open");
    }
}

const std::string& HTMLDialogElement::returnValue() const {
    return impl_->returnValue;
}

void HTMLDialogElement::setReturnValue(const std::string& value) {
    impl_->returnValue = value;
}

std::string HTMLDialogElement::closedBy() const {
    return Impl::closedByToString(impl_->closedByMode);
}

void HTMLDialogElement::setClosedBy(const std::string& value) {
    impl_->closedByMode = Impl::parseClosedBy(value);
    setAttribute("closedby", value);
}

bool HTMLDialogElement::isModal() const {
    return impl_->isModal;
}

// =============================================================================
// Instance Methods
// =============================================================================

void HTMLDialogElement::show() {
    // Do nothing if already open
    if (open()) {
        return;
    }

    // Fire beforetoggle event (cancelable when opening)
    if (!fireBeforeToggle("closed", "open")) {
        return; // Event was canceled
    }

    // Update open state
    impl_->isModal = false;
    updateOpenState(true);

    // Fire toggle event
    fireToggle("closed", "open");
}

void HTMLDialogElement::showModal() {
    // Do nothing if already open
    if (open()) {
        return;
    }

    // Fire beforetoggle event (cancelable when opening)
    if (!fireBeforeToggle("closed", "open")) {
        return; // Event was canceled
    }

    // Save currently focused element for restoration
    saveFocus();

    // Set as modal
    impl_->isModal = true;

    // Add to top layer (above all other content)
    addToTopLayer();

    // Show backdrop
    showBackdrop();

    // Update open state
    updateOpenState(true);

    // Set ARIA modal attribute for accessibility
    setAttribute("aria-modal", "true");

    // Fire toggle event
    fireToggle("closed", "open");

    // Focus the dialog or first focusable element within
    // In production, would find first focusable descendant
    // For now, focus the dialog itself
}

void HTMLDialogElement::close(const std::string& returnVal) {
    // Do nothing if already closed
    if (!open()) {
        return;
    }

    // Set return value if provided
    if (!returnVal.empty()) {
        impl_->returnValue = returnVal;
    }

    // Fire beforetoggle event (not cancelable when closing)
    fireBeforeToggle("open", "closed");

    // Update state
    impl_->isModal = false;

    // Hide backdrop
    hideBackdrop();

    // Remove from top layer
    removeFromTopLayer();

    // Update open state
    updateOpenState(false);

    // Remove modal ARIA attribute
    removeAttribute("aria-modal");

    // Fire toggle event
    fireToggle("open", "closed");

    // Fire close event
    fireClose();

    // Restore focus to previously focused element
    restoreFocus();
}

bool HTMLDialogElement::requestClose(const std::string& returnVal) {
    // Do nothing if already closed
    if (!open()) {
        return false;
    }

    // Check closedBy mode
    if (impl_->closedByMode == DialogClosedBy::None) {
        // Cannot close via request, only via close()
        return false;
    }

    // Set return value if provided
    if (!returnVal.empty()) {
        impl_->returnValue = returnVal;
    }

    // Fire cancelable cancel event
    if (!fireCancel()) {
        // Event was prevented, dialog stays open
        return false;
    }

    // Close the dialog
    close(returnVal);
    return true;
}

// =============================================================================
// Event Handlers
// =============================================================================

void HTMLDialogElement::setOnCancel(EventListener callback) {
    impl_->onCancel = std::move(callback);
}

void HTMLDialogElement::setOnClose(EventListener callback) {
    impl_->onClose = std::move(callback);
}

void HTMLDialogElement::setOnBeforeToggle(EventListener callback) {
    impl_->onBeforeToggle = std::move(callback);
}

void HTMLDialogElement::setOnToggle(EventListener callback) {
    impl_->onToggle = std::move(callback);
}

// =============================================================================
// Internal Methods
// =============================================================================

bool HTMLDialogElement::handleEscapeKey() {
    if (!open() || !impl_->isModal) {
        return false;
    }

    // Attempt to close via requestClose
    return requestClose();
}

bool HTMLDialogElement::handleBackdropClick() {
    if (!open() || !impl_->isModal) {
        return false;
    }

    // Only close if closedBy is "any" (light dismiss enabled)
    if (impl_->closedByMode != DialogClosedBy::Any) {
        return false;
    }

    return requestClose();
}

std::unique_ptr<DOMNode> HTMLDialogElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLDialogElement>();

    // Copy attributes
    for (const auto& [name, value] : attributes()) {
        clone->setAttribute(name, value);
    }

    // Copy state
    clone->impl_->returnValue = impl_->returnValue;
    clone->impl_->closedByMode = impl_->closedByMode;

    // Deep clone children
    if (deep) {
        for (const auto& child : childNodes()) {
            auto childClone = child->cloneNode(true);
            clone->appendChild(std::move(childClone));
        }
    }

    return clone;
}

// =============================================================================
// Private Helper Methods
// =============================================================================

bool HTMLDialogElement::fireBeforeToggle(const std::string& oldState,
                                          const std::string& newState) {
    ToggleEvent event("beforetoggle", oldState, newState);
    dispatchEvent(event);

    // Also call handler if set
    if (impl_->onBeforeToggle) {
        impl_->onBeforeToggle(event);
    }

    // Return true if event was NOT prevented (for cancelable open)
    return !event.defaultPrevented();
}

void HTMLDialogElement::fireToggle(const std::string& oldState,
                                    const std::string& newState) {
    ToggleEvent event("toggle", oldState, newState);
    dispatchEvent(event);

    // Also call handler if set
    if (impl_->onToggle) {
        impl_->onToggle(event);
    }
}

bool HTMLDialogElement::fireCancel() {
    Event event("cancel", true, true); // Bubbles and cancelable
    dispatchEvent(event);

    // Also call handler if set
    if (impl_->onCancel) {
        impl_->onCancel(event);
    }

    // Return true if event was NOT prevented
    return !event.defaultPrevented();
}

void HTMLDialogElement::fireClose() {
    Event event("close", true, false); // Bubbles but not cancelable
    dispatchEvent(event);

    // Also call handler if set
    if (impl_->onClose) {
        impl_->onClose(event);
    }
}

void HTMLDialogElement::updateOpenState(bool isOpen) {
    if (isOpen) {
        setAttribute("open", "");
    } else {
        removeAttribute("open");
    }
}

void HTMLDialogElement::addToTopLayer() {
    // In a full implementation, this would add the dialog to the browser's
    // top layer rendering stack, above all other document content.
    //
    // The top layer is a special rendering layer that:
    // - Renders above all other document content
    // - Cannot be clipped by ancestors
    // - Has a ::backdrop pseudo-element rendered behind it
    //
    // For now, we set a marker attribute. The render tree implementation
    // should check for this and render appropriately.
    setAttribute("data-top-layer", "true");
}

void HTMLDialogElement::removeFromTopLayer() {
    removeAttribute("data-top-layer");
}

void HTMLDialogElement::showBackdrop() {
    impl_->backdropVisible = true;

    // The backdrop is rendered via CSS ::backdrop pseudo-element.
    // In the render tree, we need to check if this dialog is modal
    // and render a backdrop if so.
    //
    // The backdrop should:
    // - Cover the entire viewport
    // - Be below the dialog in the top layer
    // - Block pointer events to content behind it
    // - Be styleable via dialog::backdrop CSS
    setAttribute("data-backdrop-visible", "true");
}

void HTMLDialogElement::hideBackdrop() {
    impl_->backdropVisible = false;
    removeAttribute("data-backdrop-visible");
}

void HTMLDialogElement::saveFocus() {
    // In a full implementation, this would query the document for
    // the currently focused element via document.activeElement
    // and store it for later restoration.
    //
    // We store a pointer that would be set by the Page/Document.
    impl_->previouslyFocusedElement = nullptr;
}

void HTMLDialogElement::restoreFocus() {
    // In a full implementation, this would focus the previously
    // focused element if it's still in the document.
    //
    // The focus would be restored via Element.focus() API.
    if (impl_->previouslyFocusedElement) {
        // Would call: impl_->previouslyFocusedElement->focus();
        impl_->previouslyFocusedElement = nullptr;
    }
}

} // namespace Zepra::WebCore
