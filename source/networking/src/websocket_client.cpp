/**
 * @file websocket_client.cpp
 * @brief WebSocket client implementation (RFC 6455)
 */

#include "networking/websocket_client.hpp"
#include "networking/ssl_context.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>

// For base64 encoding
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace Zepra::Networking {

namespace {

std::string base64Encode(const unsigned char* data, size_t len) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    BIO_write(bio, data, static_cast<int>(len));
    BIO_flush(bio);
    
    BUF_MEM* bufPtr;
    BIO_get_mem_ptr(bio, &bufPtr);
    
    std::string result(bufPtr->data, bufPtr->length);
    BIO_free_all(bio);
    return result;
}

std::string generateKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    unsigned char key[16];
    for (int i = 0; i < 16; ++i) {
        key[i] = static_cast<unsigned char>(dis(gen));
    }
    
    return base64Encode(key, 16);
}

std::string computeAccept(const std::string& key) {
    const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string concat = key + magic;
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(concat.c_str()), 
         concat.length(), hash);
    
    return base64Encode(hash, SHA_DIGEST_LENGTH);
}

} // anonymous namespace

// =============================================================================
// WebSocketClient Implementation
// =============================================================================

WebSocketClient::WebSocketClient() = default;

WebSocketClient::~WebSocketClient() {
    if (state_ != ReadyState::Closed) {
        close(CloseCode::GoingAway, "Destructor called");
    }
}

bool WebSocketClient::connect(const std::string& url) {
    url_ = url;
    state_ = ReadyState::Connecting;
    
    // Parse URL
    bool secure = false;
    std::string host;
    int port = 80;
    std::string path = "/";
    
    size_t schemeEnd = url.find("://");
    if (schemeEnd != std::string::npos) {
        std::string scheme = url.substr(0, schemeEnd);
        if (scheme == "wss") {
            secure = true;
            port = 443;
        }
        
        size_t hostStart = schemeEnd + 3;
        size_t hostEnd = url.find('/', hostStart);
        
        std::string hostPort = (hostEnd != std::string::npos) 
            ? url.substr(hostStart, hostEnd - hostStart)
            : url.substr(hostStart);
        
        size_t colonPos = hostPort.find(':');
        if (colonPos != std::string::npos) {
            host = hostPort.substr(0, colonPos);
            port = std::stoi(hostPort.substr(colonPos + 1));
        } else {
            host = hostPort;
        }
        
        if (hostEnd != std::string::npos) {
            path = url.substr(hostEnd);
        }
    }
    
    // Resolve host
    struct hostent* he = gethostbyname(host.c_str());
    if (!he) {
        if (onError_) onError_("DNS resolution failed");
        state_ = ReadyState::Closed;
        return false;
    }
    
    // Create socket
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        if (onError_) onError_("Failed to create socket");
        state_ = ReadyState::Closed;
        return false;
    }
    
    // Connect
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    memcpy(&serverAddr.sin_addr, he->h_addr, he->h_length);
    
    if (::connect(socket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        ::close(socket_);
        socket_ = -1;
        if (onError_) onError_("Connection failed");
        state_ = ReadyState::Closed;
        return false;
    }
    
    // SSL handshake if secure
    if (secure) {
        auto sslSocket = getSSLContext().createSocket(socket_);
        if (!sslSocket->connect(host)) {
            ::close(socket_);
            socket_ = -1;
            if (onError_) onError_("SSL handshake failed");
            state_ = ReadyState::Closed;
            return false;
        }
        ssl_ = sslSocket.release();
    }
    
    // WebSocket handshake
    std::string key = generateKey();
    
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << key << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";
    
    std::string reqStr = request.str();
    
    ssize_t sent;
    if (ssl_) {
        sent = static_cast<SSLContext::SSLSocket*>(ssl_)->write(reqStr.c_str(), reqStr.length());
    } else {
        sent = ::send(socket_, reqStr.c_str(), reqStr.length(), 0);
    }
    
    if (sent < 0) {
        if (onError_) onError_("Failed to send handshake");
        close();
        return false;
    }
    
    // Read response
    char buffer[4096];
    ssize_t received;
    if (ssl_) {
        received = static_cast<SSLContext::SSLSocket*>(ssl_)->read(buffer, sizeof(buffer) - 1);
    } else {
        received = ::recv(socket_, buffer, sizeof(buffer) - 1, 0);
    }
    
    if (received <= 0) {
        if (onError_) onError_("Failed to receive handshake response");
        close();
        return false;
    }
    
    buffer[received] = '\0';
    std::string response(buffer);
    
    // Verify response
    if (response.find("HTTP/1.1 101") == std::string::npos) {
        if (onError_) onError_("WebSocket handshake failed: not 101");
        close();
        return false;
    }
    
    std::string expectedAccept = computeAccept(key);
    if (response.find(expectedAccept) == std::string::npos) {
        if (onError_) onError_("WebSocket handshake failed: invalid accept");
        close();
        return false;
    }
    
    // Set non-blocking
    int flags = fcntl(socket_, F_GETFL, 0);
    fcntl(socket_, F_SETFL, flags | O_NONBLOCK);
    
    state_ = ReadyState::Open;
    if (onOpen_) onOpen_();
    
    return true;
}

