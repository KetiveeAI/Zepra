/**
 * @file browser_context.hpp
 * @brief Per-page browser execution context
 */

#pragma once

#include "dom_bridge.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace Zepra {
    class ScriptEngine;
}

namespace Zepra::WebCore {
    class DOMDocument;
    class PageRenderer;
}

namespace Zepra::Integration {

/**
 * @brief Script tag info
 */
struct ScriptInfo {
    std::string source;
    std::string src;      // External URL
    bool async = false;
    bool defer = false;
    bool module = false;
    int order = 0;
};

/**
 * @brief Timer info
 */
struct TimerInfo {
    uint32_t id;
    std::function<void()> callback;
    uint64_t triggerTime;
    uint32_t interval;
    bool repeat;
};

/**
 * @brief Complete browser execution context for a page
 */
class BrowserContext {
public:
    BrowserContext();
    ~BrowserContext();
    
    /**
     * @brief Initialize with document and renderer
     */
    void initialize(WebCore::DOMDocument* doc, WebCore::PageRenderer* renderer);
    
    /**
     * @brief Get the script engine
     */
    ScriptEngine* engine() const { return engine_.get(); }
    
    /**
     * @brief Get the DOM bridge
     */
    DOMBridge* bridge() const { return bridge_.get(); }
    
    /**
     * @brief Collect scripts from document
     */
    std::vector<ScriptInfo> collectScripts();
    
    /**
     * @brief Execute collected scripts
     */
    void executeScripts();
    
    /**
     * @brief Execute inline script
     */
    void executeScript(const std::string& source, const std::string& filename = "<script>");
    
    /**
     * @brief Handle DOMContentLoaded
     */
    void fireDOMContentLoaded();
    
    /**
     * @brief Handle window load
     */
    void fireLoad();
    
    /**
     * @brief Process event loop tick
     */
    void tick(double timestamp);
    
    /**
     * @brief Check if context has pending work
     */
    bool hasPendingWork() const;
    
    // Timer management
    uint32_t setTimeout(std::function<void()> callback, uint32_t delay);
    uint32_t setInterval(std::function<void()> callback, uint32_t delay);
    void clearTimeout(uint32_t id);
    
    // Animation frames
    uint32_t requestAnimationFrame(std::function<void(double)> callback);
    void cancelAnimationFrame(uint32_t id);
    
private:
    std::unique_ptr<ScriptEngine> engine_;
    std::unique_ptr<DOMBridge> bridge_;
    WebCore::DOMDocument* document_ = nullptr;
    WebCore::PageRenderer* renderer_ = nullptr;
    
    // Timers
    std::vector<TimerInfo> timers_;
    uint32_t nextTimerId_ = 1;
    
    // Animation frames
    std::vector<std::pair<uint32_t, std::function<void(double)>>> animFrames_;
    uint32_t nextAnimFrameId_ = 1;
    
    void setupGlobals();
};

/**
 * @brief Factory for browser contexts
 */
class BrowserContextFactory {
public:
    static std::unique_ptr<BrowserContext> create();
    static std::unique_ptr<BrowserContext> createForPage(
        WebCore::DOMDocument* doc, 
        WebCore::PageRenderer* renderer);
};

} // namespace Zepra::Integration
