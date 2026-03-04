#pragma once

#include "../common/types.h"
#include "../common/constants.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace zepra {

// Forward declarations
class SearchResult;
class SearchQuery;
class SearchEngine;

// Search Result Structure
struct SearchResult {
    String title;
    String url;
    String description;
    String snippet;
    String thumbnail;
    float relevance;
    String source;
    uint64_t timestamp;
    
    SearchResult() : relevance(0.0f), timestamp(0) {}
    SearchResult(const String& t, const String& u, const String& desc)
        : title(t), url(u), description(desc), relevance(0.0f), timestamp(0) {}
};

// Search Query Structure
struct SearchQuery {
    String query;
    String language;
    String region;
    int maxResults;
    int page;
    bool safeSearch;
    String filters;
    
    SearchQuery() : maxResults(20), page(1), safeSearch(true) {}
    SearchQuery(const String& q) : query(q), maxResults(20), page(1), safeSearch(true) {}
};

// Search Engine Interface
class SearchEngine {
public:
    virtual ~SearchEngine() = default;
    
    // Main search interface
    virtual std::vector<SearchResult> search(const SearchQuery& query) = 0;
    virtual std::vector<SearchResult> search(const String& query) = 0;
    
    // Engine information
    virtual String getName() const = 0;
    virtual String getDescription() const = 0;
    virtual String getHomepage() const = 0;
    
    // Configuration
    virtual void setApiKey(const String& key) = 0;
    virtual String getApiKey() const = 0;
    virtual bool isConfigured() const = 0;
    
    // Suggestions
    virtual std::vector<String> getSuggestions(const String& query) = 0;
    
    // Trending searches
    virtual std::vector<String> getTrendingSearches() = 0;
};

// KetiveeSearch Engine Implementation
class KetiveeSearchEngine : public SearchEngine {
public:
    KetiveeSearchEngine();
    ~KetiveeSearchEngine() override = default;
    
    // SearchEngine interface
    std::vector<SearchResult> search(const SearchQuery& query) override;
    std::vector<SearchResult> search(const String& query) override;
    
    String getName() const override { return "KetiveeSearch"; }
    String getDescription() const override { 
        return "Fast, privacy-focused search engine powered by Ketivee"; 
    }
    String getHomepage() const override { return KETIVEE_SEARCH_URL; }
    
    void setApiKey(const String& key) override { apiKey = key; }
    String getApiKey() const override { return apiKey; }
    bool isConfigured() const override { return !apiKey.empty(); }
    
    std::vector<String> getSuggestions(const String& query) override;
    std::vector<String> getTrendingSearches() override;
    
    // KetiveeSearch specific methods
    void setSearchUrl(const String& url) { searchUrl = url; }
    String getSearchUrl() const { return searchUrl; }
    
    void enableLocalSearch(bool enable) { localSearchEnabled = enable; }
    bool isLocalSearchEnabled() const { return localSearchEnabled; }
    
    void setMaxLocalResults(int max) { maxLocalResults = max; }
    int getMaxLocalResults() const { return maxLocalResults; }
    
    // Advanced search features
    std::vector<SearchResult> searchImages(const String& query);
    std::vector<SearchResult> searchVideos(const String& query);
    std::vector<SearchResult> searchNews(const String& query);
    std::vector<SearchResult> searchDocuments(const String& query);
    
    // Search history and personalization
    void addToHistory(const String& query);
    std::vector<String> getSearchHistory() const;
    void clearSearchHistory();
    
    // Bookmarks and favorites
    void addBookmark(const SearchResult& result);
    void removeBookmark(const String& url);
    std::vector<SearchResult> getBookmarks() const;
    bool isBookmarked(const String& url) const;
    
    // Main loop integration
    void update() {}  // Stub for main loop
    
private:
    String apiKey;
    String searchUrl;
    bool localSearchEnabled;
    int maxLocalResults;
    
    // Local search index
    std::unordered_map<String, std::vector<SearchResult>> localIndex;
    std::vector<String> searchHistory;
    std::vector<SearchResult> bookmarks;
    
