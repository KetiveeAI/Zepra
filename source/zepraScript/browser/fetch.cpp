/**
 * @file fetch.cpp
 * @brief JavaScript Fetch API implementation with real HTTP
 * 
 * Production-ready Fetch API with:
 * - Full Response body methods (text, json, blob, arrayBuffer)
 * - Request body handling for POST/PUT/PATCH
 * - Custom headers support
 * - Timeout configuration
 * - AbortController integration
 */

#include "browser/fetch.hpp"
#include "builtins/json.hpp"
#include "runtime/objects/function.hpp"
#include "nxhttp.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdexcept>

namespace Zepra::Browser {

// =============================================================================
// Headers Implementation
// =============================================================================

void Headers::append(const std::string& name, const std::string& value) {
    std::string normalizedName = normalizeName(name);
    auto it = headers_.find(normalizedName);
    if (it != headers_.end()) {
        it->second += ", " + value;
    } else {
        headers_[normalizedName] = value;
    }
}

void Headers::setHeader(const std::string& name, const std::string& value) {
    headers_[normalizeName(name)] = value;
}

std::string Headers::getHeader(const std::string& name) const {
    auto it = headers_.find(normalizeName(name));
    return it != headers_.end() ? it->second : "";
}

bool Headers::has(const std::string& name) const {
    return headers_.find(normalizeName(name)) != headers_.end();
}

void Headers::remove(const std::string& name) {
    headers_.erase(normalizeName(name));
}

std::string Headers::normalizeName(const std::string& name) const {
    // Convert to lowercase for case-insensitive comparison
    std::string result = name;
    for (auto& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

// =============================================================================
// Request Implementation
// =============================================================================

Request::Request(const std::string& url, const std::string& method)
    : Object(Runtime::ObjectType::Ordinary)
    , url_(url)
    , method_(method)
    , headers_(new Headers()) {}

void Request::setHeader(const std::string& name, const std::string& value) {
    headers_->setHeader(name, value);
}

// =============================================================================
// Blob Implementation (for response.blob())
// =============================================================================

class Blob : public Runtime::Object {
public:
    Blob(std::vector<uint8_t>&& data, const std::string& type)
        : Object(Runtime::ObjectType::Ordinary)
        , data_(std::move(data))
        , type_(type) {}
    
    size_t size() const { return data_.size(); }
    const std::string& type() const { return type_; }
    const std::vector<uint8_t>& data() const { return data_; }
    
    /**
     * @brief Slice blob
     */
    Blob* slice(size_t start, size_t end, const std::string& contentType = "") {
        if (start > data_.size()) start = data_.size();
        if (end > data_.size()) end = data_.size();
        if (start > end) start = end;
        
        std::vector<uint8_t> sliceData(data_.begin() + start, data_.begin() + end);
        return new Blob(std::move(sliceData), contentType.empty() ? type_ : contentType);
    }
    
    /**
     * @brief Convert to text (UTF-8)
     */
    std::string text() const {
        return std::string(reinterpret_cast<const char*>(data_.data()), data_.size());
    }
    
private:
    std::vector<uint8_t> data_;
    std::string type_;
};

// =============================================================================
// ArrayBuffer Implementation (for response.arrayBuffer())
// =============================================================================

class ArrayBuffer : public Runtime::Object {
public:
    explicit ArrayBuffer(size_t byteLength)
        : Object(Runtime::ObjectType::Ordinary)
        , data_(byteLength, 0) {}
    
    explicit ArrayBuffer(std::vector<uint8_t>&& data)
        : Object(Runtime::ObjectType::Ordinary)
        , data_(std::move(data)) {}
    
    size_t byteLength() const { return data_.size(); }
    uint8_t* data() { return data_.data(); }
    const uint8_t* data() const { return data_.data(); }
    
    /**
     * @brief Slice buffer
     */
    ArrayBuffer* slice(size_t begin, size_t end) {
        if (begin > data_.size()) begin = data_.size();
        if (end > data_.size()) end = data_.size();
        if (begin > end) begin = end;
        
        std::vector<uint8_t> sliceData(data_.begin() + begin, data_.begin() + end);
        return new ArrayBuffer(std::move(sliceData));
    }
    
private:
    std::vector<uint8_t> data_;
};

// =============================================================================
// Response Implementation
// =============================================================================

Response::Response(int status, const std::string& statusText)
    : Object(Runtime::ObjectType::Ordinary)
    , status_(status)
    , statusText_(statusText)
    , headers_(new Headers()) {}

Promise* Response::text() const {
    Promise* p = new Promise();
    
    if (bodyUsed_) {
        p->reject(Value::string(new Runtime::String("Body has already been consumed")));
        return p;
    }
    
    bodyUsed_ = true;
    p->resolve(Value::string(new Runtime::String(body_)));
    return p;
}

Promise* Response::json() const {
    Promise* p = new Promise();
    
    if (bodyUsed_) {
        p->reject(Value::string(new Runtime::String("Body has already been consumed")));
        return p;
    }
    
    bodyUsed_ = true;
    
    // Parse the JSON body
    try {
        // Create a simple JSON parser for the response body
        // This duplicates some logic from JSONBuiltin but works standalone
        Value result = parseJsonString(body_);
        p->resolve(result);
    } catch (const std::exception& e) {
        p->reject(Value::string(new Runtime::String(std::string("JSON parse error: ") + e.what())));
    }
    
    return p;
}

Promise* Response::blob() const {
    Promise* p = new Promise();
    
    if (bodyUsed_) {
        p->reject(Value::string(new Runtime::String("Body has already been consumed")));
        return p;
    }
    
    bodyUsed_ = true;
    
    // Convert body to binary data
    std::vector<uint8_t> data(body_.begin(), body_.end());
    
    // Get content type from headers
    std::string contentType = headers_->getHeader("content-type");
    
    Blob* blob = new Blob(std::move(data), contentType);
    p->resolve(Value::object(blob));
    
    return p;
}

Promise* Response::arrayBuffer() const {
    Promise* p = new Promise();
    
    if (bodyUsed_) {
        p->reject(Value::string(new Runtime::String("Body has already been consumed")));
        return p;
    }
    
    bodyUsed_ = true;
    
    // Convert body to ArrayBuffer
    std::vector<uint8_t> data(body_.begin(), body_.end());
    
    ArrayBuffer* buffer = new ArrayBuffer(std::move(data));
    p->resolve(Value::object(buffer));
    
    return p;
}

Promise* Response::formData() const {
    Promise* p = new Promise();
    
    if (body_.empty()) {
        p->reject(Value::string(new Runtime::String("No body to parse")));
        return p;
    }
    
    // Check content type
    std::string contentType = headers_->getHeader("Content-Type");
    
    // Create FormData object
    Object* formData = new Object();
    
    if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        // Parse URL-encoded form data: key1=value1&key2=value2
        std::string decoded;
        size_t i = 0;
        while (i < body_.size()) {
            std::string key, value;
            
            // Read key until '='
            while (i < body_.size() && body_[i] != '=' && body_[i] != '&') {
                if (body_[i] == '+') {
                    key += ' ';
                } else if (body_[i] == '%' && i + 2 < body_.size()) {
                    // Decode %XX
                    char hex[3] = {body_[i+1], body_[i+2], 0};
                    key += static_cast<char>(std::strtol(hex, nullptr, 16));
                    i += 2;
                } else {
                    key += body_[i];
                }
                i++;
            }
            
            if (i < body_.size() && body_[i] == '=') {
                i++; // Skip '='
                
                // Read value until '&' or end
                while (i < body_.size() && body_[i] != '&') {
                    if (body_[i] == '+') {
                        value += ' ';
                    } else if (body_[i] == '%' && i + 2 < body_.size()) {
                        char hex[3] = {body_[i+1], body_[i+2], 0};
                        value += static_cast<char>(std::strtol(hex, nullptr, 16));
                        i += 2;
                    } else {
                        value += body_[i];
                    }
                    i++;
                }
            }
            
            if (!key.empty()) {
                formData->set(key, Value::string(new Runtime::String(value)));
            }
            
            if (i < body_.size() && body_[i] == '&') i++; // Skip '&'
        }
        
        p->resolve(Value::object(formData));
        
    } else if (contentType.find("multipart/form-data") != std::string::npos) {
        // Parse multipart form data
        // Find boundary from content type
        std::string boundary;
        size_t boundaryPos = contentType.find("boundary=");
        if (boundaryPos != std::string::npos) {
            boundary = "--" + contentType.substr(boundaryPos + 9);
            // Remove trailing semicolon or quotes if present
            size_t endPos = boundary.find_first_of(";\"");
            if (endPos != std::string::npos) {
                boundary = boundary.substr(0, endPos);
            }
        }
        
        if (!boundary.empty()) {
            std::string bodyStr = body_;
            size_t pos = 0;
            
            // Skip to first boundary
            pos = bodyStr.find(boundary, pos);
            if (pos != std::string::npos) pos += boundary.size();
            
            while (pos < bodyStr.size()) {
                // Skip CRLF after boundary
                if (bodyStr[pos] == '\r') pos++;
                if (bodyStr[pos] == '\n') pos++;
                
                // Check for end boundary
                if (pos + 1 < bodyStr.size() && bodyStr[pos] == '-' && bodyStr[pos+1] == '-') {
                    break;
                }
                
                // Parse headers until empty line
                std::string name;
                std::string filename;
                
                while (pos < bodyStr.size()) {
                    size_t lineEnd = bodyStr.find("\r\n", pos);
                    if (lineEnd == std::string::npos) break;
                    
                    std::string line = bodyStr.substr(pos, lineEnd - pos);
                    pos = lineEnd + 2;
                    
                    if (line.empty()) break; // End of headers
                    
                    // Parse Content-Disposition
                    if (line.find("Content-Disposition:") != std::string::npos) {
                        size_t namePos = line.find("name=\"");
                        if (namePos != std::string::npos) {
                            namePos += 6;
                            size_t nameEnd = line.find("\"", namePos);
                            name = line.substr(namePos, nameEnd - namePos);
                        }
                        size_t filePos = line.find("filename=\"");
                        if (filePos != std::string::npos) {
                            filePos += 10;
                            size_t fileEnd = line.find("\"", filePos);
                            filename = line.substr(filePos, fileEnd - filePos);
                        }
                    }
                }
                
                // Read content until next boundary
                size_t contentEnd = bodyStr.find(boundary, pos);
                if (contentEnd == std::string::npos) break;
                
                // Remove trailing CRLF from content
                if (contentEnd >= 2 && bodyStr[contentEnd-1] == '\n' && bodyStr[contentEnd-2] == '\r') {
                    contentEnd -= 2;
                }
                
                std::string content = bodyStr.substr(pos, contentEnd - pos);
                
                if (!name.empty()) {
                    formData->set(name, Value::string(new Runtime::String(content)));
                }
                
                // Move past this boundary to next part
                pos = bodyStr.find(boundary, contentEnd);
                if (pos != std::string::npos) pos += boundary.size();
            }
        }
        
        p->resolve(Value::object(formData));
        
    } else {
        p->reject(Value::string(new Runtime::String("Unsupported content type for FormData")));
    }
    
    return p;
}

Response* Response::clone() const {
    if (bodyUsed_) {
        return nullptr;  // Cannot clone after body consumed
    }
    
    Response* r = new Response(status_, statusText_);
    r->body_ = body_;
    r->bodyUsed_ = false;
    
    // Clone headers
    for (const auto& [name, value] : headers_->entries()) {
        r->headers_->setHeader(name, value);
    }
    
    return r;
}

// Simple JSON parser for Response.json()
Value Response::parseJsonString(const std::string& json) const {
    size_t pos = 0;
    return parseValue(json, pos);
}

void Response::skipWhitespace(const std::string& json, size_t& pos) const {
    while (pos < json.size() && 
           (json[pos] == ' ' || json[pos] == '\t' || 
            json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }
}

Value Response::parseValue(const std::string& json, size_t& pos) const {
    skipWhitespace(json, pos);
    
    if (pos >= json.size()) {
        throw std::runtime_error("Unexpected end of JSON");
    }
    
    char c = json[pos];
    
    if (c == '"') {
        return parseString(json, pos);
    } else if (c == '{') {
        return parseObject(json, pos);
    } else if (c == '[') {
        return parseArray(json, pos);
    } else if (c == 't' || c == 'f') {
        return parseBool(json, pos);
    } else if (c == 'n') {
        return parseNull(json, pos);
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        return parseNumber(json, pos);
    }
    
    throw std::runtime_error("Invalid JSON value");
}

Value Response::parseString(const std::string& json, size_t& pos) const {
    if (json[pos] != '"') {
        throw std::runtime_error("Expected string");
    }
    pos++; // Skip opening quote
    
    std::string result;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            pos++;
            switch (json[pos]) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u':
                    // Unicode escape - simplified handling
                    if (pos + 4 < json.size()) {
                        pos += 4;
                    }
                    result += '?'; // Placeholder for Unicode
                    break;
                default:
                    result += json[pos];
            }
        } else {
            result += json[pos];
        }
        pos++;
    }
    
