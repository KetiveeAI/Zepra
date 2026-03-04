/**
 * @file websocket.cpp
 * @brief WebSocket API implementation for ZepraScript
 */

#include "browser/WebSocketAPI.h"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <algorithm>
#include <regex>

namespace Zepra::Browser {

// =============================================================================
// WebSocket Implementation
// =============================================================================

WebSocket::WebSocket(const std::string& url, 
                     const std::vector<std::string>& protocols)
    : Object(Runtime::ObjectType::Ordinary)
    , url_(url)
    , protocols_(protocols) {
    
    // Validate URL scheme
    if (url.substr(0, 5) != "ws://" && url.substr(0, 6) != "wss://") {
        onErrorOccurred("Invalid WebSocket URL: must start with ws:// or wss://");
        readyState_ = ReadyState::CLOSED;
        return;
    }
    
    // Start connection asynchronously
    connectInternal();
}

WebSocket::~WebSocket() {
    if (readyState_ == ReadyState::OPEN || readyState_ == ReadyState::CONNECTING) {
        close(1001, "Going away");
    }
}

void WebSocket::connectInternal() {
    readyState_ = ReadyState::CONNECTING;
    
    // Platform-specific connection would be initiated here
    // For now, simulate immediate connection for testing
    
    // In a real implementation, this would:
    // 1. Parse URL into host, port, path
    // 2. Create TCP/TLS connection
    // 3. Perform WebSocket handshake
    // 4. Call onConnected() on success
}

void WebSocket::send(const Value& data) {
    if (readyState_ != ReadyState::OPEN) {
        if (readyState_ == ReadyState::CONNECTING) {
            // Queue message for later
            if (data.isString()) {
                std::string text = data.asString()->value();
                std::vector<uint8_t> bytes(text.begin(), text.end());
                pendingMessages_.push(std::move(bytes));
                bufferedAmount_ += text.size();
            }
        }
        return;
    }
    
    if (data.isString()) {
        send(data.asString()->value());
    } else if (data.isObject()) {
        // Handle ArrayBuffer/TypedArray
        Object* obj = data.asObject();
        // Check if it's an ArrayBuffer by type
        if (obj->objectType() == Runtime::ObjectType::ArrayBuffer) {
            // Get buffer data and send as binary
            // Placeholder for ArrayBuffer handling
        }
    }
}

void WebSocket::send(const std::string& text) {
    if (readyState_ != ReadyState::OPEN) {
        onErrorOccurred("WebSocket is not open");
        return;
    }
    
    // Create text frame
    // Frame format: FIN=1, opcode=0x01 (text), mask, payload
    std::vector<uint8_t> frame;
    
    // First byte: FIN + opcode
    frame.push_back(0x81);  // FIN=1, opcode=1 (text)
    
    // Payload length
    size_t len = text.size();
    if (len <= 125) {
        frame.push_back(static_cast<uint8_t>(len | 0x80));  // Mask bit set
    } else if (len <= 65535) {
        frame.push_back(126 | 0x80);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127 | 0x80);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    
    // Masking key (random 4 bytes)
    uint8_t mask[4] = { 0x12, 0x34, 0x56, 0x78 };  // Should be random
    frame.insert(frame.end(), mask, mask + 4);
    
    // Masked payload
    for (size_t i = 0; i < text.size(); i++) {
        frame.push_back(text[i] ^ mask[i % 4]);
    }
    
    // Send frame through native layer
    // nativeSend(nativeHandle_, frame.data(), frame.size());
    
    bufferedAmount_ -= std::min(bufferedAmount_, text.size());
}

void WebSocket::send(const uint8_t* data, size_t length) {
    if (readyState_ != ReadyState::OPEN) {
        onErrorOccurred("WebSocket is not open");
        return;
    }
    
    // Create binary frame (opcode=0x02)
    std::vector<uint8_t> frame;
    frame.push_back(0x82);  // FIN=1, opcode=2 (binary)
    
    // Same length encoding as text
    if (length <= 125) {
        frame.push_back(static_cast<uint8_t>(length | 0x80));
    } else if (length <= 65535) {
        frame.push_back(126 | 0x80);
        frame.push_back((length >> 8) & 0xFF);
        frame.push_back(length & 0xFF);
    } else {
        frame.push_back(127 | 0x80);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((length >> (i * 8)) & 0xFF);
        }
    }
    
    // Masking key
    uint8_t mask[4] = { 0x12, 0x34, 0x56, 0x78 };
    frame.insert(frame.end(), mask, mask + 4);
    
