/**
 * @file debugger.cpp
 * @brief ZepraScript Debugger implementation
 */

#include "zeprascript/debug/debugger.hpp"
#include "zeprascript/runtime/vm.hpp"
#include <sstream>

namespace Zepra::Debug {

Debugger::Debugger(Runtime::VM* vm) : vm_(vm) {}

// =============================================================================
// Breakpoint Management
// =============================================================================

uint32_t Debugger::setBreakpoint(const std::string& file, uint32_t line) {
    return setBreakpoint(file, line, "");
}

uint32_t Debugger::setBreakpoint(const std::string& file, uint32_t line,
                                  const std::string& condition) {
    Breakpoint bp;
    bp.id = nextBreakpointId_++;
    bp.location.sourceFile = file;
    bp.location.line = line;
    bp.condition = condition;
    bp.enabled = true;
    
    breakpoints_[bp.id] = bp;
    
    // Add to fast lookup set
    std::ostringstream key;
    key << file << ":" << line;
    breakpointLocations_.insert(key.str());
    
    return bp.id;
}

bool Debugger::removeBreakpoint(uint32_t id) {
    auto it = breakpoints_.find(id);
    if (it == breakpoints_.end()) return false;
    
    // Remove from fast lookup
    std::ostringstream key;
    key << it->second.location.sourceFile << ":" << it->second.location.line;
    breakpointLocations_.erase(key.str());
    
    breakpoints_.erase(it);
    return true;
}

void Debugger::setBreakpointEnabled(uint32_t id, bool enabled) {
    auto it = breakpoints_.find(id);
    if (it != breakpoints_.end()) {
        it->second.enabled = enabled;
    }
}

std::vector<Breakpoint> Debugger::getBreakpoints() const {
    std::vector<Breakpoint> result;
    result.reserve(breakpoints_.size());
    for (const auto& [id, bp] : breakpoints_) {
        result.push_back(bp);
    }
    return result;
}

void Debugger::clearAllBreakpoints() {
    breakpoints_.clear();
    breakpointLocations_.clear();
}

// =============================================================================
// Execution Control
// =============================================================================

void Debugger::pause() {
    paused_ = true;
    notifyEvent(DebugEvent::ExecutionPaused);
}

void Debugger::resume() {
    paused_ = false;
    stepMode_ = StepMode::Continue;
    notifyEvent(DebugEvent::ExecutionResumed);
}

void Debugger::stepInto() {
    stepMode_ = StepMode::StepInto;
    paused_ = false;
}

void Debugger::stepOver() {
    stepMode_ = StepMode::StepOver;
    stepStartDepth_ = getCallStack().size();
    paused_ = false;
}

void Debugger::stepOut() {
    stepMode_ = StepMode::StepOut;
    stepStartDepth_ = getCallStack().size();
    paused_ = false;
}

// =============================================================================
// Call Stack
// =============================================================================

std::vector<DebugCallFrame> Debugger::getCallStack() const {
    std::vector<DebugCallFrame> frames;
    
    // Get call depth from VM
    size_t depth = vm_->getCallDepth();
    
    for (size_t i = 0; i < depth; ++i) {
        DebugCallFrame frame;
        frame.functionName = "<frame " + std::to_string(i) + ">";
        frame.line = 0;
        // TODO: Get actual frame info from VM
        frames.push_back(frame);
    }
    
    return frames;
}

std::unordered_map<std::string, Value> Debugger::getScopeVariables(size_t) const {
    std::unordered_map<std::string, Value> vars;
    // TODO: Get variables from VM environment
    return vars;
}

// =============================================================================
// Variable Inspection
// =============================================================================

Value Debugger::evaluate(const std::string&) {
    // TODO: Compile and evaluate expression in current context
    return Value::undefined();
}

void Debugger::addWatch(const std::string& expression) {
    watches_.push_back(expression);
}

std::vector<std::pair<std::string, Value>> Debugger::getWatches() const {
    std::vector<std::pair<std::string, Value>> result;
    for (const auto& expr : watches_) {
        // TODO: Evaluate each expression
        result.push_back({expr, Value::undefined()});
    }
    return result;
}

// =============================================================================
// Callbacks
// =============================================================================

void Debugger::setCallback(DebugCallback callback) {
    callback_ = std::move(callback);
}

void Debugger::notifyEvent(DebugEvent event, const Breakpoint* bp) {
    if (callback_) {
        callback_(event, bp);
    }
}

// =============================================================================
// VM Integration
// =============================================================================

bool Debugger::onInstruction(const std::string& file, uint32_t line) {
    // Check for breakpoints
    if (checkBreakpoint(file, line)) {
        paused_ = true;
        return false; // Pause execution
    }
    
    // Handle stepping
    switch (stepMode_) {
        case StepMode::StepInto:
            // Pause on any line change
            paused_ = true;
            stepMode_ = StepMode::None;
            notifyEvent(DebugEvent::StepComplete);
            return false;
            
        case StepMode::StepOver: {
            size_t currentDepth = getCallStack().size();
            if (currentDepth <= stepStartDepth_) {
                paused_ = true;
                stepMode_ = StepMode::None;
                notifyEvent(DebugEvent::StepComplete);
                return false;
            }
            break;
        }
        
        case StepMode::StepOut: {
            size_t currentDepth = getCallStack().size();
            if (currentDepth < stepStartDepth_) {
                paused_ = true;
                stepMode_ = StepMode::None;
                notifyEvent(DebugEvent::StepComplete);
                return false;
            }
            break;
        }
        
        default:
            break;
    }
    
    return !paused_; // Continue if not paused
}

bool Debugger::checkBreakpoint(const std::string& file, uint32_t line) {
    // Fast path: check if any breakpoint at this location
    std::ostringstream key;
    key << file << ":" << line;
    if (breakpointLocations_.find(key.str()) == breakpointLocations_.end()) {
        return false;
    }
    
    // Find and check the breakpoint
    for (auto& [id, bp] : breakpoints_) {
        if (bp.location.sourceFile == file && 
            bp.location.line == line && 
            bp.enabled) {
            
            bp.hitCount++;
            
            // TODO: Evaluate condition if present
            
            notifyEvent(DebugEvent::BreakpointHit, &bp);
            return true;
        }
    }
    
    return false;
}

} // namespace Zepra::Debug
