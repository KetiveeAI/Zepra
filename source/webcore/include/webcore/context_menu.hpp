/**
 * @file context_menu.hpp
 * @brief Right-click context menu for browser
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <SDL2/SDL.h>

namespace Zepra::WebCore {

enum class MenuItemType {
    Normal,
    Separator,
    Submenu
};

struct MenuItem {
    std::string id;
    std::string label;
    std::string shortcut;
    MenuItemType type = MenuItemType::Normal;
    bool enabled = true;
    bool visible = true;
    std::function<void()> action;
    std::vector<MenuItem> submenu;
};

/**
 * @class ContextMenu
 * @brief Popup context menu with common browser actions
 */
class ContextMenu {
public:
    ContextMenu();
    ~ContextMenu() = default;
    
    // Show menu at position
    void show(int x, int y);
    void hide();
    bool isVisible() const { return visible_; }
    
    // Configuration
    void setHasSelection(bool has) { hasSelection_ = has; }
    void setSelectedText(const std::string& text) { selectedText_ = text; }
    void setCanGoBack(bool can) { canGoBack_ = can; }
    void setCanGoForward(bool can) { canGoForward_ = can; }
    void setIsEditable(bool editable) { isEditable_ = editable; }
    void setPageUrl(const std::string& url) { pageUrl_ = url; }
    
    // Callbacks
    using CopyCallback = std::function<void()>;
    using PasteCallback = std::function<void()>;
    using SearchCallback = std::function<void(const std::string&)>;
    using InspectCallback = std::function<void()>;
    using ViewSourceCallback = std::function<void()>;
    using BackCallback = std::function<void()>;
    using ForwardCallback = std::function<void()>;
    using ReloadCallback = std::function<void()>;
    using SaveCallback = std::function<void()>;
    using PrintCallback = std::function<void()>;
    
    void setOnCopy(CopyCallback cb) { onCopy_ = std::move(cb); }
    void setOnPaste(PasteCallback cb) { onPaste_ = std::move(cb); }
    void setOnSearch(SearchCallback cb) { onSearch_ = std::move(cb); }
    void setOnInspect(InspectCallback cb) { onInspect_ = std::move(cb); }
    void setOnViewSource(ViewSourceCallback cb) { onViewSource_ = std::move(cb); }
    void setOnBack(BackCallback cb) { onBack_ = std::move(cb); }
    void setOnForward(ForwardCallback cb) { onForward_ = std::move(cb); }
    void setOnReload(ReloadCallback cb) { onReload_ = std::move(cb); }
    void setOnSave(SaveCallback cb) { onSave_ = std::move(cb); }
    void setOnPrint(PrintCallback cb) { onPrint_ = std::move(cb); }
    
    // Event handling
    bool handleMouseDown(int x, int y, int button);
    bool handleMouseMove(int x, int y);
    
    // Rendering
    void render(SDL_Renderer* renderer, TTF_Font* font);
    
    // Get menu bounds
    SDL_Rect bounds() const { return {x_, y_, width_, height_}; }
    
private:
    void buildMenu();
    void executeItem(int index);
    int getItemAtY(int y);
    
    std::vector<MenuItem> items_;
    
    int x_ = 0, y_ = 0;
    int width_ = 200;
    int height_ = 0;
    int itemHeight_ = 28;
    int hoverIndex_ = -1;
    
    bool visible_ = false;
    bool hasSelection_ = false;
    bool isEditable_ = false;
    bool canGoBack_ = false;
    bool canGoForward_ = false;
    
    std::string selectedText_;
    std::string pageUrl_;
    
    CopyCallback onCopy_;
    PasteCallback onPaste_;
    SearchCallback onSearch_;
    InspectCallback onInspect_;
    ViewSourceCallback onViewSource_;
    BackCallback onBack_;
    ForwardCallback onForward_;
    ReloadCallback onReload_;
    SaveCallback onSave_;
    PrintCallback onPrint_;
};

} // namespace Zepra::WebCore
