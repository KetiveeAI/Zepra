/**
 * @file jit_profiler.cpp
 * @brief JIT profiler implementation
 */

#include "jit/jit_profiler.hpp"
#include <algorithm>

namespace Zepra::JIT {

bool JITProfiler::recordCall(uintptr_t functionId) {
    if (!enabled_) return false;
    
    // Check if we're at capacity
    if (profiles_.find(functionId) == profiles_.end()) {
        if (profiles_.size() >= MAX_FUNCTIONS) {
            // At capacity - no more tracking (bounded memory)
            return false;
        }
        // New function - create profile
        profiles_[functionId] = FunctionProfile{};
    }
    
    auto& profile = profiles_[functionId];
    uint32_t prevCount = profile.callCount;
    profile.callCount++;
    
    // Check if we just crossed the hot threshold
    bool becameHot = (prevCount < FunctionProfile::HOT_THRESHOLD && 
                      profile.callCount >= FunctionProfile::HOT_THRESHOLD);
    
    if (becameHot) {
        profile.markedForCompilation = true;
    }
    
    return becameHot;
}

void JITProfiler::recordLoopIteration(uintptr_t functionId) {
    if (!enabled_) return;
    
    auto it = profiles_.find(functionId);
    if (it != profiles_.end()) {
        it->second.loopIterations++;
    }
}

FunctionProfile* JITProfiler::getProfile(uintptr_t functionId) {
    auto it = profiles_.find(functionId);
    if (it != profiles_.end()) {
        return &it->second;
    }
    return nullptr;
}

const FunctionProfile* JITProfiler::getProfile(uintptr_t functionId) const {
    auto it = profiles_.find(functionId);
    if (it != profiles_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<uintptr_t> JITProfiler::getHotFunctions() const {
    std::vector<uintptr_t> hot;
    hot.reserve(profiles_.size() / 4); // Estimate 25% hot
    
    for (const auto& [id, profile] : profiles_) {
        if (profile.isHot()) {
            hot.push_back(id);
        }
    }
    
    // Sort by call count (hottest first)
    std::sort(hot.begin(), hot.end(), [this](uintptr_t a, uintptr_t b) {
        return profiles_.at(a).callCount > profiles_.at(b).callCount;
    });
    
    return hot;
}

size_t JITProfiler::hotFunctionCount() const {
    size_t count = 0;
    for (const auto& [id, profile] : profiles_) {
        if (profile.isHot()) {
            count++;
        }
    }
    return count;
}

void JITProfiler::reset() {
    profiles_.clear();
}

// Global singleton
JITProfiler& getProfiler() {
    static JITProfiler instance;
    return instance;
}

} // namespace Zepra::JIT
