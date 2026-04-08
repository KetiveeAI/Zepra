// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file http_client.cpp
 * @brief HTTP client implementation with socket operations
 */

#include "http_client.hpp"
#include "ssl_context.hpp"
#include "cookie_manager.hpp"
#include "dns_resolver.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <fstream>
#include <mutex>
#include <algorithm>
#include <iostream>
#include <cstring>

namespace Zepra::Networking {

// =============================================================================
// Connection Pool Entry
// =============================================================================

struct PooledConnection {
    int socket = -1;
    std::unique_ptr<SSLContext::SSLSocket> sslSocket;
    std::string host;
    int port = 0;
    bool secure = false;
    std::chrono::steady_clock::time_point lastUsed;
    bool inUse = false;
    
    bool isValid() const {
        if (socket < 0) return false;
        // Check if connection still alive (poll for errors)
        struct pollfd pfd = {socket, POLLOUT, 0};
        return poll(&pfd, 1, 0) >= 0 && !(pfd.revents & (POLLERR | POLLHUP));
    }
    
    void close() {
        if (sslSocket) {
            sslSocket->close();
            sslSocket.reset();
        }
        if (socket >= 0) {
            ::close(socket);
            socket = -1;
        }
    }
};

// =============================================================================
// Connection Pool
// =============================================================================

class ConnectionPool {
public:
    static constexpr size_t MAX_CONNECTIONS = 16;
    static constexpr size_t MAX_PER_HOST = 6;
    static constexpr int IDLE_TIMEOUT_SEC = 30;
    
    PooledConnection* acquire(const std::string& host, int port, bool secure) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Find existing idle connection
        for (auto& conn : connections_) {
            if (!conn.inUse && conn.host == host && conn.port == port && 
                conn.secure == secure && conn.isValid()) {
                conn.inUse = true;
                conn.lastUsed = std::chrono::steady_clock::now();
                return &conn;
            }
        }
        
        // Create new if under limit
        size_t hostCount = 0;
        for (const auto& conn : connections_) {
            if (conn.host == host && conn.port == port) hostCount++;
        }
        
        if (connections_.size() < MAX_CONNECTIONS && hostCount < MAX_PER_HOST) {
            connections_.push_back({});
            auto& conn = connections_.back();
            conn.host = host;
            conn.port = port;
            conn.secure = secure;
            conn.inUse = true;
            conn.lastUsed = std::chrono::steady_clock::now();
            return &conn;
        }
        
        return nullptr;  // Pool exhausted
    }
    
    void release(PooledConnection* conn) {
        if (!conn) return;
        std::lock_guard<std::mutex> lock(mutex_);
        conn->inUse = false;
        conn->lastUsed = std::chrono::steady_clock::now();
    }
    
    void evictIdle() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                [&](PooledConnection& conn) {
                    if (!conn.inUse) {
                        auto idle = std::chrono::duration_cast<std::chrono::seconds>(
                            now - conn.lastUsed).count();
                        if (idle > IDLE_TIMEOUT_SEC || !conn.isValid()) {
                            conn.close();
                            return true;
                        }
                    }
                    return false;
                }),
            connections_.end());
    }
    
private:
    std::mutex mutex_;
    std::vector<PooledConnection> connections_;
};

static ConnectionPool& getConnectionPool() {
    static ConnectionPool pool;
    return pool;
}

// =============================================================================
// Internal Implementation
// =============================================================================

class HttpClientImpl {
public:
    HttpClientImpl(HttpClientConfig& config) : config_(config) {}
    
    HttpResponse send(const HttpRequest& request) {
        return sendWithRetry(request, 0);
    }
    
