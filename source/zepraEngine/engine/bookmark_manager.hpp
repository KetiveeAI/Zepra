/**
 * @file bookmark_manager.hpp
 * @brief Bookmark management
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>

namespace Zepra::Engine {

struct Bookmark {
    int64_t id;
    std::string url;
    std::string title;
    std::string faviconUrl;
    int64_t folderId;  // -1 for root
    std::chrono::system_clock::time_point dateAdded;
    int position;
};

struct BookmarkFolder {
    int64_t id;
    std::string name;
    int64_t parentId;  // -1 for root
    std::chrono::system_clock::time_point dateCreated;
    std::vector<Bookmark> bookmarks;
    std::vector<BookmarkFolder> subfolders;
};

/**
 * @class BookmarkManager
 * @brief Manages bookmarks and folders
 */
class BookmarkManager {
public:
    BookmarkManager();
    ~BookmarkManager();
    
    // Bookmarks
    int64_t addBookmark(const std::string& url, const std::string& title, 
                        int64_t folderId = -1);
    void removeBookmark(int64_t id);
    void updateBookmark(int64_t id, const std::string& title);
    void moveBookmark(int64_t id, int64_t folderId, int position = -1);
    
    std::optional<Bookmark> getBookmark(int64_t id);
    std::vector<Bookmark> getBookmarksInFolder(int64_t folderId = -1);
    std::vector<Bookmark> searchBookmarks(const std::string& query);
    bool isBookmarked(const std::string& url);
    
    // Folders
    int64_t createFolder(const std::string& name, int64_t parentId = -1);
    void removeFolder(int64_t id);
    void renameFolder(int64_t id, const std::string& name);
    BookmarkFolder getRootFolder();
    std::optional<BookmarkFolder> getFolder(int64_t id);
    
    // Import/Export
    bool importFromHtml(const std::string& path);
    bool exportToHtml(const std::string& path);
    
    // Persistence
    bool load(const std::string& path);
    bool save(const std::string& path);
    
    // Callback
    using ChangeCallback = std::function<void()>;
    void setOnChange(ChangeCallback callback) { onChange_ = std::move(callback); }
    
private:
    std::vector<Bookmark> bookmarks_;
    std::vector<BookmarkFolder> folders_;
    int64_t nextBookmarkId_ = 1;
    int64_t nextFolderId_ = 1;
    ChangeCallback onChange_;
};

BookmarkManager& getBookmarkManager();

} // namespace Zepra::Engine
