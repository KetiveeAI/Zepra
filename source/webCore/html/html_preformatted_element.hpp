/**
 * @file html_preformatted_element.hpp
 * @brief HTMLPreElement and code-related elements
 *
 * Implements <pre>, <code>, <kbd>, <samp>, <var> elements.
 * Preformatted text and code semantics.
 *
 * @see https://html.spec.whatwg.org/multipage/grouping-content.html#the-pre-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTMLPreElement - preformatted text
 *
 * The <pre> element represents a block of preformatted text,
 * where whitespace is significant.
 */
class HTMLPreElement : public HTMLElement {
public:
    HTMLPreElement();
    ~HTMLPreElement() override;

    // =========================================================================
    // Deprecated Attributes (for compatibility)
    // =========================================================================

    /// Width in characters (deprecated)
    int width() const;
    void setWidth(int width);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLCodeElement - code fragment
 *
 * The <code> element represents a fragment of computer code.
 */
class HTMLCodeElement : public HTMLElement {
public:
    HTMLCodeElement();
    ~HTMLCodeElement() override;

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLKbdElement - keyboard input
 *
 * The <kbd> element represents user input (typically keyboard).
 */
class HTMLKbdElement : public HTMLElement {
public:
    HTMLKbdElement();
    ~HTMLKbdElement() override;

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLSampElement - sample output
 *
 * The <samp> element represents sample or quoted output from a program.
 */
class HTMLSampElement : public HTMLElement {
public:
    HTMLSampElement();
    ~HTMLSampElement() override;

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLVarElement - variable
 *
 * The <var> element represents a variable in a mathematical expression
 * or programming context.
 */
class HTMLVarElement : public HTMLElement {
public:
    HTMLVarElement();
    ~HTMLVarElement() override;

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
