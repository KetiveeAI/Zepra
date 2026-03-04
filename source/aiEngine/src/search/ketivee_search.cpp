#include "search/ketivee_search.h"
#include "common/constants.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <nxhttp.h>
#include <nlohmann/json.hpp>
#include <unordered_set>

using json = nlohmann::json;

namespace zepra {

// HTTP utility functions using nxhttp (replaces libcurl)
namespace http_utils {
    std::string get(const std::string& url) {
        try {
            nx::HttpClient client;
            auto response = client.get(url);
            if (response.ok()) {
                return response.body();
            } else {
                std::cerr << "HTTP GET failed: " << response.status() << " " << response.statusText() << std::endl;
                return "";
            }
        } catch (const nx::HttpException& e) {
            std::cerr << "HTTP request failed: " << e.what() << std::endl;
            return "";
        }
    }
    
    std::string post(const std::string& url, const std::string& data) {
        try {
            nx::HttpClient client;
            auto response = client.post(url, data, "application/x-www-form-urlencoded");
            if (response.ok()) {
                return response.body();
            } else {
                std::cerr << "HTTP POST failed: " << response.status() << " " << response.statusText() << std::endl;
                return "";
            }
        } catch (const nx::HttpException& e) {
            std::cerr << "HTTP POST failed: " << e.what() << std::endl;
            return "";
        }
    }
}

// http_utils already defined above using nxhttp

// KetiveeSearchEngine Implementation
KetiveeSearchEngine::KetiveeSearchEngine() 
    : searchUrl(KETIVEE_SEARCH_URL)
    , localSearchEnabled(true)
    , maxLocalResults(100) {
    
    // nxhttp does not require global initialization
    
    // TODO: Load search history and bookmarks from persistent storage
    // loadSearchHistory();
    // loadBookmarks();
}

std::vector<SearchResult> KetiveeSearchEngine::search(const SearchQuery& query) {
    if (localSearchEnabled) {
        return performHybridSearch(query);
    } else {
        return performWebSearch(query);
    }
}

std::vector<SearchResult> KetiveeSearchEngine::search(const String& query) {
    SearchQuery searchQuery(query);
    return search(searchQuery);
}

std::vector<String> KetiveeSearchEngine::getSuggestions(const String& query) {
    std::vector<String> suggestions;
    
    try {
        // Connect to the Node.js search backend for suggestions
        String suggestionsUrl = "http://localhost:6329/api/search/suggestions";
        
        // Build JSON request
        json requestData;
        requestData["query"] = query;
        
        String jsonRequest = requestData.dump();
        
        // Make HTTP POST request to suggestions endpoint using nxhttp
        try {
            nx::HttpClient client;
            auto response = client.post(suggestionsUrl, jsonRequest, "application/json");
            
            if (response.ok()) {
                // Parse JSON response
                try {
                    json responseData = json::parse(response.body());
                    
                    if (responseData.is_array()) {
                        for (const auto& suggestion : responseData) {
                            suggestions.push_back(suggestion.get<String>());
                        }
                    }
                } catch (const json::exception& e) {
                    std::cerr << "Failed to parse suggestions response: " << e.what() << std::endl;
                }
            } else {
                std::cerr << "Suggestions request failed: " << response.status() << std::endl;
            }
        } catch (const nx::HttpException& e) {
            std::cerr << "Suggestions HTTP error: " << e.what() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Suggestions error: " << e.what() << std::endl;
    }
    
    // Fallback to local suggestions if backend is not available
    if (suggestions.empty()) {
        suggestions.push_back(query + " tutorial");
        suggestions.push_back(query + " guide");
        suggestions.push_back(query + " examples");
        suggestions.push_back(query + " documentation");
    }
    
    return suggestions;
}

std::vector<String> KetiveeSearchEngine::getTrendingSearches() {
    return {
        "Ketivee OS",
        "Zepra Browser",
        "privacy search",
        "open source browser",
        "fast browser",
        "lightweight browser",
        "browser engine",
        "web development",
        "programming",
        "technology news"
    };
}

std::vector<SearchResult> KetiveeSearchEngine::searchImages(const String& query) {
    SearchQuery searchQuery(query);
    searchQuery.filters = "type:image";
    return search(searchQuery);
}

std::vector<SearchResult> KetiveeSearchEngine::searchVideos(const String& query) {
    SearchQuery searchQuery(query);
    searchQuery.filters = "type:video";
    return search(searchQuery);
}

std::vector<SearchResult> KetiveeSearchEngine::searchNews(const String& query) {
    SearchQuery searchQuery(query);
    searchQuery.filters = "type:news";
    return search(searchQuery);
}

std::vector<SearchResult> KetiveeSearchEngine::searchDocuments(const String& query) {
    SearchQuery searchQuery(query);
    searchQuery.filters = "type:document";
    return search(searchQuery);
}

void KetiveeSearchEngine::addToHistory(const String& query) {
    // Remove if already exists
    auto it = std::find(searchHistory.begin(), searchHistory.end(), query);
    if (it != searchHistory.end()) {
        searchHistory.erase(it);
    }
    
    // Add to front
    searchHistory.insert(searchHistory.begin(), query);
    
    // Limit history size
    if (searchHistory.size() > 100) {
        searchHistory.resize(100);
    }
}

std::vector<String> KetiveeSearchEngine::getSearchHistory() const {
    return searchHistory;
}

void KetiveeSearchEngine::clearSearchHistory() {
    searchHistory.clear();
}

void KetiveeSearchEngine::addBookmark(const SearchResult& result) {
    // Check if already bookmarked
    for (const auto& bookmark : bookmarks) {
        if (bookmark.url == result.url) {
            return;
        }
    }
    
    bookmarks.push_back(result);
}

void KetiveeSearchEngine::removeBookmark(const String& url) {
    bookmarks.erase(
        std::remove_if(bookmarks.begin(), bookmarks.end(),
            [&url](const SearchResult& result) { return result.url == url; }),
        bookmarks.end()
    );
}

std::vector<SearchResult> KetiveeSearchEngine::getBookmarks() const {
    return bookmarks;
}

bool KetiveeSearchEngine::isBookmarked(const String& url) const {
    for (const auto& bookmark : bookmarks) {
        if (bookmark.url == url) {
            return true;
        }
    }
    return false;
}

std::vector<SearchResult> KetiveeSearchEngine::performWebSearch(const SearchQuery& query) {
    std::vector<SearchResult> results;
    
    try {
        // Connect to the unified ZepraSearch API Gateway
        String searchUrl = SEARCH_API_ENDPOINT;
        searchUrl += "?q=" + urlEncode(query.query);
        searchUrl += "&page=" + std::to_string(query.page);
        searchUrl += "&limit=" + std::to_string(query.maxResults);
        
        // Make HTTP GET request to search backend using nxhttp
        try {
            nx::HttpClient client;
            auto response = client.get(searchUrl);
            
            if (response.ok()) {
                // Parse JSON response from unified API
                try {
                    json responseData = json::parse(response.body());
                    
                    // Handle new API response format
                    if (responseData.contains("results") && responseData["results"].is_array()) {
                        for (const auto& item : responseData["results"]) {
                            SearchResult result;
                            result.title = item.value("title", "Untitled");
                            result.url = item.value("url", "");
                            result.description = item.value("description", "");
                            result.snippet = item.value("snippet", result.description);
                            result.relevance = item.value("relevance_score", 0.8f);
                            result.timestamp = std::time(nullptr);
                            result.source = item.value("source", "ZepraSearch");
                            
                            if (!result.url.empty()) {
                                results.push_back(result);
                            }
                        }
                    }
                    // Fallback for old format
                    else if (responseData.contains("hits") && responseData["hits"].is_array()) {
                        for (const auto& hit : responseData["hits"]) {
                            SearchResult result;
                            result.title = hit.value("title", "");
                            result.url = hit.value("url", "");
                            result.description = hit.value("content", "");
                            result.relevance = 0.8f;
                            result.timestamp = std::time(nullptr);
                            result.source = "ZepraSearch";
                            
                            if (!result.url.empty()) {
                                results.push_back(result);
                            }
                        }
                    }
                } catch (const json::exception& e) {
                    std::cerr << "Failed to parse search response: " << e.what() << std::endl;
                    std::cerr << "Response: " << response.body() << std::endl;
                }
            } else {
                std::cerr << "Search request failed: " << response.status() << std::endl;
            }
        } catch (const nx::HttpException& e) {
            std::cerr << "Search HTTP error: " << e.what() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Search error: " << e.what() << std::endl;
    }
    
    return results;
}

std::vector<SearchResult> KetiveeSearchEngine::performLocalSearch(const SearchQuery& query) {
    std::vector<SearchResult> results;
    
    // Search in local index
    for (const auto& entry : localIndex) {
        float relevance = calculateRelevance(query.query, entry.first);
        if (relevance > 0.1f) { // Threshold for relevance
            for (const auto& result : entry.second) {
                SearchResult newResult = result;
                newResult.relevance = relevance;
                results.push_back(newResult);
            }
        }
    }
    
    // Sort by relevance
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.relevance > b.relevance;
        });
    
    // Limit results
    if (results.size() > maxLocalResults) {
        results.resize(maxLocalResults);
    }
    
    return results;
}

std::vector<SearchResult> KetiveeSearchEngine::performHybridSearch(const SearchQuery& query) {
    // Combine local and web search results
    auto localResults = performLocalSearch(query);
    auto webResults = performWebSearch(query);
    
    // Merge results
    std::vector<SearchResult> combined;
    combined.insert(combined.end(), localResults.begin(), localResults.end());
    combined.insert(combined.end(), webResults.begin(), webResults.end());
    
    // Remove duplicates
    combined = search_utils::deduplicateResults(combined);
    
    // Sort by relevance
    combined = search_utils::sortByRelevance(combined);
    
    return combined;
}

String KetiveeSearchEngine::buildSearchUrl(const SearchQuery& query) const {
    std::ostringstream url;
    url << searchUrl << "/search?q=" << urlEncode(query.query);
    
    if (!query.language.empty()) {
        url << "&lang=" << urlEncode(query.language);
    }
    
    if (!query.region.empty()) {
        url << "&region=" << urlEncode(query.region);
    }
    
    url << "&max=" << query.maxResults;
    url << "&page=" << query.page;
    url << "&safe=" << (query.safeSearch ? "1" : "0");
    
    if (!query.filters.empty()) {
        url << "&filters=" << urlEncode(query.filters);
    }
    
    if (!apiKey.empty()) {
        url << "&key=" << urlEncode(apiKey);
    }
    
    return url.str();
}

std::vector<SearchResult> KetiveeSearchEngine::parseSearchResponse(const String& response) {
    try {
        json j = json::parse(response);
        std::vector<SearchResult> results;
        
        if (j.contains("results") && j["results"].is_array()) {
            for (const auto& item : j["results"]) {
                SearchResult result;
                
                if (item.contains("title")) result.title = item["title"];
                if (item.contains("url")) result.url = item["url"];
                if (item.contains("description")) result.description = item["description"];
                if (item.contains("snippet")) result.snippet = item["snippet"];
                if (item.contains("thumbnail")) result.thumbnail = item["thumbnail"];
                if (item.contains("relevance")) result.relevance = item["relevance"];
                if (item.contains("source")) result.source = item["source"];
                if (item.contains("timestamp")) result.timestamp = item["timestamp"];
                
                if (!result.title.empty() && !result.url.empty()) {
                    results.push_back(result);
                }
            }
        }
        
        return results;
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing failed: " << e.what() << std::endl;
        return std::vector<SearchResult>();
    }
}

void KetiveeSearchEngine::buildLocalIndex() {
    // Add some default content to the local index
    addToLocalIndex(
        "https://ketivee.org",
        "Ketivee - Privacy-Focused Operating System",
        "Ketivee is a modern, privacy-focused operating system designed for developers and power users. Built with security and performance in mind."
    );
    
    addToLocalIndex(
        "https://zepra.ketivee.org",
        "Zepra Browser - Fast and Lightweight",
        "Zepra is a high-performance web browser built with C++ and SDL2. Features integrated KetiveeSearch engine and modern UI."
    );
    
    addToLocalIndex(
        "https://developer.ketivee.org",
        "Ketivee Developer Portal",
        "Resources for developers building applications on the Ketivee platform. Documentation, APIs, and development tools."
    );
    
    addToLocalIndex(
        "https://search.ketivee.org",
        "KetiveeSearch - Privacy Search Engine",
        "Fast, privacy-focused search engine that doesn't track your searches or collect personal data."
    );
}

void KetiveeSearchEngine::addToLocalIndex(const String& url, const String& title, const String& content) {
    SearchResult result(title, url, content);
    result.snippet = search_utils::generateSnippet(content, "", 150);
    result.source = "local";
    
    // Index by content
    localIndex[content] = {result};
    
    // Also index by title
    localIndex[title] = {result};
}

float KetiveeSearchEngine::calculateRelevance(const String& query, const String& content) {
    if (query.empty() || content.empty()) {
        return 0.0f;
    }
    
    String lowerQuery = query;
    String lowerContent = content;
    
    // Convert to lowercase
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
    
    // Tokenize query
    std::vector<String> queryTokens = search_utils::tokenizeQuery(lowerQuery);
    
    float relevance = 0.0f;
    int totalTokens = queryTokens.size();
    
    for (const String& token : queryTokens) {
        size_t pos = 0;
        int count = 0;
        
        while ((pos = lowerContent.find(token, pos)) != String::npos) {
            count++;
            pos += token.length();
        }
        
        if (count > 0) {
            relevance += static_cast<float>(count) / totalTokens;
        }
    }
    
    // Normalize relevance
    return std::min(relevance / 10.0f, 1.0f);
}

String KetiveeSearchEngine::urlEncode(const String& text) const {
    // Manual URL encoding without curl dependency
    String encoded;
    encoded.reserve(text.length() * 3);  // Worst case: every char encoded
    
    static const char* hex = "0123456789ABCDEF";
    
    for (unsigned char c : text) {
        // Unreserved characters (RFC 3986)
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || 
            c == '.' || c == '~') {
            encoded += c;
        } else if (c == ' ') {
            encoded += '+';  // Space as plus (form encoding)
        } else {
            // Percent-encode
            encoded += '%';
            encoded += hex[(c >> 4) & 0x0F];
            encoded += hex[c & 0x0F];
        }
    }
    