    HttpResponse sendWithRetry(const HttpRequest& request, int attempt) {
        HttpResponse response;
        
        auto startTime = std::chrono::steady_clock::now();
        
        std::string host = request.host();
        int port = request.port();
        bool secure = request.isSecure();
        
        // Resolve DNS
        DnsResult dns = getDnsResolver().resolve(host);
        if (!dns.success || dns.addresses.empty()) {
            response.setError("DNS resolution failed: " + dns.error);
            return response;
        }
        
        // Loop over addresses to connect robustly
        int sock = -1;
        bool connected = false;
        
        for (const auto& addrStr : dns.addresses) {
            bool isIPv6 = addrStr.find(':') != std::string::npos;
            sock = socket(isIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
            if (sock < 0) continue;
            
            // Set socket options
            struct timeval timeout;
            timeout.tv_sec = config_.connectTimeoutMs / 1000;
            timeout.tv_usec = (config_.connectTimeoutMs % 1000) * 1000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
            
            // Connect
            if (isIPv6) {
                struct sockaddr_in6 errAddr;
                memset(&errAddr, 0, sizeof(errAddr));
                errAddr.sin6_family = AF_INET6;
                errAddr.sin6_port = htons(port);
                inet_pton(AF_INET6, addrStr.c_str(), &errAddr.sin6_addr);
                if (connect(sock, (struct sockaddr*)&errAddr, sizeof(errAddr)) == 0) {
                    connected = true; break;
                }
            } else {
                struct sockaddr_in errAddr;
                memset(&errAddr, 0, sizeof(errAddr));
                errAddr.sin_family = AF_INET;
                errAddr.sin_port = htons(port);
                inet_pton(AF_INET, addrStr.c_str(), &errAddr.sin_addr);
                if (connect(sock, (struct sockaddr*)&errAddr, sizeof(errAddr)) == 0) {
                    connected = true; break;
                }
            }
            close(sock);
            sock = -1;
        }
        
        if (!connected || sock < 0) {
            response.setError("Connection failed to all resolved IPs");
            return response;
        }
        
        // SSL handshake if HTTPS
        std::unique_ptr<SSLContext::SSLSocket> sslSocket;
        if (secure) {
            // Always create SSL socket for HTTPS - verifySsl only affects cert verification
            SSLContext& ctx = getSSLContext();
            ctx.setVerifyCertificate(config_.verifySsl);
            
            sslSocket = ctx.createSocket(sock);
            if (!sslSocket) {
                close(sock);
                response.setError("Failed to create SSL socket");
                return response;
            }
            
            if (!sslSocket->connect(host)) {
                close(sock);
                response.setError("SSL handshake failed");
                return response;
            }
        }
        
        // Build HTTP request
        std::ostringstream reqStream;
        std::string pathAndQuery = request.path();
        if (pathAndQuery.empty()) pathAndQuery = "/";  // Use / for empty path
        if (!request.query().empty()) {
            pathAndQuery += "?" + request.query();
        }
        
        reqStream << request.methodString() << " " << pathAndQuery << " HTTP/1.1\r\n";
        reqStream << "Host: " << host;
        if ((secure && port != 443) || (!secure && port != 80)) {
            reqStream << ":" << port;
        }
        reqStream << "\r\n";
        
        // Add headers
        for (const auto& [name, value] : request.headers()) {
            reqStream << name << ": " << value << "\r\n";
        }
        
        // Add default headers if not present
        if (!request.hasHeader("User-Agent")) {
            reqStream << "User-Agent: " << config_.userAgent << "\r\n";
        }
        if (!request.hasHeader("Accept")) {
            reqStream << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
        }
        if (!request.hasHeader("Accept-Language")) {
            reqStream << "Accept-Language: en-US,en;q=0.5\r\n";
        }
        if (!request.hasHeader("Accept-Encoding")) {
            reqStream << "Accept-Encoding: identity\r\n";  // No compression for now
        }
        if (!request.hasHeader("Connection")) {
            reqStream << "Connection: keep-alive\r\n";
        }
        
        // Add cookies
        if (config_.useCookies) {
            std::string cookies = getCookieManager().getCookiesForUrl(request.url(), secure);
            if (!cookies.empty()) {
                reqStream << "Cookie: " << cookies << "\r\n";
            }
        }
        
        // Add body
        if (!request.body().empty()) {
            reqStream << "Content-Length: " << request.body().size() << "\r\n";
        }
        
        reqStream << "\r\n";
        
        std::string requestStr = reqStream.str();
        
        // Send request
        const char* data = requestStr.c_str();
        size_t len = requestStr.length();
        ssize_t sent = 0;
        
        if (sslSocket) {
            sent = sslSocket->write(data, len);
        } else {
            sent = ::send(sock, data, len, 0);
        }
        
        if (sent < 0) {
            std::cerr << "[HTTPClient] Send failed!" << std::endl;
            if (sslSocket) sslSocket->close();
            close(sock);
            response.setError("Failed to send request");
            return response;
        }
        
        // Send body
        if (!request.body().empty()) {
            if (sslSocket) {
                sslSocket->write(request.body().data(), request.body().size());
            } else {
                ::send(sock, request.body().data(), request.body().size(), 0);
            }
        }
        
        // Read response with proper waiting
        std::vector<char> buffer(8192);
        std::string rawResponse;
        int readTimeoutMs = config_.readTimeoutMs > 0 ? config_.readTimeoutMs : 30000;
        auto startRead = std::chrono::steady_clock::now();
        
        while (true) {
            // Check if we've exceeded timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startRead).count();
            if (elapsed > readTimeoutMs) {
                std::cerr << "[HTTPClient] Read timeout after " << elapsed << "ms" << std::endl;
                break;
            }
            
            // Use poll to wait for data (with timeout)
            struct pollfd pfd;
            pfd.fd = sock;
            pfd.events = POLLIN;
            
            int pollResult = poll(&pfd, 1, 1000);  // Wait up to 1 second
            if (pollResult < 0) {
                std::cerr << "[HTTPClient] Poll error" << std::endl;
                break;
            }
            
            if (pollResult == 0) {
                // Timeout - check if we have data already
                if (!rawResponse.empty()) {
                    std::cout << "[HTTPClient] Read complete: " << rawResponse.size() << " bytes" << std::endl;
                    break;
                }
                continue;  // Keep waiting
            }
            
            // Data available
            ssize_t received;
            if (sslSocket) {
                received = sslSocket->read(buffer.data(), buffer.size());
            } else {
                received = ::recv(sock, buffer.data(), buffer.size(), 0);
            }
            
            if (received < 0) break;
            if (received == 0) break;  // Connection closed
            
            rawResponse.append(buffer.data(), received);
        }
        
        // Close connection
        if (sslSocket) sslSocket->close();
        close(sock);
        
        // Parse response
        parseResponse(rawResponse, response);
        
        // Handle cookies
        if (config_.useCookies) {
            for (const auto& setCookie : response.setCookieHeaders()) {
                getCookieManager().setCookie(setCookie, request.url());
            }
        }
        
        // Handle redirects
        if (config_.followRedirects && response.isRedirect() && 
            !response.location().empty()) {
            static int redirectCount = 0;
            if (redirectCount < config_.maxRedirects) {
                redirectCount++;
                HttpRequest redirectReq(HttpMethod::GET, response.location());
                return send(redirectReq);
            }
        }
        
        response.setUrl(request.url());
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime);
        response.setTimingMs(static_cast<double>(duration.count()));
        
        return response;
    }
    
private:
    void parseResponse(const std::string& raw, HttpResponse& response) {
        size_t headerEnd = raw.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            response.setError("Invalid response: no header/body separator");
            return;
        }
        
