/**
 * @file websocket_client.hpp
 * @brief WebSocket client implementation
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

namespace Zepra::Networking {

/**
 * @brief WebSocket close codes
 */
enum class CloseCode {
    Normal = 1000,
    GoingAway = 1001,
    ProtocolError = 1002,
    UnsupportedData = 1003,
    InvalidPayload = 1007,
    PolicyViolation = 1008,
    MessageTooBig = 1009,
    InternalError = 1011
};

/**
 * @brief WebSocket ready state
 */
enum class ReadyState {
    Connecting,
    Open,
    Closing,
    Closed
};

/**
 * @brief WebSocket message
 */
struct WebSocketMessage {
    enum Type { Text, Binary, Ping, Pong, Close };
    
    Type type = Text;
    std::vector<uint8_t> data;
    
    std::string text() const { return std::string(data.begin(), data.end()); }
};

/**
 * @brief WebSocket Client
 */
class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    
    /**
     * @brief Connect to WebSocket server
     */
    bool connect(const std::string& url);
    
    /**
     * @brief Close connection
     */
    void close(CloseCode code = CloseCode::Normal, const std::string& reason = "");
    
    /**
     * @brief Send text message
     */
    bool send(const std::string& message);
    
    /**
     * @brief Send binary message
     */
    bool send(const std::vector<uint8_t>& data);
    
    /**
     * @brief Send ping
     */
    void ping();
    
    /**
     * @brief Get ready state
     */
    ReadyState readyState() const { return state_; }
    
    /**
     * @brief Get URL
     */
    const std::string& url() const { return url_; }
    
    // Event handlers
    using OpenHandler = std::function<void()>;
    using MessageHandler = std::function<void(const WebSocketMessage&)>;
    using CloseHandler = std::function<void(CloseCode, const std::string&)>;
    using ErrorHandler = std::function<void(const std::string&)>;
    
    void setOnOpen(OpenHandler handler) { onOpen_ = std::move(handler); }
    void setOnMessage(MessageHandler handler) { onMessage_ = std::move(handler); }
    void setOnClose(CloseHandler handler) { onClose_ = std::move(handler); }
    void setOnError(ErrorHandler handler) { onError_ = std::move(handler); }
    
    /**
     * @brief Process pending events (call from event loop)
     */
    void poll();
    
private:
    bool doHandshake();
    bool sendFrame(uint8_t opcode, const void* data, size_t len, bool mask);
    bool receiveFrame(WebSocketMessage& msg);
    void processMessage(const WebSocketMessage& msg);
    
    std::string url_;
    ReadyState state_ = ReadyState::Closed;
    int socket_ = -1;
    void* ssl_ = nullptr;
    
    OpenHandler onOpen_;
    MessageHandler onMessage_;
    CloseHandler onClose_;
    ErrorHandler onError_;
    
    std::vector<uint8_t> receiveBuffer_;
};

} // namespace Zepra::Networking
