#pragma once

/**
 * @file fetch.hpp
 * @brief JavaScript Fetch API
 * 
 * Production-ready implementation with:
 * - Full Response body methods (text, json, blob, arrayBuffer)
 * - Request body and headers handling
 * - AbortController for cancellation
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include "../runtime/promise.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace Zepra::Runtime { class Context; class Promise; }

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;
using Runtime::Promise;

/**
 * @brief HTTP Headers
 */
class Headers : public Object {
public:
    Headers() : Object(Runtime::ObjectType::Ordinary) {}
    
    void append(const std::string& name, const std::string& value);
    void setHeader(const std::string& name, const std::string& value);
    std::string getHeader(const std::string& name) const;
    bool has(const std::string& name) const;
    void remove(const std::string& name);
    
    const std::unordered_map<std::string, std::string>& entries() const { return headers_; }
    
private:
    std::string normalizeName(const std::string& name) const;
    std::unordered_map<std::string, std::string> headers_;
};

/**
 * @brief HTTP Request
 */
class Request : public Object {
public:
    Request(const std::string& url, const std::string& method = "GET");
    
    const std::string& url() const { return url_; }
    const std::string& method() const { return method_; }
    Headers* headers() const { return headers_; }
    const std::string& body() const { return body_; }
    void setBody(const std::string& body) { body_ = body; }
    void setHeader(const std::string& name, const std::string& value);
    
    // Request mode
    const std::string& mode() const { return mode_; }
    const std::string& credentials() const { return credentials_; }
    const std::string& cache() const { return cache_; }
    
private:
    std::string url_;
    std::string method_;
    Headers* headers_;
    std::string body_;
    std::string mode_ = "cors";
    std::string credentials_ = "same-origin";
    std::string cache_ = "default";
};

/**
 * @brief HTTP Response
 */
class Response : public Object {
public:
    Response(int status = 200, const std::string& statusText = "OK");
    
    int status() const { return status_; }
    const std::string& statusText() const { return statusText_; }
    bool ok() const { return status_ >= 200 && status_ < 300; }
    Headers* headers() const { return headers_; }
    
    const std::string& body() const { return body_; }
    void setBody(const std::string& body) { body_ = body; }
    
    // Body consumption methods - each can only be called once
    Promise* text() const;
    Promise* json() const;
    Promise* blob() const;
    Promise* arrayBuffer() const;
    Promise* formData() const;
    
    // Clone (fails if body already consumed)
    Response* clone() const;
    
    bool bodyUsed() const { return bodyUsed_; }
    
private:
    // JSON parsing helpers
    Value parseJsonString(const std::string& json) const;
    void skipWhitespace(const std::string& json, size_t& pos) const;
    Value parseValue(const std::string& json, size_t& pos) const;
    Value parseString(const std::string& json, size_t& pos) const;
    Value parseNumber(const std::string& json, size_t& pos) const;
    Value parseBool(const std::string& json, size_t& pos) const;
    Value parseNull(const std::string& json, size_t& pos) const;
    Value parseArray(const std::string& json, size_t& pos) const;
    Value parseObject(const std::string& json, size_t& pos) const;
    
    int status_;
    std::string statusText_;
    Headers* headers_;
    std::string body_;
    mutable bool bodyUsed_ = false;
};

/**
 * @brief Fetch API implementation
 */
class FetchAPI {
public:
    /**
     * @brief Perform fetch request
     * @return Promise that resolves to Response
     */
    static Promise* fetch(const std::string& url, Request* request = nullptr);
    
    /**
     * @brief Perform fetch with callback (for internal use)
     */
    using FetchCallback = std::function<void(Response*)>;
    static void fetchAsync(const std::string& url, Request* request, FetchCallback callback);
    
    // Builtin
    static Value fetchBuiltin(Runtime::Context* ctx, const std::vector<Value>& args);
};

// Global initialization
void initFetchAPI();
void shutdownFetchAPI();

} // namespace Zepra::Browser
