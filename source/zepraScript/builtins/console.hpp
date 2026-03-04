#pragma once

/**
 * @file console.hpp
 * @brief console.log and related functions
 */

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

namespace Zepra::Runtime {
class Context;
class Object;
}

namespace Zepra::Builtins {

/**
 * @brief Console output callback
 */
using ConsoleCallback = std::function<void(const std::string& message, 
                                            const std::string& level)>;

/**
 * @brief Console implementation
 */
class Console {
public:
    Console();
    
    /**
     * @brief Set output callback
     */
    void setCallback(ConsoleCallback callback) { callback_ = callback; }
    
    /**
     * @brief Log methods
     */
    void log(const std::vector<Runtime::Value>& args);
    void info(const std::vector<Runtime::Value>& args);
    void warn(const std::vector<Runtime::Value>& args);
    void error(const std::vector<Runtime::Value>& args);
    void debug(const std::vector<Runtime::Value>& args);
    void trace(const std::vector<Runtime::Value>& args);
    
    /**
     * @brief Assertion
     */
    void assert_(bool condition, const std::vector<Runtime::Value>& args);
    
    /**
     * @brief Grouping
     */
    void group(const std::string& label);
    void groupEnd();
    
    /**
     * @brief Timing
     */
    void time(const std::string& label);
    void timeEnd(const std::string& label);
    void timeLog(const std::string& label, const std::vector<Runtime::Value>& args);
    
    /**
     * @brief Counting
     */
    void count(const std::string& label);
    void countReset(const std::string& label);
    
    /**
     * @brief Table output
     */
    void table(const Runtime::Value& data);
    
    /**
     * @brief Clear console
     */
    void clear();
    
    /**
     * @brief Create console object with all methods
     */
    static Runtime::Object* createConsoleObject(Runtime::Context* ctx);
    
private:
    void output(const std::string& level, const std::string& message);
    std::string formatArgs(const std::vector<Runtime::Value>& args);
    
    ConsoleCallback callback_;
    std::unordered_map<std::string, int> counters_;
    std::unordered_map<std::string, double> timers_;
    int groupDepth_ = 0;
};

} // namespace Zepra::Builtins
