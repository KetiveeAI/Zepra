/**
 * @file page_info_fetcher.h
 * @brief Fetch web page info (title, favicon) using NxHTTP
 * 
 * Lightweight page metadata fetcher for browser tabs.
 */

#ifndef PAGE_INFO_FETCHER_H
#define PAGE_INFO_FETCHER_H

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <thread>
#include <mutex>

namespace zepra {

struct PageInfo {
    std::string url;
    std::string title;
    std::string favicon_url;
    std::vector<uint8_t> favicon_data;
    bool success = false;
    std::string error;
};

/**
 * Fetch page info (title, favicon) from URL.
 * Uses HEAD request first, then GET for HTML if needed.
 */
class PageInfoFetcher {
public:
    using Callback = std::function<void(const PageInfo&)>;
    
    PageInfoFetcher();
    ~PageInfoFetcher();
    
    /**
     * Fetch page info asynchronously.
     * @param url URL to fetch
     * @param callback Called when complete (on worker thread)
     */
    void fetchAsync(const std::string& url, Callback callback);
    
    /**
     * Fetch page info synchronously.
     */
    PageInfo fetch(const std::string& url);
    
    /**
     * Extract title from HTML content.
     */
    static std::string extractTitle(const std::string& html);
    
    /**
     * Extract favicon URL from HTML content.
     */
    static std::string extractFaviconUrl(const std::string& html, const std::string& baseUrl);
    
    /**
     * Get default favicon URL for a host.
     */
    static std::string getDefaultFaviconUrl(const std::string& url);
    
private:
    std::thread worker_;
    bool running_ = true;
};

// Simple synchronous fetch functions
std::string fetchPageTitle(const std::string& url);
std::string fetchFaviconUrl(const std::string& url);

} // namespace zepra

#endif // PAGE_INFO_FETCHER_H
