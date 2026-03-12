/**
 * @file html_input_types.hpp
 * @brief Specialized HTML Input Types
 */

#pragma once

#include "html/html_input_element.hpp"
#include <string>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief Input type: text
 */
class HTMLTextInputElement : public HTMLInputElement {
public:
    HTMLTextInputElement();
    
    std::string type() const override { return "text"; }
    
    std::string dirName() const { return getAttribute("dirname"); }
    void setDirName(const std::string& d) { setAttribute("dirname", d); }
};

/**
 * @brief Input type: password
 */
class HTMLPasswordInputElement : public HTMLInputElement {
public:
    HTMLPasswordInputElement();
    
    std::string type() const override { return "password"; }
    
    void toggleVisibility();
    bool isVisible() const { return visible_; }
    
private:
    bool visible_ = false;
};

/**
 * @brief Input type: email
 */
class HTMLEmailInputElement : public HTMLInputElement {
public:
    HTMLEmailInputElement();
    
    std::string type() const override { return "email"; }
    
    bool validateEmail() const;
    bool multiple() const { return hasAttribute("multiple"); }
    void setMultiple(bool m) { if (m) setAttribute("multiple", ""); else removeAttribute("multiple"); }
};

/**
 * @brief Input type: url
 */
class HTMLURLInputElement : public HTMLInputElement {
public:
    HTMLURLInputElement();
    
    std::string type() const override { return "url"; }
    
    bool validateURL() const;
};

/**
 * @brief Input type: tel
 */
class HTMLTelInputElement : public HTMLInputElement {
public:
    HTMLTelInputElement();
    
    std::string type() const override { return "tel"; }
};

/**
 * @brief Input type: number
 */
class HTMLNumberInputElement : public HTMLInputElement {
public:
    HTMLNumberInputElement();
    
    std::string type() const override { return "number"; }
    
    double valueAsNumber() const;
    void setValueAsNumber(double v);
    
    double min() const;
    void setMin(double m);
    
    double max() const;
    void setMax(double m);
    
    double step() const;
    void setStep(double s);
    
    void stepUp(int n = 1);
    void stepDown(int n = 1);
};

/**
 * @brief Input type: range
 */
class HTMLRangeInputElement : public HTMLInputElement {
public:
    HTMLRangeInputElement();
    
    std::string type() const override { return "range"; }
    
    double valueAsNumber() const;
    void setValueAsNumber(double v);
    
    double min() const;
    void setMin(double m);
    
    double max() const;
    void setMax(double m);
    
    double step() const;
    void setStep(double s);
};

/**
 * @brief Input type: date
 */
class HTMLDateInputElement : public HTMLInputElement {
public:
    HTMLDateInputElement();
    
    std::string type() const override { return "date"; }
    
    std::string valueAsDate() const;
    void setValueAsDate(const std::string& d);
    
    std::string min() const { return getAttribute("min"); }
    void setMin(const std::string& m) { setAttribute("min", m); }
    
    std::string max() const { return getAttribute("max"); }
    void setMax(const std::string& m) { setAttribute("max", m); }
    
    std::string step() const { return getAttribute("step"); }
    void setStep(const std::string& s) { setAttribute("step", s); }
};

/**
 * @brief Input type: time
 */
class HTMLTimeInputElement : public HTMLInputElement {
public:
    HTMLTimeInputElement();
    
    std::string type() const override { return "time"; }
    
    std::string valueAsTime() const;
    void setValueAsTime(const std::string& t);
};

/**
 * @brief Input type: datetime-local
 */
class HTMLDateTimeLocalInputElement : public HTMLInputElement {
public:
    HTMLDateTimeLocalInputElement();
    
    std::string type() const override { return "datetime-local"; }
};

/**
 * @brief Input type: month
 */
class HTMLMonthInputElement : public HTMLInputElement {
public:
    HTMLMonthInputElement();
    
    std::string type() const override { return "month"; }
};

/**
 * @brief Input type: week
 */
class HTMLWeekInputElement : public HTMLInputElement {
public:
    HTMLWeekInputElement();
    
    std::string type() const override { return "week"; }
};

/**
 * @brief Input type: color
 */
class HTMLColorInputElement : public HTMLInputElement {
public:
    HTMLColorInputElement();
    