void WebSocketClient::close(CloseCode code, const std::string& reason) {
    if (state_ == ReadyState::Closed) return;
    
    state_ = ReadyState::Closing;
    
    // Send close frame
    std::vector<uint8_t> payload;
    uint16_t codeNet = htons(static_cast<uint16_t>(code));
    payload.push_back(static_cast<uint8_t>((codeNet >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>(codeNet & 0xFF));
    payload.insert(payload.end(), reason.begin(), reason.end());
    
    sendFrame(0x08, payload.data(), payload.size(), true);
    
    // Close socket
    if (ssl_) {
        static_cast<SSLContext::SSLSocket*>(ssl_)->close();
        delete static_cast<SSLContext::SSLSocket*>(ssl_);
        ssl_ = nullptr;
    }
    
    if (socket_ >= 0) {
        ::close(socket_);
        socket_ = -1;
    }
    
    state_ = ReadyState::Closed;
    if (onClose_) onClose_(code, reason);
}

bool WebSocketClient::send(const std::string& message) {
    if (state_ != ReadyState::Open) return false;
    return sendFrame(0x01, message.c_str(), message.length(), true);
}

bool WebSocketClient::send(const std::vector<uint8_t>& data) {
    if (state_ != ReadyState::Open) return false;
    return sendFrame(0x02, data.data(), data.size(), true);
}

void WebSocketClient::ping() {
    if (state_ != ReadyState::Open) return;
    sendFrame(0x09, nullptr, 0, true);
}

bool WebSocketClient::sendFrame(uint8_t opcode, const void* data, size_t len, bool mask) {
    std::vector<uint8_t> frame;
    
    // FIN + opcode
    frame.push_back(0x80 | opcode);
    
    // Mask + length
    uint8_t maskBit = mask ? 0x80 : 0x00;
    
    if (len <= 125) {
        frame.push_back(maskBit | static_cast<uint8_t>(len));
    } else if (len <= 65535) {
        frame.push_back(maskBit | 126);
        frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(len & 0xFF));
    } else {
        frame.push_back(maskBit | 127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
        }
    }
    
    // Mask key
    uint8_t maskKey[4] = {0, 0, 0, 0};
    if (mask) {
        std::random_device rd;
        for (int i = 0; i < 4; ++i) {
            maskKey[i] = static_cast<uint8_t>(rd() & 0xFF);
            frame.push_back(maskKey[i]);
        }
    }
    
    // Payload (masked if client)
    const uint8_t* payload = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        if (mask) {
            frame.push_back(payload[i] ^ maskKey[i % 4]);
        } else {
            frame.push_back(payload[i]);
        }
    }
    
    ssize_t sent;
    if (ssl_) {
        sent = static_cast<SSLContext::SSLSocket*>(ssl_)->write(frame.data(), frame.size());
    } else {
        sent = ::send(socket_, frame.data(), frame.size(), 0);
    }
    
    return sent == static_cast<ssize_t>(frame.size());
}

