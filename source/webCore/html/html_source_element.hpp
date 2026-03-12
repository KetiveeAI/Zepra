/**
 * @file html_source_element.hpp
 * @brief HTMLSourceElement - Media source
 *
 * Implements the <source> element per HTML Living Standard.
 * Specifies alternative media sources for <audio>, <video>, and <picture>.
 *
 * @see https://html.spec.whatwg.org/multipage/embedded-content.html#the-source-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTMLSourceElement - media source
 *
 * The <source> element allows authors to specify multiple alternative
 * media resources for media elements (<video>, <audio>) and picture
 * elements (<picture>).
 */
class HTMLSourceElement : public HTMLElement {
public:
    HTMLSourceElement();
    ~HTMLSourceElement() override;

    // =========================================================================
    // Core Attributes (for all contexts)
    // =========================================================================

    /// URL of the media resource
    std::string src() const;
    void setSrc(const std::string& src);

    /// MIME type of the resource
    std::string type() const;
    void setType(const std::string& type);

    // =========================================================================
    // Picture Element Attributes
    // =========================================================================

    /// Responsive image sources
    std::string srcset() const;
    void setSrcset(const std::string& srcset);

    /// Source sizes for responsive images
    std::string sizes() const;
    void setSizes(const std::string& sizes);

    /// Media query for this source
    std::string media() const;
    void setMedia(const std::string& media);

    /// Image width (for lazy loading optimization)
    int width() const;
    void setWidth(int width);

    /// Image height (for lazy loading optimization)
    int height() const;
    void setHeight(int height);

    // =========================================================================
    // Context Detection
    // =========================================================================

    /// Whether this source is inside a picture element
    bool isInPicture() const;

    /// Whether this source is inside a media element
    bool isInMedia() const;

    /// Whether the media type matches current device
    bool mediaMatches() const;

    /// Whether the type is supported
    bool typeSupported() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
