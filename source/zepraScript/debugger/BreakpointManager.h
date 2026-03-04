/**
 * @file BreakpointManager.h
 * @brief Breakpoint management for debugger
 * 
 * Implements:
 * - Line/column breakpoints
 * - Conditional breakpoints
 * - Hit count tracking
 * - Breakpoint patching in JIT code
 * 
 * Based on V8 Debug and JSC Inspector
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace Zepra::Debug {

// =============================================================================
// Breakpoint Types
// =============================================================================

enum class BreakpointType : uint8_t {
    Line,           // Line breakpoint
    Column,         // Line:column breakpoint
    Function,       // Function entry breakpoint
    Exception,      // Break on exception
    Conditional,    // Conditional breakpoint
    Logpoint,       // Log without stopping
    DOM             // DOM mutation breakpoint
};

enum class BreakpointState : uint8_t {
    Disabled,
    Enabled,
    Pending,        // Script not loaded yet
    Resolved        // Bound to actual location
};

// =============================================================================
// Source Location
// =============================================================================

struct SourceLocation {
    std::string scriptId;
    uint32_t lineNumber = 0;
    uint32_t columnNumber = 0;
    
    bool operator==(const SourceLocation& other) const {
        return scriptId == other.scriptId && 
               lineNumber == other.lineNumber && 
               columnNumber == other.columnNumber;
    }
};

struct SourceLocationHash {
    size_t operator()(const SourceLocation& loc) const {
        return std::hash<std::string>()(loc.scriptId) ^ 
               (std::hash<uint32_t>()(loc.lineNumber) << 16) ^
               std::hash<uint32_t>()(loc.columnNumber);
    }
};

// =============================================================================
// Breakpoint
// =============================================================================

struct Breakpoint {
    uint32_t id;
    BreakpointType type;
    BreakpointState state = BreakpointState::Enabled;
    
    // Location
    SourceLocation requestedLocation;
    SourceLocation actualLocation;
    
    // Condition (for conditional breakpoints)
    std::string condition;
    
    // Hit tracking
    uint32_t hitCount = 0;
    uint32_t ignoreCount = 0;    // Ignore first N hits
    
    // Logpoint message template
    std::string logMessageFormat;
    
    // User data
    std::string userLabel;
    std::unordered_map<std::string, std::string> metadata;
};

// =============================================================================
// Breakpoint Patch Point
// =============================================================================

/**
 * @brief Location in compiled code where breakpoint can be set
 */
struct BreakpointPatchPoint {
    void* codeAddress;          // Address in JIT code
    size_t bytecodeOffset;      // Corresponding bytecode offset
    SourceLocation location;    // Source location
    uint8_t originalBytes[8];   // Original instruction bytes
    bool isPatched = false;
};

// =============================================================================
// Breakpoint Manager
// =============================================================================

class BreakpointManager {
public:
    using BreakpointCallback = std::function<void(const Breakpoint&)>;
    
    BreakpointManager() = default;
    
    // =========================================================================
    // Breakpoint CRUD
    // =========================================================================
    
    /**
     * @brief Set a line breakpoint
     * @return Breakpoint ID
     */
    uint32_t setBreakpoint(const std::string& scriptId, 
                           uint32_t lineNumber,
                           uint32_t columnNumber = 0);
    
    /**
     * @brief Set a conditional breakpoint
     */
    uint32_t setConditionalBreakpoint(const std::string& scriptId,
                                       uint32_t lineNumber,
                                       const std::string& condition);
    
    /**
     * @brief Set a logpoint (log without stopping)
     */
    uint32_t setLogpoint(const std::string& scriptId,
                         uint32_t lineNumber,
                         const std::string& messageFormat);
    
    /**
     * @brief Set function entry breakpoint
     */
    uint32_t setFunctionBreakpoint(const std::string& functionName);
    
    /**
     * @brief Set exception breakpoint
     * @param caught Break on caught exceptions
     * @param uncaught Break on uncaught exceptions
     */
    void setExceptionBreakpoints(bool caught, bool uncaught);
    
