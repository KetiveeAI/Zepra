/**
 * @file performance_panel.hpp
 * @brief Performance Panel - CPU profiling, flame charts, timeline
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>

namespace Zepra::DevTools {

/**
 * @brief Profile node (call tree)
 */
struct ProfileNode {
    int id;
    std::string functionName;
    std::string url;
    int lineNumber;
    int columnNumber;
    
    double selfTime;        // Time in this function only
    double totalTime;       // Time including children
    int hitCount;
    
    std::vector<int> childrenIds;
    int parentId;
    
    // For flame chart
    double startTime;
    double endTime;
    int depth;
};

/**
 * @brief CPU profile
 */
struct CPUProfile {
    int id;
    std::string title;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    double duration;        // Milliseconds
    
    std::vector<ProfileNode> nodes;
    int rootNodeId;
    
    // Sample data
    std::vector<int> samples;       // Node IDs
    std::vector<double> timestamps; // Sample times
};

/**
 * @brief Timeline event types
 */
enum class TimelineEventType {
    Task,
    Script,
    Layout,
    Paint,
    Composite,
    Style,
    GC,
    ParseHTML,
    ParseCSS,
    XHR,
    Timer,
    Event,
    Animation
};

/**
 * @brief Timeline event
 */
struct TimelineEvent {
    int id;
    TimelineEventType type;
    std::string name;
    double startTime;
    double endTime;
    double duration;
    int depth;      // Nesting depth
    
    std::string url;
    int lineNumber;
    
    // Additional data
    std::string details;
    int parentId;
    std::vector<int> childrenIds;
};

/**
 * @brief Frame timing info
 */
struct FrameInfo {
    double startTime;
    double duration;
    double fps;
    bool dropped;
    
    double scriptTime;
    double layoutTime;
    double paintTime;
    double compositeTime;
    double idleTime;
};

/**
 * @brief Recording settings
 */
struct PerformanceSettings {
    bool recordNetwork = true;
    bool recordJS = true;
    bool recordGC = true;
    bool recordLayout = true;
    bool recordPaint = true;
    bool screenshots = false;
    bool highlightLongTasks = true;
    double longTaskThreshold = 50.0;  // ms
};

/**
 * @brief Performance callbacks
 */
using ProfileCallback = std::function<void(const CPUProfile&)>;
using FrameCallback = std::function<void(const FrameInfo&)>;

/**
 * @brief Performance Panel - Profiling & Timeline
 */
class PerformancePanel {
public:
    PerformancePanel();
    ~PerformancePanel();
    
    // --- Recording ---
    
    /**
     * @brief Start recording
     */
    void startRecording();
    
    /**
     * @brief Stop recording
     */
    CPUProfile stopRecording();
    
    /**
     * @brief Is recording
     */
    bool isRecording() const { return recording_; }
    
    /**
     * @brief Set recording settings
     */
    void setSettings(const PerformanceSettings& settings);
    PerformanceSettings settings() const { return settings_; }
    
    // --- Profiles ---
    
    /**
     * @brief Get all recorded profiles
     */
    const std::vector<CPUProfile>& profiles() const { return profiles_; }
    
    /**
     * @brief Select a profile
     */
    void selectProfile(int id);
    int selectedProfile() const { return selectedProfileId_; }
    
    /**
     * @brief Delete profile
     */
    void deleteProfile(int id);
    
    /**
     * @brief Clear all profiles
     */
    void clearProfiles();
    
    // --- Timeline ---
    
    /**
     * @brief Get timeline events
     */
    const std::vector<TimelineEvent>& timeline() const { return timeline_; }
    
    /**
     * @brief Get frame info
     */
    const std::vector<FrameInfo>& frames() const { return frames_; }
    
    // --- Analysis ---
    
    /**
     * @brief Get hottest functions (by self time)
     */
    std::vector<ProfileNode> getHotFunctions(int count = 10);
    
    /**
     * @brief Get heavy call paths
     */
    std::vector<std::vector<ProfileNode>> getHeavyPaths(int count = 5);
    
    /**
     * @brief Get long tasks
     */
    std::vector<TimelineEvent> getLongTasks();
    
    /**
     * @brief Get summary stats
     */
    struct Summary {
        double totalTime;
        double scriptTime;
        double layoutTime;
        double paintTime;
        double gcTime;
        double idleTime;
        double avgFPS;
        int droppedFrames;
    };
    
    Summary getSummary() const;
    
    // --- Export ---
    
    /**
     * @brief Export as JSON (Chrome-compatible format)
     */
    std::string exportProfile(int id);
    
    /**
     * @brief Import profile
     */
    int importProfile(const std::string& json);
    
    // --- Callbacks ---
    void onProfileComplete(ProfileCallback callback);
    void onFrame(FrameCallback callback);
    
    // --- UI ---
    void update();
    void render();
    
private:
    void renderSummary();
    void renderTimeline();
    void renderFlameChart();
    void renderCallTree();
    void renderBottomUp();
    
    std::vector<CPUProfile> profiles_;
    std::vector<TimelineEvent> timeline_;
    std::vector<FrameInfo> frames_;
    
    int selectedProfileId_ = -1;
    int nextProfileId_ = 1;
    bool recording_ = false;
    
    PerformanceSettings settings_;
    std::chrono::system_clock::time_point recordStart_;
    
    // View settings
    double zoomLevel_ = 1.0;
    double scrollOffset_ = 0.0;
    
    std::vector<ProfileCallback> profileCallbacks_;
    std::vector<FrameCallback> frameCallbacks_;
};

} // namespace Zepra::DevTools
