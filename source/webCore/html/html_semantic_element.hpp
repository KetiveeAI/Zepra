/**
 * @file html_semantic_element.hpp
 * @brief HTML5 Semantic elements
 */

#pragma once

#include "html/html_element.hpp"
#include <string>

namespace Zepra::WebCore {

/**
 * @brief HTML Article Element (<article>)
 */
class HTMLArticleElement : public HTMLElement {
public:
    HTMLArticleElement();
    ~HTMLArticleElement() override = default;
    std::string tagName() const { return "ARTICLE"; }
};

/**
 * @brief HTML Section Element (<section>)
 */
class HTMLSectionElement : public HTMLElement {
public:
    HTMLSectionElement();
    ~HTMLSectionElement() override = default;
    std::string tagName() const { return "SECTION"; }
};

/**
 * @brief HTML Nav Element (<nav>)
 */
class HTMLNavElement : public HTMLElement {
public:
    HTMLNavElement();
    ~HTMLNavElement() override = default;
    std::string tagName() const { return "NAV"; }
};

/**
 * @brief HTML Aside Element (<aside>)
 */
class HTMLAsideElement : public HTMLElement {
public:
    HTMLAsideElement();
    ~HTMLAsideElement() override = default;
    std::string tagName() const { return "ASIDE"; }
};

/**
 * @brief HTML Header Element (<header>)
 */
class HTMLHeaderElement : public HTMLElement {
public:
    HTMLHeaderElement();
    ~HTMLHeaderElement() override = default;
    std::string tagName() const { return "HEADER"; }
};

/**
 * @brief HTML Footer Element (<footer>)
 */
class HTMLFooterElement : public HTMLElement {
public:
    HTMLFooterElement();
    ~HTMLFooterElement() override = default;
    std::string tagName() const { return "FOOTER"; }
};

/**
 * @brief HTML Main Element (<main>)
 */
class HTMLMainElement : public HTMLElement {
public:
    HTMLMainElement();
    ~HTMLMainElement() override = default;
    std::string tagName() const { return "MAIN"; }
};

/**
 * @brief HTML Figure Element (<figure>)
 */
class HTMLFigureElement : public HTMLElement {
public:
    HTMLFigureElement();
    ~HTMLFigureElement() override = default;
    std::string tagName() const { return "FIGURE"; }
};

/**
 * @brief HTML FigCaption Element (<figcaption>)
 */
class HTMLFigCaptionElement : public HTMLElement {
public:
    HTMLFigCaptionElement();
    ~HTMLFigCaptionElement() override = default;
    std::string tagName() const { return "FIGCAPTION"; }
};

/**
 * @brief HTML Address Element (<address>)
 */
class HTMLAddressElement : public HTMLElement {
public:
    HTMLAddressElement();
    ~HTMLAddressElement() override = default;
    std::string tagName() const { return "ADDRESS"; }
};

/**
 * @brief HTML Time Element (<time>)
 */
class HTMLTimeElement : public HTMLElement {
public:
    HTMLTimeElement();
    ~HTMLTimeElement() override = default;
    
    std::string dateTime() const { return getAttribute("datetime"); }
    void setDateTime(const std::string& dt) { setAttribute("datetime", dt); }
    
    std::string tagName() const { return "TIME"; }
};

/**
 * @brief HTML Details Element (<details>)
 */
class HTMLDetailsElement : public HTMLElement {
public:
    HTMLDetailsElement();
    ~HTMLDetailsElement() override = default;
    
    bool open() const { return hasAttribute("open"); }
    void setOpen(bool o) { if (o) setAttribute("open", ""); else removeAttribute("open"); }
    
    std::string tagName() const { return "DETAILS"; }
};

/**
 * @brief HTML Summary Element (<summary>)
 */
class HTMLSummaryElement : public HTMLElement {
public:
    HTMLSummaryElement();
    ~HTMLSummaryElement() override = default;
    std::string tagName() const { return "SUMMARY"; }
};

/**
 * @brief HTML Progress Element (<progress>)
 */
class HTMLProgressElement : public HTMLElement {
public:
    HTMLProgressElement();
    ~HTMLProgressElement() override = default;
    
    double value() const { return value_; }
    void setValue(double v) { value_ = v; }
    
    double max() const { return max_; }
    void setMax(double m) { max_ = m; }
    
    double position() const { return max_ > 0 ? value_ / max_ : -1; }
    
    std::string tagName() const { return "PROGRESS"; }
    
private:
    double value_ = 0;
    double max_ = 1;
};

/**
 * @brief HTML Meter Element (<meter>)
 */
class HTMLMeterElement : public HTMLElement {
public:
    HTMLMeterElement();
    ~HTMLMeterElement() override = default;
    
    double value() const { return value_; }
    void setValue(double v) { value_ = v; }
    
    double min() const { return min_; }
    void setMin(double m) { min_ = m; }
    
    double max() const { return max_; }
    void setMax(double m) { max_ = m; }
    
    double low() const { return low_; }
    void setLow(double l) { low_ = l; }
    
    double high() const { return high_; }
    void setHigh(double h) { high_ = h; }
    
    double optimum() const { return optimum_; }
    void setOptimum(double o) { optimum_ = o; }
    
    std::string tagName() const { return "METER"; }
    
private:
    double value_ = 0;
    double min_ = 0;
    double max_ = 1;
    double low_ = 0;
    double high_ = 1;
    double optimum_ = 0.5;
};

/**
 * @brief HTML Output Element (<output>)
 */
class HTMLOutputElement : public HTMLElement {
public:
    HTMLOutputElement();
    ~HTMLOutputElement() override = default;
    
    std::string htmlFor() const { return getAttribute("for"); }
    void setHtmlFor(const std::string& f) { setAttribute("for", f); }
    
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    std::string value() const { return value_; }
    void setValue(const std::string& v) { value_ = v; }
    
    std::string defaultValue() const { return defaultValue_; }
    void setDefaultValue(const std::string& v) { defaultValue_ = v; }
    
    std::string tagName() const { return "OUTPUT"; }
    bool isFormControl() const { return true; }
    
private:
    std::string value_;
    std::string defaultValue_;
};

/**
 * @brief HTML Template Element (<template>)
 */
class HTMLTemplateElement : public HTMLElement {
public:
    HTMLTemplateElement();
    ~HTMLTemplateElement() override = default;
    
    // Content is a document fragment
    HTMLElement* content() const { return content_; }
    
    std::string tagName() const { return "TEMPLATE"; }
    
private:
    HTMLElement* content_ = nullptr;
};

/**
 * @brief HTML Slot Element (<slot>)
 */
class HTMLSlotElement : public HTMLElement {
public:
    HTMLSlotElement();
    ~HTMLSlotElement() override = default;
    
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    std::string tagName() const { return "SLOT"; }
};

} // namespace Zepra::WebCore