        std::string headers = raw.substr(0, headerEnd);
        std::string body = raw.substr(headerEnd + 4);
        
        // Parse status line
        size_t firstLine = headers.find("\r\n");
        std::string statusLine = headers.substr(0, firstLine);
        
        // HTTP/1.1 200 OK
        size_t space1 = statusLine.find(' ');
        size_t space2 = statusLine.find(' ', space1 + 1);
        
        if (space1 != std::string::npos) {
            try {
                std::string codeStr = statusLine.substr(space1 + 1, 
                    (space2 != std::string::npos) ? space2 - space1 - 1 : std::string::npos);
                response.setStatusCode(std::stoi(codeStr));
            } catch (...) {
                response.setStatusCode(0);
            }
            
            if (space2 != std::string::npos) {
                response.setStatusMessage(statusLine.substr(space2 + 1));
            }
        }
        
        // Parse headers
        std::istringstream headerStream(headers.substr(firstLine + 2));
        std::string line;
        std::string lastHeaderName;
        
        while (std::getline(headerStream, line)) {
            if (line.empty() || line == "\r") continue;
            
            // Remove trailing \r
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                
                // Trim leading whitespace from value
                size_t start = value.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    value = value.substr(start);
                }
                
                // Handle multiple Set-Cookie headers
                std::string nameLower = name;
                std::transform(nameLower.begin(), nameLower.end(), 
                               nameLower.begin(), ::tolower);
                
