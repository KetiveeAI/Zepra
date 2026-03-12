/**
 * @file html_textarea_element.hpp
 * @brief HTMLTextAreaElement - Multi-line text input
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief HTML TextArea Element (<textarea>)
 */
class HTMLTextAreaElement : public HTMLElement {
public:
    using InputCallback = std::function<void(const std::string& value)>;
    using ChangeCallback = std::function<void(const std::string& value)>;
    
    HTMLTextAreaElement();
    explicit HTMLTextAreaElement(const std::string& name);
    ~HTMLTextAreaElement() override = default;
    
    // Value
    std::string value() const { return value_; }
    void setValue(const std::string& v);
    
    std::string defaultValue() const { return defaultValue_; }
    void setDefaultValue(const std::string& v) { defaultValue_ = v; }
    
    // Text length
    size_t textLength() const { return value_.length(); }
    
    // Selection
    size_t selectionStart() const { return selectionStart_; }
    size_t selectionEnd() const { return selectionEnd_; }
    std::string selectionDirection() const { return selectionDirection_; }
    
    void setSelectionStart(size_t pos) { selectionStart_ = pos; }
    void setSelectionEnd(size_t pos) { selectionEnd_ = pos; }
    void setSelectionRange(size_t start, size_t end, const std::string& dir = "forward");
    void select();  // Select all
    
    // Dimensions
    int cols() const { return cols_; }
    void setCols(int c) { cols_ = c; }
    
    int rows() const { return rows_; }
    void setRows(int r) { rows_ = r; }
    
    // Wrapping
    std::string wrap() const { return getAttribute("wrap"); }  // soft, hard, off
    void setWrap(const std::string& w) { setAttribute("wrap", w); }
    
    // Constraints
    int maxLength() const { return maxLength_; }
    void setMaxLength(int m) { maxLength_ = m; }
    
    int minLength() const { return minLength_; }
    void setMinLength(int m) { minLength_ = m; }
    
    // Form association
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    std::string placeholder() const { return getAttribute("placeholder"); }
    void setPlaceholder(const std::string& p) { setAttribute("placeholder", p); }
    
    // State
    bool readOnly() const { return hasAttribute("readonly"); }
    void setReadOnly(bool r) { if (r) setAttribute("readonly", ""); else removeAttribute("readonly"); }
    
    bool required() const { return hasAttribute("required"); }
    void setRequired(bool r) { if (r) setAttribute("required", ""); else removeAttribute("required"); }
    
    bool disabled() const { return hasAttribute("disabled"); }
    void setDisabled(bool d) { if (d) setAttribute("disabled", ""); else removeAttribute("disabled"); }
    
    // Auto-complete
    std::string autocomplete() const { return getAttribute("autocomplete"); }
    void setAutocomplete(const std::string& a) { setAttribute("autocomplete", a); }
    
    // Validation
    bool checkValidity() const;
    bool reportValidity() const;
    std::string validationMessage() const;
    void setCustomValidity(const std::string& msg) { customValidity_ = msg; }
    
    // Callbacks
    void onInput(InputCallback cb) { onInput_ = cb; }
    void onChange(ChangeCallback cb) { onChange_ = cb; }
    
    // HTMLElement overrides
    std::string tagName() const { return "TEXTAREA"; }
    bool isFormControl() const { return true; }
    
    // Focus
    void focus();
    void blur();
    
protected:
    void parseAttribute(const std::string& name, const std::string& value);
    
private:
    std::string value_;
    std::string defaultValue_;
    
    size_t selectionStart_ = 0;
    size_t selectionEnd_ = 0;
    std::string selectionDirection_ = "forward";
    
    int cols_ = 20;
    int rows_ = 2;
    int maxLength_ = -1;
    int minLength_ = -1;
    
    std::string customValidity_;
    
    InputCallback onInput_;
    ChangeCallback onChange_;
};

} // namespace Zepra::WebCore
