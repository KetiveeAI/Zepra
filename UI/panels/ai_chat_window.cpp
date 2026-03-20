// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file ai_chat_window.cpp
 * @brief AI chat window implementation (pure C++, no SDL)
 */

#include "../../source/zepraEngine/include/engine/ui/ai_chat_window.h"
#include <chrono>

namespace zepra {
namespace ui {

struct AIChatWindow::Impl {
    AIChatConfig config;
    bool visible = false;
    bool loading = false;
    
    float x = 0, y = 0, width = 400, height = 500;
    
    std::vector<ChatMessage> messages;
    std::string inputText;
    int cursorPosition = 0;
    float scrollOffset = 0;
    
    std::string streamingMessageId;
    
    // Page context
    std::string pageUrl;
    std::string pageTitle;
    std::string selectedText;
    
    SendCallback sendCallback;
    CloseCallback closeCallback;
    
    Impl() : config() {}
    explicit Impl(const AIChatConfig& cfg) : config(cfg) {}
    
    int64_t now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    std::string generateId() {
        return "msg_" + std::to_string(now());
    }
};

AIChatWindow::AIChatWindow() : impl_(std::make_unique<Impl>()) {}

AIChatWindow::AIChatWindow(const AIChatConfig& config) 
    : impl_(std::make_unique<Impl>(config)) {}

AIChatWindow::~AIChatWindow() = default;

void AIChatWindow::setConfig(const AIChatConfig& config) {
    impl_->config = config;
}

AIChatConfig AIChatWindow::getConfig() const {
    return impl_->config;
}

void AIChatWindow::show() {
    impl_->visible = true;
}

void AIChatWindow::hide() {
    impl_->visible = false;
    if (impl_->closeCallback) {
        impl_->closeCallback();
    }
}

void AIChatWindow::toggle() {
    if (impl_->visible) {
        hide();
    } else {
        show();
    }
}

bool AIChatWindow::isVisible() const {
    return impl_->visible;
}

void AIChatWindow::addMessage(const ChatMessage& message) {
    impl_->messages.push_back(message);
    
    // Scroll to bottom
    // (In actual implementation, calculate content height)
}

void AIChatWindow::clearMessages() {
    impl_->messages.clear();
    impl_->streamingMessageId.clear();
}

std::vector<ChatMessage> AIChatWindow::getMessages() const {
    return impl_->messages;
}

void AIChatWindow::startStreaming(const std::string& messageId) {
    impl_->streamingMessageId = messageId;
    
    // Find or create message
    bool found = false;
    for (auto& msg : impl_->messages) {
        if (msg.id == messageId) {
            msg.isStreaming = true;
            found = true;
            break;
        }
    }
    
    if (!found) {
        ChatMessage msg;
        msg.id = messageId;
        msg.role = ChatRole::Assistant;
        msg.isStreaming = true;
        msg.timestamp = impl_->now();
        impl_->messages.push_back(msg);
    }
}

void AIChatWindow::appendToStreaming(const std::string& content) {
    for (auto& msg : impl_->messages) {
        if (msg.id == impl_->streamingMessageId) {
            msg.content += content;
            break;
        }
    }
}

void AIChatWindow::finishStreaming() {
    for (auto& msg : impl_->messages) {
        if (msg.id == impl_->streamingMessageId) {
            msg.isStreaming = false;
            break;
        }
    }
    impl_->streamingMessageId.clear();
}

void AIChatWindow::setPageContext(const std::string& url, const std::string& title, 
                                   const std::string& selectedText) {
    impl_->pageUrl = url;
    impl_->pageTitle = title;
    impl_->selectedText = selectedText;
}

void AIChatWindow::setSendCallback(SendCallback callback) {
    impl_->sendCallback = std::move(callback);
}

void AIChatWindow::setCloseCallback(CloseCallback callback) {
    impl_->closeCallback = std::move(callback);
}

bool AIChatWindow::isLoading() const {
    return impl_->loading;
}

void AIChatWindow::setLoading(bool loading) {
    impl_->loading = loading;
}

void AIChatWindow::setBounds(float x, float y, float width, float height) {
    impl_->x = x;
    impl_->y = y;
    impl_->width = width;
    impl_->height = height;
}

void AIChatWindow::render() {
    // Note: Actual rendering is done by the browser's OpenGL code
    // This component provides state and callbacks
}

void AIChatWindow::update(float) {
    // Update animations, cursor blink, etc.
}

bool AIChatWindow::handleKeyPress(int keyCode, bool ctrl, bool) {
    if (!impl_->visible) return false;
    
    // Handle Enter to send
    if (keyCode == 0xFF0D || keyCode == '\r') {
        if (!impl_->inputText.empty() && impl_->sendCallback) {
            // Add user message
            ChatMessage userMsg;
            userMsg.id = impl_->generateId();
            userMsg.role = ChatRole::User;
            userMsg.content = impl_->inputText;
            userMsg.timestamp = impl_->now();
            impl_->messages.push_back(userMsg);
            
            // Call send callback
            impl_->sendCallback(impl_->inputText);
            
            // Clear input
            impl_->inputText.clear();
            impl_->cursorPosition = 0;
        }
        return true;
    }
    
    // Handle Escape to close
    if (keyCode == 0xFF1B) {
        hide();
        return true;
    }
    
    // Handle Backspace
    if (keyCode == 0xFF08) {
        if (impl_->cursorPosition > 0) {
            impl_->inputText.erase(impl_->cursorPosition - 1, 1);
            impl_->cursorPosition--;
        }
        return true;
    }
    
    // Handle Delete
    if (keyCode == 0xFFFF) {
        if (impl_->cursorPosition < static_cast<int>(impl_->inputText.length())) {
            impl_->inputText.erase(impl_->cursorPosition, 1);
        }
        return true;
    }
    
    // Handle Ctrl+A (select all - not implemented in simple version)
    if (ctrl && (keyCode == 'a' || keyCode == 'A')) {
        return true;
    }
    
    return false;
}

bool AIChatWindow::handleTextInput(const std::string& text) {
    if (!impl_->visible) return false;
    
    impl_->inputText.insert(impl_->cursorPosition, text);
    impl_->cursorPosition += static_cast<int>(text.length());
    return true;
}

bool AIChatWindow::handleMouseClick(float x, float y) {
    if (!impl_->visible) return false;
    
    // Check if within bounds
    if (x < impl_->x || x > impl_->x + impl_->width) return false;
    if (y < impl_->y || y > impl_->y + impl_->height) return false;
    
    // Check close button (top right)
    float closeX = impl_->x + impl_->width - 32;
    float closeY = impl_->y + 8;
    if (x >= closeX && x <= closeX + 24 && y >= closeY && y <= closeY + 24) {
        hide();
        return true;
    }
    
    return true;
}

bool AIChatWindow::handleMouseMove(float, float) {
    // Handle hover states
    return false;
}

bool AIChatWindow::handleScroll(float x, float, float delta) {
    if (!impl_->visible) return false;
    
    if (x < impl_->x || x > impl_->x + impl_->width) return false;
    
    impl_->scrollOffset -= delta * 40;
    impl_->scrollOffset = std::max(0.0f, impl_->scrollOffset);
    
    return true;
}

} // namespace ui
} // namespace zepra
