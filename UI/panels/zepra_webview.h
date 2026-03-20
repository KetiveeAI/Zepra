// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file zepra_webview.h
 * @brief ZepraWebView - Browser Developer Tools (Zepra branded)
 * 
 * ZepraWebView is the integrated developer tools for ZepraBrowser.
 * It provides console, elements, network, performance, sources,
 * application, and security panels.
 */

#pragma once

#include "../common/types.h"
#include "../engine/dev_tools.h"
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

namespace zepra {

// ZepraWebView Panel Types
enum class WebViewPanel {
    CONSOLE,
    ELEMENTS,
    NETWORK,
    PERFORMANCE,
    SOURCES,
    APPLICATION,
    SECURITY,
    NODE_JS,
    SETTINGS
};

// Console Filter Options
struct ConsoleFilter {
    bool showLog = true;
    bool showInfo = true;
    bool showWarn = true;
    bool showError = true;
    bool showDebug = true;
    bool showAssert = true;
    String searchText;
    String sourceFilter;
    
    ConsoleFilter() = default;
};

// Network Filter Options
struct NetworkFilter {
    bool showAll = true;
    bool showXHR = true;
    bool showJS = true;
    bool showCSS = true;
    bool showImages = true;
    bool showFonts = true;
    bool showDocuments = true;
    bool showWebSockets = true;
    String searchText;
    String statusFilter;
    
    NetworkFilter() = default;
};

/**
 * ZepraWebView - Integrated Developer Tools for ZepraBrowser
 * 
 * Features:
 * - Console with command execution
 * - DOM Elements inspector
 * - Network request monitoring
 * - Performance profiling
 * - Source debugging
 * - Application storage inspection
 * - Security certificate viewer
 */
class ZepraWebView {
public:
    ZepraWebView();
    ~ZepraWebView();
    
    // Initialization
    bool initialize(std::shared_ptr<DeveloperTools> tools);
    void shutdown();
    bool isInitialized() const;
    
    // Panel Management
    void showPanel(WebViewPanel panel);
    void hidePanel(WebViewPanel panel);
    bool isPanelVisible(WebViewPanel panel) const;
    WebViewPanel getCurrentPanel() const;
    void setCurrentPanel(WebViewPanel panel);
    
    // Console Panel
    void updateConsole();
    void clearConsole();
    void setConsoleFilter(const ConsoleFilter& filter);
    ConsoleFilter getConsoleFilter() const;
    void setConsoleCallback(std::function<void(const String&)> callback);
    void executeConsoleCommand(const String& command);
    std::vector<String> getConsoleHistory() const;
    void addConsoleHistory(const String& command);
    
    // Elements Panel (DOM Inspector)
    void updateDOMTree();
    void highlightElement(int nodeId);
    void inspectElement(int x, int y);
    void expandNode(int nodeId);
    void collapseNode(int nodeId);
    void selectNode(int nodeId);
    int getSelectedNodeId() const;
    void setDOMCallback(std::function<void(const String&)> callback);
    
    // Network Panel
    void updateNetworkLog();
    void clearNetworkLog();
    void setNetworkFilter(const NetworkFilter& filter);
    NetworkFilter getNetworkFilter() const;
    void setNetworkCallback(std::function<void(const String&)> callback);
    void exportNetworkLog(const String& filePath);
    void importNetworkLog(const String& filePath);
    
    // Performance Panel
    void startPerformanceRecording();
    void stopPerformanceRecording();
    bool isPerformanceRecording() const;
    void updatePerformanceMetrics();
    void exportPerformanceData(const String& filePath);
    void setPerformanceCallback(std::function<void(const String&)> callback);
    
    // Sources Panel
    void updateSources();
    void setBreakpoint(const String& url, int line);
    void removeBreakpoint(const String& url, int line);
    void stepOver();
    void stepInto();
    void stepOut();
    void continueExecution();
    void pauseExecution();
    bool isDebugging() const;
    void setDebugCallback(std::function<void(const String&)> callback);
    
    // Application Panel
    void updateApplicationData();
    void inspectLocalStorage();
    void inspectSessionStorage();
    void inspectCookies();
    void inspectIndexedDB();
    void inspectCache();
    void inspectServiceWorkers();
    void setApplicationCallback(std::function<void(const String&)> callback);
    
    // Security Panel
    void updateSecurityInfo();
    void inspectCertificates();
    void inspectPermissions();
    void setSecurityCallback(std::function<void(const String&)> callback);
    
    // Node.js Panel
    void updateNodeModules();
    void executeNodeScript(const String& script);
    void installNodePackage(const String& packageName, const String& version = "");
    void uninstallNodePackage(const String& packageName);
    std::vector<String> getInstalledPackages() const;
    void setNodeCallback(std::function<void(const String&)> callback);
    
    // Settings Panel
    void updateSettings();
    void saveSettings();
    void loadSettings();
    void resetSettings();
    void setSettingsCallback(std::function<void(const String&)> callback);
    
    // UI Rendering
    void render();
    void renderConsole();
    void renderElements();
    void renderNetwork();
    void renderPerformance();
    void renderSources();
    void renderApplication();
    void renderSecurity();
    void renderNodeJS();
    void renderSettings();
    
