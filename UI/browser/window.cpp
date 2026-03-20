// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "../../source/zepraEngine/include/engine/ui/window.h"
#include "../../source/zepraEngine/include/engine/ui/tab_manager.h"
#include "../../source/integration/include/common/constants.h"
#include <iostream>
#include <algorithm>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>

namespace zepra {

// Window class implementation
Window::Window() 
    : window(nullptr), glContext(nullptr), width(0), height(0), x(0), y(0),
      fullscreen(false), minimized(false), maximized(false), visible(false), 
      focused(false), resizable(true) {
}

Window::~Window() {
    destroy();
}

bool Window::create(const String& title, int w, int height, bool fs) {
    if (window) {
        std::cerr << "Window already exists" << std::endl;
        return false;
    }
    
    this->title = title;
    this->width = w;
    this->height = height;
    this->fullscreen = fs;
    
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if (resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    
    window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        flags
    );
    
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    
    visible = true;
    focused = true;
    
    // Get initial position
    SDL_GetWindowPosition(window, &x, &y);
    
    // Create OpenGL context if needed
    if (glContext == nullptr) {
        createOpenGLContext();
    }
    
    return true;
}

void Window::destroy() {
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    
    visible = false;
    focused = false;
}

void Window::show() {
    if (window) {
        SDL_ShowWindow(window);
        visible = true;
    }
}

void Window::hide() {
    if (window) {
        SDL_HideWindow(window);
        visible = false;
    }
}

void Window::minimize() {
    if (window) {
        SDL_MinimizeWindow(window);
        minimized = true;
        maximized = false;
    }
}

void Window::maximize() {
    if (window) {
        SDL_MaximizeWindow(window);
        maximized = true;
        minimized = false;
    }
}

void Window::restore() {
    if (window) {
        SDL_RestoreWindow(window);
        minimized = false;
        maximized = false;
    }
}

void Window::close() {
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void Window::setTitle(const String& newTitle) {
    title = newTitle;
    if (window) {
        SDL_SetWindowTitle(window, title.c_str());
    }
}

String Window::getTitle() const {
    return title;
}

void Window::setSize(int w, int h) {
    width = w;
    height = h;
    if (window) {
        SDL_SetWindowSize(window, width, height);
    }
}

void Window::getSize(int& w, int& h) const {
    w = width;
    h = height;
}

void Window::setPosition(int newX, int newY) {
    x = newX;
    y = newY;
    if (window) {
        SDL_SetWindowPosition(window, x, y);
    }
}

void Window::getPosition(int& newX, int& newY) const {
    newX = x;
    newY = y;
}

void Window::setFullscreen(bool fs) {
    if (fullscreen != fs && window) {
        if (fs) {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        } else {
            SDL_SetWindowFullscreen(window, 0);
        }
        fullscreen = fs;
    }
}

bool Window::isFullscreen() const {
    return fullscreen;
}

bool Window::isMinimized() const {
    return minimized;
}

bool Window::isMaximized() const {
    return maximized;
}

bool Window::isVisible() const {
    return visible;
}

bool Window::isFocused() const {
    return focused;
}

void Window::setEventCallback(WindowEventCallback callback) {
    eventCallback = callback;
}

void Window::setRenderCallback(RenderCallback callback) {
    renderCallback = callback;
}

void Window::processEvents() {
    if (!window) return;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handleSDLEvent(event);
    }
}

void Window::render() {
    if (renderCallback) {
        renderCallback();
    }
    // Render settings UI if visible
    if (m_settingsUI.isVisible()) {
        m_settingsUI.render();
    }
}

bool Window::isKeyPressed(int keyCode) const {
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    return state[SDL_GetScancodeFromKey(keyCode)] != 0;
}

bool Window::isMouseButtonPressed(int button) const {
    return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(button)) != 0;
}

void Window::getMousePosition(int& mouseX, int& mouseY) const {
    SDL_GetMouseState(&mouseX, &mouseY);
}

