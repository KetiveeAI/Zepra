// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file client.cpp
 * @brief HTTP/HTTPS client implementation
 * 
 * Uses raw sockets for HTTP/1.1 communication.
 * For HTTPS, uses OpenSSL via the networking/SSLContext wrapper.
 */

#include "nxhttp.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <thread>
#include <chrono>

// SSL/TLS Support via networking library
#include "ssl_context.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define SOCKET_INVALID INVALID_SOCKET
#define SOCKET_ERROR_VAL SOCKET_ERROR
#define close_socket closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
typedef int socket_t;
#define SOCKET_INVALID (-1)
#define SOCKET_ERROR_VAL (-1)
#define close_socket close
#endif

// ============================================================================
// Request Implementation
// ============================================================================

struct NxHttpRequest {
    NxHttpMethod method;
    NxUrl* url;
    NxHttpHeaders* headers;
    std::vector<uint8_t> body;
    int timeout_ms;
    bool follow_redirects;
    int max_redirects;
};

extern "C" {

const char* nx_http_error_string(NxHttpError err) {
    switch (err) {
        case NX_HTTP_OK: return "OK";
        case NX_HTTP_ERROR: return "General error";
        case NX_HTTP_ERROR_NOMEM: return "Out of memory";
        case NX_HTTP_ERROR_INVALID_URL: return "Invalid URL";
        case NX_HTTP_ERROR_DNS: return "DNS resolution failed";
        case NX_HTTP_ERROR_CONNECT: return "Connection failed";
        case NX_HTTP_ERROR_TIMEOUT: return "Request timeout";
        case NX_HTTP_ERROR_SSL: return "SSL/TLS error";
        case NX_HTTP_ERROR_SEND: return "Send failed";
        case NX_HTTP_ERROR_RECV: return "Receive failed";
        case NX_HTTP_ERROR_PARSE: return "Response parse error";
        case NX_HTTP_ERROR_TOO_MANY_REDIRECTS: return "Too many redirects";
        default: return "Unknown error";
    }
}

NxHttpRequest* nx_http_request_create(NxHttpMethod method, const char* url) {
    NxHttpRequest* req = new NxHttpRequest();
    req->method = method;
    req->url = nx_url_parse(url);
    req->headers = nx_http_headers_create();
    req->timeout_ms = 30000;
    req->follow_redirects = true;
    req->max_redirects = 10;
    
    if (!req->url) {
        delete req;
        return nullptr;
    }
    
    // Set default headers
    nx_http_headers_set(req->headers, "Host", req->url->host);
    nx_http_headers_set(req->headers, "User-Agent", "NxHTTP/1.0");
    nx_http_headers_set(req->headers, "Accept", "*/*");
    nx_http_headers_set(req->headers, "Connection", "close");
    
    return req;
}

void nx_http_request_free(NxHttpRequest* req) {
    if (req) {
        nx_url_free(req->url);
        nx_http_headers_free(req->headers);
        delete req;
    }
}

void nx_http_request_set_header(NxHttpRequest* req, const char* name, const char* value) {
    if (req) nx_http_headers_set(req->headers, name, value);
}

void nx_http_request_set_body(NxHttpRequest* req, const void* data, size_t len) {
    if (req && data && len > 0) {
        req->body.assign(static_cast<const uint8_t*>(data), 
                         static_cast<const uint8_t*>(data) + len);
        char len_str[32];
        snprintf(len_str, sizeof(len_str), "%zu", len);
        nx_http_headers_set(req->headers, "Content-Length", len_str);
    }
}

void nx_http_request_set_body_string(NxHttpRequest* req, const char* body) {
    if (body) {
        nx_http_request_set_body(req, body, strlen(body));
    }
}

void nx_http_request_set_timeout(NxHttpRequest* req, int timeout_ms) {
    if (req) req->timeout_ms = timeout_ms;
}

void nx_http_request_set_follow_redirects(NxHttpRequest* req, bool follow, int max_redirects) {
    if (req) {
        req->follow_redirects = follow;
        req->max_redirects = max_redirects;
    }
}

} // extern "C"

// ============================================================================
// Response Implementation
// ============================================================================

struct NxHttpResponse {
    int status;
    std::string status_text;
    NxHttpHeaders* headers;
    std::vector<uint8_t> body;
    std::string body_string_cache;
};

