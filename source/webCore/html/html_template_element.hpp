/**
 * @file html_template_element.hpp
 * @brief HTMLTemplateElement - Content template
 *
 * Implements the <template> element per HTML Living Standard.
 * Holds HTML that is not rendered but can be instantiated via JavaScript.
 *
 * @see https://html.spec.whatwg.org/multipage/scripting.html#the-template-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

// Forward declarations
class DocumentFragment;

/**
 * @brief HTMLTemplateElement - content template
 *
 * The <template> element is a mechanism for holding HTML that is not
 * rendered when a page is loaded but may be instantiated subsequently
 * during runtime using JavaScript.
 */
class HTMLTemplateElement : public HTMLElement {
public:
    HTMLTemplateElement();
    ~HTMLTemplateElement() override;

    // =========================================================================
    // Template Content
    // =========================================================================

    /// The template's DocumentFragment content
    DocumentFragment* content() const;

    // =========================================================================
    // Declarative Shadow DOM (Modern)
    // =========================================================================

    /// Shadow root mode for declarative shadow DOM
    std::string shadowRootMode() const;
    void setShadowRootMode(const std::string& mode);

    /// Whether shadow root delegates focus
    bool shadowRootDelegatesFocus() const;
    void setShadowRootDelegatesFocus(bool delegates);

    /// Whether shadow root is clonable
    bool shadowRootClonable() const;
    void setShadowRootClonable(bool clonable);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