    if (pos >= json.size()) {
        throw std::runtime_error("Unterminated string");
    }
    
    pos++; // Skip closing quote
    return Value::string(new Runtime::String(result));
}

Value Response::parseNumber(const std::string& json, size_t& pos) const {
    size_t start = pos;
    
    if (json[pos] == '-') pos++;
    
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
    
    if (pos < json.size() && json[pos] == '.') {
        pos++;
        while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
    }
    
    if (pos < json.size() && (json[pos] == 'e' || json[pos] == 'E')) {
        pos++;
        if (pos < json.size() && (json[pos] == '+' || json[pos] == '-')) pos++;
        while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
    }
    
    std::string numStr = json.substr(start, pos - start);
    return Value::number(std::stod(numStr));
}

Value Response::parseBool(const std::string& json, size_t& pos) const {
    if (json.compare(pos, 4, "true") == 0) {
        pos += 4;
        return Value::boolean(true);
    } else if (json.compare(pos, 5, "false") == 0) {
        pos += 5;
        return Value::boolean(false);
    }
    throw std::runtime_error("Invalid boolean");
}

Value Response::parseNull(const std::string& json, size_t& pos) const {
    if (json.compare(pos, 4, "null") == 0) {
        pos += 4;
        return Value::null();
    }
    throw std::runtime_error("Invalid null");
}

