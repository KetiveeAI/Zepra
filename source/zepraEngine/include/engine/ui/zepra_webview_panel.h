/**
 * @file devtools_panel.h
 * @brief Professional DevTools Panel UI (like Firefox/Safari)
 * 
 * Unified developer tools interface with:
 * - Console (JS logs)
 * - Network (requests/responses) 
 * - Elements (DOM tree)
 * - Security (TLS, CORS)
 * - Performance (timing)
 */

#ifndef DEVTOOLS_PANEL_H
#define DEVTOOLS_PANEL_H

#include <string>
#include <vector>
#include <functional>
#include "engine/console_log.h"
#include "network/network_monitor.h"
#include "security/security_checker.h"

namespace zepra {

// Forward declare renderer
class TextRenderer;

// ============================================================================
// DevTools Tab Configuration
// ============================================================================

enum class DevToolsTab {
    ELEMENTS = 0,     // DOM Tree + CSS Inspector (first for layout)
    CONSOLE,          // JavaScript Console
    NETWORK,          // Network Requests/Responses  
    SOURCES,          // JavaScript Source Viewer
    PERFORMANCE,      // Performance Profiling
    APPLICATION,      // Storage, Cache, Service Workers
    SECURITY,         // TLS/SSL Certificates, Permissions
    SETTINGS,         // DevTools Settings
    COUNT             // Total tab count
};

struct DevToolsColors {
    // Panel
    uint8_t bg[3] = {28, 28, 32};
    uint8_t header[3] = {38, 38, 44};
    uint8_t border[3] = {55, 55, 65};
    
    // Tabs
    uint8_t tab_active[3] = {58, 58, 68};
    uint8_t tab_text[3] = {180, 180, 200};
    uint8_t tab_text_active[3] = {255, 255, 255};
    
    // Content
    uint8_t row_alt[3] = {32, 32, 38};
    uint8_t text[3] = {200, 200, 220};
    uint8_t text_dim[3] = {120, 120, 140};
    
    // Status colors
    uint8_t success[3] = {80, 200, 120};
    uint8_t warning[3] = {255, 200, 80};
    uint8_t error[3] = {255, 100, 100};
    uint8_t info[3] = {100, 180, 255};
};

// ============================================================================
// DevTools Panel
// ============================================================================

class DevToolsPanel {
public:
    DevToolsPanel();
    ~DevToolsPanel();
    
    // Show/hide
    void toggle();
    void show();
    void hide();
    bool isVisible() const { return visible_; }
    
    // Tab management
    void setActiveTab(DevToolsTab tab);
    DevToolsTab activeTab() const { return activeTab_; }
    
    // Panel dimensions
    void setHeight(float height);
    float height() const { return height_; }
    void setY(float y) { y_ = y; }
    float y() const { return y_; }
    
    // Rendering (called each frame)
    void render(float windowWidth, float windowHeight);
    
    // Input handling
    bool handleClick(float x, float y);
    bool handleKey(int key);
    
    // Network: Selected request for detail view
    void selectRequest(uint64_t requestId);
    uint64_t selectedRequest() const { return selectedRequest_; }
    
    // Console input
    void setConsoleInput(const std::string& text) { consoleInput_ = text; }
    std::string consoleInput() const { return consoleInput_; }
    void executeConsoleCommand();
    
    // Colors
    const DevToolsColors& colors() const { return colors_; }
    
private:
    void renderHeader(float width);
    void renderElementsTab(float width, float contentHeight);
    void renderConsoleTab(float width, float contentHeight);
    void renderNetworkTab(float width, float contentHeight);
    void renderSourcesTab(float width, float contentHeight);
    void renderPerformanceTab(float width, float contentHeight);
    void renderApplicationTab(float width, float contentHeight);
    void renderSecurityTab(float width, float contentHeight);
    void renderSettingsTab(float width, float contentHeight);
    
    // State
    bool visible_ = false;
    DevToolsTab activeTab_ = DevToolsTab::ELEMENTS;
    float height_ = 280;
    float y_ = 0;
    
    // Network
    uint64_t selectedRequest_ = 0;
    float networkScrollY_ = 0;
    
    // Console
    std::string consoleInput_;
    float consoleScrollY_ = 0;
    
    // Elements
    int selectedDOMNode_ = -1;
    float elementsScrollY_ = 0;
    
    // Colors
    DevToolsColors colors_;
    
    // Tab names (matches DevToolsTab enum order)
    static constexpr const char* TAB_NAMES[] = {
        "Elements", "Console", "Network", "Sources", 
        "Performance", "Application", "Security", "Settings"
    };
};

// Global devtools instance
DevToolsPanel& getDevTools();

} // namespace zepra

#endif // DEVTOOLS_PANEL_H
