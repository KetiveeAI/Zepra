/**
 * @file SamplingProfiler.h
 * @brief Low-Overhead CPU Profiling
 * 
 * JSC Sampling Profiler pattern:
 * - Background sampling thread
 * - Stack capture with minimal overhead
 * - Flame graph generation
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <unordered_map>

namespace Zepra::Perf {

// =============================================================================
// Stack Frame
// =============================================================================

/**
 * @brief Single frame in a stack trace
 */
struct StackFrame {
    uintptr_t returnAddress;    // Code address
    std::string functionName;   // Function name (if resolved)
    std::string sourceFile;     // Source file (if available)
    int lineNumber = -1;        // Line number (if available)
    
    enum class Type : uint8_t {
        JS,         // JavaScript function
        Builtin,    // Built-in function
        Native,     // Native C++ function
        JIT,        // JIT-compiled code
        GC,         // Garbage collection
        Unknown
    };
    Type type = Type::Unknown;
    
    bool operator==(const StackFrame& other) const {
        return returnAddress == other.returnAddress;
    }
};

// =============================================================================
// Stack Sample
// =============================================================================

/**
 * @brief Complete stack trace at a point in time
 */
struct StackSample {
    std::chrono::steady_clock::time_point timestamp;
    std::vector<StackFrame> frames;
    uint32_t threadId;
    
    // Metadata
    bool wasInGC = false;
    bool wasInJIT = false;
    bool wasIdle = false;
};

// =============================================================================
// Profile Node (Aggregated)
// =============================================================================

/**
 * @brief Node in the profile tree
 */
struct ProfileNode {
    StackFrame frame;
    uint64_t selfSamples = 0;    // Samples where this is top of stack
    uint64_t totalSamples = 0;   // Samples including children
    std::vector<ProfileNode*> children;
    ProfileNode* parent = nullptr;
    
    double selfPercentage(uint64_t total) const {
        return total > 0 ? (selfSamples * 100.0 / total) : 0;
    }
    
    double totalPercentage(uint64_t total) const {
        return total > 0 ? (totalSamples * 100.0 / total) : 0;
    }
};

// =============================================================================
// Flame Graph Data
// =============================================================================

/**
 * @brief Data for flame graph visualization
 */
struct FlameGraphEntry {
    std::string name;
    int depth;
    uint64_t start;   // X position (sample index)
    uint64_t width;   // Width (sample count)
    StackFrame::Type type;
};

// =============================================================================
// Sampling Profiler
// =============================================================================

/**
 * @brief Low-overhead sampling CPU profiler
 */
class SamplingProfiler {
public:
    struct Config {
        std::chrono::microseconds interval{1000};  // 1ms = 1kHz
        size_t maxSamples = 100000;
        bool captureSourceInfo = true;
        bool profileNativeFrames = false;
    };
    
    explicit SamplingProfiler(const Config& config = {})
        : config_(config) {}
    
    ~SamplingProfiler() {
        Stop();
    }
    
    // Start profiling
    void Start() {
        if (running_.load()) return;
        
        running_.store(true);
        samples_.clear();
        
        samplerThread_ = std::thread([this]() {
            SamplerLoop();
        });
    }
    
    // Stop profiling
    void Stop() {
        running_.store(false);
        if (samplerThread_.joinable()) {
            samplerThread_.join();
        }
    }
    
    bool IsRunning() const { return running_.load(); }
    
    // Get samples
    const std::vector<StackSample>& Samples() const {
        return samples_;
    }
    