    return encoded;
}

String KetiveeSearchEngine::extractDomain(const String& url) const {
    size_t start = url.find("://");
    if (start == String::npos) {
        start = 0;
    } else {
        start += 3;
    }
    
    size_t end = url.find('/', start);
    if (end == String::npos) {
        end = url.length();
    }
    
    return url.substr(start, end - start);
}

bool KetiveeSearchEngine::isValidUrl(const String& url) const {
    return url.find("http://") == 0 || url.find("https://") == 0 || url.find("file://") == 0;
}

String KetiveeSearchEngine::sanitizeQuery(const String& query) const {
    String sanitized = query;
    
    // Remove special characters that might cause issues
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
        [](char c) { return !std::isprint(c); }), sanitized.end());
    
    // Trim whitespace
    sanitized.erase(0, sanitized.find_first_not_of(" \t\r\n"));
    sanitized.erase(sanitized.find_last_not_of(" \t\r\n") + 1);
    
    return sanitized;
}

// SearchManager Implementation
SearchManager::SearchManager() {
    loadDefaultEngines();
}

void SearchManager::addEngine(std::shared_ptr<SearchEngine> engine) {
    if (engine) {
        engines[engine->getName()] = engine;
    }
}

void SearchManager::removeEngine(const String& name) {
    engines.erase(name);
}

