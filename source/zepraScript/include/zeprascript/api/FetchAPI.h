/**
 * @file FetchAPI.h
 * @brief Fetch API Implementation
 * 
 * Modern fetch() API:
 * - Headers: HTTP header map
 * - Request: Fetch request
 * - Response: Fetch response
 * - fetch(): Promise-based network request
 */

#pragma once

#include "../core/EmbedderAPI.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

namespace Zepra::API {

// =============================================================================
// Headers
// =============================================================================

/**
 * @brief HTTP Headers (case-insensitive)
 */
class Headers {
public:
    Headers() = default;
    
    // From init object
    explicit Headers(std::initializer_list<std::pair<std::string, std::string>> init) {
        for (const auto& [key, value] : init) {
            append(key, value);
        }
    }
    
    // Append (allows duplicates)
    void append(const std::string& name, const std::string& value) {
        std::string key = normalize(name);
        headers_[key].push_back(value);
    }
    
    // Set (replaces existing)
    void set(const std::string& name, const std::string& value) {
        std::string key = normalize(name);
        headers_[key] = {value};
    }
    
    // Get first value
    std::optional<std::string> get(const std::string& name) const {
        std::string key = normalize(name);
        auto it = headers_.find(key);
        if (it != headers_.end() && !it->second.empty()) {
            return it->second[0];
        }
        return std::nullopt;
    }
    
    // Get all values
    std::vector<std::string> getAll(const std::string& name) const {
        std::string key = normalize(name);
        auto it = headers_.find(key);
        return it != headers_.end() ? it->second : std::vector<std::string>{};
    }
    
    // Has header
    bool has(const std::string& name) const {
        return headers_.count(normalize(name)) > 0;
    }
    
    // Delete header
    void remove(const std::string& name) {
        headers_.erase(normalize(name));
    }
    
    // Iteration
    using Iterator = std::unordered_map<std::string, std::vector<std::string>>::const_iterator;
    Iterator begin() const { return headers_.begin(); }
    Iterator end() const { return headers_.end(); }
    
    // Keys
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        for (const auto& [key, _] : headers_) {
            result.push_back(key);
        }
        return result;
    }
    
private:
    static std::string normalize(const std::string& name) {
        std::string result = name;
        for (char& c : result) {
            c = std::tolower(c);
        }
        return result;
    }
    
    std::unordered_map<std::string, std::vector<std::string>> headers_;
};

// =============================================================================
// Request Init
// =============================================================================

enum class RequestMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
};

enum class RequestMode {
    Cors,
    NoCors,
    SameOrigin,
    Navigate
};

enum class RequestCredentials {
    Omit,
    SameOrigin,
    Include
};

enum class RequestCache {
    Default,
    NoStore,
    Reload,
    NoCache,
    ForceCache,
    OnlyIfCached
};

enum class RequestRedirect {
    Follow,
    Error,
    Manual
};

/**
 * @brief Request initialization options
 */
struct RequestInit {
    RequestMethod method = RequestMethod::GET;
    Headers headers;
    std::optional<std::string> body;
    RequestMode mode = RequestMode::Cors;
    RequestCredentials credentials = RequestCredentials::SameOrigin;
    RequestCache cache = RequestCache::Default;
    RequestRedirect redirect = RequestRedirect::Follow;
    std::optional<std::string> referrer;
    std::optional<std::string> integrity;
    bool keepalive = false;
};

// =============================================================================
// Body Mixin
// =============================================================================

/**
 * @brief Body handling for Request/Response
 */
class Body {
public:
    bool bodyUsed() const { return bodyUsed_; }
    
    // Consume body as text
    std::string text() {
        if (bodyUsed_) throw std::runtime_error("Body already used");
        bodyUsed_ = true;
        return body_;
    }
    
    // Consume body as JSON
    ZebraValue json() {
        std::string t = text();
        // Would parse JSON here
        return ZebraValue();
    }
    
    // Consume body as ArrayBuffer
    std::vector<uint8_t> arrayBuffer() {
        if (bodyUsed_) throw std::runtime_error("Body already used");
        bodyUsed_ = true;
        return std::vector<uint8_t>(body_.begin(), body_.end());
    }
    
    // Consume body as Blob
    // Blob blob();
    
    // Consume body as FormData
    // FormData formData();
    
protected:
    std::string body_;
    bool bodyUsed_ = false;
};

// =============================================================================
// Request
// =============================================================================

/**
 * @brief Fetch Request
 */
class Request : public Body {
public:
    // From URL string
    explicit Request(const std::string& url, const RequestInit& init = {})
        : url_(url), init_(init) {
        if (init.body) {
            body_ = *init.body;
        }
    }
    
