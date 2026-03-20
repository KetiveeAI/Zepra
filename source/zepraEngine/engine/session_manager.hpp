/**
 * @file session_manager.hpp
 * @brief Browser session persistence
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace Zepra::Engine {

struct TabState {
    int tabId;
    std::string url;
    std::string title;
    float scrollPosition;
    bool isPinned;
    std::vector<std::string> backHistory;
    std::vector<std::string> forwardHistory;
};

struct WindowState {
    int windowId;
    int x, y, width, height;
    bool isMaximized;
    bool isFullscreen;
    std::vector<TabState> tabs;
    int activeTabIndex;
};

struct SessionData {
    std::chrono::system_clock::time_point timestamp;
    std::vector<WindowState> windows;
    int activeWindowIndex;
};

/**
 * @class SessionManager
 * @brief Manages session state persistence and restoration
 */
class SessionManager {
public:
    SessionManager();
    ~SessionManager();
    
    // Save current session
    bool saveSession(const std::string& path = "");
    bool saveOnExit();
    
    // Load session
    bool loadSession(const std::string& path = "");
    SessionData getLastSession();
    
    // Auto-save
    void enableAutoSave(int intervalMs = 30000);
    void disableAutoSave();
    
    // Crash recovery
    bool hasCrashRecoveryData();
    SessionData getCrashRecoveryData();
    void clearCrashRecoveryData();
    
    // Session export/import
    bool exportSession(const std::string& path, const SessionData& data);
    SessionData importSession(const std::string& path);
    
    // Callbacks
    using RestoreCallback = std::function<void(const SessionData&)>;
    void setOnRestore(RestoreCallback callback) { onRestore_ = std::move(callback); }
    
    // Default session directory
    void setSessionDirectory(const std::string& dir) { sessionDir_ = dir; }
    std::string sessionDirectory() const { return sessionDir_; }
    
private:
    std::string sessionDir_;
    bool autoSaveEnabled_ = false;
    int autoSaveInterval_ = 30000;
    
    RestoreCallback onRestore_;
};

SessionManager& getSessionManager();

} // namespace Zepra::Engine