std::shared_ptr<SearchEngine> SearchManager::getEngine(const String& name) {
    auto it = engines.find(name);
    return it != engines.end() ? it->second : nullptr;
}

std::vector<String> SearchManager::getAvailableEngines() const {
    std::vector<String> names;
    for (const auto& engine : engines) {
        names.push_back(engine.first);
    }
    return names;
}

void SearchManager::setDefaultEngine(const String& name) {
    if (engines.find(name) != engines.end()) {
        defaultEngine = name;
    }
}

String SearchManager::getDefaultEngine() const {
    return defaultEngine;
}

std::shared_ptr<SearchEngine> SearchManager::getDefaultEngine() {
    if (defaultEngine.empty() && !engines.empty()) {
        return engines.begin()->second;
    }
    return getEngine(defaultEngine);
}

std::vector<SearchResult> SearchManager::search(const String& query) {
    auto engine = getDefaultEngine();
    if (engine) {
        return engine->search(query);
    }
    return std::vector<SearchResult>();
}

std::vector<SearchResult> SearchManager::search(const SearchQuery& query) {
    auto engine = getDefaultEngine();
    if (engine) {
        return engine->search(query);
    }
    return std::vector<SearchResult>();
}

std::vector<SearchResult> SearchManager::searchAll(const String& query) {
    std::vector<std::vector<SearchResult>> allResults;
    
    for (const auto& engine : engines) {
        auto results = engine.second->search(query);
        allResults.push_back(results);
    }
    
    return search_utils::mergeResults(allResults);
}