    // Search methods
    std::vector<SearchResult> performWebSearch(const SearchQuery& query);
    std::vector<SearchResult> performLocalSearch(const SearchQuery& query);
    std::vector<SearchResult> performHybridSearch(const SearchQuery& query);
    
    // API methods
    String buildSearchUrl(const SearchQuery& query) const;
    std::vector<SearchResult> parseSearchResponse(const String& response);
    std::vector<SearchResult> parseJsonResponse(const String& json);
    
    // Local search methods
    void buildLocalIndex();
    void addToLocalIndex(const String& url, const String& title, const String& content);
    float calculateRelevance(const String& query, const String& content);
    
    // Utility methods
    String urlEncode(const String& text) const;
    String extractDomain(const String& url) const;
    bool isValidUrl(const String& url) const;
    String sanitizeQuery(const String& query) const;
};

// Search Manager - Manages multiple search engines
class SearchManager {
public:
    SearchManager();
    ~SearchManager() = default;
    
    // Engine management
    void addEngine(std::shared_ptr<SearchEngine> engine);
    void removeEngine(const String& name);
    std::shared_ptr<SearchEngine> getEngine(const String& name);
    std::vector<String> getAvailableEngines() const;
    
    // Default engine
    void setDefaultEngine(const String& name);
    String getDefaultEngine() const;
    std::shared_ptr<SearchEngine> getDefaultEngine();
    
    // Search operations
    std::vector<SearchResult> search(const String& query);
    std::vector<SearchResult> search(const SearchQuery& query);
    std::vector<SearchResult> searchAll(const String& query);
    
    // Suggestions
    std::vector<String> getSuggestions(const String& query);
    std::vector<String> getTrendingSearches();
    
    // Configuration
    void loadConfiguration(const String& filename);
    void saveConfiguration(const String& filename) const;
    
    // Search history
    void addToHistory(const String& query);
    std::vector<String> getSearchHistory() const;
    void clearSearchHistory();
    
    // Bookmarks
    void addBookmark(const SearchResult& result);
    void removeBookmark(const String& url);
    std::vector<SearchResult> getBookmarks() const;
    
private:
    std::unordered_map<String, std::shared_ptr<SearchEngine>> engines;
    String defaultEngine;
    std::vector<String> searchHistory;
    std::vector<SearchResult> bookmarks;
    
    // Configuration
    void loadDefaultEngines();
    void loadSearchHistory();
    void saveSearchHistory();
    void loadBookmarks();
    void saveBookmarks();
};

// Search Utilities
namespace search_utils {
    // Query processing
    String normalizeQuery(const String& query);
    std::vector<String> tokenizeQuery(const String& query);
    String buildQueryString(const std::vector<String>& tokens);
    
    // Result processing
    std::vector<SearchResult> mergeResults(const std::vector<std::vector<SearchResult>>& results);
    std::vector<SearchResult> deduplicateResults(const std::vector<SearchResult>& results);
    std::vector<SearchResult> sortByRelevance(const std::vector<SearchResult>& results);
    
    // URL processing
    String normalizeUrl(const String& url);
    String extractDomain(const String& url);
    bool isValidUrl(const String& url);
    
    // Content processing
    String extractTextContent(const String& html);
    String generateSnippet(const String& content, const String& query, int maxLength = 200);
    String extractTitle(const String& html);
    
    // Scoring and ranking
    float calculateRelevance(const String& query, const SearchResult& result);
    float calculatePageRank(const String& url);
    float calculateFreshness(const SearchResult& result);
    
    // Filtering
    std::vector<SearchResult> filterByDomain(const std::vector<SearchResult>& results, const String& domain);
    std::vector<SearchResult> filterByDate(const std::vector<SearchResult>& results, uint64_t minDate);
    std::vector<SearchResult> filterSafeSearch(const std::vector<SearchResult>& results, bool safeSearch);
}

// Search Callbacks (specific to search results)
using SearchResultCallback = std::function<void(const std::vector<SearchResult>&)>;
using SuggestionCallback = std::function<void(const std::vector<String>&)>;
using ErrorCallback = std::function<void(const String&)>;

} // namespace zepra 