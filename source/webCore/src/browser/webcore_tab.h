/**
 * @file webcore_tab.h
 * @brief Per-Tab WebCore Instance
 * 
 * Each tab owns its own WebCore instance with isolated:
 * - DOM tree (HTML elements)
 * - CSS engine (styles)
 * - JavaScript VM (ZebraScript context)
 * - Layout tree (computed boxes)
 * 
 * Rendering is delegated to NXRender (nxgfx + nxsvg).
 */

#pragma once

#include <string>
#include <memory>

// WebCore components
#ifdef USE_WEBCORE
namespace Zepra::WebCore {
    class DOMDocument;
    class CSSEngine;
    class ScriptContext;
}
#endif

// Layout engine
#include "layout_engine.h"

// Forward declarations
namespace NXRender {
    class GpuContext;
}

namespace ZepraBrowser {

/**
 * @brief Per-tab WebCore instance
 * 
 * Owns all state for a single browser tab including DOM, CSS, and JavaScript.
 * Isolated from other tabs to prevent cross-tab contamination.
 */
class WebCoreTab {
public:
    /**
     * @brief Create a new tab instance
     * @param url Initial URL to load
     */
    explicit WebCoreTab(const std::string& url = "zepra://start");
    
    /**
     * @brief Destroy tab and cleanup all resources
     */
    ~WebCoreTab();
    
    // Lifecycle
    void loadURL(const std::string& url);
    void reload();
    void stop();
    
    // Rendering (delegates to NXRender)
    void render(NXRender::GpuContext* gpu, float x, float y, float width, float height);
    
    // JavaScript execution
    bool executeScript(const std::string& code);
    
    // Properties
    std::string title() const;
    std::string url() const { return url_; }
    bool isLoading() const { return isLoading_; }
    std::string getError() const { return loadError_; }
    
    // Layout access (for hit-testing, text selection)
    LayoutBox* layoutRoot() { return layoutRoot_.get(); }
    const LayoutBox* layoutRoot() const { return layoutRoot_.get(); }
    
private:
    void parseHTML(const std::string& html);
    void buildLayoutTree();
    void paintLayoutBox(NXRender::GpuContext* gpu, const LayoutBox& box, 
                        float offsetX, float offsetY, float maxWidth, float maxHeight, float scrollY);
    
#ifdef USE_WEBCORE
    // Layout tree building helpers
    void buildLayoutFromElement(Zepra::WebCore::DOMElement* element, LayoutBox* parent);
    static std::string normalizeWhitespace(const std::string& text);
    static uint32_t cssColorToUint32(const Zepra::WebCore::CSSColor& color);
    static size_t countLayoutBoxes(const LayoutBox& box);
#endif
    
    std::string url_;
    std::string pageContent_;
    std::string loadError_;
    bool isLoading_ = false;
    
#ifdef USE_WEBCORE
    // Per-tab WebCore instances (isolated)
    std::unique_ptr<Zepra::WebCore::DOMDocument> document_;
    std::unique_ptr<Zepra::WebCore::CSSEngine> cssEngine_;
    std::unique_ptr<Zepra::WebCore::ScriptContext> scriptContext_;
#endif
    
    // Layout tree (rebuilt on navigation)
    std::unique_ptr<LayoutBox> layoutRoot_;
};

} // namespace ZepraBrowser
