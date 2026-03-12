/**
 * @file html_body_element.hpp
 * @brief HTMLBodyElement - Document body container
 *
 * Implements the <body> element per HTML Living Standard.
 * Contains the main renderable content of the document.
 *
 * @see https://html.spec.whatwg.org/multipage/sections.html#the-body-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief HTMLBodyElement - document body container
 *
 * The <body> element represents the contents of the document.
 */
class HTMLBodyElement : public HTMLElement {
public:
    HTMLBodyElement();
    ~HTMLBodyElement() override;

    // =========================================================================
    // Window Event Handlers (reflected from window)
    // =========================================================================

    using EventCallback = std::function<void(Event*)>;

    /// Handler for window load events
    void setOnLoad(EventCallback callback);
    EventCallback onLoad() const;

    /// Handler for before unload
    void setOnBeforeUnload(EventCallback callback);
    EventCallback onBeforeUnload() const;

    /// Handler for unload
    void setOnUnload(EventCallback callback);
    EventCallback onUnload() const;

    /// Handler for offline event
    void setOnOffline(EventCallback callback);
    EventCallback onOffline() const;

    /// Handler for online event
    void setOnOnline(EventCallback callback);
    EventCallback onOnline() const;

    /// Handler for page hide
    void setOnPageHide(EventCallback callback);
    EventCallback onPageHide() const;

    /// Handler for page show
    void setOnPageShow(EventCallback callback);
    EventCallback onPageShow() const;

    /// Handler for popstate
    void setOnPopState(EventCallback callback);
    EventCallback onPopState() const;

    /// Handler for storage event
    void setOnStorage(EventCallback callback);
    EventCallback onStorage() const;

    /// Handler for hash change
    void setOnHashChange(EventCallback callback);
    EventCallback onHashChange() const;

    /// Handler for message events
    void setOnMessage(EventCallback callback);
    EventCallback onMessage() const;

    /// Handler for error events
    void setOnError(EventCallback callback);
    EventCallback onError() const;

    /// Handler for resize
    void setOnResize(EventCallback callback);
    EventCallback onResize() const;

    /// Handler for scroll
    void setOnScroll(EventCallback callback);
    EventCallback onScroll() const;

    // =========================================================================
    // Deprecated Presentational Attributes (for compatibility)
    // Modern CSS should be used instead
    // =========================================================================

    // These are included only for legacy content compatibility
    // and should not be used in new content

    /// Background color (deprecated, use CSS)
    std::string bgColor() const;
    void setBgColor(const std::string& color);

    /// Text color (deprecated, use CSS)
    std::string text() const;
    void setText(const std::string& color);

    /// Link color (deprecated, use CSS)
    std::string link() const;
    void setLink(const std::string& color);

    /// Visited link color (deprecated, use CSS)
    std::string vLink() const;
    void setVLink(const std::string& color);

    /// Active link color (deprecated, use CSS)
    std::string aLink() const;
    void setALink(const std::string& color);

    /// Background image (deprecated, use CSS)
    std::string background() const;
    void setBackground(const std::string& url);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