void Window::setMousePosition(int mouseX, int mouseY) {
    SDL_WarpMouseInWindow(window, mouseX, mouseY);
}

void Window::showCursor(bool show) {
    SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

void Window::setCursor(const String& cursorType) {
    // Simple cursor implementation
    if (cursorType == "arrow") {
        SDL_SetCursor(SDL_GetDefaultCursor());
    } else if (cursorType == "hand") {
        // Would need to create custom cursor
        SDL_SetCursor(SDL_GetDefaultCursor());
    }
}

bool Window::createOpenGLContext(int majorVersion, int minorVersion) {
    if (!window) {
        std::cerr << "Cannot create OpenGL context without window" << std::endl;
        return false;
    }
    
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
    }
    
    setupOpenGLAttributes(majorVersion, minorVersion);
    
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

void Window::makeCurrent() {
    if (glContext) {
        SDL_GL_MakeCurrent(window, glContext);
    }
}

void Window::swapBuffers() {
    if (glContext) {
        SDL_GL_SwapWindow(window);
    }
}

void Window::centerOnScreen() {
    if (window) {
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_GetWindowPosition(window, &x, &y);
    }
}

void Window::setMinimumSize(int minWidth, int minHeight) {
    if (window) {
        SDL_SetWindowMinimumSize(window, minWidth, minHeight);
    }
}

void Window::setMaximumSize(int maxWidth, int maxHeight) {
    if (window) {
        SDL_SetWindowMaximumSize(window, maxWidth, maxHeight);
    }
}

void Window::setResizable(bool resizable) {
    this->resizable = resizable;
    if (window) {
        SDL_SetWindowResizable(window, resizable ? SDL_TRUE : SDL_FALSE);
    }
}

bool Window::isResizable() const {
    return resizable;
}

void Window::toggleSettingsUI() {
    if (m_settingsUI.isVisible()) {
        m_settingsUI.hide();
    } else {
        m_settingsUI.show();
    }
}

ui::SettingsUI& Window::getSettingsUI() {
    return m_settingsUI;
}

void Window::handleSDLEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    width = event.window.data1;
                    height = event.window.data2;
                    if (eventCallback) {
                        WindowEvent windowEvent(WindowEventType::RESIZE);
                        windowEvent.width = width;
                        windowEvent.height = height;
                        eventCallback(windowEvent);
                    }
                    break;
                    
                case SDL_WINDOWEVENT_MOVED:
                    x = event.window.data1;
                    y = event.window.data2;
                    if (eventCallback) {
                        WindowEvent windowEvent(WindowEventType::MOVE);
                        windowEvent.x = x;
                        windowEvent.y = y;
                        eventCallback(windowEvent);
                    }
                    break;
                    
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    focused = true;
                    if (eventCallback) {
                        WindowEvent windowEvent(WindowEventType::FOCUS);
                        windowEvent.focused = true;
                        eventCallback(windowEvent);
                    }
                    break;
                    
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    focused = false;
                    if (eventCallback) {
                        WindowEvent windowEvent(WindowEventType::BLUR);
                        windowEvent.focused = false;
                        eventCallback(windowEvent);
                    }
                    break;
                    
                case SDL_WINDOWEVENT_MINIMIZED:
                    minimized = true;
                    if (eventCallback) {
                        WindowEvent windowEvent(WindowEventType::MINIMIZE);
                        eventCallback(windowEvent);
                    }
                    break;
                    
                case SDL_WINDOWEVENT_MAXIMIZED:
                    maximized = true;
                    if (eventCallback) {
                        WindowEvent windowEvent(WindowEventType::MAXIMIZE);
                        eventCallback(windowEvent);
                    }
                    break;
                    
                case SDL_WINDOWEVENT_RESTORED:
                    minimized = false;
                    maximized = false;
                    if (eventCallback) {
                        WindowEvent windowEvent(WindowEventType::RESTORE);
                        eventCallback(windowEvent);
                    }
                    break;
                    
                case SDL_WINDOWEVENT_CLOSE:
                    if (eventCallback) {
                        WindowEvent windowEvent(WindowEventType::CLOSE);
                        eventCallback(windowEvent);
                    }
                    break;
            }
            break;
            
        case SDL_QUIT:
            if (eventCallback) {
                WindowEvent windowEvent(WindowEventType::CLOSE);
                eventCallback(windowEvent);
            }
            break;

        case SDL_KEYDOWN:
            if (eventCallback) {
                WindowEvent windowEvent(WindowEventType::KEY_DOWN);
                windowEvent.keyCode = event.key.keysym.sym;
                eventCallback(windowEvent);
            }
            break;
    }
}

