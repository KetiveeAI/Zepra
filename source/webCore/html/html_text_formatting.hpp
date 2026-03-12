/**
 * @file html_text_formatting.hpp
 * @brief Text formatting elements
 *
 * Implements text formatting elements per HTML Living Standard:
 * <strong>, <em>, <b>, <i>, <u>, <s>, <mark>, <small>, <sub>, <sup>,
 * <abbr>, <dfn>, <address>
 *
 * @see https://html.spec.whatwg.org/multipage/text-level-semantics.html
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

// =============================================================================
// Semantic Importance
// =============================================================================

/**
 * @brief HTMLStrongElement - strong importance
 *
 * The <strong> element represents strong importance, seriousness, or urgency.
 */
class HTMLStrongElement : public HTMLElement {
public:
    HTMLStrongElement();
    ~HTMLStrongElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLEmElement - stress emphasis
 *
 * The <em> element represents stress emphasis.
 */
class HTMLEmElement : public HTMLElement {
public:
    HTMLEmElement();
    ~HTMLEmElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Presentation (Semantic meanings have been added in HTML5)
// =============================================================================

/**
 * @brief HTMLBElement - bring attention
 *
 * The <b> element represents text to bring attention to.
 */
class HTMLBElement : public HTMLElement {
public:
    HTMLBElement();
    ~HTMLBElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLIElement - alternate voice/mood
 *
 * The <i> element represents text in an alternate voice or mood.
 */
class HTMLIElement : public HTMLElement {
public:
    HTMLIElement();
    ~HTMLIElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLUElement - unarticulated annotation
 *
 * The <u> element represents unarticulated annotation (e.g., misspelling).
 */
class HTMLUElement : public HTMLElement {
public:
    HTMLUElement();
    ~HTMLUElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLSElement - no longer accurate/relevant
 *
 * The <s> element represents content that is no longer accurate or relevant.
 */
class HTMLSElement : public HTMLElement {
public:
    HTMLSElement();
    ~HTMLSElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Formatting
// =============================================================================

/**
 * @brief HTMLMarkElement - highlighted text
 *
 * The <mark> element represents highlighted or marked text.
 */
class HTMLMarkElement : public HTMLElement {
public:
    HTMLMarkElement();
    ~HTMLMarkElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLSmallElement - side comment
 *
 * The <small> element represents side comments like fine print.
 */
class HTMLSmallElement : public HTMLElement {
public:
    HTMLSmallElement();
    ~HTMLSmallElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLSubElement - subscript
 *
 * The <sub> element represents subscript text.
 */
class HTMLSubElement : public HTMLElement {
public:
    HTMLSubElement();
    ~HTMLSubElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLSupElement - superscript
 *
 * The <sup> element represents superscript text.
 */
class HTMLSupElement : public HTMLElement {
public:
    HTMLSupElement();
    ~HTMLSupElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Definitions and Abbreviations
// =============================================================================

/**
 * @brief HTMLAbbrElement - abbreviation
 *
 * The <abbr> element represents an abbreviation or acronym.
 * Use the title attribute for the expansion.
 */
class HTMLAbbrElement : public HTMLElement {
public:
    HTMLAbbrElement();
    ~HTMLAbbrElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLDfnElement - definition
 *
 * The <dfn> element represents the defining instance of a term.
 */
class HTMLDfnElement : public HTMLElement {
public:
    HTMLDfnElement();
    ~HTMLDfnElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLAddressElement - contact information
 *
 * The <address> element represents contact information for its nearest
 * article or body ancestor.
 */
class HTMLAddressElement : public HTMLElement {
public:
    HTMLAddressElement();
    ~HTMLAddressElement() override;
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLDelElement - deleted text
 *
 * The <del> element represents a removal from the document.
 */
class HTMLDelElement : public HTMLElement {
public:
    HTMLDelElement();
    ~HTMLDelElement() override;
    
    /// URL of document explaining the change
    std::string cite() const;
    void setCite(const std::string& cite);
    
    /// Date/time of the change
    std::string dateTime() const;
    void setDateTime(const std::string& datetime);
    
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLInsElement - inserted text
 *
 * The <ins> element represents an addition to the document.
 */
class HTMLInsElement : public HTMLElement {
public:
    HTMLInsElement();
    ~HTMLInsElement() override;
    
    /// URL of document explaining the change
    std::string cite() const;
    void setCite(const std::string& cite);
    
    /// Date/time of the change
    std::string dateTime() const;
    void setDateTime(const std::string& datetime);
    
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
