/**
 * @file breakpoint_manager.cpp
 * @brief Centralized breakpoint management
 * 
 * Provides efficient breakpoint storage and lookup with:
 * - Fast bytecode-offset → breakpoint mapping
 * - Source map integration for transpiled code
 * - Conditional breakpoint evaluation
 * - Logpoint support
 */

#include "debugger/debugger.hpp"
#include "config.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <memory>
#include <cassert>

namespace Zepra::Debug {

// =============================================================================
// BreakpointType - Types of breakpoints
// =============================================================================

enum class BreakpointType {
    Line,           // Standard line breakpoint
    Conditional,    // Break only when condition is true
    Logpoint,       // Log message without pausing
    Exception,      // Break on exception
    DataWatch       // Break on data change (future)
};

// =============================================================================
// BreakpointInfo - Extended breakpoint information
// =============================================================================

struct BreakpointInfo {
    uint32_t id;
    BreakpointLocation location;
    BreakpointType type;
    bool enabled;
    std::string condition;       // For conditional breakpoints
    std::string logMessage;      // For logpoints
    uint32_t hitCount;
    uint32_t ignoreCount;        // Skip this many hits before breaking
    
    // Bytecode mapping (populated after compilation)
    size_t bytecodeOffset;
    bool bytecodeResolved;
    
    BreakpointInfo()
        : id(0), type(BreakpointType::Line), enabled(true)
        , hitCount(0), ignoreCount(0)
        , bytecodeOffset(0), bytecodeResolved(false) {}
};

// =============================================================================
// SourceLocation - For source map lookup
// =============================================================================

struct SourceLocation {
    std::string sourceFile;
    uint32_t line;
    uint32_t column;
    
