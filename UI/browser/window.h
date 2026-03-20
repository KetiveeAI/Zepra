// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "../common/types.h"
#include "../common/constants.h"
#include <SDL2/SDL.h>
#include <memory>
#include <functional>
#include "../ui/settings_ui.h" // Include the new settings UI header

namespace zepra {

// Forward declarations
class Tab;
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
    int keyCode; // Add keyCode for keyboard events

    WindowEvent(WindowEventType type = WindowEventType::NONE) : type(type), width(0), height(0), x(0), y(0), focused(false), keyCode(0) {}
};

// Window callback types
using WindowEventCallback = std::function<void(const WindowEvent&)>;
using RenderCallback = std::function<void()>;

// Window class
class Window {
public:
    Window();
    ~Window();
    
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
    
    // OpenGL context
    bool createOpenGLContext(int majorVersion = 3, int minorVersion = 3);
    void makeCurrent();
    void swapBuffers();
    
    // Window information
    SDL_Window* getSDLWindow() const { return window; }
    SDL_GLContext getGLContext() const { return glContext; }
    
    // Utility methods
    void centerOnScreen();
    void setMinimumSize(int width, int height);
    void setMaximumSize(int width, int height);
    void setResizable(bool resizable);
    bool isResizable() const;
    
    // UI Integration (for main loop)
    bool handleEvent(const SDL_Event& event) { handleSDLEvent(event); return false; }
    void update() {}  // Stub
    void toggleFullscreen() { setFullscreen(!isFullscreen()); }
    void handleResize(int w, int h) { width = w; height = h; }
    
    // New methods for settings UI
    void toggleSettingsUI();
    ui::SettingsUI& getSettingsUI();
    
private:
    SDL_Window* window;
    SDL_GLContext glContext;
    String title;
    int width, height;
    int x, y;
    bool fullscreen;
    bool minimized;
    bool maximized;
    bool visible;
    bool focused;
    bool resizable;
    
    // Callbacks
    WindowEventCallback eventCallback;
    RenderCallback renderCallback;
    ui::SettingsUI m_settingsUI; // Member for settings UI
    
    // Event processing helpers
    void handleSDLEvent(const SDL_Event& event);
    void updateWindowState();
    
    // OpenGL helpers
    void setupOpenGLAttributes(int majorVersion, int minorVersion);
};

// Window Manager - Manages multiple windows
class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    // Window management
    std::shared_ptr<Window> createWindow(const String& title, int width, int height);
    void destroyWindow(std::shared_ptr<Window> window);
    std::vector<std::shared_ptr<Window>> getWindows() const;
    
    // Global event processing
    void processEvents();
    void renderAll();
    
    // Global window operations
    void minimizeAll();
    void restoreAll();
    void closeAll();
    
    // Focus management
    std::shared_ptr<Window> getFocusedWindow() const;
    void setFocusedWindow(std::shared_ptr<Window> window);
    
    // Configuration
    void setDefaultWindowSize(int width, int height);
    void setDefaultFullscreen(bool fullscreen);
    
private:
    std::vector<std::shared_ptr<Window>> windows;
    std::shared_ptr<Window> focusedWindow;
    int defaultWidth, defaultHeight;
    bool defaultFullscreen;
    
    // Event handling
    void handleWindowEvent(std::shared_ptr<Window> window, const WindowEvent& event);
};

// Browser-specific window class
class BrowserWindow : public Window {
public:
    BrowserWindow();
    ~BrowserWindow();
    
    // Browser-specific initialization
    bool initialize(const String& title = BROWSER_NAME);
    
    // Tab management
    void setTabManager(std::shared_ptr<TabManager> tabManager);
    std::shared_ptr<TabManager> getTabManager() const;
    
    // Browser UI rendering
    void renderBrowserUI();
    void renderTabBar();
    void renderAddressBar();
    void renderContent();
    void renderSidebar();
    
    // Browser-specific event handling
    void handleBrowserEvents();
    void handleKeyboardShortcuts(const SDL_Event& event);
    void handleMouseNavigation(const SDL_Event& event);
    
    // Browser window features
    void setHomePage(const String& url);
    String getHomePage() const;
    void setDefaultSearchEngine(const String& engine);
    String getDefaultSearchEngine() const;
    
    // Browser settings
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

    // UI state
    bool addressBarFocused;
    String addressBarText;
    bool isNavigating;
    
    // Browser-specific rendering
    void renderToolbar();
    void renderBookmarksBar();
    void renderStatusBar();
    void renderDeveloperTools();
    
    // Input handling
    void handleAddressBarInput(const SDL_Event& event);
    void handleTabBarInput(const SDL_Event& event);
    void handleToolbarInput(const SDL_Event& event);
};

} // namespace zepra