/**
 * @file console.cpp
 * @brief DevTools Console implementation
 */

#include "zeprascript/debug/console.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include <sstream>
#include <iomanip>

namespace Zepra::Debug {

using Runtime::Object;

Console& Console::instance() {
    static Console console;
    return console;
}

void Console::log(const std::vector<Value>& args) {
    addMessage(LogLevel::Log, formatArgs(args), args);
}

void Console::info(const std::vector<Value>& args) {
    addMessage(LogLevel::Info, formatArgs(args), args);
}

void Console::warn(const std::vector<Value>& args) {
    addMessage(LogLevel::Warn, formatArgs(args), args);
}

void Console::error(const std::vector<Value>& args) {
    ConsoleMessage msg;
    msg.id = nextMessageId_++;
    msg.level = LogLevel::Error;
    msg.text = formatArgs(args);
    msg.args = args;
    msg.sourceFile = currentFile_;
    msg.lineNumber = currentLine_;
    msg.stackTrace = getCurrentStackTrace();
    msg.timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    messages_.push_back(msg);
    if (callback_) callback_(msg);
}

void Console::debug(const std::vector<Value>& args) {
    addMessage(LogLevel::Debug, formatArgs(args), args);
}

void Console::table(const Value& data, const std::vector<std::string>&) {
    std::vector<Value> args = {data};
    addMessage(LogLevel::Table, formatArgs(args), args);
}

void Console::dir(const Value& object) {
    std::vector<Value> args = {object};
    addMessage(LogLevel::Log, formatArgs(args), args);
}

void Console::dirxml(const Value& element) {
    dir(element);
}

void Console::group(const std::string& label) {
    ConsoleMessage msg;
    msg.id = nextMessageId_++;
    msg.level = LogLevel::Group;
    msg.text = label.empty() ? "console.group" : label;
    msg.timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    messages_.push_back(msg);
    if (callback_) callback_(msg);
    groupDepth_++;
}

void Console::groupCollapsed(const std::string& label) {
    group(label);
}

void Console::groupEnd() {
    if (groupDepth_ > 0) {
        groupDepth_--;
        ConsoleMessage msg;
        msg.id = nextMessageId_++;
        msg.level = LogLevel::GroupEnd;
        msg.timestamp = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        messages_.push_back(msg);
        if (callback_) callback_(msg);
    }
}

void Console::time(const std::string& label) {
    timers_[label] = std::chrono::steady_clock::now();
}

void Console::timeLog(const std::string& label, const std::vector<Value>& args) {
    auto it = timers_.find(label);
    if (it == timers_.end()) {
        warn({});
        return;
    }
    
    auto elapsed = std::chrono::steady_clock::now() - it->second;
    double ms = std::chrono::duration<double, std::milli>(elapsed).count();
    
    std::ostringstream ss;
    ss << label << ": " << std::fixed << std::setprecision(3) << ms << "ms";
    if (!args.empty()) {
        ss << " " << formatArgs(args);
    }
    
    addMessage(LogLevel::Log, ss.str());
}

void Console::timeEnd(const std::string& label) {
    auto it = timers_.find(label);
    if (it == timers_.end()) {
        warn({});
        return;
    }
    
    auto elapsed = std::chrono::steady_clock::now() - it->second;
    double ms = std::chrono::duration<double, std::milli>(elapsed).count();
    
    std::ostringstream ss;
    ss << label << ": " << std::fixed << std::setprecision(3) << ms << "ms";
    
    addMessage(LogLevel::Log, ss.str());
    timers_.erase(it);
}

void Console::count(const std::string& label) {
    counters_[label]++;
    std::ostringstream ss;
    ss << label << ": " << counters_[label];
    addMessage(LogLevel::Log, ss.str());
}

void Console::countReset(const std::string& label) {
    counters_[label] = 0;
}

void Console::assert_(bool condition, const std::vector<Value>& args) {
    if (!condition) {
        std::string text = "Assertion failed";
        if (!args.empty()) {
            text += ": " + formatArgs(args);
        }
        
        ConsoleMessage msg;
        msg.id = nextMessageId_++;
        msg.level = LogLevel::Error;
        msg.text = text;
        msg.args = args;
        msg.stackTrace = getCurrentStackTrace();
        msg.timestamp = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        messages_.push_back(msg);
        if (callback_) callback_(msg);
    }
}

void Console::trace(const std::string& label) {
    ConsoleMessage msg;
    msg.id = nextMessageId_++;
    msg.level = LogLevel::Log;
    msg.text = label.empty() ? "console.trace" : label;
    msg.stackTrace = getCurrentStackTrace();
    msg.timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    messages_.push_back(msg);
    if (callback_) callback_(msg);
}

void Console::clear() {
    messages_.clear();
    
    ConsoleMessage msg;
    msg.id = nextMessageId_++;
    msg.level = LogLevel::Clear;
    msg.text = "Console was cleared";
    msg.timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (callback_) callback_(msg);
}

void Console::setSourceInfo(const std::string& file, int line, int column) {
    currentFile_ = file;
    currentLine_ = line;
    currentColumn_ = column;
}

void Console::addMessage(LogLevel level, const std::string& text, 
                          const std::vector<Value>& args) {
    ConsoleMessage msg;
    msg.id = nextMessageId_++;
    msg.level = level;
    msg.text = text;
    msg.args = args;
    msg.sourceFile = currentFile_;
    msg.lineNumber = currentLine_;
    msg.columnNumber = currentColumn_;
    msg.timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Check for repeat
    if (!messages_.empty() && messages_.back().text == text && 
        messages_.back().level == level) {
        messages_.back().repeatCount++;
        return;
    }
    
    messages_.push_back(msg);
    if (callback_) callback_(msg);
}

std::string Console::formatArgs(const std::vector<Value>& args) {
    std::ostringstream ss;
    
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) ss << " ";
        