void Window::updateWindowState() {
    if (!window) return;
    
    Uint32 flags = SDL_GetWindowFlags(window);
    minimized = (flags & SDL_WINDOW_MINIMIZED) != 0;
    maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
    visible = (flags & SDL_WINDOW_SHOWN) != 0;
    focused = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void Window::setupOpenGLAttributes(int majorVersion, int minorVersion) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
}

// WindowManager implementation
WindowManager::WindowManager() 
    : defaultWidth(DEFAULT_WINDOW_WIDTH), defaultHeight(DEFAULT_WINDOW_HEIGHT), 
      defaultFullscreen(false) {
}

WindowManager::~WindowManager() {
    closeAll();
}

std::shared_ptr<Window> WindowManager::createWindow(const String& title, int width, int height) {
    auto window = std::make_shared<Window>();
    if (window->create(title, width, height, defaultFullscreen)) {
        windows.push_back(window);
        
        // Set event callback
        window->setEventCallback([this, window](const WindowEvent& event) {
            handleWindowEvent(window, event);
        });
        
        // Set as focused if it's the first window
        if (windows.size() == 1) {
            setFocusedWindow(window);
        }
        
        return window;
    }
    return nullptr;
}

void WindowManager::destroyWindow(std::shared_ptr<Window> window) {
    auto it = std::find(windows.begin(), windows.end(), window);
    if (it != windows.end()) {
        if (focusedWindow == window) {
            focusedWindow = nullptr;
        }
        windows.erase(it);
    }
}

std::vector<std::shared_ptr<Window>> WindowManager::getWindows() const {
    return windows;
}

void WindowManager::processEvents() {
    for (auto& window : windows) {
        window->processEvents();
    }
}

void WindowManager::renderAll() {
    for (auto& window : windows) {
        window->render();
    }
}

void WindowManager::minimizeAll() {
    for (auto& window : windows) {
        window->minimize();
    }
}

void WindowManager::restoreAll() {
    for (auto& window : windows) {
        window->restore();
    }
}

void WindowManager::closeAll() {
    for (auto& window : windows) {
        window->close();
    }
    windows.clear();
    focusedWindow = nullptr;
}

std::shared_ptr<Window> WindowManager::getFocusedWindow() const {
    return focusedWindow;
}

void WindowManager::setFocusedWindow(std::shared_ptr<Window> window) {
    focusedWindow = window;
}

void WindowManager::setDefaultWindowSize(int width, int height) {
    defaultWidth = width;
    defaultHeight = height;
}

void WindowManager::setDefaultFullscreen(bool fullscreen) {
    defaultFullscreen = fullscreen;
}

void WindowManager::handleWindowEvent(std::shared_ptr<Window> window, const WindowEvent& event) {
    // Handle window-specific events
    switch (event.type) {
        case WindowEventType::FOCUS:
            setFocusedWindow(window);
            break;
            
        case WindowEventType::CLOSE:
            destroyWindow(window);
            break;
            
        default:
            break;
    }
}

// BrowserWindow implementation
BrowserWindow::BrowserWindow() 
    : homePage("https://ketivee.org"), defaultSearchEngine("KetiveeSearch"),
      showBookmarksBar(true), showStatusBar(true), showDeveloperTools(false),
      addressBarFocused(false), isNavigating(false) {
}

