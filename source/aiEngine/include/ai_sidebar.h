/**
 * @file ai_sidebar.h
 * @brief AI-powered sidebar using ZepraSearch ML services
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ZepraBrowser {

struct ChatMessage {
    std::string role;      // "user" or "assistant"
    std::string content;
    int64_t timestamp;
};

struct PageAnalysis {
    std::string summary;
    std::vector<std::string> keyPoints;
    std::vector<std::string> entities;
    float relevanceScore;
};

/**
 * AISidebar - Integrates ZepraSearch AI for page intelligence
 * 
 * Features:
 * - Chat about current page
 * - Page summarization
 * - Entity extraction
 * - Q&A using RAG
 * - Uses ZepraSearch's unified_ml_api.py (localhost:5000)
 */
class AISidebar {
public:
    static AISidebar& instance();
    
    // Sidebar state
    bool isVisible() const;
    void show();
    void hide();
    void toggle();
    
    // Page analysis
    PageAnalysis analyzePage(const std::string& html, const std::string& url);
    std::string summarizePage(const std::string& html);
    std::vector<std::string> extractEntities(const std::string& text);
    
    // Chat
    std::string chat(const std::string& userMessage, const std::string& pageContext);
    std::vector<ChatMessage> getChatHistory() const;
    void clearChatHistory();
    
    // RAG (Retrieval-Augmented Generation)
    std::string askAboutPage(const std::string& question, const std::string& pageContent);
    
    // ML Service connection
    bool isMLServiceAvailable() const;
    std::string getMLServiceStatus() const;
    
    // Callbacks
    using ResponseCallback = std::function<void(const std::string& response)>;
    void onAIResponse(ResponseCallback callback);
    
private:
    AISidebar();
    ~AISidebar();
    AISidebar(const AISidebar&) = delete;
    AISidebar& operator=(const AISidebar&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // HTTP calls to ZepraSearch ML services
    std::string callMLService(const std::string& endpoint, const std::string& payload);
};

} // namespace ZepraBrowser
