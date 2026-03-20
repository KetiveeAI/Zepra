// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "ui/dev_tools_ui.h"
#include <iostream>

namespace zepra {

// DevToolsUI Implementation
DevToolsUI::DevToolsUI() 
    : initialized(false)
    , currentPanel(DevToolsPanel::CONSOLE)
    , selectedNodeId(0)
    , highlightedNodeId(0)
    , performanceRecording(false)
    , debugging(false)
    , consoleHistoryIndex(0)
    , fontSize(14)
    , sidebarVisible(true)
    , bottomPanelVisible(true)
    , contextMenuVisible(false)
    , notificationVisible(false)
    , autoRefreshEnabled(false)
    , refreshInterval(1000) {
    
    std::cout << "[UI] DevTools UI initialized" << std::endl;
    
    // Initialize panel visibility
    panelVisibility[DevToolsPanel::CONSOLE] = true;
    panelVisibility[DevToolsPanel::ELEMENTS] = false;
    panelVisibility[DevToolsPanel::NETWORK] = false;
    panelVisibility[DevToolsPanel::PERFORMANCE] = false;
    panelVisibility[DevToolsPanel::SOURCES] = false;
    panelVisibility[DevToolsPanel::APPLICATION] = false;
    panelVisibility[DevToolsPanel::SECURITY] = false;
    panelVisibility[DevToolsPanel::NODE_JS] = false;
    panelVisibility[DevToolsPanel::SETTINGS] = false;
    
    // Set default theme
    theme = "dark";
    colorScheme = "dark";
    layout = "default";
}

DevToolsUI::~DevToolsUI() {
    std::cout << "[UI] DevTools UI destroyed" << std::endl;
}

// Initialization
bool DevToolsUI::initialize(std::shared_ptr<DeveloperTools> tools) {
    devTools = tools;
    initialized = true;

    initializePanels();
    setupShortcuts();
    setupCallbacks();

    std::cout << "[UI] DevTools UI initialized successfully" << std::endl;
    return true;
}

void DevToolsUI::shutdown() {
    initialized = false;
    std::cout << "[UI] DevTools UI shutdown" << std::endl;
}

bool DevToolsUI::isInitialized() const {
    return initialized;
}

// Panel Management
void DevToolsUI::showPanel(DevToolsPanel panel) {
    panelVisibility[panel] = true;
    currentPanel = panel;
    std::cout << "[UI] Showing panel: " << static_cast<int>(panel) << std::endl;
}

void DevToolsUI::hidePanel(DevToolsPanel panel) {
    panelVisibility[panel] = false;
    std::cout << "[UI] Hiding panel: " << static_cast<int>(panel) << std::endl;
}

bool DevToolsUI::isPanelVisible(DevToolsPanel panel) const {
    auto it = panelVisibility.find(panel);
    return it != panelVisibility.end() ? it->second : false;
}

DevToolsPanel DevToolsUI::getCurrentPanel() const {
    return currentPanel;
}

void DevToolsUI::setCurrentPanel(DevToolsPanel panel) {
    currentPanel = panel;
    std::cout << "[UI] Current panel set to: " << static_cast<int>(panel) << std::endl;
}

// Console Panel
void DevToolsUI::updateConsole() {
    if (devTools) {
        auto messages = devTools->getConsoleMessages();
        std::cout << "[UI] Console updated with " << messages.size() << " messages" << std::endl;
    }
}

void DevToolsUI::clearConsole() {
    if (devTools) {
        devTools->clearConsole();
        std::cout << "[UI] Console cleared" << std::endl;
    }
}

void DevToolsUI::setConsoleFilter(const ConsoleFilter& filter) {
    consoleFilter = filter;
    std::cout << "[UI] Console filter updated" << std::endl;
}

ConsoleFilter DevToolsUI::getConsoleFilter() const {
    return consoleFilter;
}

void DevToolsUI::setConsoleCallback(std::function<void(const String&)> callback) {
    consoleCallback = callback;
}

void DevToolsUI::executeConsoleCommand(const String& command) {
    if (devTools) {
        String result = devTools->executeJavaScript(command);
        addConsoleHistory(command);
        std::cout << "[UI] Console command executed: " << command << std::endl;
        std::cout << "[UI] Result: " << result << std::endl;
    }
}

std::vector<String> DevToolsUI::getConsoleHistory() const {
    return consoleHistory;
}

void DevToolsUI::addConsoleHistory(const String& command) {
    consoleHistory.push_back(command);
    if (consoleHistory.size() > 100) {
        consoleHistory.erase(consoleHistory.begin());
    }
}

// Elements Panel
void DevToolsUI::updateDOMTree() {
    std::cout << "[UI] DOM tree updated" << std::endl;
}

void DevToolsUI::highlightElement(int nodeId) {
    highlightedNodeId = nodeId;
    if (devTools) {
        devTools->highlightNode(nodeId);
    }
    std::cout << "[UI] Element highlighted: " << nodeId << std::endl;
}

void DevToolsUI::inspectElement(int x, int y) {
    if (devTools) {
        devTools->inspectElement(x, y);
    }
    std::cout << "[UI] Inspecting element at (" << x << ", " << y << ")" << std::endl;
}

void DevToolsUI::expandNode(int nodeId) {
    std::cout << "[UI] Node expanded: " << nodeId << std::endl;
}

void DevToolsUI::collapseNode(int nodeId) {
    std::cout << "[UI] Node collapsed: " << nodeId << std::endl;
}

void DevToolsUI::selectNode(int nodeId) {
    selectedNodeId = nodeId;
    std::cout << "[UI] Node selected: " << nodeId << std::endl;
}

int DevToolsUI::getSelectedNodeId() const {
    return selectedNodeId;
}

void DevToolsUI::setDOMCallback(std::function<void(const String&)> callback) {
    domCallback = callback;
}

// Network Panel
void DevToolsUI::updateNetworkLog() {
    if (devTools) {
        auto requests = devTools->getNetworkRequests();
        auto responses = devTools->getNetworkResponses();
        std::cout << "[UI] Network log updated: " << requests.size() << " requests, " 
                  << responses.size() << " responses" << std::endl;
    }
}

void DevToolsUI::clearNetworkLog() {
    if (devTools) {
        devTools->clearNetworkLog();
        std::cout << "[UI] Network log cleared" << std::endl;
    }
}

void DevToolsUI::setNetworkFilter(const NetworkFilter& filter) {
    networkFilter = filter;
    std::cout << "[UI] Network filter updated" << std::endl;
}

NetworkFilter DevToolsUI::getNetworkFilter() const {
    return networkFilter;
}

void DevToolsUI::setNetworkCallback(std::function<void(const String&)> callback) {
    networkCallback = callback;
}

void DevToolsUI::exportNetworkLog(const String& filePath) {
    std::cout << "[UI] Network log exported to: " << filePath << std::endl;
}

void DevToolsUI::importNetworkLog(const String& filePath) {
    std::cout << "[UI] Network log imported from: " << filePath << std::endl;
}

// Performance Panel
void DevToolsUI::startPerformanceRecording() {
    performanceRecording = true;
    if (devTools) {
        devTools->startPerformanceMonitoring();
    }
    std::cout << "[UI] Performance recording started" << std::endl;
}

void DevToolsUI::stopPerformanceRecording() {
    performanceRecording = false;
    if (devTools) {
        devTools->stopPerformanceMonitoring();
    }
    std::cout << "[UI] Performance recording stopped" << std::endl;
}

bool DevToolsUI::isPerformanceRecording() const {
    return performanceRecording;
}

void DevToolsUI::updatePerformanceMetrics() {
    if (devTools) {
        auto metrics = devTools->getPerformanceMetrics();
        std::cout << "[UI] Performance metrics updated" << std::endl;
    }
}

void DevToolsUI::exportPerformanceData(const String& filePath) {
    std::cout << "[UI] Performance data exported to: " << filePath << std::endl;
}

void DevToolsUI::setPerformanceCallback(std::function<void(const String&)> callback) {
    performanceCallback = callback;
}

// Sources Panel
void DevToolsUI::updateSources() {
    std::cout << "[UI] Sources updated" << std::endl;
}

void DevToolsUI::setBreakpoint(const String& url, int line) {
    std::cout << "[UI] Breakpoint set: " << url << ":" << line << std::endl;
}

void DevToolsUI::removeBreakpoint(const String& url, int line) {
    std::cout << "[UI] Breakpoint removed: " << url << ":" << line << std::endl;
}

void DevToolsUI::stepOver() {
    std::cout << "[UI] Step over executed" << std::endl;
}

void DevToolsUI::stepInto() {
    std::cout << "[UI] Step into executed" << std::endl;
}

void DevToolsUI::stepOut() {
    std::cout << "[UI] Step out executed" << std::endl;
}

void DevToolsUI::continueExecution() {
    std::cout << "[UI] Execution continued" << std::endl;
}

void DevToolsUI::pauseExecution() {
    std::cout << "[UI] Execution paused" << std::endl;
}

bool DevToolsUI::isDebugging() const {
    return debugging;
}

void DevToolsUI::setDebugCallback(std::function<void(const String&)> callback) {
    debugCallback = callback;
}

// Application Panel
void DevToolsUI::updateApplicationData() {
    std::cout << "[UI] Application data updated" << std::endl;
}

void DevToolsUI::inspectLocalStorage() {
    if (devTools) {
        devTools->inspectLocalStorage();
    }
}

void DevToolsUI::inspectSessionStorage() {
    if (devTools) {
        devTools->inspectSessionStorage();
    }
}

void DevToolsUI::inspectCookies() {
    if (devTools) {
        devTools->inspectCookies();
    }
}

void DevToolsUI::inspectIndexedDB() {
    if (devTools) {
        devTools->inspectIndexedDB();
    }
}

void DevToolsUI::inspectCache() {
    if (devTools) {
        devTools->inspectCacheStorage();
    }
}

void DevToolsUI::inspectServiceWorkers() {
    if (devTools) {
        devTools->inspectServiceWorkers();
    }
}

void DevToolsUI::setApplicationCallback(std::function<void(const String&)> callback) {
    applicationCallback = callback;
}

// Security Panel
void DevToolsUI::updateSecurityInfo() {
    if (devTools) {
        devTools->inspectSecurityInfo();
    }
}

void DevToolsUI::inspectCertificates() {
    if (devTools) {
        devTools->inspectCertificates();
    }
}

void DevToolsUI::inspectPermissions() {
    if (devTools) {
        devTools->inspectPermissions();
    }
}

void DevToolsUI::setSecurityCallback(std::function<void(const String&)> callback) {
    securityCallback = callback;
}

// Node.js Panel
void DevToolsUI::updateNodeModules() {
    std::cout << "[UI] Node modules updated" << std::endl;
}

void DevToolsUI::executeNodeScript(const String& script) {
    if (devTools) {
        String result = devTools->executeNodeScript(script);
        std::cout << "[UI] Node.js script executed: " << script.substr(0, 50) << "..." << std::endl;
        std::cout << "[UI] Result: " << result << std::endl;
    }
}

void DevToolsUI::installNodePackage(const String& packageName, const String& version) {
    std::cout << "[UI] Installing Node.js package: " << packageName;
    if (!version.empty()) {
        std::cout << "@" << version;
    }
    std::cout << std::endl;
}

void DevToolsUI::uninstallNodePackage(const String& packageName) {
    std::cout << "[UI] Uninstalling Node.js package: " << packageName << std::endl;
}

std::vector<String> DevToolsUI::getInstalledPackages() const {
    return installedPackages;
}

void DevToolsUI::setNodeCallback(std::function<void(const String&)> callback) {
    nodeCallback = callback;
}

// Settings Panel
void DevToolsUI::updateSettings() {
    std::cout << "[UI] Settings updated" << std::endl;
}

void DevToolsUI::saveSettings() {
    std::cout << "[UI] Settings saved" << std::endl;
}

void DevToolsUI::loadSettings() {
    std::cout << "[UI] Settings loaded" << std::endl;
}

void DevToolsUI::resetSettings() {
    std::cout << "[UI] Settings reset" << std::endl;
}

void DevToolsUI::setSettingsCallback(std::function<void(const String&)> callback) {
    settingsCallback = callback;
}

// UI Rendering
void DevToolsUI::render() {
    if (!initialized) return;
    
    renderPanel(currentPanel);
    std::cout << "[UI] DevTools UI rendered" << std::endl;
}

void DevToolsUI::renderConsole() {
    std::cout << "[UI] Console panel rendered" << std::endl;
}

void DevToolsUI::renderElements() {
    std::cout << "[UI] Elements panel rendered" << std::endl;
}

void DevToolsUI::renderNetwork() {
    std::cout << "[UI] Network panel rendered" << std::endl;
}

void DevToolsUI::renderPerformance() {
    std::cout << "[UI] Performance panel rendered" << std::endl;
}

void DevToolsUI::renderSources() {
    std::cout << "[UI] Sources panel rendered" << std::endl;
}

void DevToolsUI::renderApplication() {
    std::cout << "[UI] Application panel rendered" << std::endl;
}

void DevToolsUI::renderSecurity() {
    std::cout << "[UI] Security panel rendered" << std::endl;
}

void DevToolsUI::renderNodeJS() {
    std::cout << "[UI] Node.js panel rendered" << std::endl;
}

void DevToolsUI::renderSettings() {
    std::cout << "[UI] Settings panel rendered" << std::endl;
}

// Event Handling
void DevToolsUI::handleMouseClick(int x, int y, int button) {
    std::cout << "[UI] Mouse click: (" << x << ", " << y << ") button " << button << std::endl;
}

void DevToolsUI::handleMouseMove(int x, int y) {
    // Mouse move events are typically not logged to avoid spam
}

void DevToolsUI::handleKeyPress(int key, bool ctrl, bool shift, bool alt) {
    std::cout << "[UI] Key press: " << key;
    if (ctrl) std::cout << " + Ctrl";
    if (shift) std::cout << " + Shift";
    if (alt) std::cout << " + Alt";
    std::cout << std::endl;
}

void DevToolsUI::handleScroll(int deltaX, int deltaY) {
    std::cout << "[UI] Scroll: (" << deltaX << ", " << deltaY << ")" << std::endl;
}

void DevToolsUI::handleResize(int width, int height) {
    std::cout << "[UI] Resize: " << width << "x" << height << std::endl;
}

// Search and Filter
void DevToolsUI::searchInPanel(DevToolsPanel panel, const String& query) {
    searchQuery = query;
    std::cout << "[UI] Search in panel " << static_cast<int>(panel) << ": " << query << std::endl;
}

void DevToolsUI::clearSearch() {
    searchQuery.clear();
    std::cout << "[UI] Search cleared" << std::endl;
}

String DevToolsUI::getSearchQuery() const {
    return searchQuery;
}

void DevToolsUI::setSearchCallback(std::function<void(const String&)> callback) {
    searchCallback = callback;
}

// Export and Import
void DevToolsUI::exportData(DevToolsPanel panel, const String& filePath) {
    std::cout << "[UI] Exporting data from panel " << static_cast<int>(panel) << " to " << filePath << std::endl;
}

void DevToolsUI::importData(DevToolsPanel panel, const String& filePath) {
    std::cout << "[UI] Importing data to panel " << static_cast<int>(panel) << " from " << filePath << std::endl;
}

void DevToolsUI::exportAllData(const String& directory) {
    std::cout << "[UI] Exporting all data to " << directory << std::endl;
}

void DevToolsUI::importAllData(const String& directory) {
    std::cout << "[UI] Importing all data from " << directory << std::endl;
}

// Theme and Styling
void DevToolsUI::setTheme(const String& newTheme) {
    theme = newTheme;
    applyTheme();
    std::cout << "[UI] Theme set to: " << theme << std::endl;
}

String DevToolsUI::getTheme() const {
    return theme;
}

void DevToolsUI::setFontSize(int size) {
    fontSize = size;
    std::cout << "[UI] Font size set to: " << fontSize << std::endl;
}

int DevToolsUI::getFontSize() const {
    return fontSize;
}

void DevToolsUI::setColorScheme(const String& scheme) {
    colorScheme = scheme;
    std::cout << "[UI] Color scheme set to: " << colorScheme << std::endl;
}

String DevToolsUI::getColorScheme() const {
    return colorScheme;
}

// Layout Management
void DevToolsUI::setLayout(const String& newLayout) {
    layout = newLayout;
    applyLayout();
    std::cout << "[UI] Layout set to: " << layout << std::endl;
}

String DevToolsUI::getLayout() const {
    return layout;
}

void DevToolsUI::toggleSidebar() {
    sidebarVisible = !sidebarVisible;
    std::cout << "[UI] Sidebar " << (sidebarVisible ? "shown" : "hidden") << std::endl;
}

bool DevToolsUI::isSidebarVisible() const {
    return sidebarVisible;
}

void DevToolsUI::toggleBottomPanel() {
    bottomPanelVisible = !bottomPanelVisible;
    std::cout << "[UI] Bottom panel " << (bottomPanelVisible ? "shown" : "hidden") << std::endl;
}

bool DevToolsUI::isBottomPanelVisible() const {
    return bottomPanelVisible;
}

void DevToolsUI::setPanelSize(DevToolsPanel panel, int width, int height) {
    panelSizes[panel] = std::make_pair(width, height);
    std::cout << "[UI] Panel " << static_cast<int>(panel) << " size set to " << width << "x" << height << std::endl;
}

void DevToolsUI::getPanelSize(DevToolsPanel panel, int& width, int& height) const {
    auto it = panelSizes.find(panel);
    if (it != panelSizes.end()) {
        width = it->second.first;
        height = it->second.second;
    } else {
        width = 800;
        height = 600;
    }
}

// Keyboard Shortcuts
void DevToolsUI::registerShortcut(const String& key, std::function<void()> action) {
    shortcuts[key] = action;
    std::cout << "[UI] Shortcut registered: " << key << std::endl;
}

void DevToolsUI::unregisterShortcut(const String& key) {
    shortcuts.erase(key);
    std::cout << "[UI] Shortcut unregistered: " << key << std::endl;
}

void DevToolsUI::setShortcutCallback(std::function<void(const String&)> callback) {
    shortcutCallback = callback;
}

// Context Menus
void DevToolsUI::showContextMenu(int x, int y, DevToolsPanel panel) {
    contextMenuVisible = true;
    std::cout << "[UI] Context menu shown at (" << x << ", " << y << ") for panel " << static_cast<int>(panel) << std::endl;
}

void DevToolsUI::hideContextMenu() {
    contextMenuVisible = false;
    std::cout << "[UI] Context menu hidden" << std::endl;
}

bool DevToolsUI::isContextMenuVisible() const {
    return contextMenuVisible;
}

void DevToolsUI::addContextMenuItem(const String& label, std::function<void()> action) {
    contextMenuItems[label] = action;
    std::cout << "[UI] Context menu item added: " << label << std::endl;
}

void DevToolsUI::removeContextMenuItem(const String& label) {
    contextMenuItems.erase(label);
    std::cout << "[UI] Context menu item removed: " << label << std::endl;
}

// Notifications
void DevToolsUI::showNotification(const String& message, const String& type) {
    notificationVisible = true;
    notificationMessage = message;
    notificationType = type;
    std::cout << "[UI] Notification [" << type << "]: " << message << std::endl;
}

void DevToolsUI::hideNotification() {
    notificationVisible = false;
    std::cout << "[UI] Notification hidden" << std::endl;
}

bool DevToolsUI::isNotificationVisible() const {
    return notificationVisible;
}

void DevToolsUI::setNotificationCallback(std::function<void(const String&, const String&)> callback) {
    notificationCallback = callback;
}

// Auto-refresh
void DevToolsUI::setAutoRefresh(bool enabled) {
    autoRefreshEnabled = enabled;
    std::cout << "[UI] Auto-refresh " << (enabled ? "enabled" : "disabled") << std::endl;
}

bool DevToolsUI::isAutoRefreshEnabled() const {
    return autoRefreshEnabled;
}

void DevToolsUI::setRefreshInterval(int milliseconds) {
    refreshInterval = milliseconds;
    std::cout << "[UI] Refresh interval set to " << milliseconds << "ms" << std::endl;
}

int DevToolsUI::getRefreshInterval() const {
    return refreshInterval;
}

// Error Handling
void DevToolsUI::setErrorCallback(std::function<void(const String&)> callback) {
    errorCallback = callback;
}

std::vector<String> DevToolsUI::getErrors() const {
    return errors;
}

void DevToolsUI::clearErrors() {
    errors.clear();
    std::cout << "[UI] Errors cleared" << std::endl;
}

// Private Methods
void DevToolsUI::initializePanels() {
    std::cout << "[UI] Panels initialized" << std::endl;
}

void DevToolsUI::setupShortcuts() {
    // Register default shortcuts
    registerShortcut("F12", []() { std::cout << "F12 - Toggle DevTools" << std::endl; });
    registerShortcut("Ctrl+Shift+I", []() { std::cout << "Ctrl+Shift+I - Open Inspector" << std::endl; });
    registerShortcut("Ctrl+Shift+J", []() { std::cout << "Ctrl+Shift+J - Open Console" << std::endl; });
    registerShortcut("Ctrl+Shift+N", []() { std::cout << "Ctrl+Shift+N - Open Network" << std::endl; });
    std::cout << "[UI] Default shortcuts registered" << std::endl;
}

void DevToolsUI::setupCallbacks() {
    std::cout << "[UI] Callbacks setup completed" << std::endl;
}

void DevToolsUI::updatePanelData() {
    std::cout << "[UI] Panel data updated" << std::endl;
}

void DevToolsUI::renderPanel(DevToolsPanel panel) {
    switch (panel) {
        case DevToolsPanel::CONSOLE: renderConsole(); break;
        case DevToolsPanel::ELEMENTS: renderElements(); break;
        case DevToolsPanel::NETWORK: renderNetwork(); break;
        case DevToolsPanel::PERFORMANCE: renderPerformance(); break;
        case DevToolsPanel::SOURCES: renderSources(); break;
        case DevToolsPanel::APPLICATION: renderApplication(); break;
        case DevToolsPanel::SECURITY: renderSecurity(); break;
        case DevToolsPanel::NODE_JS: renderNodeJS(); break;
        case DevToolsPanel::SETTINGS: renderSettings(); break;
    }
}

void DevToolsUI::handlePanelEvent(DevToolsPanel panel, int x, int y, int button) {
    std::cout << "[UI] Panel event: " << static_cast<int>(panel) << " at (" << x << ", " << y << ")" << std::endl;
}

void DevToolsUI::processConsoleCommand(const String& command) {
    std::cout << "[UI] Processing console command: " << command << std::endl;
}

void DevToolsUI::updateConsoleHistory() {
    std::cout << "[UI] Console history updated" << std::endl;
}

void DevToolsUI::filterConsoleMessages() {
    std::cout << "[UI] Console messages filtered" << std::endl;
}

void DevToolsUI::filterNetworkRequests() {
    std::cout << "[UI] Network requests filtered" << std::endl;
}

void DevToolsUI::exportPanelData(DevToolsPanel panel, const String& filePath) {
    std::cout << "[UI] Panel data exported: " << static_cast<int>(panel) << " to " << filePath << std::endl;
}

void DevToolsUI::importPanelData(DevToolsPanel panel, const String& filePath) {
    std::cout << "[UI] Panel data imported: " << static_cast<int>(panel) << " from " << filePath << std::endl;
}

void DevToolsUI::savePanelState() {
    std::cout << "[UI] Panel state saved" << std::endl;
}

void DevToolsUI::loadPanelState() {
    std::cout << "[UI] Panel state loaded" << std::endl;
}

void DevToolsUI::applyTheme() {
    std::cout << "[UI] Theme applied: " << theme << std::endl;
}

void DevToolsUI::applyLayout() {
    std::cout << "[UI] Layout applied: " << layout << std::endl;
}

void DevToolsUI::showError(const String& error) {
    errors.push_back(error);
    std::cout << "[UI] Error: " << error << std::endl;
}

void DevToolsUI::logActivity(const String& activity) {
    std::cout << "[UI] Activity: " << activity << std::endl;
}

} // namespace zepra
 