Value Response::parseArray(const std::string& json, size_t& pos) const {
    if (json[pos] != '[') {
        throw std::runtime_error("Expected array");
    }
    pos++; // Skip [
    
    Runtime::Array* arr = new Runtime::Array();
    skipWhitespace(json, pos);
    
    if (pos < json.size() && json[pos] == ']') {
        pos++;
        return Value::object(arr);
    }
    
    while (pos < json.size()) {
        arr->push(parseValue(json, pos));
        
        skipWhitespace(json, pos);
        
        if (pos < json.size() && json[pos] == ']') {
            pos++;
            break;
        }
        
        if (pos >= json.size() || json[pos] != ',') {
            throw std::runtime_error("Expected comma or closing bracket");
        }
        pos++; // Skip comma
    }
    
    return Value::object(arr);
}

Value Response::parseObject(const std::string& json, size_t& pos) const {
    if (json[pos] != '{') {
        throw std::runtime_error("Expected object");
    }
    pos++; // Skip {
    
    Runtime::Object* obj = new Runtime::Object();
    skipWhitespace(json, pos);
    
    if (pos < json.size() && json[pos] == '}') {
        pos++;
        return Value::object(obj);
    }
    
    while (pos < json.size()) {
        skipWhitespace(json, pos);
        
        // Parse key
        if (json[pos] != '"') {
            throw std::runtime_error("Expected string key");
        }
        Value keyValue = parseString(json, pos);
        std::string key = keyValue.toString();
        
        skipWhitespace(json, pos);
        
        if (pos >= json.size() || json[pos] != ':') {
            throw std::runtime_error("Expected colon");
        }
        pos++; // Skip :
        
        // Parse value
        Value value = parseValue(json, pos);
        obj->set(key, value);
        
        skipWhitespace(json, pos);
        
        if (pos < json.size() && json[pos] == '}') {
            pos++;
            break;
        }
        
        if (pos >= json.size() || json[pos] != ',') {
            throw std::runtime_error("Expected comma or closing brace");
        }
        pos++; // Skip comma
    }
    
    return Value::object(obj);
}

