/**
 * @file EnhancedDebugger.h
 * @brief Enhanced Debugging UX
 * 
 * Implements:
 * - Clear, formatted stack traces
 * - Source location mapping
 * - Actionable error messages
 * - Reliable breakpoint handling
 * - Step debugging
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <sstream>

namespace Zepra::Debug {

// =============================================================================
// Source Location
// =============================================================================

/**
 * @brief Precise source location
 */
struct SourceLocation {
    std::string filename;
    int line = 0;
    int column = 0;
    std::string functionName;
    
    std::string toString() const {
        std::ostringstream oss;
        oss << filename << ":" << line;
        if (column > 0) oss << ":" << column;
        if (!functionName.empty()) oss << " (" << functionName << ")";
        return oss.str();
    }
};

// =============================================================================
// Stack Frame
// =============================================================================

/**
 * @brief Enhanced stack frame with full context
 */
struct StackFrame {
    size_t index;
    SourceLocation location;
    std::string sourceSnippet;  // Line of source code
    std::unordered_map<std::string, std::string> locals;  // name -> value string
    bool isNative = false;
    bool isAsync = false;
    
    std::string format() const {
        std::ostringstream oss;
        oss << "  #" << index << " ";
        
        if (isNative) {
            oss << "[native] ";
        }
        if (isAsync) {
            oss << "[async] ";
        }
        
        if (!location.functionName.empty()) {
            oss << location.functionName;
        } else {
            oss << "<anonymous>";
        }
        
        oss << "\n       at " << location.filename << ":" << location.line;
        
        if (!sourceSnippet.empty()) {
            oss << "\n       > " << sourceSnippet;
        }
        
        return oss.str();
    }
};

// =============================================================================
// Stack Trace
// =============================================================================

/**
 * @brief Formatted stack trace
 */
class StackTrace {
public:
    void push(const StackFrame& frame) {
        frames_.push_back(frame);
        frames_.back().index = frames_.size() - 1;
    }
    
    void pop() {
        if (!frames_.empty()) frames_.pop_back();
    }
    
    const std::vector<StackFrame>& frames() const { return frames_; }
    
    std::string format(size_t maxFrames = 10) const {
        std::ostringstream oss;
        
        size_t count = std::min(frames_.size(), maxFrames);
        for (size_t i = 0; i < count; i++) {
            oss << frames_[frames_.size() - 1 - i].format() << "\n";
        }
        
        if (frames_.size() > maxFrames) {
            oss << "  ... " << (frames_.size() - maxFrames) << " more frames\n";
        }
        
        return oss.str();
    }
    
    // Get frame at depth (0 = current)
    const StackFrame* at(size_t depth) const {
        if (depth >= frames_.size()) return nullptr;
        return &frames_[frames_.size() - 1 - depth];
    }
    
private:
    std::vector<StackFrame> frames_;
};

// =============================================================================
// Error Message Builder
// =============================================================================

/**
 * @brief Builds clear, actionable error messages
 */
class ErrorMessageBuilder {
public:
    ErrorMessageBuilder& type(const std::string& errorType) {
        type_ = errorType;
        return *this;
    }
    
    ErrorMessageBuilder& message(const std::string& msg) {
        message_ = msg;
        return *this;
    }
    
    ErrorMessageBuilder& location(const SourceLocation& loc) {
        location_ = loc;
        return *this;
    }
    
    ErrorMessageBuilder& hint(const std::string& hint) {
        hints_.push_back(hint);
        return *this;
    }
    
    ErrorMessageBuilder& context(const std::string& key, const std::string& value) {
        context_[key] = value;
        return *this;
    }
    
    ErrorMessageBuilder& stackTrace(const StackTrace& trace) {
        stackTrace_ = trace.format();
        return *this;
    }
    
    std::string build() const {
        std::ostringstream oss;
        
        // Type and message
        oss << type_ << ": " << message_ << "\n";
        
        // Location
        if (location_.line > 0) {
            oss << "  at " << location_.toString() << "\n";
        }
        
        // Context
        if (!context_.empty()) {
            oss << "\nContext:\n";
            for (const auto& [key, value] : context_) {
                oss << "  " << key << ": " << value << "\n";
            }
        }
        
        // Hints
        if (!hints_.empty()) {
            oss << "\nHints:\n";
            for (const auto& hint : hints_) {
                oss << "  - " << hint << "\n";
            }
        }
        
        // Stack trace
        if (!stackTrace_.empty()) {
            oss << "\nStack trace:\n" << stackTrace_;
        }
        
        return oss.str();
    }
    
