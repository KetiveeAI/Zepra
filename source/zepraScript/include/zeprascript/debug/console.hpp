#pragma once

/**
 * @file console.hpp
 * @brief DevTools Console - logging and REPL
 */

#include "../config.hpp"
#include "../runtime/value.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Debug {

using Runtime::Value;

/**
 * @brief Console message level
 */
enum class LogLevel {
    Log,
    Info,
    Warn,
    Error,
    Debug,
    Table,
    Group,
    GroupEnd,
    Clear
};

/**
 * @brief Console message
 */
struct ConsoleMessage {
    int id;
    LogLevel level;
    std::string text;
    std::string source;           // "javascript", "network", "security"
    std::string sourceFile;
    int lineNumber = 0;
    int columnNumber = 0;
    double timestamp;
    std::vector<Value> args;      // Original arguments
    std::string stackTrace;       // For errors
    int repeatCount = 1;
};

/**
 * @brief Console callback for new messages
 */
using ConsoleCallback = std::function<void(const ConsoleMessage&)>;

/**
 * @brief DevTools Console
 */
class Console {
public:
    static Console& instance();
    
    // --- Logging Methods ---
    
    void log(const std::vector<Value>& args);
    void info(const std::vector<Value>& args);
    void warn(const std::vector<Value>& args);
    void error(const std::vector<Value>& args);
    void debug(const std::vector<Value>& args);
    
    // --- Advanced ---
    
    void table(const Value& data, const std::vector<std::string>& columns = {});
    void dir(const Value& object);
    void dirxml(const Value& element);
    
    // --- Grouping ---
    
    void group(const std::string& label = "");
    void groupCollapsed(const std::string& label = "");
    void groupEnd();
    
    // --- Timing ---
    
    void time(const std::string& label = "default");
    void timeLog(const std::string& label = "default", const std::vector<Value>& args = {});
    void timeEnd(const std::string& label = "default");
    
    // --- Counting ---
    
    void count(const std::string& label = "default");
    void countReset(const std::string& label = "default");
    
    // --- Assertions ---
    
    void assert_(bool condition, const std::vector<Value>& args = {});
    
    // --- Stack Trace ---
    
    void trace(const std::string& label = "");
    
    // --- Clear ---
    
    void clear();
    
    // --- Message Access ---
    
    std::vector<ConsoleMessage> getMessages() const { return messages_; }
    void clearMessages() { messages_.clear(); }
    void setCallback(ConsoleCallback callback) { callback_ = std::move(callback); }
    
    // --- Source Info ---
    
    void setSourceInfo(const std::string& file, int line, int column);
    
private:
    Console() = default;
    
    void addMessage(LogLevel level, const std::string& text, 
                    const std::vector<Value>& args = {});
    std::string formatArgs(const std::vector<Value>& args);
    std::string getCurrentStackTrace();
    
    std::vector<ConsoleMessage> messages_;
    ConsoleCallback callback_;
    
    std::string currentFile_;
    int currentLine_ = 0;
    int currentColumn_ = 0;
    
    int groupDepth_ = 0;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> timers_;
    std::unordered_map<std::string, int> counters_;
    
    int nextMessageId_ = 1;
};

/**
 * @brief Console builtin for JavaScript
 */
class ConsoleBuiltin {
public:
    static Value log(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value info(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value warn(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value error(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value debug(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value table(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value group(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value groupEnd(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value time(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value timeEnd(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value count(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value assert_(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value trace(class Runtime::Context* ctx, const std::vector<Value>& args);
    static Value clear(class Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Runtime::Object* createConsoleObject();
};

} // namespace Zepra::Debug