BrowserWindow::~BrowserWindow() {
}

bool BrowserWindow::initialize(const String& title) {
    if (!create(title, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT)) {
        return false;
    }
    
    // Set up browser-specific event handling
    setEventCallback([this](const WindowEvent& event) {
        handleBrowserEvents();
    });
    
    return true;
}

void BrowserWindow::setTabManager(std::shared_ptr<TabManager> manager) {
    tabManager = manager;
}

std::shared_ptr<TabManager> BrowserWindow::getTabManager() const {
    return tabManager;
}

void BrowserWindow::renderBrowserUI() {
    if (!isVisible()) return;
    
    makeCurrent();
    
    // Clear background
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render browser components
    renderTabBar();
    renderAddressBar();
    renderToolbar();
    renderBookmarksBar();
    renderContent();
    renderStatusBar();
    
    if (showDeveloperTools) {
        renderDeveloperTools();
    }
    
    swapBuffers();
}

void BrowserWindow::renderTabBar() {
    glViewport(0, getHeight() - TAB_BAR_HEIGHT, getWidth(), TAB_BAR_HEIGHT);
    
    // Render tab bar background
    glBegin(GL_QUADS);
    glColor3f(0.3f, 0.3f, 0.3f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
}

void BrowserWindow::renderAddressBar() {
    int addressBarY = getHeight() - TAB_BAR_HEIGHT - ADDRESS_BAR_HEIGHT;
    glViewport(0, addressBarY, getWidth(), ADDRESS_BAR_HEIGHT);
    
    // Render address bar background
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.4f, 0.4f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
}

void BrowserWindow::renderContent() {
    int contentY = 0;
    int contentHeight = getHeight();
    
    // Adjust for bookmarks bar if shown
    if (showBookmarksBar) {
        contentY += BOOKMARKS_BAR_HEIGHT;
    }
    
    // Adjust for other UI elements
    contentHeight -= TAB_BAR_HEIGHT + ADDRESS_BAR_HEIGHT + TOOLBAR_HEIGHT;
    
    // Adjust for status bar if shown
    if (showStatusBar) {
        contentHeight -= STATUS_BAR_HEIGHT;
    }
    
    glViewport(0, contentY, getWidth(), contentHeight);
    
    // Render content area background
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
}

void BrowserWindow::handleBrowserEvents() {
    // Handle browser-specific events
    processEvents();
}

void BrowserWindow::handleKeyboardShortcuts(const SDL_Event& event) {
    if (event.type != SDL_KEYDOWN) return;
    
    bool ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;
    bool shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
    bool alt = (event.key.keysym.mod & KMOD_ALT) != 0;
    
    switch (event.key.keysym.sym) {
        case SDLK_t:
            if (ctrl) {
                // Ctrl+T: New tab
                if (tabManager) {
                    tabManager->createTab();
                }
            }
            break;
            
        case SDLK_w:
            if (ctrl) {
                // Ctrl+W: Close tab
                if (tabManager) {
                    tabManager->closeActiveTab();
                }
            }
            break;
            
        case SDLK_r:
            if (ctrl) {
                // Ctrl+R: Reload
                if (tabManager) {
                    auto activeTab = tabManager->getActiveTab();
                    if (activeTab) {
                        activeTab->reload();
                    }
                }
            }
            break;
            
        case SDLK_l:
            if (ctrl) {
                // Ctrl+L: Focus address bar
                addressBarFocused = true;
            }
            break;
            
        case SDLK_F5:
            // F5: Reload
            if (tabManager) {
                auto activeTab = tabManager->getActiveTab();
                if (activeTab) {
                    activeTab->reload();
                }
            }
            break;
            
        case SDLK_F11:
            // F11: Toggle fullscreen
            setFullscreen(!isFullscreen());
            break;
    }
}

void BrowserWindow::handleMouseNavigation(const SDL_Event& event) {
    if (event.type != SDL_MOUSEBUTTONDOWN) return;
    
    int mouseX, mouseY;
    getMousePosition(mouseX, mouseY);
    
    // Handle mouse navigation (back/forward buttons, etc.)
    // This would be implemented with proper UI coordinates
}

void BrowserWindow::setHomePage(const String& url) {
    homePage = url;
}

String BrowserWindow::getHomePage() const {
    return homePage;
}

void BrowserWindow::setDefaultSearchEngine(const String& engine) {
    defaultSearchEngine = engine;
}

String BrowserWindow::getDefaultSearchEngine() const {
    return defaultSearchEngine;
}

void BrowserWindow::setShowBookmarksBar(bool show) {
    showBookmarksBar = show;
}

bool BrowserWindow::isShowBookmarksBar() const {
    return showBookmarksBar;
}

void BrowserWindow::setShowStatusBar(bool show) {
    showStatusBar = show;
}

bool BrowserWindow::isShowStatusBar() const {
    return showStatusBar;
}

void BrowserWindow::setShowDeveloperTools(bool show) {
    showDeveloperTools = show;
}

bool BrowserWindow::isShowDeveloperTools() const {
    return showDeveloperTools;
}

void BrowserWindow::renderToolbar() {
    // Toolbar rendering
    int toolbarY = getHeight() - TAB_BAR_HEIGHT - ADDRESS_BAR_HEIGHT - TOOLBAR_HEIGHT;
    glViewport(0, toolbarY, getWidth(), TOOLBAR_HEIGHT);
    
    glBegin(GL_QUADS);
    glColor3f(0.35f, 0.35f, 0.35f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
}

void BrowserWindow::renderBookmarksBar() {
    if (!showBookmarksBar) return;
    
    glViewport(0, getHeight() - TAB_BAR_HEIGHT - ADDRESS_BAR_HEIGHT - TOOLBAR_HEIGHT - BOOKMARKS_BAR_HEIGHT, 
               getWidth(), BOOKMARKS_BAR_HEIGHT);
    
    glBegin(GL_QUADS);
    glColor3f(0.25f, 0.25f, 0.25f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
}

void BrowserWindow::renderStatusBar() {
    if (!showStatusBar) return;
    
    glViewport(0, 0, getWidth(), STATUS_BAR_HEIGHT);
    
    glBegin(GL_QUADS);
    glColor3f(0.3f, 0.3f, 0.3f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
}

void BrowserWindow::renderDeveloperTools() {
    // Developer tools rendering (would be a separate panel)
    int devToolsWidth = getWidth() / 3;
    glViewport(getWidth() - devToolsWidth, TAB_BAR_HEIGHT + ADDRESS_BAR_HEIGHT + TOOLBAR_HEIGHT, 
               devToolsWidth, getHeight() - TAB_BAR_HEIGHT - ADDRESS_BAR_HEIGHT - TOOLBAR_HEIGHT);
    
    glBegin(GL_QUADS);
    glColor3f(0.1f, 0.1f, 0.1f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
}

void BrowserWindow::handleAddressBarInput(const SDL_Event& event) {
    if (!addressBarFocused) return;
    
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN) {
            // Navigate to URL
            if (tabManager) {
                auto activeTab = tabManager->getActiveTab();
                if (activeTab) {
                    tabManager->navigateActiveTab(addressBarText);
                }
            }
            addressBarFocused = false;
        } else if (event.key.keysym.sym == SDLK_ESCAPE) {
            addressBarFocused = false;
        }
    } else if (event.type == SDL_TEXTINPUT) {
        addressBarText += event.text.text;
    }
}

void BrowserWindow::handleTabBarInput(const SDL_Event& event) {
    // Handle tab bar interactions
}

void BrowserWindow::handleToolbarInput(const SDL_Event& event) {
    // Handle toolbar interactions
}

} // namespace zepra