extern "C" {

int nx_http_response_status(const NxHttpResponse* res) {
    return res ? res->status : 0;
}

const char* nx_http_response_status_text(const NxHttpResponse* res) {
    return res ? res->status_text.c_str() : "";
}

const char* nx_http_response_header(const NxHttpResponse* res, const char* name) {
    return res ? nx_http_headers_get(res->headers, name) : nullptr;
}

const NxHttpHeaders* nx_http_response_headers(const NxHttpResponse* res) {
    return res ? res->headers : nullptr;
}

const uint8_t* nx_http_response_body(const NxHttpResponse* res) {
    return res && !res->body.empty() ? res->body.data() : nullptr;
}

size_t nx_http_response_body_len(const NxHttpResponse* res) {
    return res ? res->body.size() : 0;
}

const char* nx_http_response_body_string(const NxHttpResponse* res) {
    if (!res) return "";
    NxHttpResponse* mutable_res = const_cast<NxHttpResponse*>(res);
    if (mutable_res->body_string_cache.empty() && !res->body.empty()) {
        mutable_res->body_string_cache.assign(
            reinterpret_cast<const char*>(res->body.data()), res->body.size());
    }
    return mutable_res->body_string_cache.c_str();
}

void nx_http_response_free(NxHttpResponse* res) {
    if (res) {
        nx_http_headers_free(res->headers);
        delete res;
    }
}

} // extern "C"

// ============================================================================
// Client Implementation
// ============================================================================

struct NxHttpClient {
    NxHttpClientConfig config;
    NxHttpCookieJar* cookie_jar;
};

static const char* method_to_string(NxHttpMethod method) {
    switch (method) {
        case NX_HTTP_GET: return "GET";
        case NX_HTTP_POST: return "POST";
        case NX_HTTP_PUT: return "PUT";
        case NX_HTTP_DELETE: return "DELETE";
        case NX_HTTP_PATCH: return "PATCH";
        case NX_HTTP_HEAD: return "HEAD";
        case NX_HTTP_OPTIONS: return "OPTIONS";
        default: return "GET";
    }
}

static socket_t connect_to_host(const char* host, int port, int timeout_ms, NxHttpError* error) {
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    int ret = getaddrinfo(host, port_str, &hints, &result);
    if (ret != 0 || !result) {
        if (error) *error = NX_HTTP_ERROR_DNS;
        return SOCKET_INVALID;
    }
    
    socket_t sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == SOCKET_INVALID) {
        freeaddrinfo(result);
        if (error) *error = NX_HTTP_ERROR_CONNECT;
        return SOCKET_INVALID;
    }
    
    // Set timeout
#ifdef _WIN32
    DWORD tv = timeout_ms;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    
    if (connect(sock, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR_VAL) {
        close_socket(sock);
        freeaddrinfo(result);
        if (error) *error = NX_HTTP_ERROR_CONNECT;
        return SOCKET_INVALID;
    }
    
    freeaddrinfo(result);
    return sock;
}

static bool send_all(socket_t sock, const void* data, size_t len) {
    const char* ptr = static_cast<const char*>(data);
    while (len > 0) {
        int sent = send(sock, ptr, (int)len, 0);
        if (sent <= 0) return false;
        ptr += sent;
        len -= sent;
    }
    return true;
}

