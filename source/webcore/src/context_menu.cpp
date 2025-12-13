/**
 * @file context_menu.cpp
 * @brief Context menu implementation
 */

#include "webcore/context_menu.hpp"
#include <SDL2/SDL_ttf.h>

namespace Zepra::WebCore {

ContextMenu::ContextMenu() {
    buildMenu();
}

void ContextMenu::buildMenu() {
    items_.clear();
    
    // Back/Forward (always available)
    items_.push_back({"back", "← Back", "Alt+Left", MenuItemType::Normal, canGoBack_, true, 
        [this]() { if (onBack_) onBack_(); }});
    items_.push_back({"forward", "→ Forward", "Alt+Right", MenuItemType::Normal, canGoForward_, true,
        [this]() { if (onForward_) onForward_(); }});
    items_.push_back({"reload", "↻ Reload", "Ctrl+R", MenuItemType::Normal, true, true,
        [this]() { if (onReload_) onReload_(); }});
    
    items_.push_back({"sep1", "", "", MenuItemType::Separator, true, true, nullptr});
    
    // Text selection options
    if (hasSelection_) {
        items_.push_back({"copy", "Copy", "Ctrl+C", MenuItemType::Normal, true, true,
            [this]() { if (onCopy_) onCopy_(); }});
        
        std::string searchLabel = "Search Ketivee for \"";
        if (selectedText_.length() > 20) {
            searchLabel += selectedText_.substr(0, 17) + "...";
        } else {
            searchLabel += selectedText_;
        }
        searchLabel += "\"";
        
        items_.push_back({"search", searchLabel, "", MenuItemType::Normal, true, true,
            [this]() { if (onSearch_) onSearch_(selectedText_); }});
            
        items_.push_back({"sep2", "", "", MenuItemType::Separator, true, true, nullptr});
    }
    
    // Edit options (for editable fields)
    if (isEditable_) {
        if (!hasSelection_) {
            items_.push_back({"copy", "Copy", "Ctrl+C", MenuItemType::Normal, false, true, nullptr});
        }
        items_.push_back({"paste", "Paste", "Ctrl+V", MenuItemType::Normal, true, true,
            [this]() { if (onPaste_) onPaste_(); }});
        items_.push_back({"selectall", "Select All", "Ctrl+A", MenuItemType::Normal, true, true, nullptr});
        items_.push_back({"sep3", "", "", MenuItemType::Separator, true, true, nullptr});
    }
    
    // Page options
    items_.push_back({"saveas", "Save Page As...", "Ctrl+S", MenuItemType::Normal, true, true,
        [this]() { if (onSave_) onSave_(); }});
    items_.push_back({"print", "Print...", "Ctrl+P", MenuItemType::Normal, true, true,
        [this]() { if (onPrint_) onPrint_(); }});
    
    items_.push_back({"sep4", "", "", MenuItemType::Separator, true, true, nullptr});
    
    // Developer options
    items_.push_back({"viewsource", "View Page Source", "Ctrl+U", MenuItemType::Normal, true, true,
        [this]() { if (onViewSource_) onViewSource_(); }});
    items_.push_back({"inspect", "Inspect", "F12", MenuItemType::Normal, true, true,
        [this]() { if (onInspect_) onInspect_(); }});
    
    // Calculate height
    height_ = 8; // padding
    for (const auto& item : items_) {
        if (!item.visible) continue;
        height_ += (item.type == MenuItemType::Separator) ? 9 : itemHeight_;
    }
}

void ContextMenu::show(int x, int y) {
    buildMenu();
    x_ = x;
    y_ = y;
    visible_ = true;
    hoverIndex_ = -1;
}

void ContextMenu::hide() {
    visible_ = false;
    hoverIndex_ = -1;
}

bool ContextMenu::handleMouseDown(int x, int y, int button) {
    if (!visible_) return false;
    
    SDL_Rect bounds = {x_, y_, width_, height_};
    SDL_Point pt = {x, y};
    
    if (!SDL_PointInRect(&pt, &bounds)) {
        hide();
        return true; // consumed click to close menu
    }
    
    if (button == SDL_BUTTON_LEFT) {
        int index = getItemAtY(y);
        if (index >= 0) {
            executeItem(index);
        }
        hide();
        return true;
    }
    
    return false;
}

bool ContextMenu::handleMouseMove(int x, int y) {
    if (!visible_) return false;
    
    hoverIndex_ = getItemAtY(y);
    return true;
}

int ContextMenu::getItemAtY(int y) {
    int currentY = y_ + 4;
    
    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        if (!item.visible) continue;
        
        int h = (item.type == MenuItemType::Separator) ? 9 : itemHeight_;
        
        if (y >= currentY && y < currentY + h) {
            if (item.type == MenuItemType::Separator || !item.enabled) {
                return -1;
            }
            return static_cast<int>(i);
        }
        currentY += h;
    }
    return -1;
}

void ContextMenu::executeItem(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) return;
    
    const auto& item = items_[index];
    if (item.action && item.enabled) {
        item.action();
    }
}

void ContextMenu::render(SDL_Renderer* renderer, TTF_Font* font) {
    if (!visible_ || !renderer) return;
    
    // Background
    SDL_Rect bg = {x_, y_, width_, height_};
    SDL_SetRenderDrawColor(renderer, 45, 45, 50, 250);
    SDL_RenderFillRect(renderer, &bg);
    
    // Border
    SDL_SetRenderDrawColor(renderer, 80, 80, 90, 255);
    SDL_RenderDrawRect(renderer, &bg);
    
    int currentY = y_ + 4;
    
    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        if (!item.visible) continue;
        
        if (item.type == MenuItemType::Separator) {
            SDL_SetRenderDrawColor(renderer, 70, 70, 80, 255);
            SDL_RenderDrawLine(renderer, x_ + 8, currentY + 4, x_ + width_ - 8, currentY + 4);
            currentY += 9;
            continue;
        }
        
        // Highlight hover
        if (static_cast<int>(i) == hoverIndex_ && item.enabled) {
            SDL_Rect highlight = {x_ + 4, currentY, width_ - 8, itemHeight_};
            SDL_SetRenderDrawColor(renderer, 0, 120, 215, 200);
            SDL_RenderFillRect(renderer, &highlight);
        }
        
        // Label
        if (font) {
            SDL_Color color = item.enabled ? SDL_Color{240, 240, 240, 255} : SDL_Color{120, 120, 120, 255};
            SDL_Surface* surface = TTF_RenderUTF8_Blended(font, item.label.c_str(), color);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_Rect dst = {x_ + 12, currentY + 5, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, nullptr, &dst);
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
            
            // Shortcut (right-aligned)
            if (!item.shortcut.empty()) {
                SDL_Color shortcutColor = {140, 140, 150, 255};
                surface = TTF_RenderUTF8_Blended(font, item.shortcut.c_str(), shortcutColor);
                if (surface) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_Rect dst = {x_ + width_ - surface->w - 12, currentY + 5, surface->w, surface->h};
                    SDL_RenderCopy(renderer, texture, nullptr, &dst);
                    SDL_DestroyTexture(texture);
                    SDL_FreeSurface(surface);
                }
            }
        }
        
        currentY += itemHeight_;
    }
}

} // namespace Zepra::WebCore