    std::string type() const override { return "color"; }
    
    std::string colorValue() const;
    void setColorValue(const std::string& c);
    
    uint32_t colorAsRGB() const;
    void setColorAsRGB(uint32_t rgb);
};

/**
 * @brief Input type: file
 */
class HTMLFileInputElement : public HTMLInputElement {
public:
    HTMLFileInputElement();
    
    std::string type() const override { return "file"; }
    
    std::string accept() const { return getAttribute("accept"); }
    void setAccept(const std::string& a) { setAttribute("accept", a); }
    
    bool multiple() const { return hasAttribute("multiple"); }
    void setMultiple(bool m) { if (m) setAttribute("multiple", ""); else removeAttribute("multiple"); }
    
    std::string capture() const { return getAttribute("capture"); }
    void setCapture(const std::string& c) { setAttribute("capture", c); }
    
    std::vector<std::string> files() const { return files_; }
    
private:
    std::vector<std::string> files_;
};

/**
 * @brief Input type: hidden
 */
class HTMLHiddenInputElement : public HTMLInputElement {
public:
    HTMLHiddenInputElement();
    
    std::string type() const override { return "hidden"; }
};

/**
 * @brief Input type: search
 */
class HTMLSearchInputElement : public HTMLInputElement {
public:
    HTMLSearchInputElement();
    
    std::string type() const override { return "search"; }
};

/**
 * @brief Input type: checkbox
 */
class HTMLCheckboxInputElement : public HTMLInputElement {
public:
    HTMLCheckboxInputElement();
    
    std::string type() const override { return "checkbox"; }
    
    bool checked() const { return checked_; }
    void setChecked(bool c) { checked_ = c; }
    
    bool indeterminate() const { return indeterminate_; }
    void setIndeterminate(bool i) { indeterminate_ = i; }
    
private:
    bool checked_ = false;
    bool indeterminate_ = false;
};

/**
 * @brief Input type: radio
 */
class HTMLRadioInputElement : public HTMLInputElement {
public:
    HTMLRadioInputElement();
    
    std::string type() const override { return "radio"; }
    
    bool checked() const { return checked_; }
    void setChecked(bool c);
    
    std::vector<HTMLRadioInputElement*> radioGroup() const;
    
private:
    bool checked_ = false;
};

/**
 * @brief Input type: submit
 */
class HTMLSubmitInputElement : public HTMLInputElement {
public:
    HTMLSubmitInputElement();
    
    std::string type() const override { return "submit"; }
    
    std::string formAction() const { return getAttribute("formaction"); }
    void setFormAction(const std::string& a) { setAttribute("formaction", a); }
    
    std::string formMethod() const { return getAttribute("formmethod"); }
    void setFormMethod(const std::string& m) { setAttribute("formmethod", m); }
    
    std::string formEncType() const { return getAttribute("formenctype"); }
    void setFormEncType(const std::string& e) { setAttribute("formenctype", e); }
    
    std::string formTarget() const { return getAttribute("formtarget"); }
    void setFormTarget(const std::string& t) { setAttribute("formtarget", t); }
    
    bool formNoValidate() const { return hasAttribute("formnovalidate"); }
    void setFormNoValidate(bool n) { if (n) setAttribute("formnovalidate", ""); else removeAttribute("formnovalidate"); }
};

/**
 * @brief Input type: reset
 */
class HTMLResetInputElement : public HTMLInputElement {
public:
    HTMLResetInputElement();
    
    std::string type() const override { return "reset"; }
};

/**
 * @brief Input type: button
 */
class HTMLButtonInputElement : public HTMLInputElement {
public:
    HTMLButtonInputElement();
    
    std::string type() const override { return "button"; }
};

/**
 * @brief Input type: image
 */
class HTMLImageInputElement : public HTMLInputElement {
public:
    HTMLImageInputElement();
    
    std::string type() const override { return "image"; }
    
    std::string src() const { return getAttribute("src"); }
    void setSrc(const std::string& s) { setAttribute("src", s); }
    
    std::string alt() const { return getAttribute("alt"); }
    void setAlt(const std::string& a) { setAttribute("alt", a); }
    
    int width() const;
    void setWidth(int w);
    
    int height() const;
    void setHeight(int h);
};

} // namespace Zepra::WebCore
