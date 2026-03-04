/**
 * @file WebSocketAPI.h
 * @brief WebSocket API for ZepraScript browser integration
 * 
 * Implements the W3C WebSocket specification for real-time
 * bidirectional communication between browser and server.
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include "event_system.hpp"
#include <string>
#include <vector>
#include <queue>
#include <functional>
#include <cstdint>

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

// =============================================================================
// WebSocket Events
// =============================================================================

/**
 * @brief Event fired when WebSocket connection opens
 */
struct OpenEvent {
    std::string type = "open";
};

/**
 * @brief Event fired when WebSocket receives a message
 */
struct MessageEvent {
    std::string type = "message";
    Value data;           // String or ArrayBuffer
    std::string origin;
    std::string lastEventId;
    
    bool isBinary() const { return !data.isString(); }
};

/**
 * @brief Event fired when WebSocket connection closes
 */
struct CloseEvent {
    std::string type = "close";
    uint16_t code = 1000;
    std::string reason;
    bool wasClean = true;
};

/**
 * @brief Event fired on WebSocket error
 */
struct ErrorEvent {
    std::string type = "error";
    std::string message;
};

// =============================================================================
// WebSocket Class
// =============================================================================

/**
 * @brief WebSocket connection for real-time communication
 * 
 * Implements the WebSocket interface as defined by W3C.
 * Supports text and binary messaging.
 */
class WebSocket : public Object {
public:
    /**
     * @brief Connection ready states
     */
    enum class ReadyState : uint8_t {
        CONNECTING = 0,  // Connection in progress
        OPEN = 1,        // Connected and ready
        CLOSING = 2,     // Close handshake in progress
        CLOSED = 3       // Connection closed
    };
    
    /**
     * @brief Binary data type preference
     */
    enum class BinaryType {
        Blob,
        ArrayBuffer
    };
    
    /**
     * @brief Create WebSocket connection
     * @param url WebSocket URL (ws:// or wss://)
     * @param protocols Optional sub-protocols
     */
    WebSocket(const std::string& url, 
              const std::vector<std::string>& protocols = {});
    
    ~WebSocket();
    
    // =========================================================================
    // Properties
    // =========================================================================
    
    /** @brief Get current connection state */
    ReadyState readyState() const { return readyState_; }
    
    /** @brief Get number of bytes queued for sending */
    size_t bufferedAmount() const { return bufferedAmount_; }
    
    /** @brief Get the URL */
    const std::string& url() const { return url_; }
    
    /** @brief Get negotiated protocol */
    const std::string& protocol() const { return protocol_; }
    
    /** @brief Get/set binary type */
    BinaryType binaryType() const { return binaryType_; }
    void setBinaryType(BinaryType type) { binaryType_ = type; }
    
    /** @brief Get extensions */
    const std::string& extensions() const { return extensions_; }
    
    // =========================================================================
    // Methods
    // =========================================================================
    
    /**
     * @brief Send data over the connection
     * @param data String, ArrayBuffer, or Blob
     */
    void send(const Value& data);
    
    /**
     * @brief Send text message
     */
    void send(const std::string& text);
    
    /**
     * @brief Send binary data
     */
    void send(const uint8_t* data, size_t length);
    
    /**
     * @brief Close the connection
     * @param code Close status code (default 1000)
     * @param reason Close reason string
     */
    void close(uint16_t code = 1000, const std::string& reason = "");
    
    // =========================================================================
    // Event Handlers
    // =========================================================================
    
    using OpenHandler = std::function<void(const OpenEvent&)>;
    using MessageHandler = std::function<void(const MessageEvent&)>;
    using CloseHandler = std::function<void(const CloseEvent&)>;
    using ErrorHandler = std::function<void(const ErrorEvent&)>;
    
    void setOnOpen(OpenHandler handler) { onOpen_ = std::move(handler); }
    void setOnMessage(MessageHandler handler) { onMessage_ = std::move(handler); }
    void setOnClose(CloseHandler handler) { onClose_ = std::move(handler); }
    void setOnError(ErrorHandler handler) { onError_ = std::move(handler); }
    
    // =========================================================================
    // Internal (for platform integration)
    // =========================================================================
    
    /** @brief Handle incoming data from native layer */
    void onDataReceived(const uint8_t* data, size_t length, bool isBinary);
    
    /** @brief Handle connection established */
    void onConnected(const std::string& protocol, const std::string& extensions);
    
    /** @brief Handle connection closed */
    void onDisconnected(uint16_t code, const std::string& reason, bool wasClean);
    
    /** @brief Handle error */
    void onErrorOccurred(const std::string& message);
    
private:
    std::string url_;
    std::vector<std::string> protocols_;
    std::string protocol_;
    std::string extensions_;
    
    ReadyState readyState_ = ReadyState::CONNECTING;
    BinaryType binaryType_ = BinaryType::Blob;
    size_t bufferedAmount_ = 0;
    
    // Event handlers
    OpenHandler onOpen_;
    MessageHandler onMessage_;
    CloseHandler onClose_;
    ErrorHandler onError_;
    
    // Pending messages (queued while connecting)
    std::queue<std::vector<uint8_t>> pendingMessages_;
    
    // Platform-specific connection handle
    void* nativeHandle_ = nullptr;
    
    void connectInternal();
    void flushPendingMessages();
};

// =============================================================================
// WebSocket Server (for testing/local use)
// =============================================================================

/**
 * @brief Simple WebSocket server for local testing
 */
class WebSocketServer {
public:
    WebSocketServer(uint16_t port);
    ~WebSocketServer();
    
    void start();
    void stop();
    
    bool isRunning() const { return running_; }
    uint16_t port() const { return port_; }
    
    using ConnectionHandler = std::function<void(WebSocket*)>;
    void setOnConnection(ConnectionHandler handler) { onConnection_ = std::move(handler); }
    
private:
    uint16_t port_;
    bool running_ = false;
    void* serverHandle_ = nullptr;
    ConnectionHandler onConnection_;
};

// =============================================================================
// Builtin Functions
// =============================================================================

/**
 * @brief Register WebSocket constructor as global
 */
void initWebSocket();

/**
 * @brief WebSocket constructor builtin
 */
Value webSocketConstructor(void* ctx, const std::vector<Value>& args);

} // namespace Zepra::Browser
