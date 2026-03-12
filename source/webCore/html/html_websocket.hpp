/**
 * @file html_websocket.hpp
 * @brief WebSocket API
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Zepra::WebCore {

/**
 * @brief WebSocket ready state
 */
enum class WebSocketReadyState {
    Connecting = 0,
    Open = 1,
    Closing = 2,
    Closed = 3
};

/**
 * @brief WebSocket binary type
 */
enum class BinaryType {
    Blob,
    ArrayBuffer
};

/**
 * @brief WebSocket message event
 */
struct MessageEvent {
    std::string data;
    std::vector<uint8_t> binaryData;
    bool isBinary = false;
    std::string origin;
    std::string lastEventId;
};

/**
 * @brief WebSocket close event
 */
struct CloseEvent {
    unsigned short code = 0;
    std::string reason;
    bool wasClean = false;
};

/**
 * @brief WebSocket
 */
class WebSocket {
public:
    WebSocket(const std::string& url, const std::vector<std::string>& protocols = {});
    ~WebSocket();
    
    // Properties
    std::string url() const { return url_; }
    WebSocketReadyState readyState() const { return readyState_; }
    unsigned long bufferedAmount() const { return bufferedAmount_; }
    std::string extensions() const { return extensions_; }
    std::string protocol() const { return protocol_; }
    
    BinaryType binaryType() const { return binaryType_; }
    void setBinaryType(BinaryType type) { binaryType_ = type; }
    
    // Methods
    void send(const std::string& data);
    void send(const std::vector<uint8_t>& data);
    void close(unsigned short code = 1000, const std::string& reason = "");
    
    // Events
    std::function<void()> onOpen;
    std::function<void(const MessageEvent&)> onMessage;
    std::function<void(const std::string&)> onError;
    std::function<void(const CloseEvent&)> onClose;
    
private:
    std::string url_;
    std::vector<std::string> protocols_;
    WebSocketReadyState readyState_ = WebSocketReadyState::Connecting;
    unsigned long bufferedAmount_ = 0;
    std::string extensions_;
    std::string protocol_;
    BinaryType binaryType_ = BinaryType::Blob;
    
    void connect();
};

} // namespace Zepra::WebCore
