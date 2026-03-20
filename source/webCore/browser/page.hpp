/**
 * @file page.hpp
 * @brief Page representation - combines DOM, styles, and render tree
 */

#pragma once

#include "dom.hpp"
#include "html/html_parser.hpp"
#include "css/css_engine.hpp" // Use new engine
#include "rendering/render_tree.hpp"
#include "rendering/layout_engine.hpp"
#include "rendering/paint_context.hpp"
#include "scripting/script_context.hpp"
#include "scripting/script_loader.hpp"
#include "browser/resource_loader.hpp"
#include <memory>
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Page loading state
 */
enum class PageState {
    Initial,
    Loading,
    Parsing,
    Styling,
    Layout,
    Painting,
    Complete,
    Error
};

/**
 * @brief Page load progress
 */
struct PageProgress {
    PageState state = PageState::Initial;
    float progress = 0.0f;  // 0-1
    std::string statusText;
};

/**
 * @brief A complete web page with DOM, styles, and rendering
 */
class Page {
public:
    Page();
    ~Page();
    
    // Load content
    void loadHTML(const std::string& html, const std::string& baseURL = "");
    void loadFromURL(const std::string& url);
    
    // Add styles
    void addStylesheet(const std::string& css);
    void addInlineStyle(DOMElement* element, const std::string& style);
    
    // Access
    DOMDocument* document() { return document_.get(); }
    const DOMDocument* document() const { return document_.get(); }
    RenderNode* renderTree() { return renderTree_.get(); }
    ScriptContext* scriptContext() { return scriptContext_.get(); }
    
    // Layout and paint
    void setViewport(float width, float height);
    void layout();
    void paint(DisplayList& displayList);
    
    // Content info
    const std::string& title() const { return title_; }
    const std::string& url() const { return url_; }
    
    // State
    PageState state() const { return state_; }
    const PageProgress& progress() const { return progress_; }
    bool isLoaded() const { return state_ == PageState::Complete; }
    
    // Scroll
    void scrollTo(float x, float y);
    void scrollBy(float dx, float dy);
    float scrollX() const { return scrollX_; }
    float scrollY() const { return scrollY_; }
    float contentWidth() const;
    float contentHeight() const;
    
    // Interaction
    DOMElement* elementFromPoint(float x, float y);
    RenderNode* hitTest(float x, float y);
    
    // Events
    void setOnLoad(std::function<void()> handler) { onLoad_ = handler; }
    void setOnProgress(std::function<void(const PageProgress&)> handler) { onProgress_ = handler; }
    
private:
    void buildRenderTree();
    std::unique_ptr<RenderNode> buildRenderNode(DOMElement* element, const ComputedStyle* parentStyle);
    void updateProgress(PageState state, float progress, const std::string& status);
    
    // Content
    std::unique_ptr<DOMDocument> document_;
    std::unique_ptr<RenderNode> renderTree_;
    std::unique_ptr<ScriptContext> scriptContext_;
    std::unique_ptr<ScriptLoader> scriptLoader_;
    
    // Styles
    StyleResolver styleResolver_;
    CSSParser cssParser_;
    std::vector<std::shared_ptr<CSSStyleSheet>> stylesheets_;
    
    // Layout
    LayoutEngine layoutEngine_;
    std::unique_ptr<ResourceLoader> resourceLoader_; // Added
    float viewportWidth_ = 800;
    float viewportHeight_ = 600;
    
    // Scroll
    float scrollX_ = 0;
    float scrollY_ = 0;
    
    // Metadata
    std::string title_;
    std::string url_;
    std::string baseURL_;
    
    // State
    PageState state_ = PageState::Initial;
    PageProgress progress_;
    
    // Callbacks
    std::function<void()> onLoad_;
    std::function<void(const PageProgress&)> onProgress_;
};

/**
 * @brief Frame - an iframe or main frame
 */
class Frame {
public:
    Frame(Page* page);
    
    Page* page() { return page_; }
    const Rect& bounds() const { return bounds_; }
    void setBounds(const Rect& bounds) { bounds_ = bounds; }
    
    // Navigation
    void navigate(const std::string& url);
    void reload();
    void stop();
    
    // Rendering
    void layout();
    void paint(DisplayList& displayList);
    
private:
    Page* page_;
    Rect bounds_;
    std::string url_;
};

/**
 * @brief Viewport - manages scrolling and zoom
 */
class Viewport {
public:
    Viewport(float width, float height);
    
    // Dimensions
    void resize(float width, float height);
    float width() const { return width_; }
    float height() const { return height_; }
    
    // Scroll
    void scrollTo(float x, float y);
    void scrollBy(float dx, float dy);
    float scrollX() const { return scrollX_; }
    float scrollY() const { return scrollY_; }
    void setScrollBounds(float maxX, float maxY);
    
    // Zoom
    void setZoom(float zoom);
    float zoom() const { return zoom_; }
    
    // Visible area
    Rect visibleRect() const;
    
    // Transform point from viewport to content coords
    void viewportToContent(float& x, float& y) const;
    void contentToViewport(float& x, float& y) const;
    
private:
    float width_, height_;
    float scrollX_ = 0, scrollY_ = 0;
    float maxScrollX_ = 0, maxScrollY_ = 0;
    float zoom_ = 1.0f;
};

} // namespace Zepra::WebCore
