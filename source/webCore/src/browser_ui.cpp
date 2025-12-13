/**
 * @file browser_ui.cpp
 * @brief Browser UI components implementation
 */

#include "webcore/browser_ui.hpp"
#include "webcore/theme.hpp"
#include "webcore/simple_font.hpp"
#include <algorithm>

namespace Zepra::WebCore {

// =============================================================================
// UIComponent Base
// =============================================================================

UIComponent::UIComponent() : bounds_{0, 0, 100, 30} {}

void UIComponent::setBounds(float x, float y, float width, float height) {
    bounds_ = {x, y, width, height};
}

bool UIComponent::containsPoint(float x, float y) const {
    return x >= bounds_.x && x <= bounds_.x + bounds_.width &&
           y >= bounds_.y && y <= bounds_.y + bounds_.height;
}

bool UIComponent::handleMouseDown(float x, float y, int button) {
    (void)x; (void)y; (void)button;
    return false;
}

bool UIComponent::handleMouseUp(float x, float y, int button) {
    (void)x; (void)y; (void)button;
    return false;
}

bool UIComponent::handleMouseMove(float x, float y) {
    hovered_ = containsPoint(x, y);
    return hovered_;
}

bool UIComponent::handleKeyDown(int keycode, bool shift, bool ctrl, bool alt) {
    (void)keycode; (void)shift; (void)ctrl; (void)alt;
    return false;
}

bool UIComponent::handleTextInput(const std::string& text) {
    (void)text;
    return false;
}

// =============================================================================
// Button
// =============================================================================

Button::Button(const std::string& label) : label_(label) {}

void drawIcon(PaintContext& ctx, IconType type, const Rect& bounds, const Color& color) {
    float cx = bounds.x + bounds.width / 2;
    float cy = bounds.y + bounds.height / 2;
    
    if (type == IconType::Back) {
        // Triangle pointing left
        // Simulate with rects stepping out
        for(int i=0; i<6; i++) {
             ctx.fillRect({cx + i*2 - 4, cy - i*2, 2.0f, (float)(i*4 + 2)}, color);
        }
    } else if (type == IconType::Forward) {
        // Triangle pointing right
         for(int i=0; i<6; i++) {
             ctx.fillRect({cx - i*2 + 4, cy - i*2, 2.0f, (float)(i*4 + 2)}, color);
        }
    } else if (type == IconType::Refresh) {
        // Circular arrow (simplified as box with gap)
        ctx.strokeRect({cx - 6, cy - 6, 12, 12}, color, 2);
        ctx.fillRect({cx + 2, cy - 8, 6, 6}, color); // Arrow head attempt
    } else if (type == IconType::Stop) {
        ctx.fillRect({cx - 5, cy - 5, 10, 10}, color);
    } else if (type == IconType::Home) {
        // House
        ctx.fillRect({cx - 6, cy - 2, 12, 8}, color); // Base
        // Roof
        for(int i=0; i<7; i++) {
            ctx.fillRect({cx - 6 + i, cy - 2 - i, (float)((6-i)*2), 1}, color);
        }
    } else if (type == IconType::Menu) {
        // Hamburger
        ctx.fillRect({cx - 7, cy - 5, 14, 2}, color);
        ctx.fillRect({cx - 7, cy - 1, 14, 2}, color);
        ctx.fillRect({cx - 7, cy + 3, 14, 2}, color);
    } else if (type == IconType::Close) {
        // X
        // Cross
        ctx.fillRect({cx - 4, cy - 1, 8, 2}, color); // Horizontal? No wait, X.
        // Let's do a cross + 
        ctx.drawText("X", cx - 3, cy - 3, color, 10); // Fallback if no simple rect rotate
        // Actually we can just draw pixel steps
        ctx.fillRect({cx-3, cy-3, 2, 2}, color); ctx.fillRect({cx+1, cy-3, 2, 2}, color);
        ctx.fillRect({cx-1, cy-1, 2, 2}, color);
        ctx.fillRect({cx-3, cy+1, 2, 2}, color); ctx.fillRect({cx+1, cy+1, 2, 2}, color);
    }
}

void Button::paint(PaintContext& ctx) {
    if (!visible_) return;
    
    // Theme-based button colors
    Color bgColor = pressed_ ? Theme::Chrome::buttonPressed() :
                    hovered_ ? Theme::Chrome::buttonHover() :
                               Theme::Chrome::buttonDefault();
    ctx.fillRect(bounds_, bgColor);
    
    // Border only when focused
    if (focused_) {
        ctx.strokeRect(bounds_, Theme::primary(), 2);
    }
    
    // Content
    Color iconColor = pressed_ || hovered_ ? Color{255, 255, 255, 255} : Theme::Chrome::tabText();
    if (icon_ != IconType::None) {
        drawIcon(ctx, icon_, bounds_, iconColor);
    } else {
        // Label
        float textWidth = SimpleFont::getTextWidth(label_, 12);
        float textX = bounds_.x + (bounds_.width - textWidth) / 2;
        float textY = bounds_.y + (bounds_.height - SimpleFont::getLineHeight(12)) / 2;
        SimpleFont::drawText(ctx, label_, textX, textY, iconColor, 12);
    }
}

bool Button::handleMouseDown(float x, float y, int button) {
    if (button == 1 && containsPoint(x, y)) {
        pressed_ = true;
        return true;
    }
    return false;
}

bool Button::handleMouseUp(float x, float y, int button) {
    if (button == 1 && pressed_) {
        pressed_ = false;
        if (containsPoint(x, y) && onClick_) {
            onClick_();
        }
        return true;
    }
    return false;
}

bool Button::handleMouseMove(float x, float y) {
    return UIComponent::handleMouseMove(x, y);
}

// =============================================================================
// TextInput
// =============================================================================

TextInput::TextInput() {}

void TextInput::setText(const std::string& text) {
    text_ = text;
    cursorPos_ = text.size();
}

void TextInput::paint(PaintContext& ctx) {
    if (!visible_) return;
    
    // Theme-based address bar
    ctx.fillRect(bounds_, Theme::Chrome::addressBar());
    
    // Border with focus highlight
    Color borderColor = focused_ ? Theme::Chrome::addressBarFocus() : Theme::Chrome::addressBarBorder();
    ctx.strokeRect(bounds_, borderColor, focused_ ? 2 : 1);
    
    // Text or placeholder
    float textX = bounds_.x + 10;
    float textY = bounds_.y + (bounds_.height - SimpleFont::getLineHeight(14)) / 2;
    
    if (text_.empty() && !focused_) {
        SimpleFont::drawText(ctx, placeholder_, textX, textY, Theme::Content::textMuted(), 14);
    } else {
        SimpleFont::drawText(ctx, text_, textX, textY, Theme::Content::textPrimary(), 14);
        
        // Cursor
        if (focused_) {
            float textW = SimpleFont::getTextWidth(text_.substr(0, cursorPos_), 14);
            float cursorX = textX + textW;
            ctx.fillRect({cursorX, bounds_.y + 5, 2, bounds_.height - 10}, Theme::primaryDark());
        }
    }
}

bool TextInput::handleMouseDown(float x, float y, int button) {
    if (button == 1 && containsPoint(x, y)) {
        setFocused(true);
        // Calculate cursor position from click (approx)
        float relX = x - bounds_.x - 10;
        float charW = SimpleFont::getCharWidth(14);
        cursorPos_ = static_cast<size_t>(std::max(0.0f, relX / charW));
        cursorPos_ = std::min(cursorPos_, text_.size());
        return true;
    }
    return false;
}

bool TextInput::handleKeyDown(int keycode, bool shift, bool ctrl, bool alt) {
    if (!focused_) return false;
    (void)shift; (void)alt;
    
    // Backspace
    if (keycode == 8) {
        if (!text_.empty() && cursorPos_ > 0) {
            text_.erase(cursorPos_ - 1, 1);
            cursorPos_--;
            if (onChange_) onChange_(text_);
        }
        return true;
    }
    
    // Delete
    if (keycode == 127) {
        if (cursorPos_ < text_.size()) {
            text_.erase(cursorPos_, 1);
            if (onChange_) onChange_(text_);
        }
        return true;
    }
    
    // Enter
    if (keycode == 13) {
        if (onSubmit_) onSubmit_(text_);
        return true;
    }
    
    // Left arrow
    if (keycode == 1073741904) {
        if (cursorPos_ > 0) cursorPos_--;
        return true;
    }
    
    // Right arrow
    if (keycode == 1073741903) {
        if (cursorPos_ < text_.size()) cursorPos_++;
        return true;
    }
    
    // Ctrl+A select all
    if (ctrl && (keycode == 'a' || keycode == 'A')) {
        selectionStart_ = 0;
        selectionEnd_ = text_.size();
        return true;
    }
    
    // Ctrl+V paste - would need clipboard access
    
    return false;
}

bool TextInput::handleTextInput(const std::string& text) {
    if (!focused_) return false;
    
    text_.insert(cursorPos_, text);
    cursorPos_ += text.size();
    if (onChange_) onChange_(text_);
    
    return true;
}

// =============================================================================
// TabBar
// =============================================================================

TabBar::TabBar() {}

int TabBar::addTab(const std::string& title, const std::string& url) {
    Tab tab;
    tab.title = title;
    tab.url = url;
    tabs_.push_back(tab);
    
    if (activeTab_ < 0) {
        setActiveTab(static_cast<int>(tabs_.size()) - 1);
    }
    
    return static_cast<int>(tabs_.size()) - 1;
}

void TabBar::removeTab(int index) {
    if (index < 0 || index >= static_cast<int>(tabs_.size())) return;
    
    tabs_.erase(tabs_.begin() + index);
    
    if (activeTab_ >= static_cast<int>(tabs_.size())) {
        activeTab_ = static_cast<int>(tabs_.size()) - 1;
    }
    
    if (onCloseTab_) onCloseTab_(index);
}

void TabBar::setActiveTab(int index) {
    if (index < 0 || index >= static_cast<int>(tabs_.size())) return;
    
    if (activeTab_ >= 0 && activeTab_ < static_cast<int>(tabs_.size())) {
        tabs_[activeTab_].active = false;
    }
    
    activeTab_ = index;
    tabs_[activeTab_].active = true;
    
    if (onTabSwitch_) onTabSwitch_(index);
}

Tab* TabBar::getTab(int index) {
    if (index < 0 || index >= static_cast<int>(tabs_.size())) return nullptr;
    return &tabs_[index];
}

void TabBar::paint(PaintContext& ctx) {
    if (!visible_) return;
    
    // Figma-style: Semi-transparent tab bar that blends with gradient
    ctx.fillRect(bounds_, {240, 230, 245, 180});  // Light lavender with transparency
    
    // Very subtle shadow line at bottom
    ctx.fillRect({bounds_.x, bounds_.y + bounds_.height - 1, bounds_.width, 1}, 
                 {200, 180, 210, 100});

    // Calculate pill dimensions with better spacing
    float pillHeight = bounds_.height - 12;  // More padding top/bottom
    float pillY = bounds_.y + 6;
    float pillSpacing = 6;  // Tighter spacing between tabs
    float maxPillWidth = 220.0f;  // Slightly wider max
    float minPillWidth = 120.0f;
    
    // Calculate available width and pill widths
    float availableWidth = bounds_.width - 60;  // Reserve space for + button
    float pillWidth = std::min(maxPillWidth, 
                               std::max(minPillWidth, 
                                       (availableWidth - pillSpacing * tabs_.size()) / std::max(1ul, tabs_.size())));

    float x = bounds_.x + 10;  // Start with more padding from left
    
    for (size_t i = 0; i < tabs_.size(); ++i) {
        const auto& tab = tabs_[i];
        
        Rect pillRect = {x, pillY, pillWidth, pillHeight};
        
        // Pill background - Modern Safari style with subtle shadows
        Color bgColor;
        Color shadowColor = {0, 0, 0, 15};  // Subtle shadow
        
        if (tab.active) {
            bgColor = Theme::Chrome::tabActive();  // Pure white for active
            // Draw subtle shadow for active tab (depth effect)
            ctx.fillRect({pillRect.x, pillRect.y + 1, pillRect.width, 1}, shadowColor);
        } else if (static_cast<int>(i) == hoveredIndex_) {
            bgColor = Theme::Chrome::tabHover();   // Light purple hover
        } else {
            bgColor = Theme::Chrome::tabInactive();// Soft purple inactive
        }
        
        // Simulate rounded corners with gradient edges (top corners)
        // Main pill body
        ctx.fillRect(pillRect, bgColor);
        
        // Subtle border for active tab only
        if (tab.active) {
            ctx.strokeRect(pillRect, {200, 200, 210, 255}, 1);
        }
        
        // Tab content with better alignment
        float textY = pillY + (pillHeight - SimpleFont::getLineHeight(12)) / 2;
        float contentX = x + 14;  // More left padding
        
        // Favicon placeholder (small circle for active tab)
        if (tab.active) {
            // Draw small circle as favicon placeholder
            Color iconColor = Theme::Chrome::tabText();
            ctx.fillRect({contentX, textY + 3, 12, 12}, {iconColor.r, iconColor.g, iconColor.b, 40});
            ctx.fillRect({contentX + 3, textY + 6, 6, 6}, iconColor);
            contentX += 18;
        }
        
        // URL/Title text with better truncation
        std::string displayText = tab.url.empty() ? tab.title : tab.url;
        // Abbreviate if too long
        float maxTextWidth = pillWidth - (tab.active ? 70 : 50);
        if (SimpleFont::getTextWidth(displayText, 12) > maxTextWidth) {
            // Truncate
            while (displayText.length() > 3 && SimpleFont::getTextWidth(displayText + "...", 12) > maxTextWidth) {
                displayText.pop_back();
            }
            displayText += "...";
        }
        
        Color textColor = tab.active ? Theme::Chrome::tabText() : Theme::Chrome::tabTextInactive();
        SimpleFont::drawText(ctx, displayText, contentX, textY, textColor, 12);
        
        // Close button (×) with better positioning
        if (tabs_.size() > 1) {
            float closeX = x + pillWidth - 24;
            float closeY = textY - 2;
            Color closeColor = tab.active ? Theme::Chrome::closeButton() : Theme::Chrome::tabTextInactive();
            
            // Draw close button background on hover
            if (static_cast<int>(i) == hoveredIndex_) {
                ctx.fillRect({closeX - 2, closeY, 18, 18}, {closeColor.r, closeColor.g, closeColor.b, 30});
            }
            
            SimpleFont::drawText(ctx, "×", closeX + 2, closeY + 2, closeColor, 16);
        }
        
        // Loading indicator (bottom of tab)
        if (tab.loading) {
            float loadWidth = pillWidth * tab.progress;
            ctx.fillRect({x, pillY + pillHeight - 3, loadWidth, 3}, Theme::primary());
        }
        
        x += pillWidth + pillSpacing;
    }
    
    // New tab button (+) with modern styling
    float btnSize = pillHeight;
    Rect newTabRect = {x + 6, pillY, btnSize, btnSize};
    
    // Button background
    Color btnBg = Theme::Chrome::buttonDefault();
    ctx.fillRect(newTabRect, btnBg);
    
    // Plus symbol centered
    float plusY = pillY + (btnSize - SimpleFont::getLineHeight(20)) / 2;
    float plusX = x + 6 + (btnSize - SimpleFont::getTextWidth("+", 20)) / 2;
    SimpleFont::drawText(ctx, "+", plusX, plusY, Theme::Chrome::tabText(), 20);
}

bool TabBar::handleMouseDown(float x, float y, int button) {
    if (button != 1 || !containsPoint(x, y)) return false;
    
    float tabX = bounds_.x;
    for (size_t i = 0; i < tabs_.size(); ++i) {
        float w = std::min(tabWidth_, (bounds_.width - 30) / std::max(1ul, tabs_.size()));
        
        if (x >= tabX && x < tabX + w) {
            // Check close button
            float closeX = tabX + w - 20;
            if (x >= closeX && tabs_.size() > 1) {
                removeTab(static_cast<int>(i));
            } else {
                setActiveTab(static_cast<int>(i));
            }
            return true;
        }
        tabX += w;
    }
    
    // New tab button check (approx)
    if (x >= tabX && x < tabX + 30) {
        if (onNewTab_) onNewTab_();
        return true;
    }
    
    return false;
}

// =============================================================================
// NavigationBar
// =============================================================================

NavigationBar::NavigationBar() {
    backButton_ = std::make_unique<Button>(""); // Or icon
    backButton_->setIcon(IconType::Back);
    
    forwardButton_ = std::make_unique<Button>("");
    forwardButton_->setIcon(IconType::Forward);
    
    refreshButton_ = std::make_unique<Button>("");
    refreshButton_->setIcon(IconType::Refresh);
    
    homeButton_ = std::make_unique<Button>(""); // Home icon placeholder
    homeButton_->setIcon(IconType::Home);
    
    menuButton_ = std::make_unique<Button>(""); // Menu icon placeholder
    menuButton_->setIcon(IconType::Menu);
    
    addressBar_ = std::make_unique<TextInput>();
    addressBar_->setPlaceholder("Search or enter address");
    
    backButton_->setOnClick([this]() { if (onBack_) onBack_(); });
    forwardButton_->setOnClick([this]() { if (onForward_) onForward_(); });
    refreshButton_->setOnClick([this]() { 
        if (loading_) { if (onStop_) onStop_(); } 
        else { if (onRefresh_) onRefresh_(); } 
    });
    
    homeButton_->setOnClick([this]() { if (onHome_) onHome_(); });
    menuButton_->setOnClick([this]() { if (onMenu_) onMenu_(); });
    
    addressBar_->setOnSubmit([this](const std::string& text) {
        if (onNavigate_) onNavigate_(text);
    });
}

void NavigationBar::setURL(const std::string& url) {
    url_ = url;
    if (!addressBar_->isFocused()) {
        addressBar_->setText(url);
    }
}

void NavigationBar::paint(PaintContext& ctx) {
    // === GLASSMORPHISM: Frosted glass navigation bar ===
    // Main glass background
    ctx.fillRect(bounds_, {230, 220, 240, 200});  // Light lavender glass
    
    // Top highlight (glass shine effect)
    ctx.fillRect({bounds_.x, bounds_.y, bounds_.width, 1}, {255, 255, 255, 80});
    
    // Bottom border shadow
    ctx.fillRect({bounds_.x, bounds_.y + bounds_.height - 1, bounds_.width, 1}, 
                 {150, 140, 170, 100});
    
    float padding = 10;  // Increased padding
    float btnSize = 32;  // Slightly larger buttons
    float currentX = bounds_.x + padding;
    float y = bounds_.y + (bounds_.height - btnSize) / 2;
    
    // Layout buttons with better spacing
    backButton_->setBounds(currentX, y, btnSize, btnSize);
    backButton_->paint(ctx);
    currentX += btnSize + 6;  // Tighter spacing
    
    forwardButton_->setBounds(currentX, y, btnSize, btnSize);
    forwardButton_->paint(ctx);
    currentX += btnSize + 6;
    
    refreshButton_->setBounds(currentX, y, btnSize, btnSize);
    refreshButton_->setIcon(loading_ ? IconType::Stop : IconType::Refresh);
    refreshButton_->paint(ctx);
    currentX += btnSize + 12;  // More space before address bar
    
    homeButton_->setBounds(currentX, y, btnSize, btnSize);
    homeButton_->paint(ctx);
    currentX += btnSize + 12;
    
    // Address bar (fills remaining space minus menu button)
    float menuWidth = btnSize;
    float addressBarWidth = bounds_.width - (currentX - bounds_.x) - padding * 2 - menuWidth;
    
    addressBar_->setBounds(currentX, y, addressBarWidth, btnSize);
    addressBar_->paint(ctx);
    currentX += addressBarWidth + padding;
    
    // Menu button at the end
    menuButton_->setBounds(currentX, y, btnSize, btnSize);
    menuButton_->paint(ctx);
}

bool NavigationBar::handleMouseDown(float x, float y, int button) {
    if (!containsPoint(x, y)) return false;
    
    // Forward to child components
    if (backButton_->handleMouseDown(x, y, button)) return true;
    if (forwardButton_->handleMouseDown(x, y, button)) return true;
    if (refreshButton_->handleMouseDown(x, y, button)) return true;
    if (homeButton_->handleMouseDown(x, y, button)) return true;
    if (menuButton_->handleMouseDown(x, y, button)) return true;
    if (addressBar_->handleMouseDown(x, y, button)) return true;
    
    return true;
}

bool NavigationBar::handleKeyDown(int keycode, bool shift, bool ctrl, bool alt) {
    if (addressBar_->handleKeyDown(keycode, shift, ctrl, alt)) {
        return true;
    }
    return false;
}

bool NavigationBar::handleTextInput(const std::string& text) {
    if (addressBar_->handleTextInput(text)) {
        return true;
    }
    return false;
}

// =============================================================================
// BrowserChrome
// =============================================================================

BrowserChrome::BrowserChrome(float width, float height) 
    : width_(width), height_(height) {
    
    sidebar_ = std::make_unique<Sidebar>();
    tabBar_ = std::make_unique<TabBar>();
    navigationBar_ = std::make_unique<NavigationBar>();
    
    layout();
}

void BrowserChrome::resize(float width, float height) {
    width_ = width;
    height_ = height;
    layout();
}

void BrowserChrome::layout() {
    // Use dynamic sidebar width (8px collapsed, 60px expanded)
    const float sidebarWidth = sidebar_->width();
    const float tabBarHeight = 40;
    const float navBarHeight = 46;
    
    // Sidebar spans full height on left
    sidebar_->setBounds(0, 0, sidebar_->expandedWidth(), height_);
    
    // Tab bar and nav bar start after sidebar
    tabBar_->setBounds(sidebarWidth, 0, width_ - sidebarWidth, tabBarHeight);
    navigationBar_->setBounds(sidebarWidth, tabBarHeight, width_ - sidebarWidth, navBarHeight);
    
    contentArea_ = {
        sidebarWidth, 
        tabBarHeight + navBarHeight, 
        width_ - sidebarWidth, 
        height_ - tabBarHeight - navBarHeight
    };
}

void BrowserChrome::paint(DisplayList& displayList) {
    PaintContext ctx(displayList);
    
    // Draw background with theme color
    ctx.fillRect({0, 0, width_, height_}, Theme::background());
    
    // Draw sidebar
    sidebar_->paint(ctx);
    
    // Draw tab bar and nav bar
    tabBar_->paint(ctx);
    navigationBar_->paint(ctx);
}

bool BrowserChrome::handleMouseDown(float x, float y, int button) {
    if (sidebar_->handleMouseDown(x, y, button)) return true;
    if (tabBar_->handleMouseDown(x, y, button)) return true;
    if (navigationBar_->handleMouseDown(x, y, button)) return true;
    return false;
}

bool BrowserChrome::handleMouseUp(float x, float y, int button) {
    return false;
}

bool BrowserChrome::handleMouseMove(float x, float y) {
    sidebar_->handleMouseMove(x, y);
    tabBar_->handleMouseMove(x, y);
    navigationBar_->handleMouseMove(x, y);
    return false;
}

bool BrowserChrome::handleKeyDown(int keycode, bool shift, bool ctrl, bool alt) {
    if (navigationBar_->handleKeyDown(keycode, shift, ctrl, alt)) return true;
    return false;
}

bool BrowserChrome::handleTextInput(const std::string& text) {
    if (navigationBar_->handleTextInput(text)) return true;
    return false;
}

} // namespace Zepra::WebCore
