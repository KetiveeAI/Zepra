/**
 * @file browser_ui.hpp
 * @brief Browser UI components - address bar, tabs, navigation buttons
 */

#pragma once

#include "render_tree.hpp"
#include "paint_context.hpp"
#include "sidebar.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Zepra::WebCore {

// Forward declarations
class BrowserWindow;

/**
 * @brief Base UI component
 */
class UIComponent {
public:
    UIComponent();
    virtual ~UIComponent() = default;
    
    // Layout
    void setBounds(float x, float y, float width, float height);
    Rect bounds() const { return bounds_; }
    
    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    
    // Focus
    void setFocused(bool focused) { focused_ = focused; }
    bool isFocused() const { return focused_; }
    
    // Rendering
    virtual void paint(PaintContext& ctx) = 0;
    
    // Events
    virtual bool handleMouseDown(float x, float y, int button);
    virtual bool handleMouseUp(float x, float y, int button);
    virtual bool handleMouseMove(float x, float y);
    virtual bool handleKeyDown(int keycode, bool shift, bool ctrl, bool alt);
    virtual bool handleTextInput(const std::string& text);
    
    // Hit testing
    bool containsPoint(float x, float y) const;
    
protected:
    Rect bounds_;
    bool visible_ = true;
    bool focused_ = false;
    bool hovered_ = false;
};

// =============================================================================
// Button
// =============================================================================

enum class IconType {
    None,
    Back,
    Forward,
    Refresh,
    Stop,
    Home,
    Menu,
    NewTab,
    Close
};

class Button : public UIComponent {
public:
    explicit Button(const std::string& label);
    
    void paint(PaintContext& ctx) override;
    
    // Events
    bool handleMouseDown(float x, float y, int button) override;
    bool handleMouseUp(float x, float y, int button) override;
    bool handleMouseMove(float x, float y) override;
    
    void setOnClick(std::function<void()> handler) { onClick_ = handler; }
    void setLabel(const std::string& label) { label_ = label; }
    void setIcon(IconType icon) { icon_ = icon; }
    
private:
    std::string label_;
    IconType icon_ = IconType::None;
    bool pressed_ = false;
    std::function<void()> onClick_;
};

/**
 * @brief Text input component
 */
class TextInput : public UIComponent {
public:
    TextInput();
    
    void setText(const std::string& text);
    const std::string& text() const { return text_; }
    
    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
    void setOnSubmit(std::function<void(const std::string&)> handler) { onSubmit_ = handler; }
    void setOnChange(std::function<void(const std::string&)> handler) { onChange_ = handler; }
    
    void paint(PaintContext& ctx) override;
    bool handleMouseDown(float x, float y, int button) override;
    bool handleKeyDown(int keycode, bool shift, bool ctrl, bool alt) override;
    bool handleTextInput(const std::string& text) override;
    
private:
    std::string text_;
    std::string placeholder_;
    size_t cursorPos_ = 0;
    size_t selectionStart_ = 0;
    size_t selectionEnd_ = 0;
    std::function<void(const std::string&)> onSubmit_;
    std::function<void(const std::string&)> onChange_;
};

/**
 * @brief Tab component
 */
struct Tab {
    std::string title;
    std::string url;
    bool loading = false;
    bool active = false;
    float progress = 0;
};

/**
 * @brief Tab bar component
 */
class TabBar : public UIComponent {
public:
    TabBar();
    
    // Tab management
    int addTab(const std::string& title = "New Tab", const std::string& url = "");
    void removeTab(int index);
    void setActiveTab(int index);
    int activeTab() const { return activeTab_; }
    size_t tabCount() const { return tabs_.size(); }
    Tab* getTab(int index);
    
    // Events
    void setOnTabSwitch(std::function<void(int)> handler) { onTabSwitch_ = handler; }
    void setOnNewTab(std::function<void()> handler) { onNewTab_ = handler; }
    void setOnCloseTab(std::function<void(int)> handler) { onCloseTab_ = handler; }
    
