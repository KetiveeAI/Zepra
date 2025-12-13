/**
 * @file sources_panel.cpp
 * @brief JavaScript sources/debugger panel implementation
 */

#include "devtools/sources_panel.hpp"
#include <sstream>
#include <algorithm>

namespace Zepra::DevTools {

SourcesPanel::SourcesPanel() : DevToolsPanel("Sources") {}

SourcesPanel::~SourcesPanel() = default;

void SourcesPanel::render() {
    // Render source tree, code view, and debugger controls
}

void SourcesPanel::refresh() {
    if (onRefresh_) {
        onRefresh_();
    }
}

void SourcesPanel::addSourceFile(const SourceFile& file) {
    sources_[file.url] = file;
}

void SourcesPanel::openFile(const std::string& url) {
    auto it = sources_.find(url);
    if (it != sources_.end()) {
        currentFileUrl_ = url;
        if (onFileOpened_) {
            onFileOpened_(it->second);
        }
    }
}

const SourceFile* SourcesPanel::getCurrentFile() {
    auto it = sources_.find(currentFileUrl_);
    if (it != sources_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> SourcesPanel::getSourceUrls() {
    std::vector<std::string> urls;
    urls.reserve(sources_.size());
    for (const auto& [url, file] : sources_) {
        urls.push_back(url);
    }
    std::sort(urls.begin(), urls.end());
    return urls;
}

// Breakpoints
int SourcesPanel::setBreakpoint(const std::string& url, int line, 
                                 const std::string& condition) {
    Breakpoint bp;
    bp.id = nextBreakpointId_++;
    bp.url = url;
    bp.line = line;
    bp.condition = condition;
    bp.enabled = true;
    
    breakpoints_[bp.id] = bp;
    
    if (onBreakpointSet_) {
        onBreakpointSet_(bp);
    }
    
    return bp.id;
}

void SourcesPanel::removeBreakpoint(int breakpointId) {
    auto it = breakpoints_.find(breakpointId);
    if (it != breakpoints_.end()) {
        if (onBreakpointRemoved_) {
            onBreakpointRemoved_(breakpointId);
        }
        breakpoints_.erase(it);
    }
}

void SourcesPanel::toggleBreakpoint(int breakpointId) {
    auto it = breakpoints_.find(breakpointId);
    if (it != breakpoints_.end()) {
        it->second.enabled = !it->second.enabled;
    }
}

void SourcesPanel::clearAllBreakpoints() {
    breakpoints_.clear();
}

std::vector<Breakpoint> SourcesPanel::getBreakpoints() {
    std::vector<Breakpoint> result;
    result.reserve(breakpoints_.size());
    for (const auto& [id, bp] : breakpoints_) {
        result.push_back(bp);
    }
    return result;
}

std::vector<Breakpoint> SourcesPanel::getBreakpointsForFile(const std::string& url) {
    std::vector<Breakpoint> result;
    for (const auto& [id, bp] : breakpoints_) {
        if (bp.url == url) {
            result.push_back(bp);
        }
    }
    return result;
}

// Execution control
void SourcesPanel::pause() {
    if (onPause_) onPause_();
}

void SourcesPanel::resume() {
    if (onResume_) onResume_();
}

void SourcesPanel::stepOver() {
    if (onStepOver_) onStepOver_();
}

void SourcesPanel::stepInto() {
    if (onStepInto_) onStepInto_();
}

void SourcesPanel::stepOut() {
    if (onStepOut_) onStepOut_();
}

// Paused state
void SourcesPanel::setPausedAt(const std::string& url, int line) {
    isPaused_ = true;
    pausedUrl_ = url;
    pausedLine_ = line;
    
    // Open the file if not already open
    if (currentFileUrl_ != url) {
        openFile(url);
    }
}

void SourcesPanel::setResumed() {
    isPaused_ = false;
    pausedUrl_.clear();
    pausedLine_ = -1;
}

void SourcesPanel::setCallStack(const std::vector<CallFrame>& frames) {
    callStack_ = frames;
}

void SourcesPanel::setLocalVariables(const std::vector<DebugVariable>& vars) {
    localVariables_ = vars;
}

} // namespace Zepra::DevTools
