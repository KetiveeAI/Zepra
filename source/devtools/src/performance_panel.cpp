/**
 * @file performance_panel.cpp
 * @brief Performance profiler panel implementation
 */

#include "devtools/performance_panel.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Zepra::DevTools {

PerformancePanel::PerformancePanel() : DevToolsPanel("Performance") {}

PerformancePanel::~PerformancePanel() = default;

void PerformancePanel::render() {
    // Render timeline, flame chart, and stats
}

void PerformancePanel::refresh() {}

void PerformancePanel::startRecording() {
    isRecording_ = true;
    recordingStart_ = std::chrono::steady_clock::now();
    samples_.clear();
    events_.clear();
    
    if (onRecordingStart_) {
        onRecordingStart_();
    }
}

void PerformancePanel::stopRecording() {
    isRecording_ = false;
    recordingEnd_ = std::chrono::steady_clock::now();
    
    if (onRecordingStop_) {
        onRecordingStop_();
    }
}

void PerformancePanel::addSample(const ProfileSample& sample) {
    if (isRecording_) {
        std::lock_guard<std::mutex> lock(mutex_);
        samples_.push_back(sample);
    }
}

void PerformancePanel::addEvent(const TimelineEvent& event) {
    if (isRecording_) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
    }
}

ProfileData PerformancePanel::getProfileData() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ProfileData data;
    data.samples = samples_;
    data.events = events_;
    data.duration = std::chrono::duration<double, std::milli>(
        recordingEnd_ - recordingStart_).count();
    
    // Build call tree
    data.callTree = buildCallTree();
    
    return data;
}

std::vector<HotFunction> PerformancePanel::getHotFunctions(int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<std::string, double> functionTimes;
    
    for (const auto& sample : samples_) {
        double timePerFrame = sample.duration / sample.stack.size();
        for (const auto& frame : sample.stack) {
            functionTimes[frame.function] += timePerFrame;
        }
    }
    
    std::vector<HotFunction> hot;
    for (const auto& [name, time] : functionTimes) {
        HotFunction hf;
        hf.name = name;
        hf.selfTime = time;
        hf.totalTime = time;  // Simplified
        hot.push_back(hf);
    }
    
    std::sort(hot.begin(), hot.end(), 
        [](const HotFunction& a, const HotFunction& b) {
            return a.selfTime > b.selfTime;
        });
    
    if (static_cast<int>(hot.size()) > limit) {
        hot.resize(limit);
    }
    
    return hot;
}

PerformanceStats PerformancePanel::getStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    PerformanceStats stats;
    stats.totalTime = std::chrono::duration<double, std::milli>(
        recordingEnd_ - recordingStart_).count();
    stats.sampleCount = samples_.size();
    stats.eventCount = events_.size();
    
    // Calculate JS time
    for (const auto& sample : samples_) {
        stats.jsTime += sample.duration;
    }
    
    // Calculate scripting, rendering, etc from events
    for (const auto& event : events_) {
        if (event.category == "scripting") {
            stats.scriptingTime += event.duration;
        } else if (event.category == "rendering") {
            stats.renderingTime += event.duration;
        } else if (event.category == "painting") {
            stats.paintingTime += event.duration;
        }
    }
    
    return stats;
}

void PerformancePanel::clearData() {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.clear();
    events_.clear();
}

std::string PerformancePanel::formatDuration(double ms) {
    std::ostringstream oss;
    if (ms < 1) {
        oss << std::fixed << std::setprecision(2) << (ms * 1000) << " μs";
    } else if (ms < 1000) {
        oss << std::fixed << std::setprecision(2) << ms << " ms";
    } else {
        oss << std::fixed << std::setprecision(2) << (ms / 1000) << " s";
    }
    return oss.str();
}

CallTreeNode PerformancePanel::buildCallTree() {
    CallTreeNode root;
    root.name = "(root)";
    root.selfTime = 0;
    root.totalTime = 0;
    
    // Simplified tree building - in real implementation would be more complex
    for (const auto& sample : samples_) {
        if (!sample.stack.empty()) {
            root.totalTime += sample.duration;
        }
    }
    
    return root;
}

} // namespace Zepra::DevTools
