// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "common/types.h"
#include "common/constants.h"
#include <memory>
#include <functional>
#include <vector>
#include "settings/settings_ui.h" 

// Forward declarations
namespace NXRender {
    class GpuContext;
    class Compositor;
}

namespace zepra {

class TabManager;

// Window event types
enum class WindowEventType {
    NONE,
    RESIZE,
    MOVE,
    FOCUS,
    BLUR,
    CLOSE,
    KEY_DOWN,
    MINIMIZE,
    MAXIMIZE,
    RESTORE
};

// Window event structure
struct WindowEvent {
    WindowEventType type;
    int x, y;
    int width, height;
    bool focused;
    int keyCode;

    WindowEvent(WindowEventType type = WindowEventType::NONE) : type(type), width(0), height(0), x(0), y(0), focused(false), keyCode(0) {}
};

using WindowEventCallback = std::function<void(const WindowEvent&)>;
using RenderCallback = std::function<void()>;

// Window class (Native NXRender wrapper)
class Window {
public:
    Window();
    virtual ~Window();
    
    // Window creation and management
    bool create(const String& title, int width, int height, bool fullscreen = false);
    void destroy();
    void show();
    void hide();
    void minimize();
    void maximize();
    void restore();
    void close();
    
    // Window properties
    void setTitle(const String& title);
    String getTitle() const;
    void setSize(int width, int height);
    void getSize(int& width, int& height) const;
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    void setPosition(int x, int y);
    void getPosition(int& x, int& y) const;
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const;
    bool isMinimized() const;
    bool isMaximized() const;
    bool isVisible() const;
    bool isFocused() const;
    
    // Event handling
    void setEventCallback(WindowEventCallback callback);
    void setRenderCallback(RenderCallback callback);
    void processEvents();
    void render();
    
    // Input handling
    bool isKeyPressed(int keyCode) const;
    bool isMouseButtonPressed(int button) const;
    void getMousePosition(int& x, int& y) const;
    void setMousePosition(int x, int y);
    void showCursor(bool show);
    void setCursor(const String& cursorType);
    
    // OpenGL context (Managed by NXRender now, keeping for compatibility signature)
    bool createOpenGLContext(int majorVersion = 3, int minorVersion = 3);
    void makeCurrent();
    void swapBuffers();
    
    // Utility methods
    void centerOnScreen();
    void setMinimumSize(int width, int height);
    void setMaximumSize(int width, int height);
    void setResizable(bool resizable);
    bool isResizable() const;
    
    // UI Integration
    void update() {} 
    void toggleFullscreen() { setFullscreen(!isFullscreen()); }
    void handleResize(int w, int h) { width = w; height = h; }
    
    void toggleSettingsUI();
    ui::SettingsUI& getSettingsUI();
    
protected:
    String title;
    int width, height;
    int x, y;
    bool fullscreen;
    bool minimized;
    bool maximized;
    bool visible;
    bool focused;
    bool resizable;
    
    WindowEventCallback eventCallback;
    RenderCallback renderCallback;
    ui::SettingsUI m_settingsUI;
};

// Window Manager - Manages multiple windows
class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    std::shared_ptr<Window> createWindow(const String& title, int width, int height);
    void destroyWindow(std::shared_ptr<Window> window);
    std::vector<std::shared_ptr<Window>> getWindows() const;
    
    void processEvents();
    void renderAll();
    
    void minimizeAll();
    void restoreAll();
    void closeAll();
    
    std::shared_ptr<Window> getFocusedWindow() const;
    void setFocusedWindow(std::shared_ptr<Window> window);
    
    void setDefaultWindowSize(int width, int height);
    void setDefaultFullscreen(bool fullscreen);
    
private:
    std::vector<std::shared_ptr<Window>> windows;
    std::shared_ptr<Window> focusedWindow;
    int defaultWidth, defaultHeight;
    bool defaultFullscreen;
    
    void handleWindowEvent(std::shared_ptr<Window> window, const WindowEvent& event);
};

// Browser-specific window class
class BrowserWindow : public Window {
public:
    BrowserWindow();
    virtual ~BrowserWindow();
    
    bool initialize(const String& title = BROWSER_NAME);
    
    void setTabManager(std::shared_ptr<TabManager> tabManager);
    std::shared_ptr<TabManager> getTabManager() const;
    
    void renderBrowserUI();
    void renderTabBar();
    void renderAddressBar();
    void renderContent();
    void renderSidebar();
    
    void handleBrowserEvents();
    void handleKeyboardEvent(const WindowEvent& event);
    void handleMouseEvent(const WindowEvent& event);
    
    void setHomePage(const String& url);
    String getHomePage() const;
    void setDefaultSearchEngine(const String& engine);
    String getDefaultSearchEngine() const;
    
    void setShowBookmarksBar(bool show);
    bool isShowBookmarksBar() const;
    void setShowStatusBar(bool show);
    bool isShowStatusBar() const;
    void setShowDeveloperTools(bool show);
    bool isShowDeveloperTools() const;
    
private:
    std::shared_ptr<TabManager> tabManager;
    String homePage;
    String defaultSearchEngine;
    bool showBookmarksBar;
    bool showStatusBar;
    bool showDeveloperTools;
    bool showSidebar;
    int sidebarWidth;

    bool addressBarFocused;
    String addressBarText;
    bool isNavigating;
    
    void renderToolbar();
    void renderBookmarksBar();
    void renderStatusBar();
    void renderDeveloperTools();
    
    void handleAddressBarInput(const WindowEvent& event);
    void handleTabBarInput(const WindowEvent& event);
    void handleToolbarInput(const WindowEvent& event);
};

} // namespace zepra