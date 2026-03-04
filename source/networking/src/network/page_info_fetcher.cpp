/**
 * @file page_info_fetcher.cpp
 * @brief Fetch page title and favicon using NxHTTP
 */

#include "network/page_info_fetcher.h"

// Use NxHTTP (project's HTTP library)
#include <nxhttp.h>
#define USE_NXHTTP 1

#include <algorithm>
#include <cctype>
#include <regex>

namespace zepra {

PageInfoFetcher::PageInfoFetcher() {}

PageInfoFetcher::~PageInfoFetcher() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

void PageInfoFetcher::fetchAsync(const std::string& url, Callback callback) {
    std::thread([this, url, callback]() {
        PageInfo info = fetch(url);
        if (callback) callback(info);
    }).detach();
}

// Helper to extract between tags
static std::string extractBetween(const std::string& html, 
                                   const std::string& startTag, 
                                   const std::string& endTag) {
    std::string lower = html;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    size_t start = lower.find(startTag);
    if (start == std::string::npos) return "";
    start += startTag.length();
    
    size_t end = lower.find(endTag, start);
    if (end == std::string::npos) return "";
    
    return html.substr(start, end - start);
}

std::string PageInfoFetcher::extractTitle(const std::string& html) {
    std::string title = extractBetween(html, "<title>", "</title>");
    
    // Trim whitespace
    size_t start = title.find_first_not_of(" \t\n\r");
    size_t end = title.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    
    return title.substr(start, end - start + 1);
}

std::string PageInfoFetcher::extractFaviconUrl(const std::string& html, const std::string& baseUrl) {
    // Look for <link rel="icon" href="...">
    std::string lower = html;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Find link tags with rel="icon" or rel="shortcut icon"
    std::regex iconRegex(R"(<link[^>]*rel\s*=\s*["'](?:shortcut\s+)?icon["'][^>]*>)", 
                         std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, iconRegex)) {
        std::string linkTag = match[0];
        
        // Extract href
        std::regex hrefRegex(R"(href\s*=\s*["']([^"']+)["'])", std::regex::icase);
        std::smatch hrefMatch;
        if (std::regex_search(linkTag, hrefMatch, hrefRegex)) {
            std::string href = hrefMatch[1];
            
            // Handle relative URLs
            if (href.find("://") == std::string::npos) {
                if (href[0] == '/') {
                    // Absolute path - add origin
                    size_t protoEnd = baseUrl.find("://");
                    if (protoEnd != std::string::npos) {
                        size_t hostEnd = baseUrl.find('/', protoEnd + 3);
                        if (hostEnd == std::string::npos) hostEnd = baseUrl.length();
                        href = baseUrl.substr(0, hostEnd) + href;
                    }
                } else {
                    // Relative path
                    size_t lastSlash = baseUrl.rfind('/');
                    if (lastSlash != std::string::npos) {
                        href = baseUrl.substr(0, lastSlash + 1) + href;
                    }
                }
            }
            return href;
        }
    }
    
    return "";
}

std::string PageInfoFetcher::getDefaultFaviconUrl(const std::string& url) {
    // Parse URL to get origin
    size_t protoEnd = url.find("://");
    if (protoEnd == std::string::npos) return "";
    
    size_t hostEnd = url.find('/', protoEnd + 3);
    if (hostEnd == std::string::npos) hostEnd = url.length();
    
    return url.substr(0, hostEnd) + "/favicon.ico";
}

#if USE_NXHTTP

PageInfo PageInfoFetcher::fetch(const std::string& url) {
    PageInfo info;
    info.url = url;
    
    try {
        // Fetch the page
        NxHttpError err;
        NxHttpResponse* res = nx_http_get(url.c_str(), &err);
        
        if (!res) {
            info.error = nx_http_error_string(err);
            return info;
        }
        
        int status = nx_http_response_status(res);
        if (status < 200 || status >= 400) {
            info.error = "HTTP " + std::to_string(status);
            nx_http_response_free(res);
            return info;
        }
        
        std::string body = nx_http_response_body_string(res);
        nx_http_response_free(res);
        
        // Extract title
        info.title = extractTitle(body);
        
        // Extract favicon URL
        info.favicon_url = extractFaviconUrl(body, url);
        if (info.favicon_url.empty()) {
            info.favicon_url = getDefaultFaviconUrl(url);
        }
        
        info.success = true;
    } catch (...) {
        info.error = "Exception during fetch";
    }
    
    return info;
}

// curl fallback removed - now using nxhttp exclusively

// Simple convenience functions
std::string fetchPageTitle(const std::string& url) {
    PageInfoFetcher fetcher;
    PageInfo info = fetcher.fetch(url);
    return info.title;
}

std::string fetchFaviconUrl(const std::string& url) {
    PageInfoFetcher fetcher;
    PageInfo info = fetcher.fetch(url);
    return info.favicon_url;
}

} // namespace zepra