    // Common error builders
    static ErrorMessageBuilder typeError(const std::string& msg) {
        return ErrorMessageBuilder().type("TypeError").message(msg);
    }
    
    static ErrorMessageBuilder referenceError(const std::string& varName) {
        return ErrorMessageBuilder()
            .type("ReferenceError")
            .message(varName + " is not defined")
            .hint("Check spelling of variable name")
            .hint("Ensure variable is declared before use");
    }
    
    static ErrorMessageBuilder syntaxError(const std::string& msg, const SourceLocation& loc) {
        return ErrorMessageBuilder()
            .type("SyntaxError")
            .message(msg)
            .location(loc);
    }
    
    static ErrorMessageBuilder rangeError(const std::string& msg) {
        return ErrorMessageBuilder().type("RangeError").message(msg);
    }
    
private:
    std::string type_ = "Error";
    std::string message_;
    SourceLocation location_;
    std::vector<std::string> hints_;
    std::unordered_map<std::string, std::string> context_;
    std::string stackTrace_;
};

// =============================================================================
// Breakpoint
// =============================================================================

/**
 * @brief Enhanced breakpoint
 */
struct Breakpoint {
    uint32_t id;
    std::string filename;
    int line;
    int column = 0;
    
    // Condition (if any)
    std::string condition;
    
    // Hit count
    size_t hitCount = 0;
    size_t ignoreCount = 0;  // Skip first N hits
    
    // Log message instead of pause
    std::optional<std::string> logMessage;
    
    bool enabled = true;
    
    bool shouldPause() const {
        if (!enabled) return false;
        if (hitCount < ignoreCount) return false;
        if (logMessage.has_value()) return false;  // Log only
        return true;
    }
};

// =============================================================================
// Breakpoint Manager
// =============================================================================

/**
 * @brief Manages breakpoints with source mapping
 */
class BreakpointManager {
public:
    // Add breakpoint
    uint32_t addBreakpoint(const std::string& file, int line) {
        Breakpoint bp;
        bp.id = nextId_++;
        bp.filename = file;
        bp.line = line;
        breakpoints_[bp.id] = bp;
        
        // Index by location
        std::string key = file + ":" + std::to_string(line);
        locationIndex_[key].push_back(bp.id);
        
        return bp.id;
    }
    
    // Add conditional breakpoint
    uint32_t addConditional(const std::string& file, int line, const std::string& condition) {
        uint32_t id = addBreakpoint(file, line);
        breakpoints_[id].condition = condition;
        return id;
    }
    
    // Add logpoint
    uint32_t addLogpoint(const std::string& file, int line, const std::string& message) {
        uint32_t id = addBreakpoint(file, line);
        breakpoints_[id].logMessage = message;
        return id;
    }
    
    // Remove breakpoint
    bool remove(uint32_t id) {
        auto it = breakpoints_.find(id);
        if (it == breakpoints_.end()) return false;
        
        std::string key = it->second.filename + ":" + std::to_string(it->second.line);
        auto& vec = locationIndex_[key];
        vec.erase(std::remove(vec.begin(), vec.end(), id), vec.end());
        
        breakpoints_.erase(it);
        return true;
    }
    
    // Enable/disable
    void setEnabled(uint32_t id, bool enabled) {
        auto it = breakpoints_.find(id);
        if (it != breakpoints_.end()) {
            it->second.enabled = enabled;
        }
    }
    
    // Check for breakpoint at location
    std::vector<Breakpoint*> getAtLocation(const std::string& file, int line) {
        std::string key = file + ":" + std::to_string(line);
        std::vector<Breakpoint*> result;
        
        auto it = locationIndex_.find(key);
        if (it != locationIndex_.end()) {
            for (uint32_t id : it->second) {
                auto bpIt = breakpoints_.find(id);
                if (bpIt != breakpoints_.end() && bpIt->second.enabled) {
                    result.push_back(&bpIt->second);
                }
            }
        }
        return result;
    }
    
    // Record hit
    void recordHit(uint32_t id) {
        auto it = breakpoints_.find(id);
        if (it != breakpoints_.end()) {
            it->second.hitCount++;
        }
    }
    
private:
    std::unordered_map<uint32_t, Breakpoint> breakpoints_;
    std::unordered_map<std::string, std::vector<uint32_t>> locationIndex_;
    uint32_t nextId_ = 1;
};