    // Event Handling
    void handleMouseClick(int x, int y, int button);
    void handleMouseMove(int x, int y);
    void handleKeyPress(int key, bool ctrl, bool shift, bool alt);
    void handleScroll(int deltaX, int deltaY);
    void handleResize(int width, int height);
    
    // Search and Filter
    void searchInPanel(WebViewPanel panel, const String& query);
    void clearSearch();
    String getSearchQuery() const;
    void setSearchCallback(std::function<void(const String&)> callback);
    
    // Export and Import
    void exportData(WebViewPanel panel, const String& filePath);
    void importData(WebViewPanel panel, const String& filePath);
    void exportAllData(const String& directory);
    void importAllData(const String& directory);
    
    // Theme and Styling
    void setTheme(const String& theme);
    String getTheme() const;
    void setFontSize(int size);
    int getFontSize() const;
    void setColorScheme(const String& scheme);
    String getColorScheme() const;
    
    // Layout Management
    void setLayout(const String& layout);
    String getLayout() const;
    void toggleSidebar();
    bool isSidebarVisible() const;
    void toggleBottomPanel();
    bool isBottomPanelVisible() const;
    void setPanelSize(WebViewPanel panel, int width, int height);
    void getPanelSize(WebViewPanel panel, int& width, int& height) const;
    
    // Keyboard Shortcuts
    void registerShortcut(const String& key, std::function<void()> action);
    void unregisterShortcut(const String& key);
    void setShortcutCallback(std::function<void(const String&)> callback);
    
    // Context Menus
    void showContextMenu(int x, int y, WebViewPanel panel);
    void hideContextMenu();
    bool isContextMenuVisible() const;
    void addContextMenuItem(const String& label, std::function<void()> action);
    void removeContextMenuItem(const String& label);
    
    // Notifications
    void showNotification(const String& message, const String& type = "info");
    void hideNotification();
    bool isNotificationVisible() const;
    void setNotificationCallback(std::function<void(const String&, const String&)> callback);
    
    // Auto-refresh
    void setAutoRefresh(bool enabled);
    bool isAutoRefreshEnabled() const;
    void setRefreshInterval(int milliseconds);
    int getRefreshInterval() const;
    
    // Error Handling
    void setErrorCallback(std::function<void(const String&)> callback);
    std::vector<String> getErrors() const;
    void clearErrors();
    
private:
    std::shared_ptr<DeveloperTools> devTools;
    bool initialized;
    
    // Panel state
    WebViewPanel currentPanel;
    std::unordered_map<WebViewPanel, bool> panelVisibility;
    std::unordered_map<WebViewPanel, std::pair<int, int>> panelSizes;
    
    // Console state
    ConsoleFilter consoleFilter;
    std::vector<String> consoleHistory;
    int consoleHistoryIndex;
    String consoleInput;
    std::function<void(const String&)> consoleCallback;
    
    // Elements state
    int selectedNodeId;
    int highlightedNodeId;
    std::function<void(const String&)> domCallback;
    
    // Network state
    NetworkFilter networkFilter;
    std::function<void(const String&)> networkCallback;
    
    // Performance state
    bool performanceRecording;
    std::function<void(const String&)> performanceCallback;
    
    // Sources state
    bool debugging;
    std::function<void(const String&)> debugCallback;
    
    // Application state
    std::function<void(const String&)> applicationCallback;
    
    // Security state
    std::function<void(const String&)> securityCallback;
    
    // Node.js state
    std::vector<String> installedPackages;
    std::function<void(const String&)> nodeCallback;
    
    // Settings state
    std::function<void(const String&)> settingsCallback;
    
    // UI state
    String theme;
    int fontSize;
    String colorScheme;
    String layout;
    bool sidebarVisible;
    bool bottomPanelVisible;
    bool contextMenuVisible;
    bool notificationVisible;
    String notificationMessage;
    String notificationType;
    
    // Search state
    String searchQuery;
    std::function<void(const String&)> searchCallback;
    
    // Auto-refresh state
    bool autoRefreshEnabled;
    int refreshInterval;
    
    // Error state
    std::vector<String> errors;
    std::function<void(const String&)> errorCallback;
    
    // Shortcuts
    std::unordered_map<String, std::function<void()>> shortcuts;
    std::function<void(const String&)> shortcutCallback;
    
    // Context menu
    std::unordered_map<String, std::function<void()>> contextMenuItems;
    
    // Notifications
    std::function<void(const String&, const String&)> notificationCallback;
    
    // Internal methods
    void initializePanels();
    void setupShortcuts();
    void setupCallbacks();
    void updatePanelData();
    void renderPanel(WebViewPanel panel);
    void handlePanelEvent(WebViewPanel panel, int x, int y, int button);
    void processConsoleCommand(const String& command);
    void updateConsoleHistory();
    void filterConsoleMessages();
    void filterNetworkRequests();
    void exportPanelData(WebViewPanel panel, const String& filePath);
    void importPanelData(WebViewPanel panel, const String& filePath);
    void savePanelState();
    void loadPanelState();
    void applyTheme();
    void applyLayout();
    void showError(const String& error);
    void logActivity(const String& activity);
};

// Compatibility aliases
using DevToolsUI = ZepraWebView;
using DevToolsPanel = WebViewPanel;

} // namespace zepra
