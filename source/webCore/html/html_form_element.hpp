/**
 * @file html_form_element.hpp
 * @brief HTMLFormElement interface for <form> elements
 *
 * Implements form functionality per HTML Living Standard.
 * Handles form submission, validation, and form control collection.
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/HTMLFormElement
 * @see https://html.spec.whatwg.org/multipage/forms.html#the-form-element
 */

#pragma once

#include "html/html_element.hpp"
#include <vector>
#include <functional>

namespace Zepra::WebCore {

// Forward declarations
class HTMLInputElement;
class HTMLButtonElement;
class HTMLSelectElement;
class HTMLTextAreaElement;

/**
 * @brief Form submission method
 */
enum class FormMethod {
    Get,    ///< HTTP GET request
    Post,   ///< HTTP POST request
    Dialog  ///< Close dialog with return value
};

/**
 * @brief Form encoding type
 */
enum class FormEnctype {
    UrlEncoded,     ///< application/x-www-form-urlencoded (default)
    Multipart,      ///< multipart/form-data (for file uploads)
    TextPlain       ///< text/plain
};

/**
 * @brief Collection of form controls
 *
 * Provides access to all form controls within a form element.
 */
class HTMLFormControlsCollection {
public:
    HTMLFormControlsCollection();
    ~HTMLFormControlsCollection();

    /// Number of controls in the collection
    size_t length() const;

    /// Get control by index
    HTMLElement* item(size_t index) const;

    /// Get control by name or id
    HTMLElement* namedItem(const std::string& name) const;

    /// Add control to collection (internal use)
    void addControl(HTMLElement* control);

    /// Remove control from collection (internal use)
    void removeControl(HTMLElement* control);

    /// Clear all controls
    void clear();

    /// Iterator support
    std::vector<HTMLElement*>::const_iterator begin() const;
    std::vector<HTMLElement*>::const_iterator end() const;

private:
    std::vector<HTMLElement*> controls_;
};

/**
 * @brief Form data entry
 */
struct FormDataEntry {
    std::string name;
    std::string value;
    std::string filename;   ///< For file inputs
    std::string type;       ///< MIME type for files
};

/**
 * @brief HTMLFormElement represents a <form> element
 *
 * Provides properties and methods for HTML forms including
 * submission, validation, and access to form controls.
 */
class HTMLFormElement : public HTMLElement {
public:
    HTMLFormElement();
    ~HTMLFormElement() override;

    // =========================================================================
    // Form Properties
    // =========================================================================

    /// Accept character set for form submission
    std::string acceptCharset() const;
    void setAcceptCharset(const std::string& charset);

    /// Form submission URL
    std::string action() const;
    void setAction(const std::string& action);

    /// Autocomplete setting ("on" or "off")
    std::string autocomplete() const;
    void setAutocomplete(const std::string& autocomplete);

    /// Form encoding type for submission
    std::string enctype() const;
    void setEnctype(const std::string& enctype);

    /// Alias for enctype
    std::string encoding() const;
    void setEncoding(const std::string& encoding);

    /// HTTP method for form submission
    std::string method() const;
    void setMethod(const std::string& method);

    /// Form name
    std::string name() const;
    void setName(const std::string& name);

    /// Skip validation on submit
    bool noValidate() const;
    void setNoValidate(bool noValidate);

    /// Link relationship
    std::string rel() const;
    void setRel(const std::string& rel);

    /// Relationship token list
    DOMTokenList* relList();

    /// Target window/frame for submission response
    std::string target() const;
    void setTarget(const std::string& target);

    // =========================================================================
    // Form Controls
    // =========================================================================

    /// Collection of all form controls
    HTMLFormControlsCollection* elements();
    const HTMLFormControlsCollection* elements() const;

    /// Number of form controls
    size_t length() const;

    /// Get control by index
    HTMLElement* operator[](size_t index);

    /// Get control by name
    HTMLElement* operator[](const std::string& name);

    // =========================================================================
    // Form Methods
    // =========================================================================

    /**
     * @brief Check if all form controls are valid
     * @return true if all controls satisfy constraints
     *
     * Fires 'invalid' event on invalid controls.
     */
    bool checkValidity();

    /**
     * @brief Check validity and report to user
     * @return true if all controls are valid
     *
     * Fires 'invalid' events and shows validation UI.
     */
    bool reportValidity();

    /**
     * @brief Reset form to initial values
     *
     * Fires 'reset' event (cancelable).
     */
    void reset();

    /**
     * @brief Submit the form
     *
     * Submits without firing 'submit' event or validation.
     * Use requestSubmit() for normal form submission behavior.
     */
    void submit();

    /**
     * @brief Request form submission with validation
     * @param submitter Optional submit button that triggered submission
     *
     * Fires 'submit' event (cancelable) and validates form.
     */
    void requestSubmit(HTMLElement* submitter = nullptr);

    /**
     * @brief Get form data as entries
     * @return Vector of form data entries
     */
    std::vector<FormDataEntry> getFormData() const;

    // =========================================================================
    // Event Handlers
    // =========================================================================

    void setOnSubmit(EventListener callback);
    void setOnReset(EventListener callback);
    void setOnFormData(EventListener callback);

    // =========================================================================
    // Internal Methods
    // =========================================================================

    /// Register a form control with this form
    void registerControl(HTMLElement* control);

    /// Unregister a form control from this form
    void unregisterControl(HTMLElement* control);

    /// Clone node
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    /// Build form data from controls
    void buildFormData(std::vector<FormDataEntry>& entries) const;

    /// Encode form data as URL-encoded string
    std::string encodeFormData(const std::vector<FormDataEntry>& entries) const;

    /// Fire submit event
    bool fireSubmit(HTMLElement* submitter);

    /// Fire reset event
    bool fireReset();

    /// Fire formdata event
    void fireFormData(std::vector<FormDataEntry>& entries);
};

} // namespace Zepra::WebCore