static NxHttpResponse* parse_response(socket_t sock, NxHttpError* error) {
    NxHttpResponse* res = new NxHttpResponse();
    res->headers = nx_http_headers_create();
    res->status = 0;
    
    std::vector<char> buffer;
    char chunk[4096];
    size_t header_end = std::string::npos;
    
    // Read headers
    while (header_end == std::string::npos) {
        int n = recv(sock, chunk, sizeof(chunk), 0);
        if (n <= 0) break;
        
        size_t old_size = buffer.size();
        buffer.insert(buffer.end(), chunk, chunk + n);
        
        // Look for \r\n\r\n
        for (size_t i = old_size > 3 ? old_size - 3 : 0; i + 3 < buffer.size(); i++) {
            if (buffer[i] == '\r' && buffer[i+1] == '\n' && 
                buffer[i+2] == '\r' && buffer[i+3] == '\n') {
                header_end = i + 4;
                break;
            }
        }
    }
    
    if (header_end == std::string::npos) {
        if (error) *error = NX_HTTP_ERROR_PARSE;
        delete res;
        return nullptr;
    }
    
    // Parse status line
    std::string headers_str(buffer.begin(), buffer.begin() + header_end);
    size_t status_end = headers_str.find("\r\n");
    if (status_end == std::string::npos) {
        if (error) *error = NX_HTTP_ERROR_PARSE;
        delete res;
        return nullptr;
    }
    
    std::string status_line = headers_str.substr(0, status_end);
    // HTTP/1.1 200 OK
    size_t sp1 = status_line.find(' ');
    if (sp1 != std::string::npos) {
        size_t sp2 = status_line.find(' ', sp1 + 1);
        res->status = atoi(status_line.c_str() + sp1 + 1);
        if (sp2 != std::string::npos) {
            res->status_text = status_line.substr(sp2 + 1);
        }
    }
    
    // Parse headers
    size_t pos = status_end + 2;
    while (pos < header_end - 2) {
        size_t line_end = headers_str.find("\r\n", pos);
        if (line_end == std::string::npos) break;
        
        std::string line = headers_str.substr(pos, line_end - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string name = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Trim leading whitespace from value
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                value = value.substr(1);
            }
            nx_http_headers_add(res->headers, name.c_str(), value.c_str());
        }
        pos = line_end + 2;
    }
    
    // Copy initial body data
    if (header_end < buffer.size()) {
        res->body.assign(buffer.begin() + header_end, buffer.end());
    }
    
    // Check for Content-Length
    const char* content_length = nx_http_headers_get(res->headers, "Content-Length");
    if (content_length) {
        size_t expected = atol(content_length);
        while (res->body.size() < expected) {
            int n = recv(sock, chunk, sizeof(chunk), 0);
            if (n <= 0) break;
            res->body.insert(res->body.end(), chunk, chunk + n);
        }
    } else {
        // Check for chunked encoding
        const char* transfer_encoding = nx_http_headers_get(res->headers, "Transfer-Encoding");
        if (transfer_encoding && strstr(transfer_encoding, "chunked")) {
            // TODO: Implement chunked decoding
            // For now, read until connection close
            while (true) {
                int n = recv(sock, chunk, sizeof(chunk), 0);
                if (n <= 0) break;
                res->body.insert(res->body.end(), chunk, chunk + n);
            }
        }
    }
    
    return res;
}

// =============================================================================
// SSL/TLS Response Parser (Uses SSLContext::SSLSocket)
// =============================================================================
static NxHttpResponse* parse_response_ssl(Zepra::Networking::SSLContext::SSLSocket* sslSock, NxHttpError* error) {
    NxHttpResponse* res = new NxHttpResponse();
    res->headers = nx_http_headers_create();
    res->status = 0;
    
    std::vector<char> buffer;
    char chunk[4096];
    size_t header_end = std::string::npos;
    int read_attempts = 0;
    const int max_attempts = 50;  // Timeout after 50 empty reads
    
    // Read headers via SSL
    while (header_end == std::string::npos && read_attempts < max_attempts) {
        int n = sslSock->read(chunk, sizeof(chunk));
        if (n < 0) {
            std::cerr << "[SSL Parse] Read error: " << n << std::endl;
            break;
        }
        if (n == 0) {
            read_attempts++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        read_attempts = 0;  // Reset on successful read
        
        size_t old_size = buffer.size();
        buffer.insert(buffer.end(), chunk, chunk + n);
        
        // Debug: Show first bytes received
        if (old_size == 0 && n > 50) {
            std::string preview(chunk, std::min(n, 100));
            std::cout << "[SSL Parse] First bytes: " << preview.substr(0, 60) << "..." << std::endl;
        }
        
        // Look for \r\n\r\n (end of headers)
        for (size_t i = old_size > 3 ? old_size - 3 : 0; i + 3 < buffer.size(); i++) {
            if (buffer[i] == '\r' && buffer[i+1] == '\n' && 
                buffer[i+2] == '\r' && buffer[i+3] == '\n') {
                header_end = i + 4;
                break;
            }
        }
    }
    
    if (header_end == std::string::npos) {
        std::cerr << "[SSL Parse] No headers found after " << buffer.size() << " bytes" << std::endl;
        if (!buffer.empty()) {
            std::string preview(buffer.begin(), buffer.begin() + std::min(buffer.size(), (size_t)100));
            std::cerr << "[SSL Parse] Buffer preview: " << preview << std::endl;
        }
        if (error) *error = NX_HTTP_ERROR_PARSE;
        delete res;
        return nullptr;
    }
    
    // Parse status line
    std::string headers_str(buffer.begin(), buffer.begin() + header_end);
    size_t status_end = headers_str.find("\r\n");
    if (status_end == std::string::npos) {
        if (error) *error = NX_HTTP_ERROR_PARSE;
        delete res;
        return nullptr;
    }
    
    std::string status_line = headers_str.substr(0, status_end);
    size_t sp1 = status_line.find(' ');
    if (sp1 != std::string::npos) {
        size_t sp2 = status_line.find(' ', sp1 + 1);
        res->status = atoi(status_line.c_str() + sp1 + 1);
        if (sp2 != std::string::npos) {
            res->status_text = status_line.substr(sp2 + 1);
        }
    }
    
    // Parse headers
    size_t pos = status_end + 2;
    while (pos < header_end - 2) {
        size_t line_end = headers_str.find("\r\n", pos);
        if (line_end == std::string::npos) break;
        
        std::string line = headers_str.substr(pos, line_end - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string name = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                value = value.substr(1);
            }
            nx_http_headers_add(res->headers, name.c_str(), value.c_str());
        }
        pos = line_end + 2;
    }
    
    // Copy initial body data
    if (header_end < buffer.size()) {
        res->body.assign(buffer.begin() + header_end, buffer.end());
    }
    
    // Read remaining body via SSL
    const char* content_length = nx_http_headers_get(res->headers, "Content-Length");
    if (content_length) {
        size_t expected = atol(content_length);
        while (res->body.size() < expected) {
            int n = sslSock->read(chunk, sizeof(chunk));
            if (n <= 0) break;
            res->body.insert(res->body.end(), chunk, chunk + n);
        }
    } else {
        const char* transfer_encoding = nx_http_headers_get(res->headers, "Transfer-Encoding");
        if (transfer_encoding && strstr(transfer_encoding, "chunked")) {
            while (true) {
                int n = sslSock->read(chunk, sizeof(chunk));
                if (n <= 0) break;
                res->body.insert(res->body.end(), chunk, chunk + n);
            }
        }
    }
    
    return res;
}