// =============================================================================
// Step Controller
// =============================================================================

enum class StepMode {
    None,
    Into,   // Step into function calls
    Over,   // Step over function calls
    Out     // Step out of current function
};

/**
 * @brief Controls step debugging
 */
class StepController {
public:
    void stepInto() {
        mode_ = StepMode::Into;
        targetDepth_ = currentDepth_ + 1;
    }
    
    void stepOver() {
        mode_ = StepMode::Over;
        targetDepth_ = currentDepth_;
    }
    
    void stepOut() {
        mode_ = StepMode::Out;
        targetDepth_ = currentDepth_ - 1;
    }
    
    void continueExecution() {
        mode_ = StepMode::None;
    }
    
    // Called on function entry
    void onEnterFunction() {
        currentDepth_++;
    }
    
    // Called on function exit
    void onExitFunction() {
        currentDepth_--;
    }
    
    // Check if should pause
    bool shouldPause() const {
        switch (mode_) {
            case StepMode::Into:
                return true;  // Always pause on next line
            case StepMode::Over:
                return currentDepth_ <= targetDepth_;
            case StepMode::Out:
                return currentDepth_ < targetDepth_;
            case StepMode::None:
            default:
                return false;
        }
    }
    
    StepMode mode() const { return mode_; }
    size_t depth() const { return currentDepth_; }
    
private:
    StepMode mode_ = StepMode::None;
    size_t currentDepth_ = 0;
    size_t targetDepth_ = 0;
};

// =============================================================================
// Source Map Support
// =============================================================================

/**
 * @brief Maps compiled/minified locations to original source
 */
class SourceMapper {
public:
    struct Mapping {
        int generatedLine;
        int generatedColumn;
        int originalLine;
        int originalColumn;
        std::string originalFile;
    };
    
    // Load source map (simplified)
    void loadMap(const std::string& generatedFile, const std::string& sourceMap) {
        // Would parse source map JSON
        (void)generatedFile; (void)sourceMap;
    }
    
    // Map generated location to original
    std::optional<SourceLocation> mapToOriginal(const SourceLocation& generated) const {
        auto it = maps_.find(generated.filename);
        if (it == maps_.end()) return std::nullopt;
        
        // Find closest mapping
        for (const auto& m : it->second) {
            if (m.generatedLine == generated.line) {
                SourceLocation original;
                original.filename = m.originalFile;
                original.line = m.originalLine;
                original.column = m.originalColumn;
                original.functionName = generated.functionName;
                return original;
            }
        }
        return std::nullopt;
    }
    
    // Reverse map (for breakpoints)
    std::optional<SourceLocation> mapToGenerated(const std::string& originalFile, int line) const {
        for (const auto& [genFile, mappings] : maps_) {
            for (const auto& m : mappings) {
                if (m.originalFile == originalFile && m.originalLine == line) {
                    SourceLocation gen;
                    gen.filename = genFile;
                    gen.line = m.generatedLine;
                    gen.column = m.generatedColumn;
                    return gen;
                }
            }
        }
        return std::nullopt;
    }
    
private:
    std::unordered_map<std::string, std::vector<Mapping>> maps_;
};

// =============================================================================
// Debugger Session
// =============================================================================

/**
 * @brief Full debugging session
 */
class DebugSession {
public:
    StackTrace stackTrace;
    BreakpointManager breakpoints;
    StepController stepper;
    SourceMapper sourceMapper;
    
    // Pause callback
    using PauseCallback = std::function<void(const StackFrame&, const std::string& reason)>;
    void setPauseCallback(PauseCallback cb) { pauseCallback_ = std::move(cb); }
    
    // Called when execution pauses
    void notifyPause(const std::string& reason) {
        if (pauseCallback_ && stackTrace.at(0)) {
            pauseCallback_(*stackTrace.at(0), reason);
        }
    }
    
    // Evaluate expression in current context
    using EvalCallback = std::function<std::string(const std::string&)>;
    void setEvalCallback(EvalCallback cb) { evalCallback_ = std::move(cb); }
    
    std::string evaluate(const std::string& expr) {
        if (evalCallback_) {
            return evalCallback_(expr);
        }
        return "<eval not available>";
    }
    
private:
    PauseCallback pauseCallback_;
    EvalCallback evalCallback_;
};

} // namespace Zepra::Debug
