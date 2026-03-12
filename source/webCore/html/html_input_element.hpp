/**
 * @file html_input_element.hpp
 * @brief HTMLInputElement interface for <input> elements
 *
 * Implements all input types per HTML Living Standard.
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/HTMLInputElement
 * @see https://html.spec.whatwg.org/multipage/input.html
 */

#pragma once

#include "html/html_element.hpp"
#include <vector>
#include <functional>

namespace Zepra::WebCore {

// Forward declarations
class HTMLFormElement;
class HTMLDataListElement;
class ValidityState;
class FileList;

/**
 * @brief Input element types
 */
enum class InputType {
    Text,
    Password,
    Email,
    Url,
    Tel,
    Number,
    Range,
    Date,
    DateTimeLocal,
    Month,
    Week,
    Time,
    Color,
    Checkbox,
    Radio,
    File,
    Hidden,
    Submit,
    Reset,
    Button,
    Image,
    Search
};

/**
 * @brief Selection direction
 */
enum class SelectionDirection {
    Forward,
    Backward,
    None
};

/**
 * @brief ValidityState for constraint validation
 */
class ValidityState {
public:
    ValidityState();
    
    bool badInput() const { return badInput_; }
    bool customError() const { return customError_; }
    bool patternMismatch() const { return patternMismatch_; }
    bool rangeOverflow() const { return rangeOverflow_; }
    bool rangeUnderflow() const { return rangeUnderflow_; }
    bool stepMismatch() const { return stepMismatch_; }
    bool tooLong() const { return tooLong_; }
    bool tooShort() const { return tooShort_; }
    bool typeMismatch() const { return typeMismatch_; }
    bool valid() const { return valid_; }
    bool valueMissing() const { return valueMissing_; }
    
    // Internal setters
    void setBadInput(bool v) { badInput_ = v; updateValid(); }
    void setCustomError(bool v) { customError_ = v; updateValid(); }
    void setPatternMismatch(bool v) { patternMismatch_ = v; updateValid(); }
    void setRangeOverflow(bool v) { rangeOverflow_ = v; updateValid(); }
    void setRangeUnderflow(bool v) { rangeUnderflow_ = v; updateValid(); }
    void setStepMismatch(bool v) { stepMismatch_ = v; updateValid(); }
    void setTooLong(bool v) { tooLong_ = v; updateValid(); }
    void setTooShort(bool v) { tooShort_ = v; updateValid(); }
    void setTypeMismatch(bool v) { typeMismatch_ = v; updateValid(); }
    void setValueMissing(bool v) { valueMissing_ = v; updateValid(); }
    
private:
    void updateValid() {
        valid_ = !badInput_ && !customError_ && !patternMismatch_ &&
                 !rangeOverflow_ && !rangeUnderflow_ && !stepMismatch_ &&
                 !tooLong_ && !tooShort_ && !typeMismatch_ && !valueMissing_;
    }
    
    bool badInput_ = false;
    bool customError_ = false;
    bool patternMismatch_ = false;
    bool rangeOverflow_ = false;
    bool rangeUnderflow_ = false;
    bool stepMismatch_ = false;
    bool tooLong_ = false;
    bool tooShort_ = false;
    bool typeMismatch_ = false;
    bool valid_ = true;
    bool valueMissing_ = false;
};

/**
 * @brief HTMLInputElement represents an <input> element
 *
 * Supports all standard input types and validation.
 */
class HTMLInputElement : public HTMLElement {
public:
    HTMLInputElement();
    ~HTMLInputElement() override;

    // =========================================================================
    // Common Properties
    // =========================================================================

    /// Input type
    std::string type() const;
    void setType(const std::string& type);

    /// Current value
    std::string value() const;
    void setValue(const std::string& value);

    /// Default value (from HTML)
    std::string defaultValue() const;
    void setDefaultValue(const std::string& value);

    /// Input name for form submission
    std::string name() const;
    void setName(const std::string& name);

    /// Whether input is disabled
    bool disabled() const;
    void setDisabled(bool disabled);

    /// Whether input is required
    bool required() const;
    void setRequired(bool required);

    /// Whether input is read-only
    bool readOnly() const;
    void setReadOnly(bool readOnly);

    /// Placeholder text
    std::string placeholder() const;
    void setPlaceholder(const std::string& placeholder);

    /// Autocomplete hint
    std::string autocomplete() const;
    void setAutocomplete(const std::string& autocomplete);

    /// Autofocus on page load
    bool autofocus() const;
    void setAutofocus(bool autofocus);

