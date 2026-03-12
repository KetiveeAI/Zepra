// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file mouse_handler.cpp
 * @brief Mouse input handling implementation
 */

#include "mouse_handler.h"
#include <iostream>

namespace ZepraBrowser {

// =============================================================================
// ContextMenu
// =============================================================================

void ContextMenu::show(float posX, float posY) {
    x = posX;
    y = posY;
    visible = true;
    hoveredIndex = -1;
    
    // Build default items
    items.clear();
    items.push_back({"Back", ContextMenuAction::Back, true, "Alt+←"});
    items.push_back({"Forward", ContextMenuAction::Forward, true, "Alt+→"});
    items.push_back({"Reload", ContextMenuAction::Reload, true, "F5"});
    items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    items.push_back({"Copy", ContextMenuAction::Copy, true, "Ctrl+C"});
    items.push_back({"Paste", ContextMenuAction::Paste, true, "Ctrl+V"});
    items.push_back({"Select All", ContextMenuAction::SelectAll, true, "Ctrl+A"});
    items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    items.push_back({"Inspect", ContextMenuAction::Inspect, true, "F12"});
    items.push_back({"View Source", ContextMenuAction::ViewSource, true, "Ctrl+U"});
}

void ContextMenu::hide() {
    visible = false;
    hoveredIndex = -1;
}

int ContextMenu::getItemAt(float mouseX, float mouseY) {
    if (!visible) return -1;
    
    if (mouseX < x || mouseX > x + width) return -1;
    
    float itemY = y;
    for (size_t i = 0; i < items.size(); i++) {
        float h = items[i].label.empty() ? 8 : itemHeight;  // Separator is 8px
        if (mouseY >= itemY && mouseY < itemY + h) {
            if (items[i].label.empty() || !items[i].enabled) return -1;
            return (int)i;
        }
        itemY += h;
    }
    return -1;
}

ContextMenuAction ContextMenu::handleClick(float mouseX, float mouseY) {
    int idx = getItemAt(mouseX, mouseY);
    hide();
    if (idx >= 0 && idx < (int)items.size()) {
        return items[idx].action;
    }
    return ContextMenuAction::None;
}

// =============================================================================
// TextSelection
// =============================================================================

void TextSelection::start(float x, float y) {
    active = true;
    startX = x;
    startY = y;
    endX = x;
    endY = y;
    selectedText.clear();
}

void TextSelection::update(float x, float y) {
    if (active) {
        endX = x;
        endY = y;
    }
}

void TextSelection::end() {
    active = false;
}

void TextSelection::clear() {
    active = false;
    startX = startY = endX = endY = 0;
    selectedText.clear();
}

// =============================================================================
// MouseHandler
// =============================================================================

MouseHandler::MouseHandler() {
    // Initialize with empty callbacks
}

void MouseHandler::handleLeftClick(float x, float y) {
    // Hide context menu on left click
    if (contextMenu.visible) {
        contextMenu.hide();
    }
    
    // Start text selection
    selection.start(x, y);
}

void MouseHandler::handleRightClick(float x, float y) {
    // Show context menu
    contextMenu.show(x, y);
    std::cout << "[Mouse] Context menu opened at (" << x << ", " << y << ")" << std::endl;
}

void MouseHandler::handleRightClickOnLink(float x, float y, const std::string& url) {
    currentLinkUrl_ = url;
    contextMenu.items.clear();
    
    contextMenu.items.push_back({"Open in New Tab", ContextMenuAction::OpenInNewTab, true, ""});
    contextMenu.items.push_back({"Copy Link Address", ContextMenuAction::CopyLink, true, ""});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Back", ContextMenuAction::Back, true, "Alt+←"});
    contextMenu.items.push_back({"Forward", ContextMenuAction::Forward, true, "Alt+→"});
    contextMenu.items.push_back({"Reload", ContextMenuAction::Reload, true, "F5"});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Inspect", ContextMenuAction::Inspect, true, "F12"});
    
    contextMenu.x = x;
    contextMenu.y = y;
    contextMenu.visible = true;
    std::cout << "[Mouse] Link context menu for: " << url << std::endl;
}

void MouseHandler::handleRightClickOnImage(float x, float y, const std::string& imageUrl) {
    currentImageUrl_ = imageUrl;
    contextMenu.items.clear();
    
    contextMenu.items.push_back({"Open Image in New Tab", ContextMenuAction::OpenInNewTab, true, ""});
    contextMenu.items.push_back({"Open Image Preview", ContextMenuAction::OpenImageInPreview, true, ""});
    contextMenu.items.push_back({"Save Image As...", ContextMenuAction::SaveImage, true, ""});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Copy Image", ContextMenuAction::CopyImage, true, ""});
    contextMenu.items.push_back({"Copy Image Address", ContextMenuAction::CopyLink, true, ""});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Inspect", ContextMenuAction::Inspect, true, "F12"});
    
    contextMenu.x = x;
    contextMenu.y = y;
    contextMenu.visible = true;
    std::cout << "[Mouse] Image context menu for: " << imageUrl << std::endl;
}

void MouseHandler::handleRightClickOnVideo(float x, float y, const std::string& videoUrl) {
    currentVideoUrl_ = videoUrl;
    contextMenu.items.clear();
    
    contextMenu.items.push_back({"Open Video in New Tab", ContextMenuAction::OpenVideoInNewTab, true, ""});
    contextMenu.items.push_back({"Open Video Preview", ContextMenuAction::OpenVideoInPreview, true, ""});
    contextMenu.items.push_back({"Download Video", ContextMenuAction::DownloadVideo, true, ""});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Copy Video Address", ContextMenuAction::CopyLink, true, ""});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Inspect", ContextMenuAction::Inspect, true, "F12"});
    
    contextMenu.x = x;
    contextMenu.y = y;
    contextMenu.visible = true;
    std::cout << "[Mouse] Video context menu for: " << videoUrl << std::endl;
}