    // Copy with modifications
    Request(const Request& input, const RequestInit& init);
    
    // Properties
    const std::string& url() const { return url_; }
    RequestMethod method() const { return init_.method; }
    const Headers& headers() const { return init_.headers; }
    RequestMode mode() const { return init_.mode; }
    RequestCredentials credentials() const { return init_.credentials; }
    RequestCache cache() const { return init_.cache; }
    RequestRedirect redirect() const { return init_.redirect; }
    
    // Method as string
    std::string methodString() const {
        switch (init_.method) {
            case RequestMethod::GET: return "GET";
            case RequestMethod::POST: return "POST";
            case RequestMethod::PUT: return "PUT";
            case RequestMethod::DELETE: return "DELETE";
            case RequestMethod::PATCH: return "PATCH";
            case RequestMethod::HEAD: return "HEAD";
            case RequestMethod::OPTIONS: return "OPTIONS";
        }
        return "GET";
    }
    
    // Clone
    std::unique_ptr<Request> clone() const {
        return std::make_unique<Request>(*this);
    }
    
private:
    std::string url_;
    RequestInit init_;
};

// =============================================================================
// Response
// =============================================================================

enum class ResponseType {
    Basic,
    Cors,
    Default,
    Error,
    Opaque,
    OpaqueRedirect
};

/**
 * @brief Fetch Response
 */
class Response : public Body {
public:
    Response() = default;
    
    // From body and options
    Response(const std::string& body, int status = 200,
             const std::string& statusText = "OK",
             const Headers& headers = {})
        : status_(status), statusText_(statusText), headers_(headers) {
        body_ = body;
    }
    
    // Properties
    ResponseType type() const { return type_; }
    const std::string& url() const { return url_; }
    bool redirected() const { return redirected_; }
    int status() const { return status_; }
    const std::string& statusText() const { return statusText_; }
    bool ok() const { return status_ >= 200 && status_ < 300; }
    const Headers& headers() const { return headers_; }
    
    // Clone
    std::unique_ptr<Response> clone() const {
        auto r = std::make_unique<Response>();
        r->type_ = type_;
        r->url_ = url_;
        r->redirected_ = redirected_;
        r->status_ = status_;
        r->statusText_ = statusText_;
        r->headers_ = headers_;
        r->body_ = body_;
        return r;
    }
    
    // Static constructors
    static Response error() {
        Response r;
        r.type_ = ResponseType::Error;
        r.status_ = 0;
        return r;
    }
    
    static Response redirect(const std::string& url, int status = 302) {
        Response r;
        r.status_ = status;
        r.headers_.set("Location", url);
        return r;
    }
    
private:
    ResponseType type_ = ResponseType::Default;
    std::string url_;
    bool redirected_ = false;
    int status_ = 200;
    std::string statusText_ = "OK";
    Headers headers_;
};

// =============================================================================
// Fetch Function
// =============================================================================

/**
 * @brief Fetch callback type
 */
using FetchCallback = std::function<void(std::unique_ptr<Response>, std::optional<std::string>)>;

/**
 * @brief Network backend interface
 */
class NetworkBackend {
public:
    virtual ~NetworkBackend() = default;
    virtual void fetch(const Request& request, FetchCallback callback) = 0;
};

/**
 * @brief Global fetch function
 */
void fetch(const std::string& url, const RequestInit& init, FetchCallback callback);
void fetch(const Request& request, FetchCallback callback);

/**
 * @brief Set network backend
 */
void setNetworkBackend(std::unique_ptr<NetworkBackend> backend);

// =============================================================================
// AbortController
// =============================================================================

class AbortSignal;

/**
 * @brief Abort controller for canceling fetch
 */
class AbortController {
public:
    AbortController() : signal_(std::make_shared<AbortSignal>()) {}
    
    AbortSignal* signal() const { return signal_.get(); }
    
    void abort(const std::string& reason = "") {
        signal_->abort(reason);
    }
    
private:
    std::shared_ptr<AbortSignal> signal_;
};

/**
 * @brief Abort signal
 */
class AbortSignal {
public:
    bool aborted() const { return aborted_; }
    const std::string& reason() const { return reason_; }
    
    void onAbort(std::function<void()> callback) {
        if (aborted_) {
            callback();
        } else {
            callbacks_.push_back(std::move(callback));
        }
    }
    
private:
    friend class AbortController;
    
    void abort(const std::string& reason) {
        if (aborted_) return;
        aborted_ = true;
        reason_ = reason;
        for (const auto& cb : callbacks_) {
            cb();
        }
        callbacks_.clear();
    }
    
    bool aborted_ = false;
    std::string reason_;
    std::vector<std::function<void()>> callbacks_;
};

} // namespace Zepra::API