// =============================================================================
// AbortController / AbortSignal
// =============================================================================

class AbortSignal : public Runtime::Object {
public:
    AbortSignal() : Object(Runtime::ObjectType::Ordinary), aborted_(false) {}
    
    bool aborted() const { return aborted_; }
    
    void abort() {
        if (aborted_) return; // Already aborted
        
        aborted_ = true;
        
        // Fire abort event to all registered listeners
        for (const auto& listener : abortListeners_) {
            if (listener.isObject() && listener.asObject()->isFunction()) {
                Runtime::Function* fn = static_cast<Runtime::Function*>(listener.asObject());
                // Create abort event object
                Runtime::Object* event = new Runtime::Object();
                event->set("type", Value::string(new Runtime::String("abort")));
                event->set("target", Value::object(this));
                
                std::vector<Value> args = { Value::object(event) };
                fn->call(nullptr, Value::object(this), args);
            }
        }
    }
    
    /**
     * @brief Add an abort event listener
     */
    void addEventListener(const std::string& type, Value callback) {
        if (type == "abort" && callback.isObject()) {
            abortListeners_.push_back(callback);
        }
    }
    
    /**
     * @brief Remove an abort event listener
     */
    void removeEventListener(const std::string& type, Value callback) {
        if (type == "abort") {
            abortListeners_.erase(
                std::remove_if(abortListeners_.begin(), abortListeners_.end(),
                    [&callback](const Value& v) {
                        return v.isObject() && callback.isObject() && 
                               v.asObject() == callback.asObject();
                    }),
                abortListeners_.end());
        }
    }
    
