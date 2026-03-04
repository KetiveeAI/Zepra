/**
 * @file console_log.h
 * @brief Console log capture for ZepraWebView
 * 
 * Captures JS console.log output from ZepraScript VM for display in WebView
 */

#ifndef CONSOLE_LOG_H
#define CONSOLE_LOG_H

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <mutex>

namespace zepra {

enum class ConsoleLogLevel {
    LOG,
    INFO,
    WARN,
    ERROR,
    DEBUG
};

struct ConsoleLogEntry {
    ConsoleLogLevel level;
    std::string message;
    std::string source;      // File or origin
    int line = 0;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * Console log manager - captures and stores console output
 */
class ConsoleLog {
public:
    using LogCallback = std::function<void(const ConsoleLogEntry&)>;
    
    static ConsoleLog& instance();
    
    // Add log entries
    void log(const std::string& message, const std::string& source = "");
    void info(const std::string& message, const std::string& source = "");
    void warn(const std::string& message, const std::string& source = "");
    void error(const std::string& message, const std::string& source = "");
    void debug(const std::string& message, const std::string& source = "");
    
    void addEntry(ConsoleLogLevel level, const std::string& message, 
                  const std::string& source = "", int line = 0);
    
    // Access entries
    std::vector<ConsoleLogEntry> getEntries() const;
    std::vector<ConsoleLogEntry> getLastN(size_t n) const;
    size_t size() const;
    
    // Filter
    std::vector<ConsoleLogEntry> filterByLevel(ConsoleLogLevel level) const;
    
    // Clear
    void clear();
    
    // Callback for new entries
    void setCallback(LogCallback callback);
    
    // Max entries (oldest removed when exceeded)
    void setMaxEntries(size_t max);
    
private:
    ConsoleLog();
    ~ConsoleLog() = default;
    
    std::vector<ConsoleLogEntry> entries_;
    mutable std::mutex mutex_;
    LogCallback callback_;
    size_t max_entries_ = 1000;
};

// Convenience macros for ZepraScript integration
#define ZEPRA_CONSOLE_LOG(msg) zepra::ConsoleLog::instance().log(msg)
#define ZEPRA_CONSOLE_INFO(msg) zepra::ConsoleLog::instance().info(msg)
#define ZEPRA_CONSOLE_WARN(msg) zepra::ConsoleLog::instance().warn(msg)
#define ZEPRA_CONSOLE_ERROR(msg) zepra::ConsoleLog::instance().error(msg)

// Helper to get level color
inline void getLogLevelColor(ConsoleLogLevel level, uint8_t& r, uint8_t& g, uint8_t& b) {
    switch (level) {
        case ConsoleLogLevel::LOG:   r = 180; g = 180; b = 200; break;
        case ConsoleLogLevel::INFO:  r = 100; g = 180; b = 255; break;
        case ConsoleLogLevel::WARN:  r = 255; g = 200; b = 100; break;
        case ConsoleLogLevel::ERROR: r = 255; g = 100; b = 100; break;
        case ConsoleLogLevel::DEBUG: r = 150; g = 150; b = 170; break;
    }
}

inline const char* getLogLevelPrefix(ConsoleLogLevel level) {
    switch (level) {
        case ConsoleLogLevel::LOG:   return "";
        case ConsoleLogLevel::INFO:  return "[info]";
        case ConsoleLogLevel::WARN:  return "[warn]";
        case ConsoleLogLevel::ERROR: return "[error]";
        case ConsoleLogLevel::DEBUG: return "[debug]";
    }
    return "";
}

} // namespace zepra

#endif // CONSOLE_LOG_H
