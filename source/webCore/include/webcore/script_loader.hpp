/**
 * @file script_loader.hpp
 * @brief Script loading with async/defer support per HTML5 spec
 */

#pragma once

#include "dom.hpp"
#include "resource_loader.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

namespace Zepra::WebCore {

class ScriptContext;  // Forward declaration

/**
 * @brief Script loading mode per HTML5 spec
 */
enum class ScriptLoadMode {
    Blocking,   // Classic - blocks parsing (no async/defer)
    Defer,      // defer - executes after DOM ready, in order
    Async,      // async - executes as soon as loaded, out of order
    Module      // type="module" - always deferred, strict mode
};

/**
 * @brief Represents a pending script to be loaded/executed
 */
struct PendingScript {
    std::string url;              // External script src (empty for inline)
    std::string inlineContent;    // Inline script content
    ScriptLoadMode mode = ScriptLoadMode::Blocking;
    bool isLoaded = false;
    bool isExecuted = false;
    bool isModule = false;
    DOMElement* element = nullptr;  // Reference to <script> element
    
    // For ordering deferred scripts
    size_t documentOrder = 0;
};

/**
 * @brief Callback for when script execution completes
 */
using ScriptCallback = std::function<void(bool success, const std::string& error)>;

/**
 * @brief ScriptLoader manages loading and execution order per HTML5 spec
 * 
 * Rules implemented:
 * 1. Inline scripts without async/defer block parsing
 * 2. External scripts without async/defer block parsing
 * 3. defer scripts run after DOMContentLoaded, in document order
 * 4. async scripts run when loaded, in any order
 * 5. Module scripts are always deferred
 */
class ScriptLoader {
public:
    explicit ScriptLoader(ScriptContext* context);
    ~ScriptLoader();
    
    /**
     * @brief Set resource loader for fetching external scripts
     */
    void setResourceLoader(ResourceLoader* loader);
    
    /**
     * @brief Process a script element during parsing
     * @param element The <script> DOM element
     * @param parserBlocked Output: true if parser should wait
     * @return true if script was handled
     */
    bool processScript(DOMElement* element, bool& parserBlocked);
    
    /**
     * @brief Execute any blocking script that's ready
     * Called when parser wants to continue
     */
    void executeBlockingScripts();
    
    /**
     * @brief Called when DOM parsing is complete
     * Executes all deferred scripts in order
     */
    void onDOMContentLoaded();
    
    /**
     * @brief Called when page load is complete (images, etc)
     */
    void onLoad();
    
    /**
     * @brief Pump async script execution
     * Call in main loop to process loaded async scripts
     */
    void processAsyncQueue();
    
    /**
     * @brief Check if any scripts are blocking parsing
     */
    bool isBlocked() const;
    
    /**
     * @brief Get count of pending scripts
     */
    size_t pendingCount() const;

private:
    /**
     * @brief Determine load mode from script attributes
     */
    ScriptLoadMode determineLoadMode(DOMElement* element) const;
    
    /**
     * @brief Load external script asynchronously
     */
    void loadExternalScript(PendingScript& script);
    
    /**
     * @brief Execute a script's content
     */
    bool executeScript(PendingScript& script);
    
    /**
     * @brief Get script content from element
     */
    std::string getInlineContent(DOMElement* element) const;

    ScriptContext* context_ = nullptr;
    ResourceLoader* resourceLoader_ = nullptr;
    
    // Pending scripts by category
    std::vector<PendingScript> blockingScripts_;  // Parser-blocking
    std::vector<PendingScript> deferredScripts_;  // Execute at DOMContentLoaded
    std::vector<PendingScript> asyncScripts_;     // Execute when loaded
    
    // Currently blocking script (if any)
    PendingScript* currentBlockingScript_ = nullptr;
    
    // Thread safety for async loading
    std::mutex asyncMutex_;
    std::vector<PendingScript*> readyAsyncScripts_;
    
    // Document order counter
    size_t documentOrder_ = 0;
    
    // State flags
    std::atomic<bool> domReady_{false};
    std::atomic<bool> loadComplete_{false};
};

/**
 * @brief HTML5 script insertion point tracker
 * Tracks where dynamically inserted scripts should run
 */
class ScriptInsertionPoint {
public:
    void push(DOMElement* insertionPoint);
    void pop();
    DOMElement* current() const;
    bool isActive() const;
    
private:
    std::vector<DOMElement*> stack_;
};

} // namespace Zepra::WebCore
