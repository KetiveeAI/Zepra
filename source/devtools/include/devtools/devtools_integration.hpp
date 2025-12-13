/**
 * @file devtools_integration.hpp
 * @brief Browser-DevTools Integration Layer
 * 
 * Provides the interface between the main browser and DevTools.
 * DevTools runs in background, window opens on demand (F12).
 */

#pragma once

#include <memory>
#include <thread>
#include <atomic>

// Forward declarations
namespace Zepra::Runtime {
    class VM;
    class Context;
}

namespace Zepra::Debug {
    class Debugger;
    class Console;
}

namespace Zepra::DevTools {

class DevToolsPanel;

/**
 * @brief Manages DevTools lifecycle and connection to browser
 * 
 * Usage from browser:
 *   DevToolsIntegration devtools_;
 *   
 *   // On F12:
 *   devtools_.show(activePage_->vm());
 *   
 *   // DevTools window closes:
 *   devtools_.hide();
 */
class DevToolsIntegration {
public:
    DevToolsIntegration();
    ~DevToolsIntegration();
    
    /**
     * @brief Show DevTools window
     * @param vm VM from active tab/page to debug
     */
    void show(Runtime::VM* vm);
    
    /**
     * @brief Hide DevTools window
     */
    void hide();
    
    /**
     * @brief Check if DevTools window is visible
     */
    bool isVisible() const { return visible_; }
    
    /**
     * @brief Update DevTools (call from browser main loop)
     */
    void update();
    
    /**
     * @brief Connect to different VM (tab switch)
     */
    void connectVM(Runtime::VM* vm);
    
    /**
     * @brief Notify DevTools of network request
     */
    void notifyNetworkRequest(int id, const std::string& url, 
                               const std::string& method, int status,
                               size_t size, double time);
    
    /**
     * @brief Get the internal panel (for advanced use)
     */
    DevToolsPanel* panel() { return panel_.get(); }
    
private:
    std::unique_ptr<DevToolsPanel> panel_;
    std::atomic<bool> visible_{false};
    Runtime::VM* currentVM_ = nullptr;
    
    // Window handle (SDL)
    void* windowHandle_ = nullptr;
    void* rendererHandle_ = nullptr;
};

// =============================================================================
// Inline Implementation
// =============================================================================

inline DevToolsIntegration::DevToolsIntegration() {}

inline DevToolsIntegration::~DevToolsIntegration() {
    hide();
}

inline void DevToolsIntegration::show(Runtime::VM* vm) {
    if (visible_) return;
    
    currentVM_ = vm;
    
    // Create panel connected to VM
    panel_ = std::make_unique<DevToolsPanel>(vm);
    panel_->connect();
    
    // Create SDL window for DevTools
    // Note: In production, this would create a separate SDL window
    // For now, DevTools panel handles its own window creation
    
    visible_ = true;
}

inline void DevToolsIntegration::hide() {
    if (!visible_) return;
    
    if (panel_) {
        panel_->disconnect();
        panel_.reset();
    }
    
    visible_ = false;
}

inline void DevToolsIntegration::update() {
    // DevTools update is handled by its own event loop
    // This is called from browser main loop if needed
}

inline void DevToolsIntegration::connectVM(Runtime::VM* vm) {
    if (panel_) {
        panel_->disconnect();
    }
    
    currentVM_ = vm;
    
    if (panel_ && visible_) {
        panel_ = std::make_unique<DevToolsPanel>(vm);
        panel_->connect();
    }
}

inline void DevToolsIntegration::notifyNetworkRequest(
    int id, const std::string& url, const std::string& method,
    int status, size_t size, double time) {
    
    if (panel_) {
        NetworkEntry entry;
        entry.id = id;
        entry.url = url;
        entry.method = method;
        entry.status = status;
        entry.responseSize = size;
        entry.endTime = time;
        panel_->addNetworkEntry(entry);
    }
}

} // namespace Zepra::DevTools