    // Build profile tree
    std::unique_ptr<ProfileNode> BuildProfileTree() const {
        auto root = std::make_unique<ProfileNode>();
        root->frame.functionName = "(root)";
        
        for (const auto& sample : samples_) {
            if (sample.frames.empty()) continue;
            
            ProfileNode* current = root.get();
            current->totalSamples++;
            
            // Walk stack bottom-up
            for (int i = sample.frames.size() - 1; i >= 0; i--) {
                const auto& frame = sample.frames[i];
                
                // Find or create child
                ProfileNode* child = nullptr;
                for (auto* c : current->children) {
                    if (c->frame == frame) {
                        child = c;
                        break;
                    }
                }
                
                if (!child) {
                    child = new ProfileNode();
                    child->frame = frame;
                    child->parent = current;
                    current->children.push_back(child);
                }
                
                child->totalSamples++;
                if (i == 0) {
                    child->selfSamples++;
                }
                
                current = child;
            }
        }
        
        return root;
    }
    
    // Generate flame graph data
    std::vector<FlameGraphEntry> GenerateFlameGraph() const {
        std::vector<FlameGraphEntry> entries;
        auto root = BuildProfileTree();
        
        GenerateFlameGraphRecursive(root.get(), 0, 0, entries);
        
        return entries;
    }
    
    // Get hot functions
    std::vector<std::pair<std::string, double>> GetHotFunctions(size_t limit = 10) const {
        std::unordered_map<std::string, uint64_t> functionCounts;
        
        for (const auto& sample : samples_) {
            if (!sample.frames.empty()) {
                functionCounts[sample.frames[0].functionName]++;
            }
        }
        
        std::vector<std::pair<std::string, uint64_t>> sorted(
            functionCounts.begin(), functionCounts.end());
        
        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<std::pair<std::string, double>> result;
        for (size_t i = 0; i < std::min(limit, sorted.size()); i++) {
            double pct = sorted[i].second * 100.0 / samples_.size();
            result.push_back({sorted[i].first, pct});
        }
        
        return result;
    }
    
private:
    void SamplerLoop() {
        while (running_.load()) {
            auto start = std::chrono::steady_clock::now();
            
            // Capture sample
            StackSample sample = CaptureStack();
            
            {
                std::lock_guard lock(mutex_);
                if (samples_.size() < config_.maxSamples) {
                    samples_.push_back(std::move(sample));
                }
            }
            
            // Sleep until next interval
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto sleepTime = config_.interval - 
                std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
            
            if (sleepTime.count() > 0) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }
    
    StackSample CaptureStack() {
        StackSample sample;
        sample.timestamp = std::chrono::steady_clock::now();
        sample.threadId = 0;  // Would get actual thread ID
        
        // Would walk the JS stack here
        // For now, placeholder
        
        return sample;
    }
    
    void GenerateFlameGraphRecursive(ProfileNode* node, int depth, 
                                      uint64_t& offset,
                                      std::vector<FlameGraphEntry>& entries) const {
        FlameGraphEntry entry;
        entry.name = node->frame.functionName;
        entry.depth = depth;
        entry.start = offset;
        entry.width = node->totalSamples;
        entry.type = node->frame.type;
        entries.push_back(entry);
        
        for (auto* child : node->children) {
            GenerateFlameGraphRecursive(child, depth + 1, offset, entries);
            offset += child->totalSamples;
        }
    }
    
    Config config_;
    std::atomic<bool> running_{false};
    std::thread samplerThread_;
    std::mutex mutex_;
    std::vector<StackSample> samples_;
};

// =============================================================================
// Profiler Manager
// =============================================================================

class ProfilerManager {
public:
    static ProfilerManager& Instance() {
        static ProfilerManager mgr;
        return mgr;
    }
    
    void StartCPUProfile(const std::string& name) {
        auto profiler = std::make_unique<SamplingProfiler>();
        profiler->Start();
        profiles_[name] = std::move(profiler);
    }
    
    void StopCPUProfile(const std::string& name) {
        auto it = profiles_.find(name);
        if (it != profiles_.end()) {
            it->second->Stop();
        }
    }
    
    SamplingProfiler* GetProfile(const std::string& name) {
        auto it = profiles_.find(name);
        return it != profiles_.end() ? it->second.get() : nullptr;
    }
    
private:
    ProfilerManager() = default;
    std::unordered_map<std::string, std::unique_ptr<SamplingProfiler>> profiles_;
};

} // namespace Zepra::Perf
