/**
 * @file html_break_element.hpp
 * @brief HTMLBRElement and HTMLHRElement - Break elements
 *
 * Implements <br> and <hr> elements per HTML Living Standard.
 * Line breaks and horizontal rules.
 *
 * @see https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-br-element
 * @see https://html.spec.whatwg.org/multipage/grouping-content.html#the-hr-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTMLBRElement - line break
 *
 * The <br> element represents a line break.
 */
class HTMLBRElement : public HTMLElement {
public:
    HTMLBRElement();
    ~HTMLBRElement() override;

    // =========================================================================
    // Deprecated Attributes (for compatibility)
    // =========================================================================

    /// Clear floating elements (deprecated, use CSS clear)
    std::string clear() const;
    void setClear(const std::string& clear);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLHRElement - thematic break (horizontal rule)
 *
 * The <hr> element represents a thematic break between paragraph-level elements.
 */
class HTMLHRElement : public HTMLElement {
public:
    HTMLHRElement();
    ~HTMLHRElement() override;

    // =========================================================================
    // Deprecated Attributes (for compatibility)
    // =========================================================================

    /// Alignment (deprecated, use CSS)
    std::string align() const;
    void setAlign(const std::string& align);

    /// Color (deprecated, use CSS)
    std::string color() const;
    void setColor(const std::string& color);

    /// No shade/shadow (deprecated)
    bool noShade() const;
    void setNoShade(bool noShade);

    /// Size/height (deprecated, use CSS)
    std::string size() const;
    void setSize(const std::string& size);

    /// Width (deprecated, use CSS)
    std::string width() const;
    void setWidth(const std::string& width);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLWBRElement - word break opportunity
 *
 * The <wbr> element represents a line break opportunity.
 */
class HTMLWBRElement : public HTMLElement {
public:
    HTMLWBRElement();
    ~HTMLWBRElement() override;

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