        const Value& val = args[i];
        if (val.isUndefined()) {
            ss << "undefined";
        } else if (val.isNull()) {
            ss << "null";
        } else if (val.isBoolean()) {
            ss << (val.asBoolean() ? "true" : "false");
        } else if (val.isNumber()) {
            ss << val.asNumber();
        } else if (val.isString()) {
            ss << "string";  // Simplified
        } else if (val.isObject()) {
            Object* obj = val.asObject();
            if (obj->isArray()) {
                ss << "[Array]";
            } else if (obj->isFunction()) {
                Runtime::Function* fn = static_cast<Runtime::Function*>(obj);
                ss << "[Function: " << fn->name() << "]";
            } else {
                ss << "[object Object]";
            }
        }
    }
    
    return ss.str();
}

std::string Console::getCurrentStackTrace() {
    // TODO: Get actual stack trace from VM
    return "";
}

// =============================================================================
// ConsoleBuiltin
// =============================================================================

Value ConsoleBuiltin::log(Runtime::Context*, const std::vector<Value>& args) {
    Console::instance().log(args);
    return Value::undefined();
}

Value ConsoleBuiltin::info(Runtime::Context*, const std::vector<Value>& args) {
    Console::instance().info(args);
    return Value::undefined();
}

Value ConsoleBuiltin::warn(Runtime::Context*, const std::vector<Value>& args) {
    Console::instance().warn(args);
    return Value::undefined();
}

Value ConsoleBuiltin::error(Runtime::Context*, const std::vector<Value>& args) {
    Console::instance().error(args);
    return Value::undefined();
}

Value ConsoleBuiltin::debug(Runtime::Context*, const std::vector<Value>& args) {
    Console::instance().debug(args);
    return Value::undefined();
}

Value ConsoleBuiltin::table(Runtime::Context*, const std::vector<Value>& args) {
    if (!args.empty()) {
        Console::instance().table(args[0]);
    }
    return Value::undefined();
}

Value ConsoleBuiltin::group(Runtime::Context*, const std::vector<Value>& args) {
    std::string label = args.empty() ? "" : "label";
    Console::instance().group(label);
    return Value::undefined();
}

Value ConsoleBuiltin::groupEnd(Runtime::Context*, const std::vector<Value>&) {
    Console::instance().groupEnd();
    return Value::undefined();
}

Value ConsoleBuiltin::time(Runtime::Context*, const std::vector<Value>& args) {
    std::string label = args.empty() ? "default" : "label";
    Console::instance().time(label);
    return Value::undefined();
}

Value ConsoleBuiltin::timeEnd(Runtime::Context*, const std::vector<Value>& args) {
    std::string label = args.empty() ? "default" : "label";
    Console::instance().timeEnd(label);
    return Value::undefined();
}

Value ConsoleBuiltin::count(Runtime::Context*, const std::vector<Value>& args) {
    std::string label = args.empty() ? "default" : "label";
    Console::instance().count(label);
    return Value::undefined();
}

Value ConsoleBuiltin::assert_(Runtime::Context*, const std::vector<Value>& args) {
    bool condition = args.empty() ? false : args[0].toBoolean();
    std::vector<Value> rest(args.size() > 1 ? args.begin() + 1 : args.end(), args.end());
    Console::instance().assert_(condition, rest);
    return Value::undefined();
}

Value ConsoleBuiltin::trace(Runtime::Context*, const std::vector<Value>& args) {
    std::string label = args.empty() ? "" : "label";
    Console::instance().trace(label);
    return Value::undefined();
}

Value ConsoleBuiltin::clear(Runtime::Context*, const std::vector<Value>&) {
    Console::instance().clear();
    return Value::undefined();
}

Object* ConsoleBuiltin::createConsoleObject() {
    // TODO: Create console object with all methods
    return new Object();
}

} // namespace Zepra::Debug