    std::string key() const {
        return sourceFile + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

// =============================================================================
// SourceMap - Map transpiled code back to original source
// =============================================================================

class SourceMap {
public:
    struct Mapping {
        // Generated (transpiled) location
        std::string generatedFile;
        uint32_t generatedLine;
        uint32_t generatedColumn;
        
        // Original source location
        std::string originalFile;
        uint32_t originalLine;
        uint32_t originalColumn;
    };
    
    /**
     * @brief Add a source mapping
     */
    void addMapping(const Mapping& mapping) {
        std::string key = mapping.generatedFile + ":" + 
                         std::to_string(mapping.generatedLine);
        mappings_[key] = mapping;
    }
    
    /**
     * @brief Look up original location from generated
     */
    std::optional<SourceLocation> getOriginalLocation(
        const std::string& generatedFile, 
        uint32_t generatedLine) const {
        
        std::string key = generatedFile + ":" + std::to_string(generatedLine);
        auto it = mappings_.find(key);
        if (it == mappings_.end()) {
            return std::nullopt;
        }
        
        return SourceLocation{
            it->second.originalFile,
            it->second.originalLine,
            it->second.originalColumn
        };
    }
    
    /**
     * @brief Look up generated location from original
     */
    std::optional<SourceLocation> getGeneratedLocation(
        const std::string& originalFile,
        uint32_t originalLine) const {
        
        for (const auto& [key, mapping] : mappings_) {
            if (mapping.originalFile == originalFile && 
                mapping.originalLine == originalLine) {
                return SourceLocation{
                    mapping.generatedFile,
                    mapping.generatedLine,
                    mapping.generatedColumn
                };
            }
        }
        return std::nullopt;
    }
    
    /**
     * @brief Load source map from JSON (V3 format)
     */
    bool loadFromJson(const std::string& json);
    
    /**
     * @brief Clear all mappings
     */
    void clear() { mappings_.clear(); }
    
private:
    std::unordered_map<std::string, Mapping> mappings_;
};

// Placeholder for JSON parsing
bool SourceMap::loadFromJson(const std::string& /*json*/) {
    // TODO: Implement V3 source map parsing
    // For now, return false to indicate no source map loaded
    return false;
}

// =============================================================================
// BreakpointManager - Central breakpoint registry
// =============================================================================

class BreakpointManager {
public:
    BreakpointManager() : nextId_(1) {}
    
    // -------------------------------------------------------------------------
    // Breakpoint CRUD Operations
    // -------------------------------------------------------------------------
    
    /**
     * @brief Add a line breakpoint
     * @return Breakpoint ID
     */
    uint32_t addLineBreakpoint(const std::string& file, uint32_t line) {
        BreakpointInfo bp;
        bp.id = nextId_++;
        bp.location.sourceFile = file;
        bp.location.line = line;
        bp.type = BreakpointType::Line;
        
        addBreakpoint(bp);
        return bp.id;
    }
    
    /**
     * @brief Add a conditional breakpoint
     * @return Breakpoint ID
     */
    uint32_t addConditionalBreakpoint(const std::string& file, uint32_t line,
                                       const std::string& condition) {
        BreakpointInfo bp;
        bp.id = nextId_++;
        bp.location.sourceFile = file;
        bp.location.line = line;
        bp.type = BreakpointType::Conditional;
        bp.condition = condition;
        
        addBreakpoint(bp);
        return bp.id;
    }
    
    /**
     * @brief Add a logpoint (logs without pausing)
     * @return Breakpoint ID
     */
    uint32_t addLogpoint(const std::string& file, uint32_t line,
                         const std::string& message) {
        BreakpointInfo bp;
        bp.id = nextId_++;
        bp.location.sourceFile = file;
        bp.location.line = line;
        bp.type = BreakpointType::Logpoint;
        bp.logMessage = message;
        
        addBreakpoint(bp);
        return bp.id;
    }
    
    /**
     * @brief Remove a breakpoint by ID
     * @return true if removed
     */
    bool removeBreakpoint(uint32_t id) {
        auto it = breakpointsById_.find(id);
        if (it == breakpointsById_.end()) return false;
        
        // Remove from location index
        const auto& bp = it->second;
        std::string locKey = locationKey(bp.location);
        breakpointsByLocation_.erase(locKey);
        
        // Remove from bytecode index if resolved
        if (bp.bytecodeResolved) {
            breakpointsByOffset_.erase(bp.bytecodeOffset);
        }
        
        // Remove from main storage
        breakpointsById_.erase(it);
        return true;
    }
    
    /**
     * @brief Enable/disable a breakpoint
     */
    void setEnabled(uint32_t id, bool enabled) {
        auto it = breakpointsById_.find(id);
        if (it != breakpointsById_.end()) {
            it->second.enabled = enabled;
        }
    }
    
    /**
     * @brief Set breakpoint condition
     */
    void setCondition(uint32_t id, const std::string& condition) {
        auto it = breakpointsById_.find(id);
        if (it != breakpointsById_.end()) {
            it->second.condition = condition;
            if (!condition.empty()) {
                it->second.type = BreakpointType::Conditional;
            }
        }
    }
    
    /**
     * @brief Set ignore count (skip N hits before breaking)
     */
    void setIgnoreCount(uint32_t id, uint32_t count) {
        auto it = breakpointsById_.find(id);
        if (it != breakpointsById_.end()) {
            it->second.ignoreCount = count;
        }
    }
    
    /**
     * @brief Clear all breakpoints
     */
    void clearAll() {
        breakpointsById_.clear();
        breakpointsByLocation_.clear();
        breakpointsByOffset_.clear();
    }
    
    // -------------------------------------------------------------------------
    // Breakpoint Lookup
    // -------------------------------------------------------------------------
    
    /**
     * @brief Check if breakpoint exists at location (fast path)
     */
    bool hasBreakpointAt(const std::string& file, uint32_t line) const {
        BreakpointLocation loc{file, line, 0};
        return breakpointsByLocation_.find(locationKey(loc)) != 
               breakpointsByLocation_.end();
    }
    
    /**
     * @brief Check if breakpoint exists at bytecode offset
     */
    bool hasBreakpointAtOffset(size_t offset) const {
        return breakpointsByOffset_.find(offset) != breakpointsByOffset_.end();
    }
    
    /**
     * @brief Get breakpoint at location
     */
    BreakpointInfo* getBreakpointAt(const std::string& file, uint32_t line) {
        BreakpointLocation loc{file, line, 0};
        auto it = breakpointsByLocation_.find(locationKey(loc));
        if (it == breakpointsByLocation_.end()) return nullptr;
        return &breakpointsById_[it->second];
    }
    
    /**
     * @brief Get breakpoint by ID
     */
    BreakpointInfo* getBreakpoint(uint32_t id) {
        auto it = breakpointsById_.find(id);
        if (it == breakpointsById_.end()) return nullptr;
        return &it->second;
    }
    
    /**
     * @brief Get all breakpoints
     */
    std::vector<BreakpointInfo> getAllBreakpoints() const {
        std::vector<BreakpointInfo> result;
        result.reserve(breakpointsById_.size());
        for (const auto& [id, bp] : breakpointsById_) {
            result.push_back(bp);
        }
        return result;
    }
    
    // -------------------------------------------------------------------------
    // Bytecode Resolution
    // -------------------------------------------------------------------------
    
    /**
     * @brief Resolve breakpoint to bytecode offset
     * Called after script compilation
     */
    void resolveToBytecode(uint32_t id, size_t offset) {
        auto it = breakpointsById_.find(id);
        if (it == breakpointsById_.end()) return;
        
        it->second.bytecodeOffset = offset;
        it->second.bytecodeResolved = true;
        breakpointsByOffset_[offset] = id;
    }
    
    /**
     * @brief Invalidate bytecode resolution (on script reload)
     */
    void invalidateResolution(const std::string& file) {
        for (auto& [id, bp] : breakpointsById_) {
            if (bp.location.sourceFile == file && bp.bytecodeResolved) {
                breakpointsByOffset_.erase(bp.bytecodeOffset);
                bp.bytecodeResolved = false;
                bp.bytecodeOffset = 0;
            }
        }
    }
    
    // -------------------------------------------------------------------------
    // Hit Handling
    // -------------------------------------------------------------------------
    
    /**
     * @brief Result of processing a potential breakpoint hit
     */
    struct HitResult {
        bool shouldPause;           // Execution should pause
        bool isLogpoint;            // This is a logpoint
        std::string logMessage;     // Message to log (for logpoints)
        uint32_t breakpointId;      // ID of hit breakpoint
    };
    
    /**
     * @brief Process a potential breakpoint hit
     * @param conditionEval Function to evaluate conditions
     * @return Hit result
     */
    HitResult processHit(const std::string& file, uint32_t line,
                         std::function<bool(const std::string&)> conditionEval) {
        HitResult result{false, false, "", 0};
        
        auto* bp = getBreakpointAt(file, line);
        if (!bp || !bp->enabled) {
            return result;
        }
        
        bp->hitCount++;
        
        // Check ignore count
        if (bp->ignoreCount > 0) {
            bp->ignoreCount--;
            return result;
        }
        
        result.breakpointId = bp->id;
        
        switch (bp->type) {
            case BreakpointType::Line:
                result.shouldPause = true;
                break;
                
            case BreakpointType::Conditional:
                if (conditionEval && conditionEval(bp->condition)) {
                    result.shouldPause = true;
                }
                break;
                
            case BreakpointType::Logpoint:
                result.isLogpoint = true;
                result.logMessage = bp->logMessage;
                // Logpoints don't pause
                break;
                
            case BreakpointType::Exception:
            case BreakpointType::DataWatch:
                // Handled elsewhere
                break;
        }
        
        return result;
    }
    
    // -------------------------------------------------------------------------
    // Source Map Integration
    // -------------------------------------------------------------------------
    
    /**
     * @brief Set source map for a file
     */
    void setSourceMap(const std::string& file, std::shared_ptr<SourceMap> map) {
        sourceMaps_[file] = std::move(map);
    }
    
    /**
     * @brief Get source map for a file
     */
    std::shared_ptr<SourceMap> getSourceMap(const std::string& file) const {
        auto it = sourceMaps_.find(file);
        return it != sourceMaps_.end() ? it->second : nullptr;
    }
    
private:
    static std::string locationKey(const BreakpointLocation& loc) {
        return loc.sourceFile + ":" + std::to_string(loc.line);
    }
    
    void addBreakpoint(const BreakpointInfo& bp) {
        breakpointsById_[bp.id] = bp;
        breakpointsByLocation_[locationKey(bp.location)] = bp.id;
    }
    
    uint32_t nextId_;
    std::unordered_map<uint32_t, BreakpointInfo> breakpointsById_;
    std::unordered_map<std::string, uint32_t> breakpointsByLocation_;
    std::unordered_map<size_t, uint32_t> breakpointsByOffset_;
    std::unordered_map<std::string, std::shared_ptr<SourceMap>> sourceMaps_;
};

// =============================================================================
// Global Breakpoint Manager Instance
// =============================================================================

static BreakpointManager* globalBreakpointManager = nullptr;

BreakpointManager* getBreakpointManager() {
    if (!globalBreakpointManager) {
        globalBreakpointManager = new BreakpointManager();
    }
    return globalBreakpointManager;
}

void shutdownBreakpointManager() {
    delete globalBreakpointManager;
    globalBreakpointManager = nullptr;
}

} // namespace Zepra::Debug
