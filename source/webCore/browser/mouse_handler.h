// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file mouse_handler.h
 * @brief Mouse input handling for ZepraBrowser
 * 
 * Handles:
 * - Right-click context menu (Copy, Paste, Inspect, Select All)
 * - Text selection
 * - Clipboard operations
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <cstdint>

namespace ZepraBrowser {

// Context menu actions
enum class ContextMenuAction {
    None,
    // Text actions
    Copy,
    Cut,
    Paste,
    SelectAll,
    // Browser actions
    Inspect,
    ViewSource,
    Back,
    Forward,
    Reload,
    // Link actions
    OpenInNewTab,
    CopyLink,
    // Search action
    SearchKetivee,
    // Image actions
    SaveImage,
    CopyImage,
    OpenImageInPreview,
    // Video actions
    OpenVideoInNewTab,
    DownloadVideo,
    OpenVideoInPreview
};

// Context menu item
struct ContextMenuItem {
    std::string label;
    ContextMenuAction action;
    bool enabled = true;
    std::string shortcut;  // e.g., "Ctrl+C"
};

// Context menu state
class ContextMenu {
public:
    bool visible = false;
    float x = 0, y = 0;
    float width = 180;
    float itemHeight = 32;
    
    std::vector<ContextMenuItem> items;
    int hoveredIndex = -1;
    
    void show(float posX, float posY);
    void hide();
    int getItemAt(float mouseX, float mouseY);
    ContextMenuAction handleClick(float mouseX, float mouseY);
};

// Text selection state
struct TextSelection {
    bool active = false;
    float startX = 0, startY = 0;
    float endX = 0, endY = 0;
    std::string selectedText;
    
    void start(float x, float y);
    void update(float x, float y);
    void end();
    void clear();
};

// Mouse handler class
class MouseHandler {
public:
    // Callbacks
    using CopyCallback = std::function<void(const std::string&)>;
    using PasteCallback = std::function<std::string()>;
    using InspectCallback = std::function<void(float x, float y)>;
    using NavigateCallback = std::function<void(int direction)>;  // -1=back, 1=forward
    using ReloadCallback = std::function<void()>;
    // Text Selection Callback
    using GetTextCallback = std::function<std::string(float, float, float, float)>;
    // New callbacks for enhanced context menu
    using NewTabCallback = std::function<void(const std::string& url)>;
    using SearchCallback = std::function<void(const std::string& query)>;
    using DownloadCallback = std::function<void(const std::string& url)>;
    
    MouseHandler();
    
    // Set callbacks
    void onCopy(CopyCallback cb) { copyCallback = cb; }
    void onPaste(PasteCallback cb) { pasteCallback = cb; }
    void onInspect(InspectCallback cb) { inspectCallback = cb; }
    void onNavigate(NavigateCallback cb) { navigateCallback = cb; }
    void onReload(ReloadCallback cb) { reloadCallback = cb; }
    void onGetText(GetTextCallback cb) { getTextCallback = cb; }
    // New callback setters
    void onNewTab(NewTabCallback cb) { newTabCallback = cb; }
    void onSearch(SearchCallback cb) { searchCallback = cb; }
    void onDownload(DownloadCallback cb) { downloadCallback = cb; }
    
    // Handle mouse events
    void handleLeftClick(float x, float y);
    void handleRightClick(float x, float y);
    void handleRightClickOnLink(float x, float y, const std::string& url);
    void handleRightClickOnImage(float x, float y, const std::string& imageUrl);
    void handleRightClickOnVideo(float x, float y, const std::string& videoUrl);
    void handleRightClickOnSelection(float x, float y, const std::string& selectedText);
    void handleMouseMove(float x, float y, bool leftDown);
    void handleLeftRelease(float x, float y);
    
    // Get state
    ContextMenu& getContextMenu() { return contextMenu; }
    TextSelection& getSelection() { return selection; }
    
    // Execute action
    void executeAction(ContextMenuAction action);
    
private:
    ContextMenu contextMenu;
    TextSelection selection;
    
    CopyCallback copyCallback;
    PasteCallback pasteCallback;
    InspectCallback inspectCallback;
    NavigateCallback navigateCallback;
    ReloadCallback reloadCallback;
    GetTextCallback getTextCallback;
    // New callbacks
    NewTabCallback newTabCallback;
    SearchCallback searchCallback;
    DownloadCallback downloadCallback;
    
    // Current context for actions
    std::string currentLinkUrl_;
    std::string currentImageUrl_;
    std::string currentVideoUrl_;
    std::string currentSelectedText_;
    
    void buildContextMenu(float x, float y);
};

// Render context menu (called from main render function)
void renderContextMenu(const ContextMenu& menu,
                       void (*drawRect)(float x, float y, float w, float h, uint32_t color),
                       void (*drawText)(const std::string& text, float x, float y, uint32_t color));

// Render text selection highlight
void renderSelection(const TextSelection& selection,
                     void (*drawRect)(float x, float y, float w, float h, uint32_t color));

} // namespace ZepraBrowser
