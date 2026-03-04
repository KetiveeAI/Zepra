#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace ZepraBrowser {

// Forward declaration - full definition in src/browser/tab_manager.h
struct Tab;

/**
 * Tab Suspension State Machine
 * 
 * ACTIVE → SLEEP → LIGHT_SLEEP → DEEP_SLEEP → RESTORE → ACTIVE
 */
enum class TabSuspendState {
    ACTIVE,        // Fully active, all resources loaded
    SLEEP,         // 10 min inactive, reduced resources
    LIGHT_SLEEP,   // Video/audio delayed (1hr), minimal resources
    DEEP_SLEEP,    // OS idle/memory pressure, snapshot only
    RESTORING      // Being restored from snapshot
};

/**
 * Tab content type affects suspension timing
 */
enum class TabContentType {
    NORMAL,         // Regular page (10 min → SLEEP)
    VIDEO_PLAYING,  // Active video (1hr → LIGHT_SLEEP)
    AUDIO_PLAYING,  // Active audio (1hr → LIGHT_SLEEP)
    FORM_EDITING,   // User editing (30 min delay)
    DOWNLOAD_ACTIVE // Active download (never suspend)
};

/**
 * Snapshot for restoring suspended tabs
 */
struct TabSnapshot {
    std::string url;
    std::string title;
    std::string favicon;
    std::string dom_compressed;       // Zlib compressed DOM
    std::vector<uint8_t> scroll_pos;  // Scroll x,y
    
    struct FormField {
        std::string name;
        std::string value;
        std::string type;
    };
    std::vector<FormField> form_data;
    
    // Media state
    bool was_playing_video = false;
    bool was_playing_audio = false;
    float video_timestamp = 0.0f;
    std::string video_url;
    
    // Stats
    uint64_t memory_freed = 0;
    std::chrono::system_clock::time_point suspended_at;
    TabSuspendState previous_state = TabSuspendState::ACTIVE;
};

/**
 * State transition timings (minutes)
 */
struct SuspendPolicy {
    int active_to_sleep = 10;          // ACTIVE → SLEEP
    int sleep_to_light_sleep = 30;     // SLEEP → LIGHT_SLEEP
    int light_to_deep_on_idle = 5;     // LIGHT_SLEEP → DEEP_SLEEP (when OS idle)
    int video_delay_minutes = 60;      // Video tabs get 1hr before SLEEP
    int audio_delay_minutes = 60;      // Audio tabs get 1hr before SLEEP
    int form_delay_minutes = 30;       // Form editing delay
    bool enable_memory_pressure = true;// Force deep sleep on low memory
    int memory_threshold_mb = 100;     // Trigger deep sleep below this
    int max_deep_sleep_tabs = 50;      // Max tabs in DEEP_SLEEP
};

/**
 * Stats for monitoring
 */
struct SuspenderStats {
    size_t active_tabs = 0;
    size_t sleep_tabs = 0;
    size_t light_sleep_tabs = 0;
    size_t deep_sleep_tabs = 0;
    uint64_t total_memory_saved = 0;
    size_t total_suspensions = 0;
    size_t total_restorations = 0;
};

/**
 * Smart Tab Suspender with Multi-Level States
 */
class TabSuspender {
public:
    // Callbacks
    using MediaDetector = std::function<bool(int tabId)>;
    using MemoryCallback = std::function<uint64_t()>;  // Get available memory
    
    TabSuspender();
    ~TabSuspender();
    
    // === State Transitions ===
    TabSuspendState getState(int tabId) const;
    void setState(int tabId, TabSuspendState state);
    
    // Transition methods
    void transitionToSleep(Tab* tab);
    void transitionToLightSleep(Tab* tab);
    void transitionToDeepSleep(Tab* tab);
    void restore(Tab* tab);
    
    // === Check & Update ===
    void checkTab(Tab* tab);
    void checkAllTabs(const std::vector<Tab*>& tabs, int activeTabId);
    void tick();  // Called every second from main loop
    
    // === Content Type ===
    void setContentType(int tabId, TabContentType type);
    TabContentType getContentType(int tabId) const;
    
    // === Callbacks ===
    void setVideoDetector(MediaDetector cb) { videoDetector_ = cb; }
    void setAudioDetector(MediaDetector cb) { audioDetector_ = cb; }
    void setMemoryCallback(MemoryCallback cb) { memoryCallback_ = cb; }
    
    // === Host State ===
    void notifyHostIdle(bool idle);
    void notifyMemoryPressure(bool pressure);
    bool isHostIdle() const { return hostIdle_; }
    bool hasMemoryPressure() const { return memoryPressure_; }
    
    // === Policy ===
    void setPolicy(const SuspendPolicy& policy) { policy_ = policy; }
    SuspendPolicy getPolicy() const { return policy_; }
    
    // === Enable/Disable ===
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }
    
    // === Snapshots ===
    TabSnapshot* getSnapshot(int tabId);
    bool hasSnapshot(int tabId) const;
    void clearSnapshot(int tabId);
    
    // === Stats ===
    SuspenderStats getStats() const;
    
private:
    bool enabled_ = true;
    bool hostIdle_ = false;
    bool memoryPressure_ = false;
    SuspendPolicy policy_;
    
    // State per tab
    std::unordered_map<int, TabSuspendState> states_;
    std::unordered_map<int, TabContentType> contentTypes_;
    std::unordered_map<int, TabSnapshot> snapshots_;
    std::unordered_map<int, std::chrono::system_clock::time_point> stateEntryTime_;
    
    // Callbacks
    MediaDetector videoDetector_;
    MediaDetector audioDetector_;
    MemoryCallback memoryCallback_;
    
    // Helpers
    std::string compressDom(const std::string& html);
    std::string decompressDom(const std::string& compressed);
    int getTimeoutForState(int tabId, TabSuspendState currentState) const;
    bool shouldTransition(int tabId, TabSuspendState from, TabSuspendState to) const;
    TabSnapshot createSnapshot(Tab* tab);
    void applySnapshot(Tab* tab, const TabSnapshot& snapshot);
    void updateStats();
    
    mutable SuspenderStats stats_;
};

} // namespace ZepraBrowser
