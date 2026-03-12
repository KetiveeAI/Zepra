/**
 * @file html_table_element.hpp
 * @brief HTML Table elements - table, tr, td, th, etc.
 */

#pragma once

#include "html/html_element.hpp"
#include <vector>
#include <string>

namespace Zepra::WebCore {

class HTMLTableRowElement;
class HTMLTableCellElement;
class HTMLTableSectionElement;
class HTMLTableCaptionElement;

/**
 * @brief HTML Table Element (<table>)
 */
class HTMLTableElement : public HTMLElement {
public:
    HTMLTableElement();
    ~HTMLTableElement() override = default;
    
    // Caption
    HTMLTableCaptionElement* caption() const { return caption_; }
    void setCaption(HTMLTableCaptionElement* cap);
    HTMLTableCaptionElement* createCaption();
    void deleteCaption();
    
    // Table header
    HTMLTableSectionElement* tHead() const { return tHead_; }
    void setTHead(HTMLTableSectionElement* head);
    HTMLTableSectionElement* createTHead();
    void deleteTHead();
    
    // Table footer
    HTMLTableSectionElement* tFoot() const { return tFoot_; }
    void setTFoot(HTMLTableSectionElement* foot);
    HTMLTableSectionElement* createTFoot();
    void deleteTFoot();
    
    // Table bodies
    std::vector<HTMLTableSectionElement*> tBodies() const { return tBodies_; }
    HTMLTableSectionElement* createTBody();
    
    // Rows
    std::vector<HTMLTableRowElement*> rows() const;
    HTMLTableRowElement* insertRow(int index = -1);
    void deleteRow(int index);
    
    // Attributes
    std::string border() const { return getAttribute("border"); }
    void setBorder(const std::string& b) { setAttribute("border", b); }
    
    std::string cellPadding() const { return getAttribute("cellpadding"); }
    void setCellPadding(const std::string& p) { setAttribute("cellpadding", p); }
    
    std::string cellSpacing() const { return getAttribute("cellspacing"); }
    void setCellSpacing(const std::string& s) { setAttribute("cellspacing", s); }
    
    std::string width() const { return getAttribute("width"); }
    void setWidth(const std::string& w) { setAttribute("width", w); }
    
    std::string tagName() const { return "TABLE"; }
    
private:
    HTMLTableCaptionElement* caption_ = nullptr;
    HTMLTableSectionElement* tHead_ = nullptr;
    HTMLTableSectionElement* tFoot_ = nullptr;
    std::vector<HTMLTableSectionElement*> tBodies_;
};

/**
 * @brief HTML Table Section Element (<thead>, <tbody>, <tfoot>)
 */
class HTMLTableSectionElement : public HTMLElement {
public:
    enum class Type { THead, TBody, TFoot };
    
    explicit HTMLTableSectionElement(Type type);
    ~HTMLTableSectionElement() override = default;
    
    // Rows
    std::vector<HTMLTableRowElement*> rows() const;
    HTMLTableRowElement* insertRow(int index = -1);
    void deleteRow(int index);
    
    Type sectionType() const { return type_; }
    
    std::string tagName() const {
        switch (type_) {
            case Type::THead: return "THEAD";
            case Type::TBody: return "TBODY";
            case Type::TFoot: return "TFOOT";
        }
        return "TBODY";
    }
    
private:
    Type type_;
};

/**
 * @brief HTML Table Row Element (<tr>)
 */
class HTMLTableRowElement : public HTMLElement {
public:
    HTMLTableRowElement();
    ~HTMLTableRowElement() override = default;
    
    // Cells
    std::vector<HTMLTableCellElement*> cells() const;
    HTMLTableCellElement* insertCell(int index = -1);
    void deleteCell(int index);
    
    // Position
    int rowIndex() const { return rowIndex_; }
    int sectionRowIndex() const { return sectionRowIndex_; }
    
    std::string tagName() const { return "TR"; }
    
private:
    int rowIndex_ = -1;
    int sectionRowIndex_ = -1;
    
    friend class HTMLTableElement;
    friend class HTMLTableSectionElement;
};

/**
 * @brief HTML Table Cell Element (<td>, <th>)
 */
class HTMLTableCellElement : public HTMLElement {
public:
    enum class Type { TD, TH };
    
    explicit HTMLTableCellElement(Type type = Type::TD);
    ~HTMLTableCellElement() override = default;
    
    // Cell index
    int cellIndex() const { return cellIndex_; }
    
    // Spanning
    int colSpan() const { return colSpan_; }
    void setColSpan(int span) { colSpan_ = span; }
    
    int rowSpan() const { return rowSpan_; }
    void setRowSpan(int span) { rowSpan_ = span; }
    
    // Headers (for TH)
    std::string headers() const { return getAttribute("headers"); }
    void setHeaders(const std::string& h) { setAttribute("headers", h); }
    
    // Scope (for TH)
    std::string scope() const { return getAttribute("scope"); }
    void setScope(const std::string& s) { setAttribute("scope", s); }
    
    // Abbr (for TH)
    std::string abbr() const { return getAttribute("abbr"); }
    void setAbbr(const std::string& a) { setAttribute("abbr", a); }
    
    Type cellType() const { return type_; }
    
    std::string tagName() const {
        return type_ == Type::TH ? "TH" : "TD";
    }
    
private:
    Type type_;
    int cellIndex_ = -1;
    int colSpan_ = 1;
    int rowSpan_ = 1;
    
    friend class HTMLTableRowElement;
};

/**
 * @brief HTML Table Caption Element (<caption>)
 */
class HTMLTableCaptionElement : public HTMLElement {
public:
    HTMLTableCaptionElement();
    ~HTMLTableCaptionElement() override = default;
    
    std::string tagName() const { return "CAPTION"; }
};

/**
 * @brief HTML Table Col Element (<col>, <colgroup>)
 */
class HTMLTableColElement : public HTMLElement {
public:
    enum class Type { Col, ColGroup };
    
    explicit HTMLTableColElement(Type type = Type::Col);
    ~HTMLTableColElement() override = default;
    
    int span() const { return span_; }
    void setSpan(int s) { span_ = s; }
    
    std::string tagName() const {
        return type_ == Type::ColGroup ? "COLGROUP" : "COL";
    }
    
private:
    Type type_;
    int span_ = 1;
};

} // namespace Zepra::WebCore
