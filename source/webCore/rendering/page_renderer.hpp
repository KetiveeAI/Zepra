/**
 * @file page_renderer.hpp
 * @brief High-level page rendering orchestration
 */

#pragma once

#include "rendering/compositor.hpp"
#include "rendering/layout_engine.hpp"
#include "css/css_engine.hpp"
#include "browser/dom.hpp"
#include "rendering/render_tree.hpp"
#include <memory>
#include <functional>
#include <chrono>

namespace Zepra::WebCore {

// Forward declare style adapter
class StyleResolverAdapter;

/**
 * @brief Frame timing and performance metrics
 */
struct FrameMetrics {
    double styleTime = 0;      // CSS resolution time (ms)
    double layoutTime = 0;     // Layout time (ms)
    double paintTime = 0;      // Paint time (ms)
    double compositeTime = 0;  // Composite time (ms)
    double totalTime = 0;      // Total frame time (ms)
    size_t layerCount = 0;     // Number of layers
    size_t paintedNodes = 0;   // Nodes painted
    size_t displayCommands = 0; // Display list commands
    double fps = 0;            // Frames per second
};

/**
 * @brief Rendering configuration
 */
struct RenderConfig {
    bool enableCompositing = true;
    bool enableLayerCaching = true;
    bool enableIncrementalLayout = true;
    bool enableDebugOverlay = false;
    int maxTextureSize = 4096;
    int targetFPS = 60;
};

/**
 * @brief High-level page renderer orchestrating the full pipeline
 * 
 * Pipeline: DOM → Style → Layout → Paint → Composite → Display
 */
class PageRenderer {
public:
    PageRenderer();
    ~PageRenderer();
    
    /**
     * @brief Set the DOM document to render
     */
    void setDocument(DOMDocument* doc);
    
    /**
     * @brief Set viewport dimensions
     */
    void setViewport(int width, int height);
    
    /**
     * @brief Set render backend
     */
    void setBackend(RenderBackend* backend);
    
    /**
     * @brief Get CSS engine for style manipulation
     */
    CSSEngine& cssEngine() { return cssEngine_; }
    
    /**
     * @brief Add stylesheet
     */
    void addStyleSheet(const std::string& cssText, StyleOrigin origin = StyleOrigin::Author);
    
    /**
     * @brief Build render tree from DOM
     */
    void buildRenderTree();
    
    /**
     * @brief Perform full layout
     */
    void layout();
    
    /**
     * @brief Paint to display list
     */
    void paint();
    
    /**
     * @brief Composite layers
     */
    void composite();
    
    /**
     * @brief Full frame render
     */
    void render();
    
    /**
     * @brief Render only dirty regions
     */
    void renderIncremental();
    
    /**
     * @brief Invalidate element for re-render
     */
    void invalidate(DOMElement* element);
    
    /**
     * @brief Invalidate entire document
     */
    void invalidateAll();
    
    /**
     * @brief Get last frame metrics
     */
    const FrameMetrics& metrics() const { return metrics_; }
    
    /**
     * @brief Get/set configuration
     */
    RenderConfig& config() { return config_; }
    const RenderConfig& config() const { return config_; }
    
    /**
     * @brief Get render tree root
     */
    RenderNode* renderRoot() const { return renderRoot_.get(); }
    
    /**
     * @brief Get compositor
     */
    Compositor& compositor() { return compositor_; }
    
    /**
     * @brief Hit testing
     */
    RenderNode* hitTest(float x, float y);
    DOMElement* elementAtPoint(float x, float y);
    
    /**
     * @brief Scroll handling
     */
    void scrollTo(float x, float y);
    void scrollBy(float dx, float dy);
    float scrollX() const { return scrollX_; }
    float scrollY() const { return scrollY_; }
    
    // Callbacks
    using FrameCallback = std::function<void(const FrameMetrics&)>;
    void setFrameCallback(FrameCallback cb) { frameCallback_ = std::move(cb); }
    
private:
    // Build render node from DOM element
    std::unique_ptr<RenderNode> buildRenderNode(DOMElement* element);
    
    // Apply styles to render tree
    void applyStyles();
    
    // Timing helper
    double now() const;
    
    DOMDocument* document_ = nullptr;
    RenderBackend* backend_ = nullptr;
    
    CSSEngine cssEngine_;
    LayoutEngine layoutEngine_;
    Compositor compositor_;
    
    std::unique_ptr<RenderNode> renderRoot_;
    
    int viewportWidth_ = 800;
    int viewportHeight_ = 600;
    float scrollX_ = 0, scrollY_ = 0;
    
    FrameMetrics metrics_;
    RenderConfig config_;
    FrameCallback frameCallback_;
    
    std::chrono::steady_clock::time_point lastFrame_;
};

/**
 * @brief Render tree builder
 */
class RenderTreeBuilder {
public:
    static std::unique_ptr<RenderNode> build(DOMDocument* doc);
    static std::unique_ptr<RenderNode> buildElement(DOMElement* element);
    static std::unique_ptr<RenderText> buildText(DOMText* text);
};

} // namespace Zepra::WebCore