extern "C" {

NxHttpClient* nx_http_client_create(const NxHttpClientConfig* config) {
    NxHttpClient* client = new NxHttpClient();
    
    if (config) {
        client->config = *config;
    } else {
        client->config.connect_timeout_ms = 30000;
        client->config.read_timeout_ms = 30000;
        client->config.follow_redirects = true;
        client->config.max_redirects = 10;
        client->config.verify_ssl = true;
        client->config.user_agent = "NxHTTP/1.0";
    }
    client->cookie_jar = nullptr;
    
    return client;
}

void nx_http_client_free(NxHttpClient* client) {
    delete client;
}

NxHttpResponse* nx_http_client_send(NxHttpClient* client, NxHttpRequest* req, NxHttpError* error) {
    if (!client || !req || !req->url) {
        if (error) *error = NX_HTTP_ERROR;
        return nullptr;
    }
    
    bool is_https = nx_url_is_https(req->url);
    
    // Connect to host
    socket_t sock = connect_to_host(req->url->host, req->url->port, 
                                    client->config.connect_timeout_ms, error);
    if (sock == SOCKET_INVALID) {
        return nullptr;
    }
    
    // For HTTPS, establish TLS connection
    std::unique_ptr<Zepra::Networking::SSLContext::SSLSocket> sslSock;
    if (is_https) {
        std::cout << "[NXHTTP] Establishing TLS connection to " << req->url->host << std::endl;
        
        // Get global SSL context (auto-initializes)
        Zepra::Networking::SSLContext& sslCtx = Zepra::Networking::getSSLContext();
        
        // Set certificate verification based on client config
        sslCtx.setVerifyCertificate(client->config.verify_ssl);
        
        // Create SSL socket from raw socket
        sslSock = sslCtx.createSocket(sock);
        if (!sslSock) {
            std::cerr << "[NXHTTP] Failed to create SSL socket" << std::endl;
            close_socket(sock);
            if (error) *error = NX_HTTP_ERROR_SSL;
            return nullptr;
        }
        
        // Perform TLS handshake with hostname verification (SNI)
        if (!sslSock->connect(req->url->host)) {
            auto verifyResult = sslSock->getVerifyResult();
            std::cerr << "[NXHTTP] TLS handshake failed: " 
                      << Zepra::Networking::SSLContext::getLastError() << std::endl;
            sslSock->close();
            close_socket(sock);
            if (error) *error = NX_HTTP_ERROR_SSL;
            return nullptr;
        }
        
        // Log connection info
        auto connInfo = sslSock->getConnectionInfo();
        std::cout << "[NXHTTP] TLS connected: " << connInfo.cipherSuite 
                  << " (" << connInfo.cipherStrength << " bits)" << std::endl;
    }
    
    // Build request
    std::string request;
    request.reserve(1024);
    
    request += method_to_string(req->method);
    request += " ";
    request += req->url->path ? req->url->path : "/";
    if (req->url->query) {
        request += "?";
        request += req->url->query;
    }
    request += " HTTP/1.1\r\n";
    
    // Add headers
    for (size_t i = 0; i < nx_http_headers_count(req->headers); i++) {
        const char* name;
        const char* value;
        if (nx_http_headers_get_at(req->headers, i, &name, &value)) {
            request += name;
            request += ": ";
            request += value;
            request += "\r\n";
        }
    }
    request += "\r\n";
    
    // Send request
    bool sendOk = false;
    if (is_https && sslSock) {
        // Send via TLS
        sendOk = (sslSock->write(request.c_str(), request.size()) == (int)request.size());
    } else {
        // Send via plain socket
        sendOk = send_all(sock, request.c_str(), request.size());
    }
    
    if (!sendOk) {
        if (sslSock) sslSock->close();
        close_socket(sock);
        if (error) *error = NX_HTTP_ERROR_SEND;
        return nullptr;
    }
    
    // Send body if present
    if (!req->body.empty()) {
        if (is_https && sslSock) {
            if (sslSock->write(req->body.data(), req->body.size()) != (int)req->body.size()) {
                sslSock->close();
                close_socket(sock);
                if (error) *error = NX_HTTP_ERROR_SEND;
                return nullptr;
            }
        } else {
            if (!send_all(sock, req->body.data(), req->body.size())) {
                close_socket(sock);
                if (error) *error = NX_HTTP_ERROR_SEND;
                return nullptr;
            }
        }
    }
    
    // Receive response
    NxHttpResponse* res = nullptr;
    if (is_https && sslSock) {
        res = parse_response_ssl(sslSock.get(), error);
        sslSock->close();
    } else {
        res = parse_response(sock, error);
    }
    
    close_socket(sock);
    
    return res;
}

NxHttpResponse* nx_http_get(const char* url, NxHttpError* error) {
    NxHttpClient* client = nx_http_client_create(nullptr);
    NxHttpRequest* req = nx_http_request_create(NX_HTTP_GET, url);
    NxHttpResponse* res = nx_http_client_send(client, req, error);
    nx_http_request_free(req);
    nx_http_client_free(client);
    return res;
}

NxHttpResponse* nx_http_post(const char* url, const char* body, 
                             const char* content_type, NxHttpError* error) {
    NxHttpClient* client = nx_http_client_create(nullptr);
    NxHttpRequest* req = nx_http_request_create(NX_HTTP_POST, url);
    if (content_type) {
        nx_http_request_set_header(req, "Content-Type", content_type);
    }
    if (body) {
        nx_http_request_set_body_string(req, body);
    }
    NxHttpResponse* res = nx_http_client_send(client, req, error);
    nx_http_request_free(req);
    nx_http_client_free(client);
    return res;
}

void nx_http_client_set_cookie_jar(NxHttpClient* client, NxHttpCookieJar* jar) {
    if (client) client->cookie_jar = jar;
}

} // extern "C"

// ============================================================================
// Cookie Jar (Stub)
// ============================================================================

struct NxHttpCookieJar {
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> cookies;
};

extern "C" {

NxHttpCookieJar* nx_http_cookie_jar_create() {
    return new NxHttpCookieJar();
}

void nx_http_cookie_jar_free(NxHttpCookieJar* jar) {
    delete jar;
}

void nx_http_cookie_jar_set(NxHttpCookieJar* jar, const char* domain, 
                            const char* name, const char* value) {
    if (jar && domain && name) {
        jar->cookies[domain][name] = value ? value : "";
    }
}

const char* nx_http_cookie_jar_get(const NxHttpCookieJar* jar, const char* domain, 
                                   const char* name) {
    if (!jar || !domain || !name) return nullptr;
    auto dit = jar->cookies.find(domain);
    if (dit == jar->cookies.end()) return nullptr;
    auto nit = dit->second.find(name);
    if (nit == dit->second.end()) return nullptr;
    return nit->second.c_str();
}

} // extern "C"
