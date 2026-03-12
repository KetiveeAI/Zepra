/**
 * @file html_link_element.hpp
 * @brief HTMLLinkElement - External resource link element
 *
 * Implements the <link> element per HTML Living Standard.
 * Used for linking external stylesheets, icons, and other resources.
 *
 * @see https://html.spec.whatwg.org/multipage/semantics.html#the-link-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <functional>

namespace Zepra::WebCore {

// Forward declarations
class StyleSheet;
class CSSStyleSheet;

/**
 * @brief Link relation types
 */
enum class LinkRelType {
    Stylesheet,
    Icon,
    Preload,
    Prefetch,
    Preconnect,
    DnsPrefetch,
    Modulepreload,
    Manifest,
    Alternate,
    Canonical,
    Author,
    Help,
    License,
    Next,
    Prev,
    Search,
    Other
};

/**
 * @brief Link loading state
 */
enum class LinkLoadState {
    Idle,
    Loading,
    Loaded,
    Error
};

/**
 * @brief HTMLLinkElement - external resource links
 *
 * The <link> element allows authors to link their document to other resources.
 * Most commonly used for stylesheets and icons.
 */
class HTMLLinkElement : public HTMLElement {
public:
    HTMLLinkElement();
    ~HTMLLinkElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// URL of the linked resource
    std::string href() const;
    void setHref(const std::string& href);

    /// Relationship between document and linked resource
    std::string rel() const;
    void setRel(const std::string& rel);

    /// MIME type of the linked resource
    std::string type() const;
    void setType(const std::string& type);

    /// Media query for when resource applies
    std::string media() const;
    void setMedia(const std::string& media);

    /// Hint for expected sizes (icons)
    std::string sizes() const;
    void setSizes(const std::string& sizes);

    /// Hint for image pixel densities (icons)
    std::string imagesrcset() const;
    void setImagesrcset(const std::string& srcset);

    std::string imagesizes() const;
    void setImagesizes(const std::string& sizes);

    // =========================================================================
    // Loading Attributes
    // =========================================================================

    /// Whether to fetch in CORS mode
    std::string crossOrigin() const;
    void setCrossOrigin(const std::string& value);

    /// How to fetch the resource (preload hint)
    std::string as() const;
    void setAs(const std::string& value);

    /// Referrer policy for fetches
    std::string referrerPolicy() const;
    void setReferrerPolicy(const std::string& policy);

    /// Integrity hash for subresource integrity
    std::string integrity() const;
    void setIntegrity(const std::string& hash);

    /// Language hint (for stylesheets)
    std::string hreflang() const;
    void setHreflang(const std::string& lang);

    /// Fetch priority hint
    std::string fetchpriority() const;
    void setFetchpriority(const std::string& priority);

    /// Blocking behavior
    std::string blocking() const;
    void setBlocking(const std::string& value);

    /// Whether resource is disabled
    bool disabled() const;
    void setDisabled(bool disabled);

    // =========================================================================
    // Resource Loading
    // =========================================================================

    /// Current loading state
    LinkLoadState loadState() const;

    /// Whether the linked stylesheet is loaded
    bool isLoaded() const;

    /// Associated stylesheet (if rel=stylesheet)
    CSSStyleSheet* sheet() const;

    /// Initiate loading of the resource
    void load();

    /// Abort loading
    void abort();

    // =========================================================================
    // Rel List (DOMTokenList-like)
    // =========================================================================

    /// Check if rel contains value
    bool relContains(const std::string& token) const;

    /// Add rel value
    void relAdd(const std::string& token);

    /// Remove rel value
    void relRemove(const std::string& token);

    /// Toggle rel value
    bool relToggle(const std::string& token);

    // =========================================================================
    // Convenience Methods
    // =========================================================================

    /// Whether this is a stylesheet link
    bool isStylesheet() const;

    /// Whether this is an icon link
    bool isIcon() const;

    /// Whether this is a preload/prefetch link
    bool isPreload() const;

    /// Parsed rel type
    LinkRelType relType() const;

    // =========================================================================
    // Event Handlers
    // =========================================================================

    using LoadCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    void setOnLoad(LoadCallback callback);
    void setOnError(ErrorCallback callback);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
