/**
 * @file ai_chat_window.h
 * @brief AI assistant chat panel for browser
 */

#ifndef ZEPRA_UI_AI_CHAT_WINDOW_H
#define ZEPRA_UI_AI_CHAT_WINDOW_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zepra {
namespace ui {

/**
 * Chat message roles
 */
enum class ChatRole {
    User,
    Assistant,
    System
};

/**
 * Single chat message
 */
struct ChatMessage {
    std::string id;
    ChatRole role;
    std::string content;
    int64_t timestamp;
    bool isStreaming = false;   // For streaming responses
    std::string error;          // Error message if failed
};

/**
 * AI Chat configuration
 */
struct AIChatConfig {
    float width = 400.0f;
    float minHeight = 300.0f;
    std::string placeholder = "Ask me anything...";
    std::string assistantName = "Zepra AI";
    std::string apiEndpoint = "";
    bool enableMarkdown = true;
    bool enableCodeHighlight = true;
};

/**
 * AIChatWindow - AI assistant chat panel
 * 
 * Features:
 * - Chat message history
 * - Streaming responses
 * - Markdown rendering
 * - Code syntax highlighting
 * - Context from current page
 */
class AIChatWindow {
public:
    using SendCallback = std::function<void(const std::string& message)>;
    using CloseCallback = std::function<void()>;

    AIChatWindow();
    explicit AIChatWindow(const AIChatConfig& config);
    ~AIChatWindow();

    // Configuration
    void setConfig(const AIChatConfig& config);
    AIChatConfig getConfig() const;

    // Visibility
    void show();
    void hide();
    void toggle();
    bool isVisible() const;

    // Messages
    void addMessage(const ChatMessage& message);
    void clearMessages();
    std::vector<ChatMessage> getMessages() const;

    // Streaming support
    void startStreaming(const std::string& messageId);
    void appendToStreaming(const std::string& content);
    void finishStreaming();

    // Page context
    void setPageContext(const std::string& url, const std::string& title, 
                        const std::string& selectedText = "");

    // Callbacks
    void setSendCallback(SendCallback callback);
    void setCloseCallback(CloseCallback callback);

    // State
    bool isLoading() const;
    void setLoading(bool loading);

    // Rendering
    void setBounds(float x, float y, float width, float height);
    void render();
    void update(float deltaTime);

    // Event handling
    bool handleKeyPress(int keyCode, bool ctrl, bool shift);
    bool handleTextInput(const std::string& text);
    bool handleMouseClick(float x, float y);
    bool handleMouseMove(float x, float y);
    bool handleScroll(float x, float y, float delta);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ui
} // namespace zepra

#endif // ZEPRA_UI_AI_CHAT_WINDOW_H
