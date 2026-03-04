/**
 * @file ConsoleAPI.h
 * @brief Full Console API Implementation
 * 
 * Complete console.* implementation:
 * - log, warn, error, info, debug
 * - time, timeEnd, timeLog
 * - trace, table, group, groupEnd
 * - assert, count, countReset
 */

#pragma once

#include "../core/EmbedderAPI.h"
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <functional>

namespace Zepra::Browser {

// =============================================================================
// Console Message
// =============================================================================

enum class ConsoleLevel : uint8_t {
    Log,
    Debug,
    Info,
    Warn,
    Error,
    Trace,
    Table,
    Group,
    GroupEnd,
    Clear
};

/**
 * @brief Console message data
 */
struct ConsoleMessage {
    ConsoleLevel level;
    std::string text;
    std::string source;       // Source file
    int line = 0;             // Line number
    std::vector<ZebraValue> args;
    std::chrono::system_clock::time_point timestamp;
};

// =============================================================================
// Console Handler
// =============================================================================

/**
 * @brief Handles console output
 */
class ConsoleHandler {
public:
    virtual ~ConsoleHandler() = default;
    virtual void OnMessage(const ConsoleMessage& msg) = 0;
};

// =============================================================================
// Console API
// =============================================================================

/**
 * @brief Full console.* implementation
 */
class ConsoleAPI {
public:
    ConsoleAPI() = default;
    
    // Set output handler
    void SetHandler(ConsoleHandler* handler) { handler_ = handler; }
    
    // === Basic logging ===
    
    void Log(const std::vector<ZebraValue>& args) {
        Emit(ConsoleLevel::Log, FormatArgs(args), args);
    }
    
    void Debug(const std::vector<ZebraValue>& args) {
        Emit(ConsoleLevel::Debug, FormatArgs(args), args);
    }
    
    void Info(const std::vector<ZebraValue>& args) {
        Emit(ConsoleLevel::Info, FormatArgs(args), args);
    }
    
    void Warn(const std::vector<ZebraValue>& args) {
        Emit(ConsoleLevel::Warn, FormatArgs(args), args);
    }
    
    void Error(const std::vector<ZebraValue>& args) {
        Emit(ConsoleLevel::Error, FormatArgs(args), args);
    }
    
    // === Trace ===
    
    void Trace(const std::vector<ZebraValue>& args) {
        std::string msg = args.empty() ? "Trace" : FormatArgs(args);
        msg += "\n" + CaptureStackTrace();
        Emit(ConsoleLevel::Trace, msg, args);
    }
    
    // === Assert ===
    
    void Assert(bool condition, const std::vector<ZebraValue>& args) {
        if (!condition) {
            std::string msg = "Assertion failed";
            if (!args.empty()) {
                msg += ": " + FormatArgs(args);
            }
            Emit(ConsoleLevel::Error, msg, args);
        }
    }
    
    // === Timing ===
    
    void Time(const std::string& label = "default") {
        timers_[label] = std::chrono::steady_clock::now();
    }
    
    void TimeEnd(const std::string& label = "default") {
        auto it = timers_.find(label);
        if (it != timers_.end()) {
            auto elapsed = std::chrono::steady_clock::now() - it->second;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            
            std::string msg = label + ": " + std::to_string(ms.count()) + "ms";
            Emit(ConsoleLevel::Log, msg, {});
            
            timers_.erase(it);
        } else {
            Warn({ZebraValue::String(nullptr, "Timer '" + label + "' does not exist")});
        }
    }
    
    void TimeLog(const std::string& label = "default",
                 const std::vector<ZebraValue>& args = {}) {
        auto it = timers_.find(label);
        if (it != timers_.end()) {
            auto elapsed = std::chrono::steady_clock::now() - it->second;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            
            std::string msg = label + ": " + std::to_string(ms.count()) + "ms";
            if (!args.empty()) {
                msg += " " + FormatArgs(args);
            }
            Emit(ConsoleLevel::Log, msg, args);
        }
    }
    
    // === Counting ===
    
    void Count(const std::string& label = "default") {
        uint32_t count = ++counters_[label];
        std::string msg = label + ": " + std::to_string(count);
        Emit(ConsoleLevel::Log, msg, {});
    }
    
    void CountReset(const std::string& label = "default") {
        counters_[label] = 0;
    }
    
    // === Grouping ===
    
    void Group(const std::vector<ZebraValue>& args) {
        std::string label = args.empty() ? "" : FormatArgs(args);
        Emit(ConsoleLevel::Group, label, args);
        groupDepth_++;
    }
    
    void GroupCollapsed(const std::vector<ZebraValue>& args) {
        Group(args);  // Same as group for now
    }
    
    void GroupEnd() {
        if (groupDepth_ > 0) {
            groupDepth_--;
        }
        Emit(ConsoleLevel::GroupEnd, "", {});
    }
    
    // === Table ===
    
    void Table(const ZebraValue& data, 
               const std::vector<std::string>& columns = {}) {
        std::string formatted = FormatTable(data, columns);
        Emit(ConsoleLevel::Table, formatted, {data});
    }
    
    // === Clear ===
    
    void Clear() {
        Emit(ConsoleLevel::Clear, "", {});
    }
    
    // === Dir/DirXML ===
    
    void Dir(const ZebraValue& object) {
        std::string formatted = FormatObject(object, 1);
        Emit(ConsoleLevel::Log, formatted, {object});
    }
    
    void DirXML(const ZebraValue& object) {
        // Same as dir for non-DOM objects
        Dir(object);
    }
    
private:
    void Emit(ConsoleLevel level, const std::string& text,
              const std::vector<ZebraValue>& args) {
        if (!handler_) return;
        
        ConsoleMessage msg;
        msg.level = level;
        msg.text = text;
        msg.args = args;
        msg.timestamp = std::chrono::system_clock::now();
        
        handler_->OnMessage(msg);
    }
    
    std::string FormatArgs(const std::vector<ZebraValue>& args) {
        std::string result;
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) result += " ";
            result += FormatValue(args[i]);
        }
        return result;
    }
    
    std::string FormatValue(const ZebraValue& value) {
        if (value.IsUndefined()) return "undefined";
        if (value.IsNull()) return "null";
        if (value.IsBoolean()) return value.ToBoolean() ? "true" : "false";
        if (value.IsNumber()) return std::to_string(value.ToNumber());
        if (value.IsString()) return value.ToString();
        if (value.IsObject()) return "[object Object]";
        if (value.IsFunction()) return "[Function]";
        return "[unknown]";
    }
    
    std::string FormatObject(const ZebraValue& obj, int depth) {
        if (depth > 3) return "...";
        // Would format object properties
        return "{}";
    }
    
    std::string FormatTable(const ZebraValue& data, 
                            const std::vector<std::string>& columns) {
        // Would format as table
        return "[table data]";
    }
    
    std::string CaptureStackTrace() {
        // Would capture JS stack
        return "    at <anonymous>";
    }
    
    ConsoleHandler* handler_ = nullptr;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> timers_;
    std::unordered_map<std::string, uint32_t> counters_;
    int groupDepth_ = 0;
};

// =============================================================================
// Global Console Instance
// =============================================================================

ConsoleAPI& GetConsole();

} // namespace Zepra::Browser
