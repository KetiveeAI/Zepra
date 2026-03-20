// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "../../source/zepraEngine/include/engine/ui/tab_manager.h"
#include "../../source/zepraEngine/include/engine/webkit_engine.h"
#include <algorithm>
#include <iostream>
#include <memory>

namespace zepra {

namespace {
std::string generateTabId(size_t index) {
    return "tab_" + std::to_string(index);
}
}

// Tab implementation -------------------------------------------------

Tab::Tab()
    : id(0), loadingProgress(0.0f), pinned(false), muted(false), incognito(false),
      hasChanges(false), currentHistoryIndex(-1), scrollX(0), scrollY(0) {
    state.isActive = false;
    state.isLoading = false;
    state.isPinned = false;
    state.isMuted = false;
    state.isCrashed = false;
}

Tab::~Tab() {
    state.engine.reset();
}

void Tab::setUrl(const String& newUrl) {
    url = newUrl;
    state.url = newUrl;
    addToHistory(newUrl);
}

String Tab::getUrl() const {
    return url;
}

void Tab::setTitle(const String& newTitle) {
    title = newTitle;
    state.title = newTitle;
}

String Tab::getTitle() const {
    return title;
}

void Tab::setFavicon(const String& icon) {
    favicon = icon;
}

String Tab::getFavicon() const {
    return favicon;
}

void Tab::setState(TabEntry newState) {
    state = std::move(newState);
}

TabEntry Tab::getState() const {
    return state;
}

void Tab::setLoadingProgress(float progress) {
    loadingProgress = progress;
}

float Tab::getLoadingProgress() const {
    return loadingProgress;
}

bool Tab::isLoading() const {
    return state.isLoading;
}

bool Tab::canGoBack() const {
    return currentHistoryIndex > 0;
}

bool Tab::canGoForward() const {
    return currentHistoryIndex >= 0 && currentHistoryIndex < static_cast<int>(forwardHistory.size());
}

void Tab::goBack() {
    if (!canGoBack()) return;
    --currentHistoryIndex;
    state.engine->navigateBack();
}

void Tab::goForward() {
    if (!canGoForward()) return;
    ++currentHistoryIndex;
    state.engine->navigateForward();
}

void Tab::reload() {
    state.engine->reload();
}

void Tab::stop() {
    state.engine->stopLoading();
}

void Tab::navigate(const String& targetUrl) {
    state.engine->loadURL(targetUrl);
    setUrl(targetUrl);
}

void Tab::setContent(const String& html) {
    pageContent = html;
}

String Tab::getContent() const {
    return pageContent;
}

void Tab::setScrollPosition(int xPos, int yPos) {
    scrollX = xPos;
    scrollY = yPos;
}

void Tab::getScrollPosition(int& xPos, int& yPos) const {
    xPos = scrollX;
    yPos = scrollY;
}

void Tab::setPinned(bool value) {
    pinned = value;
    state.isPinned = value;
}

bool Tab::isPinned() const {
    return pinned;
}

void Tab::setMuted(bool value) {
    muted = value;
    state.isMuted = value;
}

bool Tab::isMuted() const {
    return muted;
}

void Tab::setIncognito(bool value) {
    incognito = value;
}

bool Tab::isIncognito() const {
    return incognito;
}

void Tab::setEventCallback(TabEventCallback callback) {
    eventCallback = std::move(callback);
}

void Tab::triggerEvent(TabEventType type) {
    if (!eventCallback) return;
    TabEvent event(type, std::shared_ptr<Tab>(this, [](Tab*) {}));
    event.url = url;
    event.title = title;
    event.progress = loadingProgress;
    eventCallback(event);
}

String Tab::getDisplayTitle() const {
    if (!title.empty()) return title;
    if (!url.empty()) return url;
    return "New Tab";
}

bool Tab::hasUncommittedChanges() const {
    return hasChanges;
}

void Tab::markAsChanged(bool value) {
    hasChanges = value;
}

void Tab::addToHistory(const String& historyUrl) {
    backHistory.push_back(historyUrl);
    currentHistoryIndex = static_cast<int>(backHistory.size()) - 1;
    clearForwardHistory();
}

void Tab::clearForwardHistory() {
    forwardHistory.clear();
}

void Tab::updateHistoryIndex() {
    if (currentHistoryIndex >= static_cast<int>(backHistory.size())) {
        currentHistoryIndex = static_cast<int>(backHistory.size()) - 1;
    }
}

// TabManager implementation -------------------------------------------

TabManager::TabManager() {}
TabManager::~TabManager() {}

String TabManager::openTab(const String& url, bool foreground) {
    String tabId = generateTabId(tabs.size() + 1);

    TabEntry entry;
    entry.id = tabId;
    entry.url = url.empty() ? "about:blank" : url;
    entry.title = entry.url;
    entry.isActive = foreground || tabs.empty();
    entry.isLoading = true;
    entry.isPinned = false;
    entry.isMuted = false;
    entry.isCrashed = false;

    tabs.push_back(entry);

    if (entry.isActive) {
        activeTabId = entry.id;
        if (tabSwitchedCallback) tabSwitchedCallback(entry);
    }

    std::cout << "Opened tab: " << entry.id << " (" << entry.url << ")" << std::endl;
    return entry.id;
}

bool TabManager::closeTab(const String& tabId) {
    auto it = std::find_if(tabs.begin(), tabs.end(), [&](const TabEntry& t) { return t.id == tabId; });
    if (it == tabs.end()) return false;

    if (tabClosedCallback) tabClosedCallback(*it);
    tabs.erase(it);

    if (tabs.empty()) {
        activeTabId.clear();
    } else if (activeTabId == tabId) {
        activeTabId = tabs.front().id;
        if (tabSwitchedCallback) tabSwitchedCallback(tabs.front());
    }

    std::cout << "Closed tab: " << tabId << std::endl;
    return true;
}

bool TabManager::closeActiveTab() {
    if (activeTabId.empty()) return false;
    return closeTab(activeTabId);
}

bool TabManager::navigateActiveTab(const String& url) {
    auto* tab = getActiveTab();
    if (!tab) return false;
    
    tab->url = url;
    if (tab->engine) {
        tab->engine->loadURL(url);
    }
    return true;
}

bool TabManager::reloadActiveTab() {
    return reloadTab(activeTabId);
}

bool TabManager::switchToTab(const String& tabId) {
    auto it = std::find_if(tabs.begin(), tabs.end(), [&](const TabEntry& t) { return t.id == tabId; });
    if (it == tabs.end()) return false;

    activeTabId = tabId;
    for (auto& entry : tabs) entry.isActive = (entry.id == tabId);
    if (tabSwitchedCallback) tabSwitchedCallback(*it);
    std::cout << "Switched to tab: " << tabId << std::endl;
    return true;
}

bool TabManager::reloadTab(const String& tabId) {
    auto entry = getTabById(tabId);
    if (!entry) return false;

    if (entry->engine) {
        entry->engine->reload();
    }
    entry->isLoading = true;
    std::cout << "Reloaded tab: " << tabId << std::endl;
    return true;
}

bool TabManager::duplicateTab(const String& tabId) {
    auto entry = getTabById(tabId);
    if (!entry) return false;
    return !openTab(entry->url, true).empty();
}

bool TabManager::moveTab(const String& tabId, int newIndex) {
    auto it = std::find_if(tabs.begin(), tabs.end(), [&](const TabEntry& t) { return t.id == tabId; });
    if (it == tabs.end()) return false;
    if (newIndex < 0 || newIndex >= static_cast<int>(tabs.size())) return false;

    TabEntry entry = *it;
    tabs.erase(it);
    tabs.insert(tabs.begin() + newIndex, entry);
    std::cout << "Moved tab: " << tabId << " to index " << newIndex << std::endl;
    return true;
}

bool TabManager::pinTab(const String& tabId, bool pinned) {
    auto entry = getTabById(tabId);
    if (!entry) return false;
    entry->isPinned = pinned;
    std::cout << (pinned ? "Pinned" : "Unpinned") << " tab: " << tabId << std::endl;
    return true;
}

bool TabManager::muteTab(const String& tabId, bool muted) {
    auto entry = getTabById(tabId);
    if (!entry) return false;
    entry->isMuted = muted;
    std::cout << (muted ? "Muted" : "Unmuted") << " tab: " << tabId << std::endl;
    return true;
}

bool TabManager::restoreTab(const String& tabId) {
    auto entry = getTabById(tabId);
    if (!entry) return false;
    entry->isCrashed = false;
    entry->isLoading = false;
    std::cout << "Restored tab: " << tabId << std::endl;
    return true;
}

bool TabManager::crashTab(const String& tabId) {
    auto entry = getTabById(tabId);
    if (!entry) return false;
    entry->isCrashed = true;
    if (tabCrashedCallback) tabCrashedCallback(*entry);
    std::cout << "Crashed tab: " << tabId << std::endl;
    return true;
}

std::vector<TabEntry> TabManager::getAllTabs() const {
    return tabs;
}

TabEntry TabManager::getTabState(const String& tabId) const {
    auto it = std::find_if(tabs.begin(), tabs.end(), [&](const TabEntry& t) { return t.id == tabId; });
    return it != tabs.end() ? *it : TabEntry{};
}

String TabManager::getActiveTabId() const {
    return activeTabId;
}

TabEntry* TabManager::getActiveTab() {
    return getTabById(activeTabId);
}

const TabEntry* TabManager::getActiveTab() const {
    return getTabById(activeTabId);
}

TabEntry* TabManager::getTabById(const String& tabId) {
    auto it = std::find_if(tabs.begin(), tabs.end(), [&](const TabEntry& t) { return t.id == tabId; });
    return it != tabs.end() ? &(*it) : nullptr;
}

const TabEntry* TabManager::getTabById(const String& tabId) const {
    auto it = std::find_if(tabs.begin(), tabs.end(), [&](const TabEntry& t) { return t.id == tabId; });
    return it != tabs.end() ? &(*it) : nullptr;
}

int TabManager::getTabIndex(const String& tabId) const {
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (tabs[i].id == tabId) return static_cast<int>(i);
    }
    return -1;
}

int TabManager::getTabCount() const {
    return static_cast<int>(tabs.size());
}

bool TabManager::isolateTabProcess(const String& tabId) {
    (void)tabId;
    return true;
}

bool TabManager::mergeTabProcess(const String& tabId) {
    (void)tabId;
    return true;
}

void TabManager::setTabSwitchedCallback(std::function<void(const TabEntry&)> cb) {
    tabSwitchedCallback = std::move(cb);
}

void TabManager::setTabClosedCallback(std::function<void(const TabEntry&)> cb) {
    tabClosedCallback = std::move(cb);
}

void TabManager::setTabCrashedCallback(std::function<void(const TabEntry&)> cb) {
    tabCrashedCallback = std::move(cb);
}

} // namespace zepra