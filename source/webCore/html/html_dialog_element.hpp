/**
 * @file html_dialog_element.hpp
 * @brief HTMLDialogElement interface implementation
 *
 * Provides methods to manipulate <dialog> elements per the HTML Living Standard.
 * Enables modal and non-modal dialog functionality with full event support.
 *
 * @see https://html.spec.whatwg.org/multipage/interactive-elements.html#the-dialog-element
 */

#pragma once

#include "browser/dom.hpp"
#include "browser/event.hpp"
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Allowed values for the closedBy attribute
 *
 * Controls which user actions can close the dialog:
 * - Any: Dialog can be closed by any method (Escape, light dismiss, close())
 * - CloseRequest: Only via Escape key or requestClose()
 * - None: Can only be closed programmatically via close()
 */
enum class DialogClosedBy {
    Any,            ///< All closing methods allowed
    CloseRequest,   ///< Escape key and requestClose() only
    None            ///< Programmatic close() only
};

/**
 * @brief Toggle event for dialog state changes
 *
 * Fired before (beforetoggle) and after (toggle) a dialog opens or closes.
 * For beforetoggle, calling preventDefault() can cancel a dialog opening.
 */
class ToggleEvent : public Event {
public:
    /**
     * @brief Construct a toggle event
     * @param type Event type ("beforetoggle" or "toggle")
     * @param oldState Previous state ("closed" or "open")
     * @param newState New state ("closed" or "open")
     */
    ToggleEvent(const std::string& type,
                const std::string& oldState,
                const std::string& newState);

    /// Get previous dialog state
    const std::string& oldState() const { return oldState_; }

    /// Get new dialog state
    const std::string& newState() const { return newState_; }

private:
    std::string oldState_;
    std::string newState_;
};

/**
 * @brief HTMLDialogElement interface
 *
 * Provides methods to manipulate <dialog> elements. Inherits from DOMElement
 * and adds dialog-specific properties and methods.
 *
 * Example usage:
 * @code
 * auto dialog = std::make_unique<HTMLDialogElement>();
 * dialog->showModal();  // Show as modal
 * dialog->close("confirmed");  // Close with return value
 * @endcode
 */
class HTMLDialogElement : public DOMElement {
public:
    HTMLDialogElement();
    ~HTMLDialogElement() override;

    // =========================================================================
    // Instance Properties
    // =========================================================================

    /**
     * @brief Check if dialog is open
     * @return true if open attribute is present
     */
    bool open() const;

    /**
     * @brief Set dialog open state (attribute only, use show/showModal for display)
     * @param open Whether dialog should be open
     */
    void setOpen(bool open);

    /**
     * @brief Get the return value of the dialog
     *
     * Set when close() is called with a value, or when a form with
     * method="dialog" is submitted.
     *
     * @return Current return value, empty string if not set
     */
    const std::string& returnValue() const;

    /**
     * @brief Set the return value of the dialog
     * @param value New return value
     */
    void setReturnValue(const std::string& value);

    /**
     * @brief Get the closedBy attribute value
     * @return One of: "any", "closerequest", "none"
     */
    std::string closedBy() const;

    /**
     * @brief Set the closedBy attribute
     * @param value One of: "any", "closerequest", "none"
     */
    void setClosedBy(const std::string& value);

    /**
     * @brief Check if dialog is modal
     * @return true if shown via showModal()
     */
    bool isModal() const;

    // =========================================================================
    // Instance Methods
    // =========================================================================

    /**
     * @brief Display the dialog modelessly
     *
     * Allows interaction with content outside the dialog.
     * Does nothing if dialog is already open.
     *
     * Fires beforetoggle and toggle events.
     */
    void show();

    /**
     * @brief Display the dialog as modal
     *
     * Creates a backdrop and makes everything outside inert.
     * Blocks interaction with content outside the dialog.
     * Does nothing if dialog is already open.
     *
     * @throw InvalidStateError if dialog is already open
     *
     * Fires beforetoggle and toggle events.
     */
    void showModal();

    /**
     * @brief Close the dialog
     *
     * @param returnVal Optional value to set as returnValue
     *
     * Fires close event. Does nothing if dialog is already closed.
     */
    void close(const std::string& returnVal = "");

    /**
     * @brief Request to close the dialog
     *
     * Fires a cancelable cancel event first. If not canceled, closes the dialog.
     * Used by platform close actions (Escape key) and light dismiss.
     *
     * @param returnVal Optional value to set as returnValue
     * @return true if dialog was closed, false if cancel event was prevented
     */
    bool requestClose(const std::string& returnVal = "");

    // =========================================================================
    // Event Handlers
    // =========================================================================

    /**
     * @brief Set handler for cancel event
     *
     * Fired when dialog is requested to close via Escape or requestClose().
     * The event is cancelable - calling preventDefault() keeps dialog open.
     *
     * @param callback Handler function
     */
    void setOnCancel(EventListener callback);

    /**
     * @brief Set handler for close event
     *
     * Fired after dialog has closed.
     *
     * @param callback Handler function
     */
    void setOnClose(EventListener callback);

    /**
     * @brief Set handler for beforetoggle event
     *
     * Fired before dialog opens or closes. Cancelable when opening.
     *
     * @param callback Handler function
     */
    void setOnBeforeToggle(EventListener callback);

    /**
     * @brief Set handler for toggle event
     *
     * Fired after dialog has opened or closed.
     *
     * @param callback Handler function
     */
    void setOnToggle(EventListener callback);

    // =========================================================================
    // Internal Methods
    // =========================================================================

    /**
     * @brief Handle Escape key press
     *
     * Called by event system when Escape is pressed while dialog is open.
     * Triggers requestClose() for modal dialogs, respecting closedBy setting.
     *
     * @return true if key was handled
     */
    bool handleEscapeKey();

    /**
     * @brief Handle backdrop click (light dismiss)
     *
     * Called when user clicks outside the dialog content on the backdrop.
     * Only triggers close if closedBy allows it.
     *
     * @return true if click was handled
     */
    bool handleBackdropClick();

    /// Clone node override
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    // Private implementation to maintain ABI stability
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Fire toggle events (beforetoggle/toggle)
    bool fireBeforeToggle(const std::string& oldState, const std::string& newState);
    void fireToggle(const std::string& oldState, const std::string& newState);

    // Fire cancel event
    bool fireCancel();

    // Fire close event
    void fireClose();

    // Update open attribute and visual state
    void updateOpenState(bool open);

    // Add/remove from top layer (for modal)
    void addToTopLayer();
    void removeFromTopLayer();

    // Manage backdrop
    void showBackdrop();
    void hideBackdrop();

    // Store previously focused element for restoration
    void saveFocus();
    void restoreFocus();
};

} // namespace Zepra::WebCore