    /**
     * @brief Remove a breakpoint
     */
    bool removeBreakpoint(uint32_t breakpointId);
    
    /**
     * @brief Remove all breakpoints
     */
    void removeAllBreakpoints();
    
    /**
     * @brief Enable/disable a breakpoint
     */
    void setBreakpointEnabled(uint32_t breakpointId, bool enabled);
    
    // =========================================================================
    // Breakpoint Query
    // =========================================================================
    
    /**
     * @brief Get breakpoint by ID
     */
    Breakpoint* getBreakpoint(uint32_t breakpointId);
    const Breakpoint* getBreakpoint(uint32_t breakpointId) const;
    
    /**
     * @brief Get all breakpoints
     */
    std::vector<Breakpoint*> getAllBreakpoints();
    
    /**
     * @brief Get breakpoints at location
     */
    std::vector<Breakpoint*> getBreakpointsAt(const SourceLocation& location);
    
    /**
     * @brief Check if location has active breakpoint
     */
    bool hasBreakpointAt(const SourceLocation& location) const;
    
    // =========================================================================
    // Code Patching
    // =========================================================================
    
    /**
     * @brief Register a patch point in compiled code
     */
    void registerPatchPoint(const BreakpointPatchPoint& point);
    
    /**
     * @brief Apply breakpoint patches to compiled code
     */
    void applyPatches(void* codeStart, size_t codeSize);
    
    /**
     * @brief Remove breakpoint patches from compiled code
     */
    void removePatches(void* codeStart, size_t codeSize);
    
    // =========================================================================
    // Script Events
    // =========================================================================
    
    /**
     * @brief Called when a script is loaded
     * Resolves pending breakpoints
     */
    void onScriptLoaded(const std::string& scriptId,
                        const std::string& sourceURL,
                        const std::vector<SourceLocation>& breakableLocations);
    
    /**
     * @brief Called when a script is unloaded
     */
    void onScriptUnloaded(const std::string& scriptId);
    
    // =========================================================================
    // Hit Handling
    // =========================================================================
    
    /**
     * @brief Called when breakpoint is hit
     * @return true if execution should pause
     */
    bool onBreakpointHit(uint32_t breakpointId);
    
    /**
     * @brief Register callback for breakpoint hits
     */
    void setBreakpointHitCallback(BreakpointCallback callback);
    
    // =========================================================================
    // Statistics
    // =========================================================================
    
    struct Stats {
        size_t totalBreakpoints = 0;
        size_t activeBreakpoints = 0;
        size_t pendingBreakpoints = 0;
        size_t totalHits = 0;
    };
    
    Stats getStats() const;
    
private:
    std::unordered_map<uint32_t, Breakpoint> breakpoints_;
    std::unordered_map<SourceLocation, std::vector<uint32_t>, SourceLocationHash> locationIndex_;
    std::vector<BreakpointPatchPoint> patchPoints_;
    
    uint32_t nextBreakpointId_ = 1;
    bool breakOnCaughtExceptions_ = false;
    bool breakOnUncaughtExceptions_ = true;
    
    BreakpointCallback hitCallback_;
    
    uint32_t allocateId() { return nextBreakpointId_++; }
    void indexBreakpoint(Breakpoint& bp);
    void unindexBreakpoint(const Breakpoint& bp);
    bool evaluateCondition(const Breakpoint& bp);
};

// =============================================================================
// Breakpoint Resolver
// =============================================================================

/**
 * @brief Resolves requested breakpoint locations to actual breakable locations
 */
class BreakpointResolver {
public:
    /**
     * @brief Find nearest breakable location
     */
    static SourceLocation resolveLocation(
        const std::string& scriptId,
        uint32_t requestedLine,
        uint32_t requestedColumn,
        const std::vector<SourceLocation>& breakableLocations);
    
    /**
     * @brief Get all breakable locations in a range
     */
    static std::vector<SourceLocation> getBreakableLocationsInRange(
        const std::string& scriptId,
        uint32_t startLine,
        uint32_t endLine,
        const std::vector<SourceLocation>& breakableLocations);
};

} // namespace Zepra::Debug