    // =========================================================================
    // Form Association
    // =========================================================================

    /// Associated form element
    HTMLFormElement* form() const;

    /// Form action override
    std::string formAction() const;
    void setFormAction(const std::string& action);

    /// Form enctype override
    std::string formEnctype() const;
    void setFormEnctype(const std::string& enctype);

    /// Form method override
    std::string formMethod() const;
    void setFormMethod(const std::string& method);

    /// Form novalidate override
    bool formNoValidate() const;
    void setFormNoValidate(bool noValidate);

    /// Form target override
    std::string formTarget() const;
    void setFormTarget(const std::string& target);

    // =========================================================================
    // Checkbox/Radio Properties
    // =========================================================================

    /// Whether checkbox/radio is checked
    bool checked() const;
    void setChecked(bool checked);

    /// Default checked state
    bool defaultChecked() const;
    void setDefaultChecked(bool checked);

    /// Whether checkbox has indeterminate state
    bool indeterminate() const;
    void setIndeterminate(bool indeterminate);

    // =========================================================================
    // Text/Number Properties
    // =========================================================================

    /// Maximum length
    int maxLength() const;
    void setMaxLength(int length);

    /// Minimum length
    int minLength() const;
    void setMinLength(int length);

    /// Pattern for validation (regex)
    std::string pattern() const;
    void setPattern(const std::string& pattern);

    /// Input size (width in characters)
    unsigned int size() const;
    void setSize(unsigned int size);

    // =========================================================================
    // Number/Range Properties
    // =========================================================================

    /// Minimum value
    std::string min() const;
    void setMin(const std::string& min);

    /// Maximum value
    std::string max() const;
    void setMax(const std::string& max);

    /// Step value
    std::string step() const;
    void setStep(const std::string& step);

    /// Value as number
    double valueAsNumber() const;
    void setValueAsNumber(double value);

    // =========================================================================
    // Date Properties
    // =========================================================================

    /// Value as Date (milliseconds since epoch)
    long long valueAsDate() const;
    void setValueAsDate(long long date);

    // =========================================================================
    // File Properties
    // =========================================================================

    /// Accept file types
    std::string accept() const;
    void setAccept(const std::string& accept);

    /// Allow multiple files
    bool multiple() const;
    void setMultiple(bool multiple);

    /// Selected files (simplified - returns filenames)
    std::vector<std::string> files() const;

    // =========================================================================
    // Selection Properties
    // =========================================================================

    /// Selection start position
    unsigned int selectionStart() const;
    void setSelectionStart(unsigned int start);

    /// Selection end position
    unsigned int selectionEnd() const;
    void setSelectionEnd(unsigned int end);

    /// Selection direction
    std::string selectionDirection() const;
    void setSelectionDirection(const std::string& direction);

    // =========================================================================
    // Validation
    // =========================================================================

    /// Validation state
    ValidityState* validity();
    const ValidityState* validity() const;

    /// Validation message
    std::string validationMessage() const;

    /// Whether input will be validated
    bool willValidate() const;

    /// Check validity
    bool checkValidity();

    /// Report validity to user
    bool reportValidity();

    /// Set custom validation message
    void setCustomValidity(const std::string& message);

    // =========================================================================
    // Methods
    // =========================================================================

    /// Select all text
    void select();

    /// Set selection range
    void setSelectionRange(unsigned int start, unsigned int end, 
                           const std::string& direction = "none");

    /// Replace text in range
    void setRangeText(const std::string& replacement);
    void setRangeText(const std::string& replacement, 
                      unsigned int start, unsigned int end,
                      const std::string& selectMode = "preserve");

    /// Step up value
    void stepUp(int n = 1);

    /// Step down value
    void stepDown(int n = 1);

    /// Show picker (date, color, file)
    void showPicker();

    // =========================================================================
    // Popover Control
    // =========================================================================

    /// Popover target element
    HTMLElement* popoverTargetElement() const;
    void setPopoverTargetElement(HTMLElement* element);

    /// Popover target action
    std::string popoverTargetAction() const;
    void setPopoverTargetAction(const std::string& action);

    // =========================================================================
    // Event Handlers
    // =========================================================================

    void setOnInput(EventListener callback);
    void setOnChange(EventListener callback);
    void setOnInvalid(EventListener callback);
    void setOnSearch(EventListener callback);
    void setOnSelect(EventListener callback);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    /// Update validity state based on current value
    void updateValidity();

    /// Parse input type string
    InputType parseType(const std::string& type) const;
};

} // namespace Zepra::WebCore