    /**
     * @brief Set onabort handler (property-style)
     */
    void setOnAbort(Value handler) {
        onAbortHandler_ = handler;
        if (handler.isObject()) {
            // Add to listeners if not already present
            addEventListener("abort", handler);
        }
    }
    
private:
    std::atomic<bool> aborted_;
    std::vector<Value> abortListeners_;
    Value onAbortHandler_;
};

class AbortController : public Runtime::Object {
public:
    AbortController() 
        : Object(Runtime::ObjectType::Ordinary)
        , signal_(new AbortSignal()) {}
    
    AbortSignal* signal() const { return signal_; }
    
    void abort() {
        signal_->abort();
    }
    
private:
    AbortSignal* signal_;
};

// =============================================================================
// HTTP Helper (nxhttp)
// =============================================================================

struct HttpResult {
    bool success = false;
    int statusCode = 0;
    std::string body;
    Headers* headers = nullptr;
    std::string error;
};

/**
 * @brief Convert method string to NxHttpMethod enum
 */
static NxHttpMethod methodStringToEnum(const std::string& method) {
    if (method == "POST") return NX_HTTP_POST;
    if (method == "PUT") return NX_HTTP_PUT;
    if (method == "DELETE") return NX_HTTP_DELETE;
    if (method == "PATCH") return NX_HTTP_PATCH;
    if (method == "HEAD") return NX_HTTP_HEAD;
    if (method == "OPTIONS") return NX_HTTP_OPTIONS;
    return NX_HTTP_GET;
}

static HttpResult performHttpRequest(
    const std::string& url, 
    const std::string& method = "GET",
    const std::string& body = "",
    Headers* requestHeaders = nullptr,
    long timeoutMs = 30000,
    AbortSignal* signal = nullptr) {
    
    HttpResult result;
    result.headers = new Headers();
    
    // Create nxhttp request
    NxHttpRequest* req = nx_http_request_create(methodStringToEnum(method), url.c_str());
    if (!req) {
        result.error = "Failed to create HTTP request";
        return result;
    }
    
    // Set timeout
    nx_http_request_set_timeout(req, static_cast<int>(timeoutMs));
    
    // Set redirects
    nx_http_request_set_follow_redirects(req, true, 10);
    
    // Set request body
    if (!body.empty()) {
        nx_http_request_set_body_string(req, body.c_str());
    }
    
    // Set custom headers
    if (requestHeaders) {
        for (const auto& [name, value] : requestHeaders->entries()) {
            nx_http_request_set_header(req, name.c_str(), value.c_str());
        }
    }
    
    // Set User-Agent header
    nx_http_request_set_header(req, "User-Agent", "ZepraBrowser/1.0");
    
    // Check abort signal before request
    if (signal && signal->aborted()) {
        nx_http_request_free(req);
        result.error = "Request aborted";
        return result;
    }
    
    // Create client and send request
    NxHttpClientConfig config = {
        .connect_timeout_ms = static_cast<int>(timeoutMs),
        .read_timeout_ms = static_cast<int>(timeoutMs),
        .follow_redirects = true,
        .max_redirects = 10,
        .verify_ssl = true,
        .user_agent = "ZepraBrowser/1.0"
    };
    
    NxHttpClient* client = nx_http_client_create(&config);
    if (!client) {
        nx_http_request_free(req);
        result.error = "Failed to create HTTP client";
        return result;
    }
    
    NxHttpError err;
    NxHttpResponse* res = nx_http_client_send(client, req, &err);
    
    if (res) {
        result.success = true;
        result.statusCode = nx_http_response_status(res);
        
        // Get body
        const char* bodyStr = nx_http_response_body_string(res);
        if (bodyStr) {
            result.body = bodyStr;
        }
        
        // Copy response headers
        const NxHttpHeaders* resHeaders = nx_http_response_headers(res);
        if (resHeaders) {
            size_t count = nx_http_headers_count(resHeaders);
            for (size_t i = 0; i < count; ++i) {
                const char* name = nullptr;
                const char* value = nullptr;
                if (nx_http_headers_get_at(resHeaders, i, &name, &value) && name && value) {
                    result.headers->setHeader(name, value);
                }
            }
        }
        
        nx_http_response_free(res);
    } else {
        // Check abort signal
        if (signal && signal->aborted()) {
            result.error = "Request aborted";
        } else {
            result.error = nx_http_error_string(err);
        }
    }
    
    nx_http_client_free(client);
    nx_http_request_free(req);
    
    return result;
}

