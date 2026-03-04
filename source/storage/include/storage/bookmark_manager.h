/**
 * @file bookmark_manager.h
 * @brief Bookmark storage and management
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace ZepraBrowser {

struct Bookmark {
    std::string url;
    std::string title;
    std::string favicon;  // Base64 encoded or URL
    std::chrono::system_clock::time_point dateAdded;
    std::string folder;   // Empty = root, otherwise folder path
    
    // For organizing
    int id = 0;
    int parentId = 0;  // 0 = root
};

struct BookmarkFolder {
    std::string name;
    int id;
    int parentId;
    std::vector<int> children;  // Bookmark IDs
};

/**
 * Bookmark Manager
 * - Add/remove/search bookmarks
 * - Organize in folders
 * - Persist to disk (JSON)
 * - Import/export
 */
class BookmarkManager {
public:
    static BookmarkManager& instance();
    
    // Bookmark operations
    int addBookmark(const std::string& url, const std::string& title, 
                    const std::string& folder = "");
    bool removeBookmark(int id);
    bool hasBookmark(const std::string& url) const;
    Bookmark* getBookmark(int id);
    std::vector<Bookmark> getAllBookmarks() const;
    std::vector<Bookmark> getBookmarksInFolder(const std::string& folder) const;
    std::vector<Bookmark> searchBookmarks(const std::string& query) const;
    
    // Folder operations
    int createFolder(const std::string& name, int parentId = 0);
    bool removeFolder(int id);
    std::vector<BookmarkFolder> getFolders() const;
    
    // Persistence
    bool load(const std::string& filepath);
    bool save(const std::string& filepath);
    
    // Import/Export
    bool importFromHTML(const std::string& filepath);
    bool exportToHTML(const std::string& filepath);
    bool importFromJSON(const std::string& filepath);
    bool exportToJSON(const std::string& filepath);
    
private:
    BookmarkManager();
    ~BookmarkManager();
    BookmarkManager(const BookmarkManager&) = delete;
    BookmarkManager& operator=(const BookmarkManager&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ZepraBrowser