std::vector<String> SearchManager::getSuggestions(const String& query) {
    auto engine = getDefaultEngine();
    if (engine) {
        return engine->getSuggestions(query);
    }
    return std::vector<String>();
}

std::vector<String> SearchManager::getTrendingSearches() {
    auto engine = getDefaultEngine();
    if (engine) {
        return engine->getTrendingSearches();
    }
    return std::vector<String>();
}

void SearchManager::loadConfiguration(const String& filename) {
    try {
        std::ifstream file(filename);
        if (file.is_open()) {
            json j = json::parse(file);
            if (j.contains("default_engine")) {
                defaultEngine = j["default_engine"];
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load configuration: " << e.what() << std::endl;
    }
}

void SearchManager::saveConfiguration(const String& filename) const {
    try {
        json j;
        j["default_engine"] = defaultEngine;
        
        std::ofstream file(filename);
        if (file.is_open()) {
            file << j.dump(2);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to save configuration: " << e.what() << std::endl;
    }
}

void SearchManager::addToHistory(const String& query) {
    // Remove if already exists
    auto it = std::find(searchHistory.begin(), searchHistory.end(), query);
    if (it != searchHistory.end()) {
        searchHistory.erase(it);
    }
    
    // Add to front
    searchHistory.insert(searchHistory.begin(), query);
    
    // Limit history size
    if (searchHistory.size() > 100) {
        searchHistory.resize(100);
    }
}

std::vector<String> SearchManager::getSearchHistory() const {
    return searchHistory;
}

void SearchManager::clearSearchHistory() {
    searchHistory.clear();
}

void SearchManager::addBookmark(const SearchResult& result) {
    // Check if already bookmarked
    for (const auto& bookmark : bookmarks) {
        if (bookmark.url == result.url) {
            return;
        }
    }
    
    bookmarks.push_back(result);
}

void SearchManager::removeBookmark(const String& url) {
    bookmarks.erase(
        std::remove_if(bookmarks.begin(), bookmarks.end(),
            [&url](const SearchResult& result) { return result.url == url; }),
        bookmarks.end()
    );
}

std::vector<SearchResult> SearchManager::getBookmarks() const {
    return bookmarks;
}

void SearchManager::loadDefaultEngines() {
    // Add KetiveeSearch as the default engine
    auto ketiveeSearch = std::make_shared<KetiveeSearchEngine>();
    addEngine(ketiveeSearch);
    setDefaultEngine("KetiveeSearch");
}

// Search Utilities Implementation
namespace search_utils {
    String normalizeQuery(const String& query) {
        String normalized = query;
        
        // Convert to lowercase
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        
        // Remove extra whitespace
        // TODO: Implement html_utils::normalizeWhitespace or use std::regex
        // normalized = html_utils::normalizeWhitespace(normalized);
        
        // Remove special characters
        normalized.erase(std::remove_if(normalized.begin(), normalized.end(), 
            [](char c) { return !std::isalnum(c) && !std::isspace(c); }), normalized.end());
        
        return normalized;
    }
    
    std::vector<String> tokenizeQuery(const String& query) {
        std::vector<String> tokens;
        std::istringstream iss(query);
        String token;
        
        while (iss >> token) {
            // Remove punctuation
            token.erase(std::remove_if(token.begin(), token.end(), ::ispunct), token.end());
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        
        return tokens;
    }
    
    String buildQueryString(const std::vector<String>& tokens) {
        String query;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) query += " ";
            query += tokens[i];
        }
        return query;
    }
    
    std::vector<SearchResult> mergeResults(const std::vector<std::vector<SearchResult>>& results) {
        std::vector<SearchResult> merged;
        
        for (const auto& resultSet : results) {
            merged.insert(merged.end(), resultSet.begin(), resultSet.end());
        }
        
        return deduplicateResults(merged);
    }
    
    std::vector<SearchResult> deduplicateResults(const std::vector<SearchResult>& results) {
        std::vector<SearchResult> uniqueResults;
        std::unordered_set<String> seenUrls;
        
        for (const auto& result : results) {
            if (seenUrls.find(result.url) == seenUrls.end()) {
                seenUrls.insert(result.url);
                uniqueResults.push_back(result);
            }
        }
        
        return uniqueResults;
    }
    
    std::vector<SearchResult> sortByRelevance(const std::vector<SearchResult>& results) {
        std::vector<SearchResult> sorted = results;
        std::sort(sorted.begin(), sorted.end(),
            [](const SearchResult& a, const SearchResult& b) {
                return a.relevance > b.relevance;
            });
        return sorted;
    }
    
    String normalizeUrl(const String& url) {
        String normalized = url;
        
        // Ensure protocol
        if (normalized.find("://") == String::npos) {
            normalized = "https://" + normalized;
        }
        
        // Remove trailing slash
        if (normalized.back() == '/') {
            normalized.pop_back();
        }
        
        return normalized;
    }
    
    String extractDomain(const String& url) {
        size_t start = url.find("://");
        if (start == String::npos) {
            start = 0;
        } else {
            start += 3;
        }
        
        size_t end = url.find('/', start);
        if (end == String::npos) {
            end = url.length();
        }
        
        return url.substr(start, end - start);
    }
    
    bool isValidUrl(const String& url) {
        return url.find("http://") == 0 || url.find("https://") == 0 || url.find("file://") == 0;
    }
    
    String extractTextContent(const String& html) {
        String content = html;
        
        // Remove HTML tags
        // TODO: Implement proper HTML tag removal
        // content = html_utils::removeTags(content);
        
        // Unescape HTML entities
        // TODO: Implement html_utils::unescapeHtml or use a library
        // content = html_utils::unescapeHtml(content);
        
        // Normalize whitespace
        // TODO: Implement html_utils::normalizeWhitespace
        // return html_utils::normalizeWhitespace(content);
        
        return content;
    }
    
    String generateSnippet(const String& content, const String& query, int maxLength) {
        if (content.length() <= maxLength) {
            return content;
        }
        
        // Find the best position to start the snippet
        size_t start = 0;
        if (!query.empty()) {
            size_t pos = content.find(query);
            if (pos != String::npos) {
                start = pos;
                if (start > 50) {
                    start -= 50;
                }
            }
        }
        
        String snippet = content.substr(start, maxLength);
        
        // Add ellipsis if truncated
        if (start > 0) {
            snippet = "..." + snippet;
        }
        if (start + maxLength < content.length()) {
            snippet += "...";
        }
        
        return snippet;
    }
    
    String extractTitle(const String& html) {
        size_t start = html.find("<title>");
        if (start == String::npos) return "";
        
        start += 7; // Length of "<title>"
        size_t end = html.find("</title>", start);
        if (end == String::npos) return "";
        
        // TODO: Implement html_utils::unescapeHtml
        // return html_utils::unescapeHtml(html.substr(start, end - start));
        return html.substr(start, end - start);
    }
    
    float calculateRelevance(const String& query, const String& text) {
        // Simple relevance calculation based on word frequency
        String lowerQuery = query;
        String lowerText = text;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
        size_t pos = 0;
        float relevance = 0.0f;
        while ((pos = lowerText.find(lowerQuery, pos)) != String::npos) {
            relevance += 1.0f;
            pos += 1;
        }
        return relevance;
    }
    
    float calculateRelevance(const String& query, const SearchResult& result) {
        float relevance = 0.0f;
        relevance += calculateRelevance(query, result.title) * 3.0f;
        relevance += calculateRelevance(query, result.description) * 2.0f;
        relevance += calculateRelevance(query, result.url) * 1.0f;
        return relevance;
    }
    
    float calculatePageRank(const String& url) {
        // Simple page rank calculation (in a real implementation, this would be more sophisticated)
        String domain = extractDomain(url);
        
        // Give higher rank to well-known domains
        if (domain.find("ketivee.org") != String::npos) return 0.9f;
        if (domain.find("github.com") != String::npos) return 0.8f;
        if (domain.find("stackoverflow.com") != String::npos) return 0.8f;
        if (domain.find("wikipedia.org") != String::npos) return 0.7f;
        
        return 0.5f;
    }
    
    float calculateFreshness(const SearchResult& result) {
        if (result.timestamp == 0) {
            return 0.5f; // Default freshness for results without timestamp
        }
        
        auto now = std::chrono::system_clock::now();
        auto resultTime = std::chrono::system_clock::from_time_t(result.timestamp);
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - resultTime);
        
        // Decay freshness over time
        float freshness = std::exp(-duration.count() / 8760.0f); // 8760 hours = 1 year
        return std::max(freshness, 0.1f);
    }
    
    std::vector<SearchResult> filterByDomain(const std::vector<SearchResult>& results, const String& domain) {
        std::vector<SearchResult> filtered;
        
        for (const auto& result : results) {
            if (extractDomain(result.url) == domain) {
                filtered.push_back(result);
            }
        }
        
        return filtered;
    }
    
    std::vector<SearchResult> filterByDate(const std::vector<SearchResult>& results, uint64_t minDate) {
        std::vector<SearchResult> filtered;
        
        for (const auto& result : results) {
            if (result.timestamp >= minDate) {
                filtered.push_back(result);
            }
        }
        
        return filtered;
    }
    
    std::vector<SearchResult> filterSafeSearch(const std::vector<SearchResult>& results, bool safeSearch) {
        if (!safeSearch) {
            return results; // Return all results if safe search is disabled
        }
        
        std::vector<SearchResult> filtered;
        
        for (const auto& result : results) {
            // Simple content filtering (in a real implementation, this would be more sophisticated)
            String content = result.title + " " + result.description;
            std::transform(content.begin(), content.end(), content.begin(), ::tolower);
            
            // Filter out potentially inappropriate content
            if (content.find("adult") == String::npos &&
                content.find("porn") == String::npos &&
                content.find("explicit") == String::npos) {
                filtered.push_back(result);
            }
        }
        
        return filtered;
    }
}

} // namespace zepra