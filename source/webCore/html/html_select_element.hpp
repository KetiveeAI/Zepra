/**
 * @file html_select_element.hpp
 * @brief HTMLSelectElement - Dropdown select element
 * 
 * HTMLOptionElement and HTMLOptGroupElement are in html_option_element.hpp
 */

#pragma once

#include "html/html_element.hpp"
#include <vector>
#include <string>
#include <optional>
#include <functional>

namespace Zepra::WebCore {

// Forward declarations - these are defined in html_option_element.hpp
class HTMLOptionElement;
class HTMLOptGroupElement;

/**
 * @brief Select element option
 */
struct SelectOption {
    std::string value;
    std::string text;
    std::string label;
    bool selected = false;
    bool disabled = false;
    bool defaultSelected = false;
    int index = 0;
    HTMLOptGroupElement* group = nullptr;
};

/**
 * @brief HTML Select Element (<select>)
 */
class HTMLSelectElement : public HTMLElement {
public:
    using ChangeCallback = std::function<void(int index, const std::string& value)>;
    
    HTMLSelectElement();
    explicit HTMLSelectElement(const std::string& name);
    ~HTMLSelectElement() override = default;
    
    // DOM interface
    std::string type() const;  // "select-one" or "select-multiple"
    
    // Options
    size_t length() const { return options_.size(); }
    HTMLOptionElement* item(size_t index);
    HTMLOptionElement* namedItem(const std::string& name);
    
    void add(HTMLOptionElement* option, int before = -1);
    void remove(int index);
    
    // Selection
    int selectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int index);
    
    std::vector<HTMLOptionElement*> selectedOptions() const;
    std::string value() const;
    void setValue(const std::string& value);
    
    // Multiple selection
    bool multiple() const { return multiple_; }
    void setMultiple(bool m) { multiple_ = m; }
    
    // Size (visible options)
    int size() const { return size_; }
    void setSize(int s) { size_ = s; }
    
    // Form association
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& name) { setAttribute("name", name); }
    
    bool required() const { return hasAttribute("required"); }
    void setRequired(bool r) { if (r) setAttribute("required", ""); else removeAttribute("required"); }
    
    bool disabled() const { return hasAttribute("disabled"); }
    void setDisabled(bool d) { if (d) setAttribute("disabled", ""); else removeAttribute("disabled"); }
    
    // Validation
    bool checkValidity() const;
    std::string validationMessage() const;
    
    // Callbacks
    void onChange(ChangeCallback cb) { onChange_ = cb; }
    
    // HTMLElement 
    std::string tagName() const { return "SELECT"; }
    bool isFormControl() const { return true; }
    
protected:
    void parseAttribute(const std::string& name, const std::string& value);
    
private:
    std::vector<SelectOption> options_;
    int selectedIndex_ = -1;
    bool multiple_ = false;
    int size_ = 1;
    
    ChangeCallback onChange_;
    
    void notifyChange();
};

// Note: HTMLOptionElement and HTMLOptGroupElement are in html_option_element.hpp

} // namespace Zepra::WebCore