    void paint(PaintContext& ctx) override;
    bool handleMouseDown(float x, float y, int button) override;
    
private:
    std::vector<Tab> tabs_;
    int activeTab_ = -1;
    int hoveredIndex_ = -1;  // Track hovered tab for hover effects
    float tabWidth_ = 200;
    std::function<void(int)> onTabSwitch_;
    std::function<void()> onNewTab_;
    std::function<void(int)> onCloseTab_;
};

/**
 * @brief Navigation bar (back, forward, refresh, address bar)
 */
class NavigationBar : public UIComponent {
public:
    NavigationBar();
    
    // URL
    void setURL(const std::string& url);
    const std::string& url() const { return url_; }
    
    // Navigation state
    void setCanGoBack(bool can) { canGoBack_ = can; }
    void setCanGoForward(bool can) { canGoForward_ = can; }
    void setLoading(bool loading) { loading_ = loading; }
    
    // UI Elements Access
    TextInput* addressBar() { return addressBar_.get(); }
    
    // Events
    void setOnNavigate(std::function<void(const std::string&)> handler) { onNavigate_ = handler; }
    void setOnBack(std::function<void()> handler) { onBack_ = handler; }
    void setOnForward(std::function<void()> handler) { onForward_ = handler; }
    void setOnRefresh(std::function<void()> handler) { onRefresh_ = handler; }
    void setOnStop(std::function<void()> handler) { onStop_ = handler; }
    void setOnHome(std::function<void()> handler) { onHome_ = handler; }
    void setOnMenu(std::function<void()> handler) { onMenu_ = handler; }
    
    void paint(PaintContext& ctx) override;
    bool handleMouseDown(float x, float y, int button) override;
    bool handleKeyDown(int keycode, bool shift, bool ctrl, bool alt) override;
    bool handleTextInput(const std::string& text) override;
    
private:
    std::string url_;
    bool canGoBack_ = false;
    bool canGoForward_ = false;
    bool loading_ = false;
    
    std::unique_ptr<Button> backButton_;
    std::unique_ptr<Button> forwardButton_;
    std::unique_ptr<Button> refreshButton_;
    std::unique_ptr<Button> homeButton_;     // [NEW] Home button
    std::unique_ptr<Button> menuButton_;     // [NEW] Menu button
    std::unique_ptr<TextInput> addressBar_;
    
    std::function<void(const std::string&)> onNavigate_;
    std::function<void()> onBack_;
    std::function<void()> onForward_;
    std::function<void()> onRefresh_;
    std::function<void()> onStop_;
    std::function<void()> onHome_;           // [NEW]
    std::function<void()> onMenu_;           // [NEW]
};

/**
 * @brief Complete browser chrome (tabs + navigation + content area)
 */
class BrowserChrome {
public:
    BrowserChrome(float width, float height);
    
    // Layout
    void resize(float width, float height);
    Rect contentArea() const { return contentArea_; }
    
    // Components
    Sidebar* sidebar() { return sidebar_.get(); }
    TabBar* tabBar() { return tabBar_.get(); }
    NavigationBar* navigationBar() { return navigationBar_.get(); }
    
    // Rendering
    void paint(DisplayList& displayList);
    
    // Events
    bool handleMouseDown(float x, float y, int button);
    bool handleMouseUp(float x, float y, int button);
    bool handleMouseMove(float x, float y);
    bool handleKeyDown(int keycode, bool shift, bool ctrl, bool alt);
    bool handleTextInput(const std::string& text);
    
private:
    void layout();
    
    float width_, height_;
    Rect contentArea_;
    
    std::unique_ptr<Sidebar> sidebar_;
    std::unique_ptr<TabBar> tabBar_;
    std::unique_ptr<NavigationBar> navigationBar_;
};

} // namespace Zepra::WebCore
