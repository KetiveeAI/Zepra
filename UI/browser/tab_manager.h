// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "../common/types.h"
#include <vector>
#include <memory>
#include <functional>
#include <SDL2/SDL.h>
#include "../engine/webkit_engine.h"
#include "../network/network_monitor.h"  // Per-tab network isolation

namespace zepra {

// Forward declarations
class Tab;
class Window;


// Tab event types
enum class TabEventType {
    CREATED,
    CLOSED,
    ACTIVATED,
    NAVIGATION_START,
    NAVIGATION_COMPLETE,
    NAVIGATION_ERROR,
    TITLE_CHANGED,
    FAVICON_CHANGED,
    LOADING_PROGRESS
};

// Tab event structure
struct TabEvent {
    TabEventType type;
    std::shared_ptr<Tab> tab;
    String url;
    String title;
    float progress;
    String error;
    
    TabEvent(TabEventType t, std::shared_ptr<Tab> t_ptr) 
        : type(t), tab(t_ptr), progress(0.0f) {}
};

// Tab callback types
using TabEventCallback = std::function<void(const TabEvent&)>;

struct TabEntry {
    String id;
    String title;
    String url;
    bool isActive;
    bool isLoading;
    bool isPinned;
    bool isMuted;
    bool isCrashed;
    // Add more tab state as needed
    
    // Stubs for browser operations
    void reload() {}
    void navigate(const String& url) { this->url = url; }
};

// Tab class
class Tab {
public:
    Tab();
    ~Tab();
    
    // Tab identification
    void setId(int id) { this->id = id; }
    int getId() const { return id; }
    
    // URL management
    void setUrl(const String& url);
    String getUrl() const;
    void setTitle(const String& title);
    String getTitle() const;
    void setFavicon(const String& favicon);
    String getFavicon() const;
    
    // Navigation state
    void setState(TabEntry state);
    TabEntry getState() const;
    void setLoadingProgress(float progress);
    float getLoadingProgress() const;
    bool isLoading() const;
    bool canGoBack() const;
    bool canGoForward() const;
    
    // Navigation history
    void goBack();
    void goForward();
    void reload();
    void stop();
    void navigate(const String& url);
    
    // Content management
    void setContent(const String& html);
    String getContent() const;
    void setScrollPosition(int x, int y);
    void getScrollPosition(int& x, int& y) const;
    
    // Tab properties
    void setPinned(bool pinned);
    bool isPinned() const;
    void setMuted(bool muted);
    bool isMuted() const;
    void setIncognito(bool incognito);
    bool isIncognito() const;
    
    // Event handling
    void setEventCallback(TabEventCallback callback);
    void triggerEvent(TabEventType type);
    
    // Utility methods
    String getDisplayTitle() const;
    bool hasUncommittedChanges() const;
    void markAsChanged(bool changed);
    
private:
    int id;
    String url;
    String title;
    String favicon;
    TabEntry state;
    float loadingProgress;
    bool pinned;
    bool muted;
    bool incognito;
    bool hasChanges;
    
    // TabSuspender integration
    bool isSuspended_ = false;
    int inactiveTimeSeconds_ = 0;
    
    // Per-tab network monitor (Tab A/B isolation)
    std::unique_ptr<NetworkMonitor> networkMonitor_;
    
    // Navigation history
    std::vector<String> backHistory;
    std::vector<String> forwardHistory;
    int currentHistoryIndex;
    
    // Content and scroll (public access for TabSuspender)
public:
    String pageContent;  // HTML content cache for suspension/restoration
    float scrollY = 0.0f;  // Per-tab scroll position (TabSuspender needs direct access)
    
    // TabSuspender-compatible methods
    void setSuspended(bool s) { isSuspended_ = s; }
    bool isActive() const { return !isSuspended_; }
    bool isSuspended() const { return isSuspended_; }
    int getInactiveTime() const { return inactiveTimeSeconds_; }
    void setInactiveTime(int seconds) { inactiveTimeSeconds_ = seconds; }
    String getCurrentUrl() const { return url; }
    
    // Per-tab network monitor (for DevTools Network tab)
    NetworkMonitor& getNetworkMonitor() { 
        if (!networkMonitor_) {
            networkMonitor_ = std::make_unique<NetworkMonitor>();
        }
        return *networkMonitor_; 
    }
    
    // Clear network log (on navigation or tab clear)
    void clearNetworkLog() {
        if (networkMonitor_) {
            networkMonitor_->clear();
        }
    }
    
private:
    int scrollX;
    
    // Event callback
    TabEventCallback eventCallback;
    
    // Navigation helpers
    void addToHistory(const String& url);
    void clearForwardHistory();
    void updateHistoryIndex();
};



// Tab Manager class
class TabManager {
public:
    TabManager();
    ~TabManager();
    
    // Tab operations
    String openTab(const String& url, bool foreground = true);
    bool closeTab(const String& tabId);
    bool switchToTab(const String& tabId);
    bool reloadTab(const String& tabId);
    bool duplicateTab(const String& tabId);
    bool moveTab(const String& tabId, int newIndex);
    bool pinTab(const String& tabId, bool pinned = true);
    bool muteTab(const String& tabId, bool muted = true);
    bool restoreTab(const String& tabId);
    bool crashTab(const String& tabId);
    bool navigateTab(const String& tabId, const String& url);
    bool navigateActiveTab(const String& url);
    bool reloadActiveTab();
    bool closeActiveTab();
    
    // Convenience method - alias for openTab
    String createTab(const String& url = "about:blank") { return openTab(url, true); }

    // Tab queries
    std::vector<TabEntry> getAllTabs() const;
    TabEntry getTabState(const String& tabId) const;
    String getActiveTabId() const;
    TabEntry* getActiveTab();
    const TabEntry* getActiveTab() const;
    TabEntry* getTabById(const String& tabId);
    const TabEntry* getTabById(const String& tabId) const;
    int getTabIndex(const String& tabId) const;
    int getTabCount() const;

    // Tab process separation
    bool isolateTabProcess(const String& tabId);
    bool mergeTabProcess(const String& tabId);

    // Events
    void setTabSwitchedCallback(std::function<void(const TabEntry&)> cb);
    void setTabClosedCallback(std::function<void(const TabEntry&)> cb);
    void setTabCrashedCallback(std::function<void(const TabEntry&)> cb);
    
    // UI Integration (for main loop)
    bool handleEvent(const SDL_Event& event) { return false; }  // Stub
    void update() {}  // Stub
    void render() {}  // Stub
    void refreshCurrentTab() { reloadActiveTab(); }

private:
    std::vector<TabEntry> tabs;
    String activeTabId;
    std::function<void(const TabEntry&)> tabSwitchedCallback;
    std::function<void(const TabEntry&)> tabClosedCallback;
    std::function<void(const TabEntry&)> tabCrashedCallback;
    // Add more private helpers as needed
};

// Tab Group class (for future grouping feature)
class TabGroup {
public:
    TabGroup(const String& name);
    ~TabGroup();
    
    String getName() const { return name; }
    void setName(const String& name) { this->name = name; }
    
    void addTab(std::shared_ptr<Tab> tab);
    void removeTab(std::shared_ptr<Tab> tab);
    std::vector<std::shared_ptr<Tab>> getTabs() const;
    int getTabCount() const;
    
    void setCollapsed(bool collapsed);
    bool isCollapsed() const;
    
    void setColor(const Color& color);
    Color getColor() const;
    
private:
    String name;
    std::vector<std::shared_ptr<Tab>> tabs;
    bool collapsed;
    Color color;
};

} // namespace zepra 