void MouseHandler::handleRightClickOnSelection(float x, float y, const std::string& selectedText) {
    currentSelectedText_ = selectedText;
    contextMenu.items.clear();
    
    // Truncate for display
    std::string displayText = selectedText;
    if (displayText.length() > 25) {
        displayText = displayText.substr(0, 22) + "...";
    }
    
    contextMenu.items.push_back({"Search Ketivee for \"" + displayText + "\"", ContextMenuAction::SearchKetivee, true, ""});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Cut", ContextMenuAction::Cut, true, "Ctrl+X"});
    contextMenu.items.push_back({"Copy", ContextMenuAction::Copy, true, "Ctrl+C"});
    contextMenu.items.push_back({"Paste", ContextMenuAction::Paste, true, "Ctrl+V"});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Select All", ContextMenuAction::SelectAll, true, "Ctrl+A"});
    contextMenu.items.push_back({"", ContextMenuAction::None, false, ""});  // Separator
    contextMenu.items.push_back({"Inspect", ContextMenuAction::Inspect, true, "F12"});
    
    contextMenu.x = x;
    contextMenu.y = y;
    contextMenu.visible = true;
    std::cout << "[Mouse] Text selection context menu for: " << displayText << std::endl;
}

void MouseHandler::handleMouseMove(float x, float y, bool leftDown) {
    // Update text selection if dragging
    if (leftDown && selection.active) {
        selection.update(x, y);
    }
    
    // Update context menu hover
    if (contextMenu.visible) {
        contextMenu.hoveredIndex = contextMenu.getItemAt(x, y);
    }
}

void MouseHandler::handleLeftRelease(float x, float y) {
    // Check if clicking context menu item
    if (contextMenu.visible) {
        ContextMenuAction action = contextMenu.handleClick(x, y);
        if (action != ContextMenuAction::None) {
            executeAction(action);
        }
        return;
    }
    
    // End text selection
    if (selection.active) {
        selection.end();
        
        // Extract text if callback available
        if (getTextCallback) {
            float x1 = std::min(selection.startX, selection.endX);
            float y1 = std::min(selection.startY, selection.endY);
            float x2 = std::max(selection.startX, selection.endX);
            float y2 = std::max(selection.startY, selection.endY);
            float w = x2 - x1;
            float h = y2 - y1;
            
            if (w > 1 && h > 1) { // Min size
                selection.selectedText = getTextCallback(x1, y1, w, h);
                if (!selection.selectedText.empty()) {
                    std::cout << "[Mouse] Selected text (" << selection.selectedText.length() << " chars): " 
                              << (selection.selectedText.length() > 20 ? selection.selectedText.substr(0, 20) + "..." : selection.selectedText) 
                              << std::endl;
                }
            }
        }
    }
}