    // Masked payload
    for (size_t i = 0; i < length; i++) {
        frame.push_back(data[i] ^ mask[i % 4]);
    }
    
    // nativeSend(nativeHandle_, frame.data(), frame.size());
}

void WebSocket::close(uint16_t code, const std::string& reason) {
    if (readyState_ == ReadyState::CLOSING || readyState_ == ReadyState::CLOSED) {
        return;
    }
    
    readyState_ = ReadyState::CLOSING;
    
    // Send close frame (opcode=0x08)
    std::vector<uint8_t> frame;
    frame.push_back(0x88);  // FIN=1, opcode=8 (close)
    
    size_t payloadLen = 2 + reason.size();  // 2 bytes for code + reason
    frame.push_back(static_cast<uint8_t>(payloadLen | 0x80));
    
    // Masking key
    uint8_t mask[4] = { 0xAB, 0xCD, 0xEF, 0x01 };
    frame.insert(frame.end(), mask, mask + 4);
    
    // Close code (masked)
    frame.push_back((code >> 8) ^ mask[0]);
    frame.push_back((code & 0xFF) ^ mask[1]);
    
    // Reason (masked)
    for (size_t i = 0; i < reason.size(); i++) {
        frame.push_back(reason[i] ^ mask[(i + 2) % 4]);
    }
    
    // nativeSend(nativeHandle_, frame.data(), frame.size());
    
    // Will transition to CLOSED when close confirmation received
}

void WebSocket::flushPendingMessages() {
    while (!pendingMessages_.empty()) {
        const auto& msg = pendingMessages_.front();
        send(msg.data(), msg.size());
        pendingMessages_.pop();
    }
}

// =============================================================================
// Internal Callbacks (called from native layer)
// =============================================================================

void WebSocket::onDataReceived(const uint8_t* data, size_t length, bool isBinary) {
    MessageEvent event;
    
    if (isBinary) {
        // Create ArrayBuffer value
        // For now, store as undefined - real impl would create ArrayBuffer
        event.data = Value::undefined();
    } else {
        // Text message
        std::string text(reinterpret_cast<const char*>(data), length);
        // Create string object and wrap in Value
        auto* strObj = new Runtime::String(text);
        event.data = Value::object(strObj);
    }
    
    event.origin = url_;
    
    if (onMessage_) {
        onMessage_(event);
    }
}

void WebSocket::onConnected(const std::string& protocol, const std::string& extensions) {
    readyState_ = ReadyState::OPEN;
    protocol_ = protocol;
    extensions_ = extensions;
    
    // Flush any pending messages
    flushPendingMessages();
    
    if (onOpen_) {
        OpenEvent event;
        onOpen_(event);
    }
}

void WebSocket::onDisconnected(uint16_t code, const std::string& reason, bool wasClean) {
    readyState_ = ReadyState::CLOSED;
    
    if (onClose_) {
        CloseEvent event;
        event.code = code;
        event.reason = reason;
        event.wasClean = wasClean;
        onClose_(event);
    }
}

void WebSocket::onErrorOccurred(const std::string& message) {
    if (onError_) {
        ErrorEvent event;
        event.message = message;
        onError_(event);
    }
}

// =============================================================================
// WebSocket Server Implementation
// =============================================================================

WebSocketServer::WebSocketServer(uint16_t port) : port_(port) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    if (running_) return;
    
    // Create server socket
    // Bind to port
    // Listen for connections
    running_ = true;
}

void WebSocketServer::stop() {
    if (!running_) return;
    
    // Close all connections
    // Close server socket
    running_ = false;
}

// =============================================================================
// Builtin Functions
// =============================================================================

Value webSocketConstructor(void* ctx, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) {
        // Throw TypeError
        return Value::undefined();
    }
    
    std::string url = args[0].asString()->value();
    std::vector<std::string> protocols;
    
    // Parse protocols if provided
    if (args.size() > 1) {
        if (args[1].isString()) {
            protocols.push_back(args[1].asString()->value());
        } else if (args[1].isObject() && args[1].asObject()->isArray()) {
            // Extract protocols from array
            Runtime::Array* arr = static_cast<Runtime::Array*>(args[1].asObject());
            for (size_t i = 0; i < arr->length(); i++) {
                Value proto = arr->at(i);
                if (proto.isString()) {
                    protocols.push_back(proto.asString()->value());
                }
            }
        }
    }
    
    WebSocket* ws = new WebSocket(url, protocols);
    return Value::object(ws);
}

void initWebSocket() {
    // Register WebSocket constructor as global
    // globalThis.WebSocket = webSocketConstructor
}

} // namespace Zepra::Browser
