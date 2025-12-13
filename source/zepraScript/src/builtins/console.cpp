/**
 * @file console.cpp
 * @brief Console built-in implementation
 */

#include "zeprascript/builtins/console.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include <iostream>
#include <chrono>
#include <sstream>

namespace Zepra::Builtins {

Console::Console()
    : callback_(nullptr)
{
}

void Console::log(const std::vector<Runtime::Value>& args) {
    output("log", formatArgs(args));
}

void Console::info(const std::vector<Runtime::Value>& args) {
    output("info", formatArgs(args));
}

void Console::warn(const std::vector<Runtime::Value>& args) {
    output("warn", formatArgs(args));
}

void Console::error(const std::vector<Runtime::Value>& args) {
    output("error", formatArgs(args));
}

void Console::debug(const std::vector<Runtime::Value>& args) {
    output("debug", formatArgs(args));
}

void Console::trace(const std::vector<Runtime::Value>& args) {
    std::string message = formatArgs(args);
    message += "\n    at <trace>";
    output("trace", message);
}

void Console::assert_(bool condition, const std::vector<Runtime::Value>& args) {
    if (!condition) {
        std::string message = "Assertion failed";
        if (!args.empty()) {
            message += ": " + formatArgs(args);
        }
        output("error", message);
    }
}

void Console::group(const std::string& label) {
    output("group", label.empty() ? "console.group" : label);
    groupDepth_++;
}

void Console::groupEnd() {
    if (groupDepth_ > 0) {
        groupDepth_--;
    }
}

void Console::time(const std::string& label) {
    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
    timers_[label.empty() ? "default" : label] = static_cast<double>(now);
}

void Console::timeEnd(const std::string& label) {
    std::string key = label.empty() ? "default" : label;
    auto it = timers_.find(key);
    if (it == timers_.end()) {
        output("warn", "Timer '" + key + "' does not exist");
        return;
    }
    
    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
    
    double elapsed = static_cast<double>(now) - it->second;
    timers_.erase(it);
    
    std::ostringstream oss;
    oss << key << ": " << elapsed << "ms";
    output("log", oss.str());
}

void Console::timeLog(const std::string& label, const std::vector<Runtime::Value>& args) {
    std::string key = label.empty() ? "default" : label;
    auto it = timers_.find(key);
    if (it == timers_.end()) {
        output("warn", "Timer '" + key + "' does not exist");
        return;
    }
    
    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
    
    double elapsed = static_cast<double>(now) - it->second;
    
    std::ostringstream oss;
    oss << key << ": " << elapsed << "ms";
    if (!args.empty()) {
        oss << " " << formatArgs(args);
    }
    output("log", oss.str());
}

void Console::count(const std::string& label) {
    std::string key = label.empty() ? "default" : label;
    int count = ++counters_[key];
    
    std::ostringstream oss;
    oss << key << ": " << count;
    output("log", oss.str());
}

void Console::countReset(const std::string& label) {
    std::string key = label.empty() ? "default" : label;
    counters_[key] = 0;
}

void Console::table(const Runtime::Value& data) {
    // Simplified table output
    if (data.isObject()) {
        Runtime::Object* obj = data.asObject();
        if (obj->isArray()) {
            Runtime::Array* arr = static_cast<Runtime::Array*>(obj);
            std::ostringstream oss;
            oss << "┌────────┬─────────────┐\n";
            oss << "│ Index  │ Value       │\n";
            oss << "├────────┼─────────────┤\n";
            for (size_t i = 0; i < arr->length(); i++) {
                oss << "│ " << i << "      │ " << arr->at(i).toString() << "\n";
            }
            oss << "└────────┴─────────────┘";
            output("log", oss.str());
            return;
        }
    }
    output("log", data.toString());
}

void Console::clear() {
    output("clear", "");
}

void Console::output(const std::string& level, const std::string& message) {
    // Add indentation for groups
    std::string indent(groupDepth_ * 2, ' ');
    std::string fullMessage = indent + message;
    
    if (callback_) {
        callback_(fullMessage, level);
    } else {
        // Default to stdout/stderr
        if (level == "error" || level == "warn") {
            std::cerr << fullMessage << std::endl;
        } else {
            std::cout << fullMessage << std::endl;
        }
    }
}

std::string Console::formatArgs(const std::vector<Runtime::Value>& args) {
    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) oss << " ";
        oss << args[i].toString();
    }
    return oss.str();
}

Runtime::Object* Console::createConsoleObject(Runtime::Context* ctx) {
    static Console console;
    
    auto* obj = new Runtime::Object();
    
    // console.log
    obj->set("log", Runtime::Value::object(
        Runtime::createNativeFunction("log", 
            [](Runtime::Context*, const std::vector<Runtime::Value>& args) {
                console.log(args);
                return Runtime::Value::undefined();
            }, 0)
    ));
    
    // console.info
    obj->set("info", Runtime::Value::object(
        Runtime::createNativeFunction("info",
            [](Runtime::Context*, const std::vector<Runtime::Value>& args) {
                console.info(args);
                return Runtime::Value::undefined();
            }, 0)
    ));
    
    // console.warn
    obj->set("warn", Runtime::Value::object(
        Runtime::createNativeFunction("warn",
            [](Runtime::Context*, const std::vector<Runtime::Value>& args) {
                console.warn(args);
                return Runtime::Value::undefined();
            }, 0)
    ));
    
    // console.error
    obj->set("error", Runtime::Value::object(
        Runtime::createNativeFunction("error",
            [](Runtime::Context*, const std::vector<Runtime::Value>& args) {
                console.error(args);
                return Runtime::Value::undefined();
            }, 0)
    ));
    
    // console.clear
    obj->set("clear", Runtime::Value::object(
        Runtime::createNativeFunction("clear",
            [](Runtime::Context*, const std::vector<Runtime::Value>&) {
                console.clear();
                return Runtime::Value::undefined();
            }, 0)
    ));
    
    // console.time
    obj->set("time", Runtime::Value::object(
        Runtime::createNativeFunction("time",
            [](Runtime::Context*, const std::vector<Runtime::Value>& args) {
                std::string label = args.empty() ? "" : args[0].toString();
                console.time(label);
                return Runtime::Value::undefined();
            }, 0)
    ));
    
    // console.timeEnd
    obj->set("timeEnd", Runtime::Value::object(
        Runtime::createNativeFunction("timeEnd",
            [](Runtime::Context*, const std::vector<Runtime::Value>& args) {
                std::string label = args.empty() ? "" : args[0].toString();
                console.timeEnd(label);
                return Runtime::Value::undefined();
            }, 0)
    ));
    
    return obj;
}

} // namespace Zepra::Builtins