void MouseHandler::executeAction(ContextMenuAction action) {
    switch (action) {
        case ContextMenuAction::Copy:
            if (copyCallback) {
                copyCallback(selection.selectedText);
            }
            std::cout << "[Mouse] Copy: " << selection.selectedText << std::endl;
            break;
            
        case ContextMenuAction::Paste:
            if (pasteCallback) {
                std::string clipboard = pasteCallback();
                std::cout << "[Mouse] Paste: " << clipboard << std::endl;
            }
            break;
            
        case ContextMenuAction::SelectAll:
            std::cout << "[Mouse] Select All" << std::endl;
            break;
            
        case ContextMenuAction::Inspect:
            if (inspectCallback) {
                inspectCallback(contextMenu.x, contextMenu.y);
            }
            std::cout << "[Mouse] Inspect element" << std::endl;
            break;
            
        case ContextMenuAction::ViewSource:
            std::cout << "[Mouse] View Source" << std::endl;
            break;
            
        case ContextMenuAction::Back:
            if (navigateCallback) navigateCallback(-1);
            std::cout << "[Mouse] Navigate Back" << std::endl;
            break;
            
        case ContextMenuAction::Forward:
            if (navigateCallback) navigateCallback(1);
            std::cout << "[Mouse] Navigate Forward" << std::endl;
            break;
            
        case ContextMenuAction::Reload:
            if (reloadCallback) reloadCallback();
            std::cout << "[Mouse] Reload" << std::endl;
            break;
            
        case ContextMenuAction::OpenInNewTab:
            if (newTabCallback) {
                // Use link URL or image URL depending on context
                std::string url = !currentLinkUrl_.empty() ? currentLinkUrl_ : currentImageUrl_;
                if (!url.empty()) {
                    newTabCallback(url);
                    std::cout << "[Mouse] Open in New Tab: " << url << std::endl;
                }
            }
            break;
            
        case ContextMenuAction::SearchKetivee:
            if (searchCallback && !currentSelectedText_.empty()) {
                searchCallback(currentSelectedText_);
                std::cout << "[Mouse] Search Ketivee for: " << currentSelectedText_ << std::endl;
            }
            break;
            
        case ContextMenuAction::SaveImage:
            if (downloadCallback && !currentImageUrl_.empty()) {
                downloadCallback(currentImageUrl_);
                std::cout << "[Mouse] Save Image: " << currentImageUrl_ << std::endl;
            }
            break;
            
        case ContextMenuAction::CopyLink:
            if (copyCallback) {
                std::string url = !currentLinkUrl_.empty() ? currentLinkUrl_ : 
                                  (!currentImageUrl_.empty() ? currentImageUrl_ : currentVideoUrl_);
                if (!url.empty()) {
                    copyCallback(url);
                    std::cout << "[Mouse] Copy Link: " << url << std::endl;
                }
            }
            break;
            
        case ContextMenuAction::Cut:
            if (copyCallback) {
                copyCallback(selection.selectedText);
                std::cout << "[Mouse] Cut: " << selection.selectedText << std::endl;
            }
            // TODO: Delete the selected text
            break;
            
        case ContextMenuAction::CopyImage:
            if (copyCallback && !currentImageUrl_.empty()) {
                // Copy image data (for now just copy URL)
                copyCallback(currentImageUrl_);
                std::cout << "[Mouse] Copy Image: " << currentImageUrl_ << std::endl;
            }
            break;
            
        case ContextMenuAction::OpenImageInPreview:
            if (newTabCallback && !currentImageUrl_.empty()) {
                // Open in preview tab (special internal URL)
                newTabCallback("zepra://preview?type=image&url=" + currentImageUrl_);
                std::cout << "[Mouse] Open Image Preview: " << currentImageUrl_ << std::endl;
            }
            break;
            
        case ContextMenuAction::OpenVideoInNewTab:
            if (newTabCallback && !currentVideoUrl_.empty()) {
                newTabCallback(currentVideoUrl_);
                std::cout << "[Mouse] Open Video in New Tab: " << currentVideoUrl_ << std::endl;
            }
            break;
            
        case ContextMenuAction::DownloadVideo:
            if (downloadCallback && !currentVideoUrl_.empty()) {
                downloadCallback(currentVideoUrl_);
                std::cout << "[Mouse] Download Video: " << currentVideoUrl_ << std::endl;
            }
            break;
            
        case ContextMenuAction::OpenVideoInPreview:
            if (newTabCallback && !currentVideoUrl_.empty()) {
                // Open in built-in video player preview
                newTabCallback("zepra://preview?type=video&url=" + currentVideoUrl_);
                std::cout << "[Mouse] Open Video Preview: " << currentVideoUrl_ << std::endl;
            }
            break;
            
        default:
            break;
    }
}

