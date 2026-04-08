// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "browser/window.h"
#include "browser/tab_manager.h"
#include "common/constants.h"
#include <iostream>
#include <algorithm>

// Native Rendering Stack (No SDL)
#include "../../source/nxrender-cpp/nxrender_cpp.h"

namespace zepra {

// Window class implementation
Window::Window() 
    : width(0), height(0), x(0), y(0),
      fullscreen(false), minimized(false), maximized(false), visible(false), 
      focused(false), resizable(true) {
}

Window::~Window() {
    destroy();
}

bool Window::create(const String& title, int w, int h, bool fs) {
    if (NXRender::isInitialized()) {
        std::cerr << "NXRender Native Context already exists." << std::endl;
        return false;
    }
    
    this->title = title;
    this->width = w;
    this->height = h;
    this->fullscreen = fs;
    
    NXRender::InitOptions opts;
    opts.width = w;
    opts.height = h;
    opts.title = title.c_str();
    opts.hardwareAccel = true;
    
    if (!NXRender::initWithOptions(opts)) {
        std::cerr << "Failed to init Native NXRender stack" << std::endl;
        return false;
    }
    
    // Bind global resize callback
    NXRender::setResizeCallback([this](int newW, int newH) {
        this->width = newW;
        this->height = newH;
        if (eventCallback) {
            WindowEvent wev(WindowEventType::RESIZE);
            wev.width = newW;
            wev.height = newH;
            eventCallback(wev);
        }
    });
    
    visible = true;
    focused = true;
    
    return true;
}

void Window::destroy() {
    if (NXRender::isInitialized()) {
        NXRender::shutdown();
    }
    visible = false;
    focused = false;
}

void Window::show() { visible = true; }
void Window::hide() { visible = false; }
void Window::minimize() { minimized = true; }
void Window::maximize() { maximized = true; }
void Window::restore() { minimized = false; maximized = false; }
void Window::close() { destroy(); }

void Window::setTitle(const String& newTitle) {
    title = newTitle;
    // Handled natively via NeolyxOS window manager externally
}

String Window::getTitle() const { return title; }

void Window::setSize(int w, int h) {
    width = w;
    height = h;
    NXRender::resize(w, h);
}

void Window::getSize(int& w, int& h) const { w = width; h = height; }
void Window::setPosition(int newX, int newY) { x = newX; y = newY; }
void Window::getPosition(int& newX, int& newY) const { newX = x; newY = y; }

void Window::setFullscreen(bool fs) {
    fullscreen = fs;
    // NeolyxOS native fullscreen toggle
}

bool Window::isFullscreen() const { return fullscreen; }
bool Window::isMinimized() const { return minimized; }
bool Window::isMaximized() const { return maximized; }
bool Window::isVisible() const { return visible; }
bool Window::isFocused() const { return focused; }

void Window::setEventCallback(WindowEventCallback callback) { eventCallback = callback; }
void Window::setRenderCallback(RenderCallback callback) { renderCallback = callback; }

void Window::processEvents() {
    NXRender::processEvents();
}

void Window::render() {
    if (renderCallback) {
        renderCallback();
    }
    if (m_settingsUI.isVisible()) {
        m_settingsUI.render();
    }
}

// Input Native Handlers
bool Window::isKeyPressed(int keyCode) const { return false; /* Native input mapped later */ }
bool Window::isMouseButtonPressed(int button) const { return false; }
void Window::getMousePosition(int& mouseX, int& mouseY) const { mouseX = 0; mouseY = 0; }
void Window::setMousePosition(int mouseX, int mouseY) {}
void Window::showCursor(bool show) {}
void Window::setCursor(const String& cursorType) {}

bool Window::createOpenGLContext(int majorVersion, int minorVersion) { return true; }
void Window::makeCurrent() {}
void Window::swapBuffers() { NXRender::render(); }

void Window::centerOnScreen() {}
void Window::setMinimumSize(int minWidth, int minHeight) {}
void Window::setMaximumSize(int maxWidth, int maxHeight) {}
void Window::setResizable(bool resizable) { this->resizable = resizable; }
bool Window::isResizable() const { return resizable; }

void Window::toggleSettingsUI() {
    if (m_settingsUI.isVisible()) m_settingsUI.hide();
    else m_settingsUI.show();
}

ui::SettingsUI& Window::getSettingsUI() { return m_settingsUI; }


// WindowManager implementation
WindowManager::WindowManager() 
    : defaultWidth(DEFAULT_WINDOW_WIDTH), defaultHeight(DEFAULT_WINDOW_HEIGHT), defaultFullscreen(false) {}

WindowManager::~WindowManager() { closeAll(); }

std::shared_ptr<Window> WindowManager::createWindow(const String& title, int width, int height) {
    auto window = std::make_shared<BrowserWindow>();
    if (window->initialize(title)) {
        windows.push_back(window);
        window->setSize(width, height);
        
        window->setEventCallback([this, window](const WindowEvent& event) {
            handleWindowEvent(window, event);
        });
        
        if (windows.size() == 1) setFocusedWindow(window);
        return window;
    }
    return nullptr;
}

void WindowManager::destroyWindow(std::shared_ptr<Window> window) {
    auto it = std::find(windows.begin(), windows.end(), window);
    if (it != windows.end()) {
        if (focusedWindow == window) focusedWindow = nullptr;
        windows.erase(it);
    }
}

std::vector<std::shared_ptr<Window>> WindowManager::getWindows() const { return windows; }

void WindowManager::processEvents() {
    for (auto& window : windows) window->processEvents();
}

void WindowManager::renderAll() {
    for (auto& window : windows) window->render();
}

void WindowManager::minimizeAll() { for (auto& w : windows) w->minimize(); }
void WindowManager::restoreAll() { for (auto& w : windows) w->restore(); }
void WindowManager::closeAll() {
    for (auto& w : windows) w->close();
    windows.clear();
    focusedWindow = nullptr;
}

std::shared_ptr<Window> WindowManager::getFocusedWindow() const { return focusedWindow; }
void WindowManager::setFocusedWindow(std::shared_ptr<Window> window) { focusedWindow = window; }
void WindowManager::setDefaultWindowSize(int width, int height) { defaultWidth = width; defaultHeight = height; }
void WindowManager::setDefaultFullscreen(bool fullscreen) { defaultFullscreen = fullscreen; }

void WindowManager::handleWindowEvent(std::shared_ptr<Window> window, const WindowEvent& event) {
    switch (event.type) {
        case WindowEventType::FOCUS: setFocusedWindow(window); break;
        case WindowEventType::CLOSE: destroyWindow(window); break;
        default: break;
    }
}

// BrowserWindow implementation
BrowserWindow::BrowserWindow() 
    : homePage("https://ketivee.org"), defaultSearchEngine("KetiveeSearch"),
      showBookmarksBar(true), showStatusBar(true), showDeveloperTools(false),
      addressBarFocused(false), isNavigating(false) {}

BrowserWindow::~BrowserWindow() {}

bool BrowserWindow::initialize(const String& title) {
    if (!create(title, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT)) {
        return false;
    }
    
    setEventCallback([this](const WindowEvent& event) {
        handleBrowserEvents();
    });
    
    return true;
}

void BrowserWindow::setTabManager(std::shared_ptr<TabManager> manager) { tabManager = manager; }
std::shared_ptr<TabManager> BrowserWindow::getTabManager() const { return tabManager; }

// Use NXRender Contexts directly
void BrowserWindow::renderBrowserUI() {
    if (!isVisible()) return;
    
    auto* ctx = NXRender::gpuContext();
    if (ctx) {
        // Native rendering logic leveraging Compositor
    }
    
    renderTabBar();
    renderAddressBar();
    renderToolbar();
    renderBookmarksBar();
    renderContent();
    renderStatusBar();
    
    if (showDeveloperTools) renderDeveloperTools();
    
    swapBuffers(); // Dispatches NXRender::render()
}

void BrowserWindow::renderTabBar() {}
void BrowserWindow::renderAddressBar() {}
void BrowserWindow::renderContent() {}
void BrowserWindow::handleBrowserEvents() { processEvents(); }

void BrowserWindow::handleKeyboardEvent(const WindowEvent& event) {
    if (event.type != WindowEventType::KEY_DOWN) return;
    // Map native keys later
}

void BrowserWindow::handleMouseEvent(const WindowEvent& event) {}

void BrowserWindow::setHomePage(const String& url) { homePage = url; }
String BrowserWindow::getHomePage() const { return homePage; }
void BrowserWindow::setDefaultSearchEngine(const String& engine) { defaultSearchEngine = engine; }
String BrowserWindow::getDefaultSearchEngine() const { return defaultSearchEngine; }
void BrowserWindow::setShowBookmarksBar(bool show) { showBookmarksBar = show; }
bool BrowserWindow::isShowBookmarksBar() const { return showBookmarksBar; }
void BrowserWindow::setShowStatusBar(bool show) { showStatusBar = show; }
bool BrowserWindow::isShowStatusBar() const { return showStatusBar; }
void BrowserWindow::setShowDeveloperTools(bool show) { showDeveloperTools = show; }
bool BrowserWindow::isShowDeveloperTools() const { return showDeveloperTools; }

void BrowserWindow::renderToolbar() {}
void BrowserWindow::renderBookmarksBar() {}
void BrowserWindow::renderStatusBar() {}
void BrowserWindow::renderDeveloperTools() {}

void BrowserWindow::handleAddressBarInput(const WindowEvent& event) {}
void BrowserWindow::handleTabBarInput(const WindowEvent& event) {}
void BrowserWindow::handleToolbarInput(const WindowEvent& event) {}

} // namespace zepra