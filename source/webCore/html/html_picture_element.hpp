/**
 * @file html_picture_element.hpp
 * @brief HTMLPictureElement - Responsive images
 *
 * Implements the <picture> element per HTML Living Standard.
 * Provides responsive image solutions with multiple sources.
 *
 * @see https://html.spec.whatwg.org/multipage/embedded-content.html#the-picture-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Zepra::WebCore {

// Forward declarations
class HTMLSourceElement;
class HTMLImageElement;

/**
 * @brief HTMLPictureElement - responsive images
 *
 * The <picture> element contains zero or more <source> elements and one 
 * <img> element to offer alternative versions of an image for different
 * display/device scenarios.
 */
class HTMLPictureElement : public HTMLElement {
public:
    HTMLPictureElement();
    ~HTMLPictureElement() override;

    // =========================================================================
    // Child Access
    // =========================================================================

    /// Get all source elements
    std::vector<HTMLSourceElement*> sources() const;

    /// Get the img element
    HTMLImageElement* image() const;

    // =========================================================================
    // Source Selection
    // =========================================================================

    /// Get the currently selected source (based on media queries and type support)
    HTMLSourceElement* selectedSource() const;

    /// Get the effective image URL after source selection
    std::string effectiveSrc() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
