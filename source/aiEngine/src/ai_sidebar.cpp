/**
 * @file ai_sidebar.cpp
 * @brief AI sidebar implementation using ZepraSearch
 */

#include "ai/ai_sidebar.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <nxhttp.h>
#include <nxjson.h>

namespace ZepraBrowser {

// ZepraSearch ML endpoints (local)
const char* ML_API_BASE = "http://localhost:5000";
const char* AI_CHAT_BASE = "http://localhost:5001";

struct AISidebar::Impl {
    bool visible = false;
    std::vector<ChatMessage> chatHistory;
    ResponseCallback responseCallback;
    
    // Service status
    bool mlServiceOnline = false;
    std::string lastError;
};

AISidebar& AISidebar::instance() {
    static AISidebar instance;
    return instance;
}

AISidebar::AISidebar() : impl_(std::make_unique<Impl>()) {
    std::cout << "[AISidebar] Initialized (ZepraSearch ML integration)" << std::endl;
    
    // Check if ML services are running
    impl_->mlServiceOnline = isMLServiceAvailable();
    if (impl_->mlServiceOnline) {
        std::cout << "[AISidebar] ✓ Connected to ZepraSearch ML services" << std::endl;
    } else {
        std::cout << "[AISidebar] ⚠ ZepraSearch ML services not available" << std::endl;
    }
}

AISidebar::~AISidebar() = default;

bool AISidebar::isVisible() const {
    return impl_->visible;
}

void AISidebar::show() {
    impl_->visible = true;
    std::cout << "[AISidebar] Sidebar shown" << std::endl;
}

void AISidebar::hide() {
    impl_->visible = false;
    std::cout << "[AISidebar] Sidebar hidden" << std::endl;
}

void AISidebar::toggle() {
    impl_->visible = !impl_->visible;
}

PageAnalysis AISidebar::analyzePage(const std::string& html, const std::string& url) {
    PageAnalysis analysis;
    
    if (!impl_->mlServiceOnline) {
        analysis.summary = "ML service unavailable";
        return analysis;
    }
    
    // Call ZepraSearch unified ML API
    std::string payload = "{\"html\":\"" + html + "\",\"url\":\"" + url + "\"}";
    std::string response = callMLService("/analyze_page", payload);
    
    // Parse JSON response using nxjson
    try {
        auto root = nx::Json::parse(response);
        
        if (root.has("summary")) {
            analysis.summary = root["summary"].asString();
        }
        if (root.has("relevance")) {
            analysis.relevanceScore = static_cast<float>(root["relevance"].asNumber());
        }
        
        // Extract entities
        if (root.has("entities") && root["entities"].isArray()) {
            auto entities = root["entities"];
            for (size_t i = 0; i < entities.size(); ++i) {
                analysis.entities.push_back(entities[i].asString());
            }
        }
        
        // Extract key points
        if (root.has("key_points") && root["key_points"].isArray()) {
            auto points = root["key_points"];
            for (size_t i = 0; i < points.size(); ++i) {
                analysis.keyPoints.push_back(points[i].asString());
            }
        }
    } catch (const nx::JsonException& e) {
        analysis.summary = "Parse error: " + std::string(e.what());
    }
    
    return analysis;
}

std::string AISidebar::summarizePage(const std::string& html) {
    if (!impl_->mlServiceOnline) {
        return "AI service offline";
    }
    
    // Call ZepraSearch RAG summarization
    std::string payload = "{\"text\":\"" + html + "\",\"max_length\":200}";
    std::string response = callMLService("/rag/summarize", payload);
    
    try {
        auto root = nx::Json::parse(response);
        if (root.has("summary")) {
            return root["summary"].asString();
        }
        return "Error generating summary";
    } catch (const nx::JsonException&) {
        return "Parse error";
    }
}

std::vector<std::string> AISidebar::extractEntities(const std::string& text) {
    std::vector<std::string> entities;
    
    if (!impl_->mlServiceOnline) {
        return entities;
    }
    
    // Call ZepraSearch entity extraction
    std::string payload = "{\"text\":\"" + text + "\"}";
    std::string response = callMLService("/extract_entities", payload);
    
    try {
        auto root = nx::Json::parse(response);
        if (root.has("entities") && root["entities"].isArray()) {
            auto arr = root["entities"];
            for (size_t i = 0; i < arr.size(); ++i) {
                entities.push_back(arr[i].asString());
            }
        }
    } catch (const nx::JsonException&) {
        // Parse error - return empty list
    }
    
    return entities;
}

std::string AISidebar::chat(const std::string& userMessage, const std::string& pageContext) {
    if (!impl_->mlServiceOnline) {
        return "AI chat service is currently offline. Please start ZepraSearch ML services.";
    }
    
    // Add user message to history
    ChatMessage userMsg;
    userMsg.role = "user";
    userMsg.content = userMessage;
    userMsg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    impl_->chatHistory.push_back(userMsg);
    
    // Call ZepraSearch AI chat service (port 5001) using nxhttp
    std::string payload = "{"
        "\"message\":\"" + userMessage + "\","
        "\"context\":\"" + pageContext + "\""
    "}";
    
    std::string url = std::string(AI_CHAT_BASE) + "/chat";
    std::string aiResponse = "No response";
    
    try {
        nx::HttpClient client;
        auto response = client.post(url, payload, "application/json");
        
        if (!response.ok()) {
            return "Error connecting to AI service: HTTP " + std::to_string(response.status());
        }
        
        // Parse response using nxjson
        try {
            auto root = nx::Json::parse(response.body());
            if (root.has("response")) {
                aiResponse = root["response"].asString();
            }
        } catch (const nx::JsonException&) {
            aiResponse = "Parse error in response";
        }
    } catch (const nx::HttpException& e) {
        return "Error connecting to AI service: " + std::string(e.what());
    }
    
    // Add AI response to history
    ChatMessage aiMsg;
    aiMsg.role = "assistant";
    aiMsg.content = aiResponse;
    aiMsg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    impl_->chatHistory.push_back(aiMsg);
    
    if (impl_->responseCallback) {
        impl_->responseCallback(aiResponse);
    }
    
    return aiResponse;
}

std::vector<ChatMessage> AISidebar::getChatHistory() const {
    return impl_->chatHistory;
}

void AISidebar::clearChatHistory() {
    impl_->chatHistory.clear();
    std::cout << "[AISidebar] Chat history cleared" << std::endl;
}

std::string AISidebar::askAboutPage(const std::string& question, const std::string& pageContent) {
    return chat(question, pageContent);
}

bool AISidebar::isMLServiceAvailable() const {
    // Quick health check using nxhttp
    try {
        std::string url = std::string(ML_API_BASE) + "/health";
        nx::HttpClient client;
        auto response = client.get(url);
        return response.ok();
    } catch (const nx::HttpException&) {
        return false;
    }
}

std::string AISidebar::getMLServiceStatus() const {
    if (impl_->mlServiceOnline) {
        return "✓ ZepraSearch ML services online";
    } else {
        return "✗ ZepraSearch ML services offline\nStart with: cd /home/swana/Documents/NEOLYXOS/zeprasearch && ./start.sh";
    }
}

void AISidebar::onAIResponse(ResponseCallback callback) {
    impl_->responseCallback = callback;
}

std::string AISidebar::callMLService(const std::string& endpoint, const std::string& payload) {
    try {
        std::string url = std::string(ML_API_BASE) + endpoint;
        nx::HttpClient client;
        auto response = client.post(url, payload, "application/json");
        
        if (response.ok()) {
            return response.body();
        } else {
            return "{\"error\":\"HTTP " + std::to_string(response.status()) + "\"}";
        }
    } catch (const nx::HttpException& e) {
        return "{\"error\":\"" + std::string(e.what()) + "\"}";
    }
}

} // namespace ZepraBrowser
