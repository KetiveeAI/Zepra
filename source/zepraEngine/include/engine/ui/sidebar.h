/**
 * @file sidebar.h
 * @brief Collapsible sidebar for bookmarks, history, and downloads
 */

#ifndef ZEPRA_UI_SIDEBAR_H
#define ZEPRA_UI_SIDEBAR_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zepra {
namespace ui {

/**
 * Sidebar section types
 */
enum class SidebarSection {
    Bookmarks,
    History,
    Downloads,
    ReadingList
};

/**
 * Sidebar item (bookmark, history entry, etc.)
 */
struct SidebarItem {
    std::string id;
    std::string title;
    std::string url;
    std::string icon;           // Favicon path
    std::string parentId;       // For folder hierarchy
    bool isFolder = false;
    bool isExpanded = false;
    int64_t timestamp = 0;      // For history sorting
    float downloadProgress = 0; // For downloads (0-1)
    std::string downloadStatus; // "downloading", "complete", "failed"
};

/**
 * Sidebar configuration
 */
struct SidebarConfig {
    float width = 280.0f;
    float collapsedWidth = 0.0f;
    float animationDuration = 200.0f; // ms
    bool showSearch = true;
    bool startCollapsed = true;
    SidebarSection defaultSection = SidebarSection::Bookmarks;
};

/**
 * Sidebar - Left panel component for browser
 * 
 * Features:
 * - Collapsible with animation
 * - Multiple sections (Bookmarks, History, Downloads)
 * - Tree view for bookmark folders
 * - Search within section
 * - Drag and drop for bookmarks
 */
class Sidebar {
public:
    using ItemClickCallback = std::function<void(const SidebarItem& item)>;
    using SectionCallback = std::function<void(SidebarSection section)>;
    using ToggleCallback = std::function<void(bool expanded)>;

    Sidebar();
    explicit Sidebar(const SidebarConfig& config);
    ~Sidebar();

    // Configuration
    void setConfig(const SidebarConfig& config);
    SidebarConfig getConfig() const;

    // Visibility
    void show();
    void hide();
    void toggle();
    bool isVisible() const;
    bool isAnimating() const;
    float getCurrentWidth() const;

    // Section management
    void setSection(SidebarSection section);
    SidebarSection getCurrentSection() const;

    // Content
    void setItems(SidebarSection section, const std::vector<SidebarItem>& items);
    void addItem(SidebarSection section, const SidebarItem& item);
    void removeItem(SidebarSection section, const std::string& id);
    void updateItem(SidebarSection section, const SidebarItem& item);
    void clearItems(SidebarSection section);

    // Folder operations
    void expandFolder(const std::string& id);
    void collapseFolder(const std::string& id);
    void toggleFolder(const std::string& id);

    // Search
    void setSearchQuery(const std::string& query);
    std::string getSearchQuery() const;

    // Callbacks
    void setItemClickCallback(ItemClickCallback callback);
    void setSectionCallback(SectionCallback callback);
    void setToggleCallback(ToggleCallback callback);

    // Rendering
    void setBounds(float x, float y, float width, float height);
    void render();
    void update(float deltaTime); // For animations

    // Event handling
    bool handleMouseClick(float x, float y);
    bool handleMouseMove(float x, float y);
    bool handleMouseDown(float x, float y);
    bool handleMouseUp(float x, float y);
    bool handleKeyPress(int keyCode, bool ctrl, bool shift);
    bool handleScroll(float x, float y, float delta);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ui
} // namespace zepra

#endif // ZEPRA_UI_SIDEBAR_H