void WebSocketClient::poll() {
    if (state_ != ReadyState::Open) return;
    
    struct pollfd pfd;
    pfd.fd = socket_;
    pfd.events = POLLIN;
    
    if (::poll(&pfd, 1, 0) <= 0) return;
    
    WebSocketMessage msg;
    if (receiveFrame(msg)) {
        processMessage(msg);
    }
}

bool WebSocketClient::receiveFrame(WebSocketMessage& msg) {
    uint8_t header[2];
    ssize_t received;
    
    if (ssl_) {
        received = static_cast<SSLContext::SSLSocket*>(ssl_)->read(header, 2);
    } else {
        received = ::recv(socket_, header, 2, 0);
    }
    
    if (received < 2) return false;
    
    uint8_t opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;
    
    if (payloadLen == 126) {
        uint8_t len16[2];
        if (ssl_) {
            static_cast<SSLContext::SSLSocket*>(ssl_)->read(len16, 2);
        } else {
            ::recv(socket_, len16, 2, 0);
        }
        payloadLen = (static_cast<uint64_t>(len16[0]) << 8) | len16[1];
    } else if (payloadLen == 127) {
        uint8_t len64[8];
        if (ssl_) {
            static_cast<SSLContext::SSLSocket*>(ssl_)->read(len64, 8);
        } else {
            ::recv(socket_, len64, 8, 0);
        }
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | len64[i];
        }
    }
    
    uint8_t maskKey[4] = {0, 0, 0, 0};
    if (masked) {
        if (ssl_) {
            static_cast<SSLContext::SSLSocket*>(ssl_)->read(maskKey, 4);
        } else {
            ::recv(socket_, maskKey, 4, 0);
        }
    }
    
    msg.data.resize(static_cast<size_t>(payloadLen));
    if (payloadLen > 0) {
        if (ssl_) {
            static_cast<SSLContext::SSLSocket*>(ssl_)->read(msg.data.data(), msg.data.size());
        } else {
            ::recv(socket_, msg.data.data(), msg.data.size(), 0);
        }
        
        if (masked) {
            for (size_t i = 0; i < msg.data.size(); ++i) {
                msg.data[i] ^= maskKey[i % 4];
            }
        }
    }
    
    switch (opcode) {
        case 0x01: msg.type = WebSocketMessage::Text; break;
        case 0x02: msg.type = WebSocketMessage::Binary; break;
        case 0x08: msg.type = WebSocketMessage::Close; break;
        case 0x09: msg.type = WebSocketMessage::Ping; break;
        case 0x0A: msg.type = WebSocketMessage::Pong; break;
        default: return false;
    }
    
    return true;
}

void WebSocketClient::processMessage(const WebSocketMessage& msg) {
    switch (msg.type) {
        case WebSocketMessage::Text:
        case WebSocketMessage::Binary:
            if (onMessage_) onMessage_(msg);
            break;
            
        case WebSocketMessage::Ping:
            sendFrame(0x0A, msg.data.data(), msg.data.size(), true);
            break;
            
        case WebSocketMessage::Pong:
            // Ignored
            break;
            
        case WebSocketMessage::Close:
            if (state_ != ReadyState::Closing) {
                CloseCode code = CloseCode::Normal;
                std::string reason;
                if (msg.data.size() >= 2) {
                    code = static_cast<CloseCode>(
                        (static_cast<uint16_t>(msg.data[0]) << 8) | msg.data[1]);
                    if (msg.data.size() > 2) {
                        reason = std::string(msg.data.begin() + 2, msg.data.end());
                    }
                }
                close(code, reason);
            }
            break;
    }
}

} // namespace Zepra::Networking
