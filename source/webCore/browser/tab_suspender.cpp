// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file tab_suspender.cpp
 * @brief Multi-level tab suspension state machine implementation
 * 
 * State Machine:
 * ACTIVE → SLEEP → LIGHT_SLEEP → DEEP_SLEEP → RESTORE → ACTIVE
 */

#include "browser/tab_suspender.h"
#include "../../source/zepraEngine/include/engine/ui/tab_manager.h"  // Use the real Tab from zepra namespace
#include <iostream>
#include <zlib.h>
#include <cstring>
#include <algorithm>

namespace ZepraBrowser {

// Use zepra::Tab via namespace alias for compatibility
using Tab = zepra::Tab;


TabSuspender::TabSuspender() {
    std::cout << "[TabSuspender] Initialized - States: ACTIVE→SLEEP(" 
              << policy_.active_to_sleep << "min)→LIGHT_SLEEP→DEEP_SLEEP" << std::endl;
}

TabSuspender::~TabSuspender() {}

// === State Management ===

TabSuspendState TabSuspender::getState(int tabId) const {
    auto it = states_.find(tabId);
    return it != states_.end() ? it->second : TabSuspendState::ACTIVE;
}

void TabSuspender::setState(int tabId, TabSuspendState state) {
    states_[tabId] = state;
    stateEntryTime_[tabId] = std::chrono::system_clock::now();
    
    const char* stateName = "UNKNOWN";
    switch (state) {
        case TabSuspendState::ACTIVE: stateName = "ACTIVE"; break;
        case TabSuspendState::SLEEP: stateName = "SLEEP"; break;
        case TabSuspendState::LIGHT_SLEEP: stateName = "LIGHT_SLEEP"; break;
        case TabSuspendState::DEEP_SLEEP: stateName = "DEEP_SLEEP"; break;
        case TabSuspendState::RESTORING: stateName = "RESTORING"; break;
    }
    std::cout << "[TabSuspender] Tab " << tabId << " → " << stateName << std::endl;
}

// === State Transitions ===

void TabSuspender::transitionToSleep(Tab* tab) {
    if (!tab) return;
    
    // SLEEP: Reduce resources, keep DOM in memory
    tab->setSuspended(true);
    setState(tab->id, TabSuspendState::SLEEP);
    
    stats_.sleep_tabs++;
    stats_.total_suspensions++;
    
    std::cout << "[TabSuspender] Tab " << tab->id << " entered SLEEP (resources reduced)" << std::endl;
}

void TabSuspender::transitionToLightSleep(Tab* tab) {
    if (!tab) return;
    
    // LIGHT_SLEEP: Minimal resources, prepare snapshot
    TabSnapshot snapshot = createSnapshot(tab);
    snapshots_[tab->id] = snapshot;
    
    // Clear some page content but keep basic state
    tab->pageContent.clear();
    tab->pageContent.shrink_to_fit();
    
    setState(tab->id, TabSuspendState::LIGHT_SLEEP);
    
    stats_.light_sleep_tabs++;
    stats_.total_memory_saved += snapshot.memory_freed;
    
    std::cout << "[TabSuspender] Tab " << tab->id << " entered LIGHT_SLEEP (saved " 
              << snapshot.memory_freed / 1024 << "KB)" << std::endl;
}

void TabSuspender::transitionToDeepSleep(Tab* tab) {
    if (!tab) return;
    
    // DEEP_SLEEP: Only snapshot, all resources freed
    if (!hasSnapshot(tab->id)) {
        TabSnapshot snapshot = createSnapshot(tab);
        snapshots_[tab->id] = snapshot;
    }
    
    // Clear everything
    tab->pageContent.clear();
    tab->pageContent.shrink_to_fit();
    
    setState(tab->id, TabSuspendState::DEEP_SLEEP);
    
    stats_.deep_sleep_tabs++;
    
    std::cout << "[TabSuspender] Tab " << tab->id << " entered DEEP_SLEEP (full hibernation)" << std::endl;
}

void TabSuspender::restore(Tab* tab) {
    if (!tab) return;
    
    setState(tab->id, TabSuspendState::RESTORING);
    
    // Restore from snapshot if available
    if (auto* snapshot = getSnapshot(tab->id)) {
        applySnapshot(tab, *snapshot);
        clearSnapshot(tab->id);
    }
    
    tab->setSuspended(false);
    setState(tab->id, TabSuspendState::ACTIVE);
    
    stats_.total_restorations++;
    updateStats();
    
    std::cout << "[TabSuspender] Tab " << tab->id << " RESTORED to ACTIVE" << std::endl;
}

// === Check Logic ===

void TabSuspender::checkTab(Tab* tab) {
    if (!enabled_ || !tab) return;
    
    TabSuspendState currentState = getState(tab->id);
    
    // Never suspend active tab
    if (tab->isActive()) {
        if (currentState != TabSuspendState::ACTIVE) {
            restore(tab);
        }
        return;
    }
    
    // Check content type - downloads never suspend
    TabContentType type = getContentType(tab->id);
    if (type == TabContentType::DOWNLOAD_ACTIVE) return;
    
    // State transitions based on current state
    switch (currentState) {
        case TabSuspendState::ACTIVE:
            if (shouldTransition(tab->id, TabSuspendState::ACTIVE, TabSuspendState::SLEEP)) {
                transitionToSleep(tab);
            }
            break;
            
        case TabSuspendState::SLEEP:
            if (shouldTransition(tab->id, TabSuspendState::SLEEP, TabSuspendState::LIGHT_SLEEP)) {
                transitionToLightSleep(tab);
            }
            break;
            
        case TabSuspendState::LIGHT_SLEEP:
            // Only go to DEEP_SLEEP on memory pressure or host idle
            if ((memoryPressure_ || hostIdle_) && 
                shouldTransition(tab->id, TabSuspendState::LIGHT_SLEEP, TabSuspendState::DEEP_SLEEP)) {
                transitionToDeepSleep(tab);
            }
            break;
            
        case TabSuspendState::DEEP_SLEEP:
            // Stay in deep sleep until user activates
            break;
            
        case TabSuspendState::RESTORING:
            // Wait for restore to complete
            break;
    }
}

void TabSuspender::checkAllTabs(const std::vector<Tab*>& tabs, int activeTabId) {
    if (!enabled_) return;
    
    // Check memory pressure
    if (memoryCallback_ && policy_.enable_memory_pressure) {
        uint64_t availMem = memoryCallback_();
        memoryPressure_ = (availMem < policy_.memory_threshold_mb * 1024 * 1024);
    }
    
    for (Tab* tab : tabs) {
        if (!tab) continue;
        if (tab->id == activeTabId) {
            // Ensure active tab is ACTIVE
            if (getState(tab->id) != TabSuspendState::ACTIVE) {
                restore(tab);
            }
            continue;
        }
        checkTab(tab);
    }
    
    updateStats();
}

void TabSuspender::tick() {
    // Called every second - could check for memory pressure here
}

// === Content Type ===

void TabSuspender::setContentType(int tabId, TabContentType type) {
    contentTypes_[tabId] = type;
}

TabContentType TabSuspender::getContentType(int tabId) const {
    auto it = contentTypes_.find(tabId);
    return it != contentTypes_.end() ? it->second : TabContentType::NORMAL;
}

// === Host State ===

void TabSuspender::notifyHostIdle(bool idle) {
    if (hostIdle_ != idle) {
        hostIdle_ = idle;
        std::cout << "[TabSuspender] Host idle: " << (idle ? "YES" : "NO") << std::endl;
    }
}

void TabSuspender::notifyMemoryPressure(bool pressure) {
    if (memoryPressure_ != pressure) {
        memoryPressure_ = pressure;
        std::cout << "[TabSuspender] Memory pressure: " << (pressure ? "HIGH" : "NORMAL") << std::endl;
    }
}

// === Snapshots ===

TabSnapshot* TabSuspender::getSnapshot(int tabId) {
    auto it = snapshots_.find(tabId);
    return it != snapshots_.end() ? &it->second : nullptr;
}

bool TabSuspender::hasSnapshot(int tabId) const {
    return snapshots_.find(tabId) != snapshots_.end();
}

void TabSuspender::clearSnapshot(int tabId) {
    snapshots_.erase(tabId);
}

// === Stats ===

SuspenderStats TabSuspender::getStats() const {
    return stats_;
}

void TabSuspender::updateStats() {
    stats_.active_tabs = 0;
    stats_.sleep_tabs = 0;
    stats_.light_sleep_tabs = 0;
    stats_.deep_sleep_tabs = 0;
    
    for (const auto& [tabId, state] : states_) {
        switch (state) {
            case TabSuspendState::ACTIVE: stats_.active_tabs++; break;
            case TabSuspendState::SLEEP: stats_.sleep_tabs++; break;
            case TabSuspendState::LIGHT_SLEEP: stats_.light_sleep_tabs++; break;
            case TabSuspendState::DEEP_SLEEP: stats_.deep_sleep_tabs++; break;
            default: break;
        }
    }
}

// === Helpers ===

int TabSuspender::getTimeoutForState(int tabId, TabSuspendState currentState) const {
    TabContentType type = getContentType(tabId);
    
    // Video/Audio tabs get longer timeouts
    if (type == TabContentType::VIDEO_PLAYING || type == TabContentType::AUDIO_PLAYING) {
        if (currentState == TabSuspendState::ACTIVE) {
            return policy_.video_delay_minutes;
        }
    }
    
    // Form editing gets moderate delay
    if (type == TabContentType::FORM_EDITING) {
        if (currentState == TabSuspendState::ACTIVE) {
            return policy_.form_delay_minutes;
        }
    }
    
    // Normal timeouts
    switch (currentState) {
        case TabSuspendState::ACTIVE:
            return policy_.active_to_sleep;
        case TabSuspendState::SLEEP:
            return policy_.sleep_to_light_sleep;
        case TabSuspendState::LIGHT_SLEEP:
            return policy_.light_to_deep_on_idle;
        default:
            return 9999;
    }
}

bool TabSuspender::shouldTransition(int tabId, TabSuspendState from, TabSuspendState to) const {
    auto it = stateEntryTime_.find(tabId);
    if (it == stateEntryTime_.end()) return false;
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - it->second).count();
    
