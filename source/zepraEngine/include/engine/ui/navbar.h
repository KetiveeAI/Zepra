/**
 * @file navbar.h
 * @brief Navigation bar component with back/forward/refresh/home buttons
 */

#ifndef ZEPRA_UI_NAVBAR_H
#define ZEPRA_UI_NAVBAR_H

#include <string>
#include <functional>
#include <memory>

namespace zepra {
namespace ui {

/**
 * Navigation action types
 */
enum class NavAction {
    Back,
    Forward,
    Refresh,
    Stop,
    Home
};

/**
 * NavBar configuration
 */
struct NavBarConfig {
    float buttonSize = 32.0f;
    float spacing = 4.0f;
    float iconSize = 20.0f;
    bool showHome = true;
    std::string homeUrl = "zepra://start";
};

/**
 * NavBar - Browser navigation buttons component
 * 
 * Features:
 * - Back/Forward with history awareness
 * - Refresh/Stop toggle based on loading state
 * - Home button
 * - Hover and pressed states
 * - Disabled state when unavailable
 */
class NavBar {
public:
    using ActionCallback = std::function<void(NavAction action)>;

    NavBar();
    explicit NavBar(const NavBarConfig& config);
    ~NavBar();

    // Configuration
    void setConfig(const NavBarConfig& config);
    NavBarConfig getConfig() const;

    // State
    void setCanGoBack(bool canGoBack);
    void setCanGoForward(bool canGoForward);
    void setIsLoading(bool isLoading);
    
    bool canGoBack() const;
    bool canGoForward() const;
    bool isLoading() const;

    // Callbacks
    void setActionCallback(ActionCallback callback);

    // Rendering
    void setBounds(float x, float y, float width, float height);
    void render();

    // Event handling
    bool handleMouseClick(float x, float y);
    bool handleMouseMove(float x, float y);
    bool handleMouseDown(float x, float y);
    bool handleMouseUp(float x, float y);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ui
} // namespace zepra

#endif // ZEPRA_UI_NAVBAR_H
