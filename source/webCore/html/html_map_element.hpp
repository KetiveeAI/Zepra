/**
 * @file html_map_element.hpp
 * @brief HTMLMapElement and HTMLAreaElement - Image maps
 *
 * Implements <map> and <area> elements per HTML Living Standard.
 * Used for clickable regions on images.
 *
 * @see https://html.spec.whatwg.org/multipage/image-maps.html
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Zepra::WebCore {

// Forward declarations
class HTMLAreaElement;
class HTMLImageElement;
class HTMLCollection;

/**
 * @brief HTMLMapElement - image map
 *
 * The <map> element is used with <area> elements to define an image map
 * (clickable areas on an image).
 */
class HTMLMapElement : public HTMLElement {
public:
    HTMLMapElement();
    ~HTMLMapElement() override;

    // =========================================================================
    // Core Attribute
    // =========================================================================

    /// Name of the map (matches usemap on img)
    std::string name() const;
    void setName(const std::string& name);

    // =========================================================================
    // Areas
    // =========================================================================

    /// Collection of area elements
    HTMLCollection* areas() const;

    /// Get all area elements as a vector
    std::vector<HTMLAreaElement*> areaElements() const;

    // =========================================================================
    // Associated Images
    // =========================================================================

    /// Images using this map
    std::vector<HTMLImageElement*> images() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Area shape types
 */
enum class AreaShape {
    Default,    // Entire image
    Rect,       // Rectangle: x1,y1,x2,y2
    Circle,     // Circle: x,y,radius
    Poly        // Polygon: x1,y1,x2,y2,...
};

/**
 * @brief HTMLAreaElement - image map area
 *
 * The <area> element defines a hot-spot region on an image map.
 */
class HTMLAreaElement : public HTMLElement {
public:
    HTMLAreaElement();
    ~HTMLAreaElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Alternate text for the area
    std::string alt() const;
    void setAlt(const std::string& alt);

    /// Coordinates of the area
    std::string coords() const;
    void setCoords(const std::string& coords);

    /// URL to navigate to
    std::string href() const;
    void setHref(const std::string& href);

    /// Shape of the area
    std::string shape() const;
    void setShape(const std::string& shape);

    /// Target browsing context
    std::string target() const;
    void setTarget(const std::string& target);

    // =========================================================================
    // Download
    // =========================================================================

    /// Download filename
    std::string download() const;
    void setDownload(const std::string& download);

    /// Ping URLs for tracking
    std::string ping() const;
    void setPing(const std::string& ping);

    // =========================================================================
    // Relationships
    // =========================================================================

    /// Relationship of linked resource
    std::string rel() const;
    void setRel(const std::string& rel);

    /// Referrer policy
    std::string referrerPolicy() const;
    void setReferrerPolicy(const std::string& policy);

    // =========================================================================
    // Computed Properties
    // =========================================================================

    /// Parsed shape type
    AreaShape shapeType() const;

    /// Parsed coordinate array
    std::vector<int> coordsArray() const;

    /// Full resolved URL
    std::string resolvedHref() const;

    /// Check if a point is inside this area
    bool containsPoint(int x, int y) const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