    int timeout = getTimeoutForState(tabId, from);
    return elapsed >= timeout;
}

TabSnapshot TabSuspender::createSnapshot(Tab* tab) {
    TabSnapshot snapshot;
    
    snapshot.url = tab->getCurrentUrl();
    snapshot.title = tab->getTitle();
    snapshot.previous_state = getState(tab->id);
    snapshot.suspended_at = std::chrono::system_clock::now();
    
    // Compress DOM
    if (!tab->pageContent.empty()) {
        snapshot.dom_compressed = compressDom(tab->pageContent);
        snapshot.memory_freed = tab->pageContent.size();
    }
    
    // Save scroll position
    snapshot.scroll_pos.resize(8);
    memcpy(snapshot.scroll_pos.data(), &tab->scrollY, 4);
    
    // Check media state
    TabContentType type = getContentType(tab->id);
    snapshot.was_playing_video = (type == TabContentType::VIDEO_PLAYING);
    snapshot.was_playing_audio = (type == TabContentType::AUDIO_PLAYING);
    
    return snapshot;
}

void TabSuspender::applySnapshot(Tab* tab, const TabSnapshot& snapshot) {
    // Decompress DOM
    if (!snapshot.dom_compressed.empty()) {
        tab->pageContent = decompressDom(snapshot.dom_compressed);
    }
    
    // Restore scroll
    if (snapshot.scroll_pos.size() >= 4) {
        memcpy(&tab->scrollY, snapshot.scroll_pos.data(), 4);
    }
}

std::string TabSuspender::compressDom(const std::string& html) {
    if (html.empty()) return "";
    
    uLong sourceLen = html.size();
    uLong destLen = compressBound(sourceLen);
    
    std::vector<Bytef> compressed(destLen);
    
    int result = compress(compressed.data(), &destLen, 
                         reinterpret_cast<const Bytef*>(html.data()), sourceLen);
    
    if (result != Z_OK) return html;
    
    std::string output(4 + destLen, '\0');
    memcpy(&output[0], &sourceLen, 4);
    memcpy(&output[4], compressed.data(), destLen);
    
    return output;
}

std::string TabSuspender::decompressDom(const std::string& compressed) {
    if (compressed.size() < 4) return compressed;
    
    uLong originalLen;
    memcpy(&originalLen, compressed.data(), 4);
    
    std::vector<Bytef> decompressed(originalLen);
    uLong destLen = originalLen;
    
    int result = uncompress(decompressed.data(), &destLen,
                           reinterpret_cast<const Bytef*>(compressed.data() + 4),
                           compressed.size() - 4);
    
    if (result != Z_OK) return "";
    
    return std::string(reinterpret_cast<char*>(decompressed.data()), destLen);
}

} // namespace ZepraBrowser
