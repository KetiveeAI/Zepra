/**
 * @file html_fetch.hpp
 * @brief Fetch API utilities
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <any>

namespace Zepra::WebCore {

/**
 * @brief Request method
 */
enum class RequestMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
};

/**
 * @brief Request mode
 */
enum class RequestMode {
    SameOrigin,
    NoCors,
    Cors,
    Navigate
};

/**
 * @brief Request credentials
 */
enum class RequestCredentials {
    Omit,
    SameOrigin,
    Include
};

/**
 * @brief Request cache
 */
enum class RequestCache {
    Default,
    NoStore,
    Reload,
    NoCache,
    ForceCache,
    OnlyIfCached
};

/**
 * @brief Request redirect
 */
enum class RequestRedirect {
    Follow,
    Error,
    Manual
};

/**
 * @brief Headers
 */
class Headers {
public:
    Headers() = default;
    explicit Headers(const std::unordered_map<std::string, std::string>& init);
    
    void append(const std::string& name, const std::string& value);
    void delete_(const std::string& name);
    std::string get(const std::string& name) const;
    std::vector<std::string> getSetCookie() const;
    bool has(const std::string& name) const;
    void set(const std::string& name, const std::string& value);
    
    // Iteration
    std::vector<std::pair<std::string, std::string>> entries() const;
    
private:
    std::unordered_map<std::string, std::vector<std::string>> headers_;
};

/**
 * @brief Body mixin
 */
class Body {
public:
    virtual ~Body() = default;
    
    bool bodyUsed() const { return bodyUsed_; }
    
    void arrayBuffer(std::function<void(const std::vector<uint8_t>&)> callback);
    void blob(std::function<void(const std::vector<uint8_t>&)> callback);
    void formData(std::function<void(const std::string&)> callback);
    void json(std::function<void(const std::any&)> callback);
    void text(std::function<void(const std::string&)> callback);
    
protected:
    bool bodyUsed_ = false;
    std::vector<uint8_t> bodyData_;
};

/**
 * @brief Request options
 */
struct RequestInit {
    RequestMethod method = RequestMethod::GET;
    Headers headers;
    std::string body;
    RequestMode mode = RequestMode::Cors;
    RequestCredentials credentials = RequestCredentials::SameOrigin;
    RequestCache cache = RequestCache::Default;
    RequestRedirect redirect = RequestRedirect::Follow;
    std::string referrer;
    std::string referrerPolicy;
    std::string integrity;
    bool keepalive = false;
    // signal: AbortSignal (not implemented here)
};

/**
 * @brief Request
 */
class Request : public Body {
public:
    Request(const std::string& url, const RequestInit& init = {});
    ~Request() override = default;
    
    std::string url() const { return url_; }
    RequestMethod method() const { return method_; }
    Headers& headers() { return headers_; }
    RequestMode mode() const { return mode_; }
    RequestCredentials credentials() const { return credentials_; }
    RequestCache cache() const { return cache_; }
    RequestRedirect redirect() const { return redirect_; }
    std::string referrer() const { return referrer_; }
    std::string referrerPolicy() const { return referrerPolicy_; }
    std::string integrity() const { return integrity_; }
    
    Request clone() const;
    
private:
    std::string url_;
    RequestMethod method_;
    Headers headers_;
    RequestMode mode_;
    RequestCredentials credentials_;
    RequestCache cache_;
    RequestRedirect redirect_;
    std::string referrer_;
    std::string referrerPolicy_;
    std::string integrity_;
};

/**
 * @brief Response type
 */
enum class ResponseType {
    Basic,
    Cors,
    Default,
    Error,
    Opaque,
    OpaqueRedirect
};

/**
 * @brief Response
 */
class Response : public Body {
public:
    Response(const std::vector<uint8_t>& body = {},
             int status = 200,
             const std::string& statusText = "OK");
    ~Response() override = default;
    
    // Static methods
    static Response error();
    static Response redirect(const std::string& url, int status = 302);
    
    // Properties
    ResponseType type() const { return type_; }
    std::string url() const { return url_; }
    bool redirected() const { return redirected_; }
    int status() const { return status_; }
    bool ok() const { return status_ >= 200 && status_ < 300; }
    std::string statusText() const { return statusText_; }
    Headers& headers() { return headers_; }
    
    Response clone() const;
    
private:
    ResponseType type_ = ResponseType::Default;
    std::string url_;
    bool redirected_ = false;
    int status_;
    std::string statusText_;
    Headers headers_;
};

/**
 * @brief Fetch function
 */
void fetch(const std::string& url,
           std::function<void(Response*)> onSuccess,
           std::function<void(const std::string&)> onError = nullptr);

void fetch(const Request& request,
           std::function<void(Response*)> onSuccess,
           std::function<void(const std::string&)> onError = nullptr);

} // namespace Zepra::WebCore
