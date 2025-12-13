/**
 * @file devtools_window.hpp
 * @brief Main DevTools Window - Container for all panels
 * 
 * Provides the main DevTools interface with:
 * - Panel tabs (Elements, Console, Network, Sources, etc.)
 * - Toolbar with common controls
 * - Connection to browser engine
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Zepra::DevTools {

// Forward declarations
class ElementsPanel;
class ConsolePanel;
class NetworkPanel;
class SourcesPanel;
class PerformancePanel;
class MemoryPanel;

/**
 * @brief DevTools panel types
 */
enum class PanelType {
    Elements,
    Console,
    Sources,
    Network,
    Performance,
    Memory,
    Application,
    Security
};

/**
 * @brief Panel change callback
 */
using PanelChangeCallback = std::function<void(PanelType)>;

/**
 * @brief DevTools window configuration
 */
struct DevToolsConfig {
    int width = 1200;
    int height = 800;
    bool docked = true;           // Docked to browser or standalone
    bool darkTheme = true;
    int fontSize = 12;
    PanelType defaultPanel = PanelType::Elements;
};

/**
 * @brief Main DevTools Window
 */
class DevToolsWindow {
public:
    DevToolsWindow();
    ~DevToolsWindow();
    
    // --- Lifecycle ---
    void open();
    void close();
    bool isOpen() const { return isOpen_; }
    
    // --- Configuration ---
    void configure(const DevToolsConfig& config);
    DevToolsConfig config() const { return config_; }
    
    // --- Panel Management ---
    void setActivePanel(PanelType panel);
    PanelType activePanel() const { return activePanel_; }
    void onPanelChange(PanelChangeCallback callback);
    
    // --- Panel Access ---
    ElementsPanel* elements() { return elementsPanel_.get(); }
    ConsolePanel* console() { return consolePanel_.get(); }
    NetworkPanel* network() { return networkPanel_.get(); }
    SourcesPanel* sources() { return sourcesPanel_.get(); }
    PerformancePanel* performance() { return performancePanel_.get(); }
    MemoryPanel* memory() { return memoryPanel_.get(); }
    
    // --- Browser Connection ---
    void connectToPage(void* pageContext);
    void disconnect();
    bool isConnected() const { return connected_; }
    
    // --- UI Update ---
    void update();
    void render();
    
private:
    DevToolsConfig config_;
    PanelType activePanel_ = PanelType::Elements;
    bool isOpen_ = false;
    bool connected_ = false;
    
    std::unique_ptr<ElementsPanel> elementsPanel_;
    std::unique_ptr<ConsolePanel> consolePanel_;
    std::unique_ptr<NetworkPanel> networkPanel_;
    std::unique_ptr<SourcesPanel> sourcesPanel_;
    std::unique_ptr<PerformancePanel> performancePanel_;
    std::unique_ptr<MemoryPanel> memoryPanel_;
    
    std::vector<PanelChangeCallback> panelCallbacks_;
};

} // namespace Zepra::DevTools
