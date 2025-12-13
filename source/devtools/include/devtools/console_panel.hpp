/**
 * @file console_panel.hpp
 * @brief Console Panel - JavaScript console with REPL
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <memory>

namespace Zepra::DevTools {

/**
 * @brief Console message level
 */
enum class ConsoleLevel {
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
 * @brief Source location
 */
struct SourceLocation {
    std::string url;
    int line;
    int column;
};

/**
 * @brief Console message
 */
struct ConsoleMessage {
    int id;
    ConsoleLevel level;
    std::string text;
    std::string formattedText;
    SourceLocation source;
    std::chrono::system_clock::time_point timestamp;
    int repeatCount = 1;
    bool expanded = false;
    
    // For objects
    bool isObject = false;
    std::string objectPreview;
    std::vector<std::pair<std::string, std::string>> properties;
};

/**
 * @brief Console evaluation result
 */
struct EvalResult {
    bool success;
    std::string value;
    std::string type;
    std::string error;
    bool isObject;
    std::string objectPreview;
};

/**
 * @brief Console filter options
 */
struct ConsoleFilter {
    bool showLogs = true;
    bool showWarnings = true;
    bool showErrors = true;
    bool showInfo = true;
    bool showDebug = true;
    std::string textFilter;
    std::string urlFilter;
};

/**
 * @brief Console message callback
 */
using ConsoleCallback = std::function<void(const ConsoleMessage&)>;
using EvalCallback = std::function<EvalResult(const std::string&)>;

/**
 * @brief Console Panel - JavaScript Console & REPL
 */
class ConsolePanel {
public:
    ConsolePanel();
    ~ConsolePanel();
    
    // --- Messages ---
    
    /**
     * @brief Add a console message
     */
    void addMessage(ConsoleLevel level, const std::string& text,
                    const SourceLocation& source = {});
    
    /**
     * @brief Add a formatted message with objects
     */
    void addMessage(const ConsoleMessage& message);
    
    /**
     * @brief Get all messages
     */
    const std::vector<ConsoleMessage>& messages() const { return messages_; }
    
    /**
     * @brief Get filtered messages
     */
    std::vector<ConsoleMessage> filteredMessages() const;
    
    /**
     * @brief Clear all messages
     */
    void clear();
    
    // --- Evaluation ---
    
    /**
     * @brief Evaluate JavaScript expression
     */
    EvalResult evaluate(const std::string& expression);
    
    /**
     * @brief Set evaluation callback (connects to JS engine)
     */
    void setEvalCallback(EvalCallback callback);
    
    /**
     * @brief Get command history
     */
    const std::vector<std::string>& history() const { return history_; }
    
    /**
     * @brief Navigate history
     */
    std::string historyUp();
    std::string historyDown();
    
    // --- Filtering ---
    
    /**
     * @brief Set filter options
     */
    void setFilter(const ConsoleFilter& filter);
    ConsoleFilter filter() const { return filter_; }
    
    // --- Callbacks ---
    void onMessage(ConsoleCallback callback);
    
    // --- UI ---
    void update();
    void render();
    
    // --- Input ---
    void setInput(const std::string& text);
    std::string input() const { return currentInput_; }
    void submitInput();
    
private:
    void renderMessage(const ConsoleMessage& msg);
    void renderInput();
    std::string getLevelIcon(ConsoleLevel level);
    std::string getLevelColor(ConsoleLevel level);
    
    std::vector<ConsoleMessage> messages_;
    std::vector<std::string> history_;
    int historyIndex_ = -1;
    int nextMessageId_ = 1;
    
    ConsoleFilter filter_;
    EvalCallback evalCallback_;
    std::vector<ConsoleCallback> callbacks_;
    
    std::string currentInput_;
    bool inputFocused_ = false;
};

} // namespace Zepra::DevTools
