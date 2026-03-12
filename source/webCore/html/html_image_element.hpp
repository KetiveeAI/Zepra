/**
 * @file html_image_element.hpp
 * @brief HTMLImageElement interface for <img> elements
 *
 * Implements image embedding per HTML Living Standard.
 *
 * @see https://html.spec.whatwg.org/multipage/embedded-content.html#the-img-element
 */

#pragma once

#include "html/html_element.hpp"

namespace Zepra::WebCore {

/**
 * @brief Image loading states
 */
enum class ImageLoadingState {
    Unavailable,    ///< No image data
    Loading,        ///< Image is loading
    Complete,       ///< Image loaded successfully
    Broken          ///< Image failed to load
};

/**
 * @brief Lazy loading options
 */
enum class LazyLoading {
    Eager,  ///< Load immediately
    Lazy    ///< Defer loading until near viewport
};

/**
 * @brief Image decoding hint
 */
enum class ImageDecoding {
    Auto,   ///< Browser decides
    Sync,   ///< Decode synchronously
    Async   ///< Decode asynchronously
};

/**
 * @brief HTMLImageElement represents an <img> element
 *
 * Provides properties and methods for embedded images,
 * including sources, dimensions, and loading state.
 */
class HTMLImageElement : public HTMLElement {
public:
    HTMLImageElement();
    ~HTMLImageElement() override;

    // =========================================================================
    // Source Properties
    // =========================================================================

    /// Image URL
    std::string src() const;
    void setSrc(const std::string& src);

    /// Srcset for responsive images (comma-separated)
    std::string srcset() const;
    void setSrcset(const std::string& srcset);

    /// Sizes for responsive images
    std::string sizes() const;
    void setSizes(const std::string& sizes);

    /// Current source (selected from srcset)
    std::string currentSrc() const;

    /// Alternative text
    std::string alt() const;
    void setAlt(const std::string& alt);

    // =========================================================================
    // Dimension Properties
    // =========================================================================

    /// Display width (attribute)
    unsigned int width() const;
    void setWidth(unsigned int width);

    /// Display height (attribute)
    unsigned int height() const;
    void setHeight(unsigned int height);

    /// Natural/intrinsic width (read-only)
    unsigned int naturalWidth() const;

    /// Natural/intrinsic height (read-only)
    unsigned int naturalHeight() const;

    // =========================================================================
    // Loading Properties
    // =========================================================================

    /// Loading behavior (eager/lazy)
    std::string loading() const;
    void setLoading(const std::string& loading);

    /// Decoding hint
    std::string decoding() const;
    void setDecoding(const std::string& decoding);

    /// Fetch priority hint
    std::string fetchPriority() const;
    void setFetchPriority(const std::string& priority);

    /// Whether image is fully loaded
    bool complete() const;

    /// X position within image map
    int x() const;

    /// Y position within image map
    int y() const;

    // =========================================================================
    // Cross-Origin
    // =========================================================================

    /// Cross-origin mode
    std::string crossOrigin() const;
    void setCrossOrigin(const std::string& mode);

    /// Referrer policy
    std::string referrerPolicy() const;
    void setReferrerPolicy(const std::string& policy);

    /// Use map name
    std::string useMap() const;
    void setUseMap(const std::string& map);

    /// Is server-side image map
    bool isMap() const;
    void setIsMap(bool isMap);

    // =========================================================================
    // Methods
    // =========================================================================

    /// Decode image asynchronously (returns promise-like)
    void decode(std::function<void(bool success)> callback);

    /// Clone node
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

    // =========================================================================
    // Event Handlers
    // =========================================================================

    void setOnLoad(EventListener callback);
    void setOnError(EventListener callback);
    void setOnAbort(EventListener callback);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
