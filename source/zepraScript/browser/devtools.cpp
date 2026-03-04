/**
 * @file devtools.cpp
 * @brief DevTools Inspector, Profiler, and MemoryInspector implementation
 *
 * Implements the native debugging API:
 * - Inspector: breakpoints, stepping, call stack, scope inspection, eval
 * - Profiler: CPU sampling, profile tree, start/stop
 * - MemoryInspector: heap snapshots, allocation tracking, GC stats
 *
 * Ref: CDPv1 Debugger domain (for interface parity)
 *      WebKit Inspector Protocol
 */

#include "browser/DevToolsAPI.h"
#include "runtime/execution/vm.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cstdio>

namespace Zepra::DevTools {

// =============================================================================
// Inspector
// =============================================================================

Inspector::Inspector() = default;

Inspector& Inspector::instance() {
    static Inspector inst;
    return inst;
}

void Inspector::pause() {
    paused_ = true;
    auto frames = getCallStack();
    if (onPaused_) {
        onPaused_(PauseReason::DebuggerStatement, frames);
    }
}

void Inspector::resume() {
    paused_ = false;
    if (onResumed_) {
        onResumed_();
    }
}

void Inspector::stepOver() {
    paused_ = false;
    // VM will re-pause at next statement at same or shallower call depth
}

void Inspector::stepInto() {
    paused_ = false;
    // VM will re-pause at next statement (any call depth)
}

void Inspector::stepOut() {
    paused_ = false;
    // VM will re-pause when current function returns
}

uint32_t Inspector::setBreakpoint(const std::string& url, uint32_t line,
                                   uint32_t column, const std::string& condition) {
    Breakpoint bp;
    bp.breakpointId = nextBreakpointId_++;
    bp.url = url;
    bp.lineNumber = line;
    bp.columnNumber = column;
    bp.condition = condition;
    bp.enabled = true;
    bp.hitCount = 0;
    breakpoints_.push_back(bp);
    return bp.breakpointId;
}

void Inspector::removeBreakpoint(uint32_t breakpointId) {
    breakpoints_.erase(
        std::remove_if(breakpoints_.begin(), breakpoints_.end(),
            [breakpointId](const Breakpoint& bp) {
                return bp.breakpointId == breakpointId;
            }),
        breakpoints_.end());
}

void Inspector::setBreakpointEnabled(uint32_t breakpointId, bool enabled) {
    for (auto& bp : breakpoints_) {
        if (bp.breakpointId == breakpointId) {
            bp.enabled = enabled;
            return;
        }
    }
}

void Inspector::setBreakOnExceptions(bool caught, bool uncaught) {
    breakOnCaughtExceptions_ = caught;
    breakOnUncaughtExceptions_ = uncaught;
}

std::vector<StackFrame> Inspector::getCallStack() const {
    std::vector<StackFrame> frames;
    // Query the VM for the current call stack
    // In production, this would walk vm->callStack_
    StackFrame top;
    top.frameId = 0;
    top.functionName = "<global>";
    top.lineNumber = 0;
    top.columnNumber = 0;
    frames.push_back(top);
    return frames;
}

std::vector<Scope> Inspector::getScopes(uint32_t frameId) const {
    std::vector<Scope> scopes;

    // Local scope
    Scope local;
    local.scopeId = frameId * 10;
    local.type = Scope::Type::Local;
    local.name = "Local";
    scopes.push_back(local);

    // Global scope
    Scope global;
    global.scopeId = frameId * 10 + 1;
    global.type = Scope::Type::Global;
    global.name = "Global";
    scopes.push_back(global);

    return scopes;
}

std::vector<Variable> Inspector::getVariables(uint32_t scopeId) const {
    (void)scopeId;
    std::vector<Variable> vars;
    // Would enumerate the VM's environment chain at the given scope
    return vars;
}

std::vector<Variable> Inspector::getObjectProperties(uint32_t objectId) const {
    (void)objectId;
    std::vector<Variable> props;
    // Would enumerate object properties via Object::ownPropertyNames()
    return props;
}

Value Inspector::evaluate(const std::string& expression, uint32_t frameId) {
    (void)expression;
    (void)frameId;
    // Would compile and execute expression in frame context
    return Value::undefined();
}

bool Inspector::setVariableValue(uint32_t scopeId, const std::string& name,
                                  const Value& value) {
    (void)scopeId;
    (void)name;
    (void)value;
    // Would set the variable in the given scope's environment
    return false;
}

// =============================================================================
// Profiler
// =============================================================================

Profiler::Profiler() = default;

Profiler& Profiler::instance() {
    static Profiler inst;
    return inst;
}

void Profiler::start() {
    if (running_) return;
    running_ = true;

    auto now = std::chrono::steady_clock::now();
    startTime_ = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();

    nodes_.clear();
    samples_.clear();
    timeDeltas_.clear();

    // Create root node
    ProfileNode root;
    root.nodeId = 0;
    root.functionName = "(root)";
    nodes_.push_back(root);
}

ProfileResult Profiler::stop() {
    running_ = false;

    auto now = std::chrono::steady_clock::now();
    uint64_t endTime = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();

    ProfileResult result;
    result.nodes = nodes_;
    result.startTime = startTime_;
    result.endTime = endTime;
    result.samples = samples_;
    result.timeDeltas = timeDeltas_;

    nodes_.clear();
    samples_.clear();
    timeDeltas_.clear();

    return result;
}

void Profiler::setSamplingInterval(uint32_t microseconds) {
    samplingInterval_ = microseconds;
}

// =============================================================================
// MemoryInspector
// =============================================================================

MemoryInspector::MemoryInspector() = default;

MemoryInspector& MemoryInspector::instance() {
    static MemoryInspector inst;
    return inst;
}

HeapSnapshot MemoryInspector::takeHeapSnapshot() {
    HeapSnapshot snapshot;
    snapshot.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Walk the GC heap to enumerate all live objects
    // In production, this iterates gc::Heap::allObjects()
    snapshot.totalSize = 0;
    snapshot.totalObjects = 0;

    return snapshot;
}

void MemoryInspector::startAllocationTracking() {
    trackingAllocations_ = true;
    allocations_.clear();
}

std::vector<AllocationRecord> MemoryInspector::stopAllocationTracking() {
    trackingAllocations_ = false;
    auto result = std::move(allocations_);
    allocations_.clear();
    return result;
}

MemoryInspector::MemoryUsage MemoryInspector::getMemoryUsage() const {
    MemoryUsage usage;
    usage.usedHeapSize = 0;
    usage.totalHeapSize = 0;
    usage.heapSizeLimit = 512 * 1024 * 1024; // 512 MB default
    usage.externalMemory = 0;
    // In production, would query gc::Heap for real stats
    return usage;
}

void MemoryInspector::forceGC() {
    // In production, triggers gc::Heap::collectGarbage(GCReason::Forced)
}

// =============================================================================
// DevTools Console Handler
// =============================================================================

ConsoleHandler::ConsoleHandler() = default;

ConsoleHandler& ConsoleHandler::instance() {
    static ConsoleHandler inst;
    return inst;
}

void ConsoleHandler::addMessage(ConsoleMessage::Level level,
                                 const std::vector<Value>& args) {
    ConsoleMessage msg;
    msg.level = level;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    msg.args = args;

    // Build message string from args
    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << ' ';
        if (args[i].isString()) {
            auto* str = static_cast<Runtime::String*>(args[i].asObject());
            if (str) oss << str->value();
        } else if (args[i].isNumber()) {
            oss << args[i].asNumber();
        } else if (args[i].isBoolean()) {
            oss << (args[i].asBoolean() ? "true" : "false");
        } else if (args[i].isNull()) {
            oss << "null";
        } else if (args[i].isUndefined()) {
            oss << "undefined";
        } else {
            oss << "[object]";
        }
    }
    msg.message = oss.str();

    messages_.push_back(msg);

    if (onMessage_) {
        onMessage_(msg);
    }
}

void ConsoleHandler::log(const std::vector<Value>& args) {
    addMessage(ConsoleMessage::Level::Log, args);
}

void ConsoleHandler::info(const std::vector<Value>& args) {
    addMessage(ConsoleMessage::Level::Info, args);
}

void ConsoleHandler::warn(const std::vector<Value>& args) {
    addMessage(ConsoleMessage::Level::Warn, args);
}

void ConsoleHandler::error(const std::vector<Value>& args) {
    addMessage(ConsoleMessage::Level::Error, args);
}

void ConsoleHandler::debug(const std::vector<Value>& args) {
    addMessage(ConsoleMessage::Level::Debug, args);
}

void ConsoleHandler::clear() {
    messages_.clear();
}

// =============================================================================
// Init
// =============================================================================

void initDevTools() {
    // Initialize inspector, profiler, and memory inspector singletons
    Inspector::instance();
    Profiler::instance();
    MemoryInspector::instance();
    ConsoleHandler::instance();
}

} // namespace Zepra::DevTools
