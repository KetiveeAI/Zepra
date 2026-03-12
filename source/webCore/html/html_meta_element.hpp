/**
 * @file html_meta_element.hpp
 * @brief HTMLMetaElement - Document metadata element
 *
 * Implements the <meta> element per HTML Living Standard.
 * Used for charset, viewport, description, and other metadata.
 *
 * @see https://html.spec.whatwg.org/multipage/semantics.html#the-meta-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief Common meta name values
 */
enum class MetaName {
    Application,        // application-name
    Author,            // author
    Description,       // description
    Generator,         // generator
    Keywords,          // keywords
    Referrer,          // referrer
    ThemeColor,        // theme-color
    ColorScheme,       // color-scheme
    Viewport,          // viewport
    Robots,            // robots
    Other
};

/**
 * @brief Viewport settings parsed from meta viewport
 */
struct ViewportSettings {
    float width = -1;           // Device width or specific value
    float height = -1;          // Device height or specific value
    float initialScale = 1.0f;  // Initial zoom level
    float minimumScale = 0.1f;  // Minimum zoom
    float maximumScale = 10.0f; // Maximum zoom
    bool userScalable = true;   // Allow user zoom
    bool widthDeviceWidth = false;  // width=device-width
    bool heightDeviceHeight = false; // height=device-height
};

/**
 * @brief HTMLMetaElement - document metadata
 *
 * The <meta> element represents various kinds of metadata that cannot
 * be expressed using other HTML elements.
 */
class HTMLMetaElement : public HTMLElement {
public:
    HTMLMetaElement();
    ~HTMLMetaElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Name of the metadata (name attribute)
    std::string name() const;
    void setName(const std::string& name);

    /// Value of the metadata (content attribute)
    std::string content() const;
    void setContent(const std::string& content);

    /// HTTP-equiv header name
    std::string httpEquiv() const;
    void setHttpEquiv(const std::string& equiv);

    /// Character encoding declaration
    std::string charset() const;
    void setCharset(const std::string& charset);

    /// Media query for theme-color
    std::string media() const;
    void setMedia(const std::string& media);

    // =========================================================================
    // Convenience Methods
    // =========================================================================

    /// Parsed name type
    MetaName nameType() const;

    /// Whether this sets charset
    bool isCharset() const;

    /// Whether this is a viewport meta
    bool isViewport() const;

    /// Whether this is an http-equiv
    bool isHttpEquiv() const;

    /// Parse viewport content into settings
    ViewportSettings parseViewport() const;

    /// Get theme-color value (if name=theme-color)
    std::string themeColor() const;

    /// Get color-scheme value (if name=color-scheme)
    std::string colorScheme() const;

    // =========================================================================
    // HTTP-Equiv Actions
    // =========================================================================

    /// Whether this triggers a refresh
    bool isRefresh() const;

    /// Get refresh timeout (if http-equiv=refresh)
    int refreshTimeout() const;

    /// Get refresh URL (if http-equiv=refresh with URL)
    std::string refreshUrl() const;

    /// Whether this sets Content-Security-Policy
    bool isCSP() const;

    /// Get CSP policy string
    std::string cspPolicy() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
