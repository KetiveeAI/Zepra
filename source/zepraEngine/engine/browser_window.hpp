#pragma once

/**
 * @file browser_window.hpp
 * @brief Browser window management
 */

#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace Zepra::WebCore {

class RenderNode;
class DisplayList;

}

namespace Zepra::Engine {

/**
 * @brief Window event types
 */
enum class WindowEventType {
    Resize,
    Close,
    Focus,
    Blur,
    Minimize,
    Maximize,
    Move
};

/**
 * @brief Window event data
 */
struct WindowEvent {
    WindowEventType type;
    int width = 0, height = 0;
    int x = 0, y = 0;
};

/**
 * @brief Mouse button identifiers
 */
enum class MouseButton {
    Left = 0,
    Middle = 1,
    Right = 2
};

/**
 * @brief Input event types
 */
enum class InputEventType {
    MouseMove,
    MouseDown,
    MouseUp,
    MouseWheel,
    KeyDown,
    KeyUp,
    KeyPress,
    TouchStart,
    TouchMove,
    TouchEnd
};

/**
 * @brief Input event data
 */
struct InputEvent {
    InputEventType type;
    
    // Mouse data
    int mouseX = 0, mouseY = 0;
    int wheelDelta = 0;
    MouseButton button = MouseButton::Left;
    
    // Keyboard data
    int keyCode = 0;
    std::string key;
    bool ctrl = false, shift = false, alt = false, meta = false;
    
    // Touch data
    std::vector<std::pair<int, int>> touches;
};

/**
 * @brief Platform-agnostic window interface
 */
class BrowserWindow {
public:
    BrowserWindow(int width, int height, const std::string& title);
    virtual ~BrowserWindow();
    
    // Lifecycle
    virtual bool create() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void close() = 0;
    virtual bool isVisible() const = 0;
    
    // Properties
    virtual void setTitle(const std::string& title) = 0;
    virtual std::string title() const { return title_; }
    virtual void resize(int width, int height) = 0;
    virtual void move(int x, int y) = 0;
    virtual int width() const { return width_; }
    virtual int height() const { return height_; }
    virtual int x() const { return x_; }
    virtual int y() const { return y_; }
    
    // State
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void restore() = 0;
    virtual void setFullscreen(bool fullscreen) = 0;
    virtual bool isFullscreen() const { return fullscreen_; }
    
    // Rendering
    virtual void* nativeHandle() const = 0;
    virtual void requestRedraw() = 0;
    
    // Event handling
    using WindowEventCallback = std::function<void(const WindowEvent&)>;
    using InputEventCallback = std::function<void(const InputEvent&)>;
    
    void setWindowEventCallback(WindowEventCallback callback) { 
        windowCallback_ = std::move(callback); 
    }
    void setInputEventCallback(InputEventCallback callback) { 
        inputCallback_ = std::move(callback); 
    }
    
protected:
    void dispatchWindowEvent(const WindowEvent& event);
    void dispatchInputEvent(const InputEvent& event);
    
    std::string title_;
    int width_, height_;
    int x_ = 0, y_ = 0;
    bool fullscreen_ = false;
    
    WindowEventCallback windowCallback_;
    InputEventCallback inputCallback_;
};

/**
 * @brief Tab/page within browser
 */
class BrowserTab {
public:
    explicit BrowserTab(const std::string& url = "");
    
    // Navigation
    void navigate(const std::string& url);
    void reload();
    void goBack();
    void goForward();
    void stop();
    
    // State
    const std::string& url() const { return url_; }
    const std::string& title() const { return title_; }
    bool isLoading() const { return loading_; }
    float loadProgress() const { return loadProgress_; }
    
    // Rendering
    WebCore::RenderNode* renderTree() const { return renderTree_.get(); }
    void paint(WebCore::DisplayList& displayList);
    
    // JavaScript
    void executeScript(const std::string& script);
    
    // Input forwarding
    void handleInput(const InputEvent& event);
    
private:
    std::string url_;
    std::string title_;
    bool loading_ = false;
    float loadProgress_ = 0;
    
    std::unique_ptr<WebCore::RenderNode> renderTree_;
    std::vector<std::string> history_;
    int historyIndex_ = -1;
};

/**
 * @brief Browser shell managing tabs and windows
 */
class BrowserShell {
public:
    BrowserShell();
    
    // Window
    void createWindow(int width, int height);
    BrowserWindow* window() const { return window_.get(); }
    
    // Tabs
    BrowserTab* newTab(const std::string& url = "");
    void closeTab(int index);
    void switchToTab(int index);
    BrowserTab* activeTab() const;
    int tabCount() const { return static_cast<int>(tabs_.size()); }
    
    // Main loop
    void run();
    void quit();
    bool isRunning() const { return running_; }
    
    // Event processing
    void processEvents();
    void render();
    
private:
    std::unique_ptr<BrowserWindow> window_;
    std::vector<std::unique_ptr<BrowserTab>> tabs_;
    int activeTabIndex_ = -1;
    bool running_ = false;
};

} // namespace Zepra::Engine
