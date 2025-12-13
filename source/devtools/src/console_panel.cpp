/**
 * @file console_panel.cpp
 * @brief JavaScript console panel implementation
 */

#include "devtools/console_panel.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Zepra::DevTools {

ConsolePanel::ConsolePanel() : DevToolsPanel("Console") {}

ConsolePanel::~ConsolePanel() = default;

void ConsolePanel::render() {
    // Console renders message list and input field
}

void ConsolePanel::refresh() {
    // No-op for console
}

void ConsolePanel::log(const std::string& message, ConsoleLevel level) {
    ConsoleMessage msg;
    msg.message = message;
    msg.level = level;
    msg.timestamp = std::chrono::system_clock::now();
    
    addMessage(msg);
}

void ConsolePanel::addMessage(const ConsoleMessage& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    messages_.push_back(message);
    
    // Limit size
    while (messages_.size() > maxMessages_) {
        messages_.erase(messages_.begin());
    }
    
    if (onMessage_) {
        onMessage_(message);
    }
}

void ConsolePanel::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
}

std::vector<ConsoleMessage> ConsolePanel::getMessages() {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
}

std::vector<ConsoleMessage> ConsolePanel::getFilteredMessages(ConsoleLevel minLevel) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ConsoleMessage> filtered;
    for (const auto& msg : messages_) {
        if (static_cast<int>(msg.level) >= static_cast<int>(minLevel)) {
            filtered.push_back(msg);
        }
    }
    return filtered;
}

std::string ConsolePanel::evaluate(const std::string& expression) {
    if (onEvaluate_) {
        return onEvaluate_(expression);
    }
    return "Error: No evaluator connected";
}

std::string ConsolePanel::formatTimestamp(std::chrono::system_clock::time_point time) {
    auto t = std::chrono::system_clock::to_time_t(time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string ConsolePanel::levelToString(ConsoleLevel level) {
    switch (level) {
        case ConsoleLevel::Log: return "log";
        case ConsoleLevel::Info: return "info";
        case ConsoleLevel::Warn: return "warn";
        case ConsoleLevel::Error: return "error";
        case ConsoleLevel::Debug: return "debug";
        default: return "log";
    }
}

} // namespace Zepra::DevTools
