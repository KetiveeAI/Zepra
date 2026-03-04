/**
 * @file ConsoleAPI.h
 * @brief Console Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>

namespace Zepra::Runtime {

class Console {
public:
    using LogHandler = std::function<void(const std::string&, const std::string&)>;
    
    Console() = default;
    explicit Console(LogHandler handler) : handler_(std::move(handler)) {}
    
    // Basic logging
    template<typename... Args>
    void log(Args&&... args) { output("log", format(std::forward<Args>(args)...)); }
    
    template<typename... Args>
    void info(Args&&... args) { output("info", format(std::forward<Args>(args)...)); }
    
    template<typename... Args>
    void warn(Args&&... args) { output("warn", format(std::forward<Args>(args)...)); }
    
    template<typename... Args>
    void error(Args&&... args) { output("error", format(std::forward<Args>(args)...)); }
    
    template<typename... Args>
    void debug(Args&&... args) { output("debug", format(std::forward<Args>(args)...)); }
    
    // Assertion
    template<typename... Args>
    void assert_(bool condition, Args&&... args) {
        if (!condition) {
            output("error", "Assertion failed: " + format(std::forward<Args>(args)...));
        }
    }
    
    // Counting
    void count(const std::string& label = "default") {
        ++counts_[label];
        output("log", label + ": " + std::to_string(counts_[label]));
    }
    
    void countReset(const std::string& label = "default") {
        counts_[label] = 0;
    }
    
    // Timing
    void time(const std::string& label = "default") {
        timers_[label] = std::chrono::steady_clock::now();
    }
    
    void timeEnd(const std::string& label = "default") {
        auto it = timers_.find(label);
        if (it != timers_.end()) {
            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - it->second).count();
            output("log", label + ": " + std::to_string(ms) + "ms");
            timers_.erase(it);
        }
    }
    
    void timeLog(const std::string& label = "default") {
        auto it = timers_.find(label);
        if (it != timers_.end()) {
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
            output("log", label + ": " + std::to_string(ms) + "ms");
        }
    }
    
    // Grouping
    void group(const std::string& label = "") {
        if (!label.empty()) output("log", label);
        ++indentLevel_;
    }
    
    void groupCollapsed(const std::string& label = "") {
        group(label);
    }
    
    void groupEnd() {
        if (indentLevel_ > 0) --indentLevel_;
    }
    
    // Trace
    void trace(const std::string& message = "") {
        output("log", "Trace: " + message);
    }
    
    // Clear
    void clear() {
        output("clear", "");
    }
    
    // Table
    template<typename T>
    void table(const std::vector<T>& data) {
        std::ostringstream oss;
        oss << "┌───────┬──────────────────┐\n";
        oss << "│ Index │ Value            │\n";
        oss << "├───────┼──────────────────┤\n";
        for (size_t i = 0; i < data.size(); ++i) {
            oss << "│ " << std::setw(5) << i << " │ " << std::setw(16) << data[i] << " │\n";
        }
        oss << "└───────┴──────────────────┘";
        output("log", oss.str());
    }
    
    // Dir
    void dir(const std::string& obj) {
        output("log", obj);
    }
    
    void dirxml(const std::string& obj) {
        output("log", obj);
    }

private:
    template<typename T>
    std::string stringify(const T& val) {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }
    
    template<typename T, typename... Rest>
    std::string format(T&& first, Rest&&... rest) {
        std::string result = stringify(std::forward<T>(first));
        if constexpr (sizeof...(rest) > 0) {
            result += " " + format(std::forward<Rest>(rest)...);
        }
        return result;
    }
    
    std::string format() { return ""; }
    
    void output(const std::string& level, const std::string& message) {
        std::string indent(indentLevel_ * 2, ' ');
        std::string output = indent + message;
        
        if (handler_) {
            handler_(level, output);
        } else {
            if (level == "error" || level == "warn") {
                std::cerr << "[" << level << "] " << output << "\n";
            } else {
                std::cout << output << "\n";
            }
        }
    }
    
    LogHandler handler_;
    std::map<std::string, int> counts_;
    std::map<std::string, std::chrono::steady_clock::time_point> timers_;
    int indentLevel_ = 0;
};

} // namespace Zepra::Runtime