// =============================================================================
// FetchAPI Implementation
// =============================================================================

Promise* FetchAPI::fetch(const std::string& url, Request* request) {
    Promise* promise = new Promise();
    
    // Get request parameters
    std::string method = request ? request->method() : "GET";
    std::string body = request ? request->body() : "";
    Headers* headers = request ? request->headers() : nullptr;
    
    // Perform actual HTTP request
    HttpResult result = performHttpRequest(url, method, body, headers);
    
    if (result.success) {
        std::string statusText = result.statusCode >= 200 && result.statusCode < 300 ? "OK" : "Error";
        Response* response = new Response(result.statusCode, statusText);
        response->setBody(result.body);
        
        // Copy response headers
        if (result.headers) {
            for (const auto& [name, value] : result.headers->entries()) {
                response->headers()->setHeader(name, value);
            }
        }
        
        promise->resolve(Value::object(response));
    } else {
        promise->reject(Value::string(new Runtime::String(result.error)));
    }
    
    // Clean up
    delete result.headers;
    
    return promise;
}

void FetchAPI::fetchAsync(const std::string& url, Request* request, FetchCallback callback) {
    std::string method = request ? request->method() : "GET";
    std::string body = request ? request->body() : "";
    
    // Copy headers to avoid lifetime issues
    std::unordered_map<std::string, std::string> headersCopy;
    if (request && request->headers()) {
        headersCopy = request->headers()->entries();
    }
    
    std::thread([url, method, body, headersCopy, callback]() {
        Headers* headers = nullptr;
        if (!headersCopy.empty()) {
            headers = new Headers();
            for (const auto& [name, value] : headersCopy) {
                headers->setHeader(name, value);
            }
        }
        
        HttpResult result = performHttpRequest(url, method, body, headers);
        
        Response* response;
        if (result.success) {
            response = new Response(result.statusCode, "OK");
            response->setBody(result.body);
            if (result.headers) {
                for (const auto& [name, value] : result.headers->entries()) {
                    response->headers()->setHeader(name, value);
                }
            }
        } else {
            response = new Response(0, "Network Error");
            response->setBody(result.error);
        }
        
        if (callback) {
            callback(response);
        }
        
        delete headers;
        delete result.headers;
    }).detach();
}

Value FetchAPI::fetchBuiltin(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty()) {
        return Value::undefined();
    }
    
    std::string url;
    if (args[0].isString()) {
        url = static_cast<Runtime::String*>(args[0].asObject())->value();
    } else {
        url = args[0].toString();
    }
    
    Request* request = nullptr;
    if (args.size() > 1 && args[1].isObject()) {
        Runtime::Object* optionsObj = args[1].asObject();
        
        // Create request from options object
        std::string method = "GET";
        if (optionsObj->has("method")) {
            method = optionsObj->get("method").toString();
        }
        
        request = new Request(url, method);
        
        // Get body
        if (optionsObj->has("body")) {
            Value bodyVal = optionsObj->get("body");
            if (bodyVal.isString()) {
                request->setBody(static_cast<Runtime::String*>(bodyVal.asObject())->value());
            } else {
                request->setBody(bodyVal.toString());
            }
        }
        
        // Get headers
        if (optionsObj->has("headers")) {
            Value headersVal = optionsObj->get("headers");
            if (headersVal.isObject()) {
                Runtime::Object* headersObj = headersVal.asObject();
                auto keys = headersObj->keys();
                for (const auto& key : keys) {
                    Value val = headersObj->get(key);
                    request->setHeader(key, val.toString());
                }
            }
        }
    }
    
    Promise* promise = fetch(url, request);
    return Value::object(promise);
}

// =============================================================================
// Global Initialization
// =============================================================================

static bool fetchInitialized = false;

void initFetchAPI() {
    // nxhttp doesn't require global initialization
    fetchInitialized = true;
}

void shutdownFetchAPI() {
    // nxhttp doesn't require global cleanup
    fetchInitialized = false;
}

} // namespace Zepra::Browser