// =============================================================================
// Rendering (uses callbacks to avoid direct gfx dependency)
// =============================================================================

void renderContextMenu(const ContextMenu& menu,
                       void (*drawRect)(float x, float y, float w, float h, uint32_t color),
                       void (*drawText)(const std::string& text, float x, float y, uint32_t color)) {
    if (!menu.visible || !drawRect || !drawText) return;
    
    // Calculate total height
    float totalHeight = 0;
    for (const auto& item : menu.items) {
        totalHeight += item.label.empty() ? 8 : menu.itemHeight;
    }
    
    // Draw shadow
    drawRect(menu.x + 4, menu.y + 4, menu.width, totalHeight, 0x40000000);
    
    // Draw background
    drawRect(menu.x, menu.y, menu.width, totalHeight, 0xFFFFFF);
    
    // Draw border
    drawRect(menu.x, menu.y, menu.width, 1, 0xDDDDDD);  // Top
    drawRect(menu.x, menu.y + totalHeight - 1, menu.width, 1, 0xDDDDDD);  // Bottom
    drawRect(menu.x, menu.y, 1, totalHeight, 0xDDDDDD);  // Left
    drawRect(menu.x + menu.width - 1, menu.y, 1, totalHeight, 0xDDDDDD);  // Right
    
    // Draw items
    float itemY = menu.y;
    for (size_t i = 0; i < menu.items.size(); i++) {
        const auto& item = menu.items[i];
        
        if (item.label.empty()) {
            // Separator
            drawRect(menu.x + 8, itemY + 3, menu.width - 16, 1, 0xDDDDDD);
            itemY += 8;
            continue;
        }
        
        // Hover highlight
        if ((int)i == menu.hoveredIndex) {
            drawRect(menu.x + 2, itemY + 2, menu.width - 4, menu.itemHeight - 4, 0xE8F0FE);
        }
        
        // Item text
        uint32_t textColor = item.enabled ? 0x333333 : 0xAAAAAA;
        drawText(item.label, menu.x + 12, itemY + 22, textColor);
        
        // Shortcut
        if (!item.shortcut.empty()) {
            drawText(item.shortcut, menu.x + menu.width - 60, itemY + 22, 0x888888);
        }
        
        itemY += menu.itemHeight;
    }
}

void renderSelection(const TextSelection& selection,
                     void (*drawRect)(float x, float y, float w, float h, uint32_t color)) {
    if (!selection.active || !drawRect) return;
    
    // Draw selection highlight (simplified rectangular selection)
    float x1 = std::min(selection.startX, selection.endX);
    float y1 = std::min(selection.startY, selection.endY);
    float x2 = std::max(selection.startX, selection.endX);
    float y2 = std::max(selection.startY, selection.endY);
    
    // Semi-transparent blue highlight
    drawRect(x1, y1, x2 - x1, y2 - y1, 0x300066FF);
}

} // namespace ZepraBrowser