                if (nameLower == "set-cookie" && response.hasHeader("Set-Cookie")) {
                    std::string existing = response.header("Set-Cookie");
                    response.setHeader(name, existing + "\n" + value);
                } else {
                    response.setHeader(name, value);
                }
                
                lastHeaderName = name;
            }
        }
        
        // Handle chunked transfer encoding
        std::string te = response.header("Transfer-Encoding");
        std::transform(te.begin(), te.end(), te.begin(), ::tolower);
        
        if (te.find("chunked") != std::string::npos) {
            std::vector<uint8_t> decodedBody;
            decodeChunked(body, decodedBody);
            response.setBody(decodedBody);
        } else {
            response.setBody(std::vector<uint8_t>(body.begin(), body.end()));
        }
    }
    
    void decodeChunked(const std::string& chunked, std::vector<uint8_t>& out) {
        size_t pos = 0;
        while (pos < chunked.size()) {
            // Read chunk size
            size_t lineEnd = chunked.find("\r\n", pos);
            if (lineEnd == std::string::npos) break;
            
            std::string sizeStr = chunked.substr(pos, lineEnd - pos);
            size_t chunkSize = 0;
            try {
                chunkSize = std::stoul(sizeStr, nullptr, 16);
            } catch (...) {
                break;
            }
            
            if (chunkSize == 0) break;
            
            pos = lineEnd + 2;
            if (pos + chunkSize > chunked.size()) break;
            
            out.insert(out.end(), 
                       chunked.begin() + pos, 
                       chunked.begin() + pos + chunkSize);
            
            pos += chunkSize + 2;  // Skip chunk data and trailing \r\n
        }
    }
    
    HttpClientConfig& config_;
};

// =============================================================================
// HttpClient Implementation
// =============================================================================

HttpClient::HttpClient() : impl_(std::make_unique<HttpClientImpl>(config_)) {}

HttpClient::HttpClient(const HttpClientConfig& config) 
    : config_(config), impl_(std::make_unique<HttpClientImpl>(config_)) {}

HttpClient::~HttpClient() = default;

HttpResponse HttpClient::send(const HttpRequest& request) {
    return impl_->send(request);
}

std::future<HttpResponse> HttpClient::sendAsync(const HttpRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return impl_->send(request);
    });
}

HttpResponse HttpClient::get(const std::string& url) {
    HttpRequest request(HttpMethod::GET, url);
    return send(request);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                               const std::string& contentType) {
    HttpRequest request(HttpMethod::POST, url);
    request.setBody(body);
    request.setContentType(contentType);
    return send(request);
}

bool HttpClient::download(const std::string& url, const std::string& filePath,
                           ProgressCallback progress) {
    HttpRequest request(HttpMethod::GET, url);
    HttpResponse response = send(request);
    
    if (!response.isSuccess()) {
        return false;
    }
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    const auto& body = response.body();
    file.write(reinterpret_cast<const char*>(body.data()), body.size());
    
    if (progress) {
        progress(body.size(), body.size());
    }
    
    return true;
}

void HttpClient::cancelAll() {
    // TODO: Implement request cancellation
}

} // namespace Zepra::Networking
