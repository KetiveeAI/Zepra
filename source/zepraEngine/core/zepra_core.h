// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_CORE_H
#define ZEPRA_CORE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>
#include <map>
#include <queue>

// Forward declarations
struct json_object;

namespace ZepraCore {

// Core system states
enum class SystemState {
    INITIALIZING,
    READY,
    RUNNING,
    PAUSED,
    SHUTDOWN,
    ERROR
};

// Download states
enum class DownloadState {
    QUEUED,
    DOWNLOADING,
    PAUSED,
    COMPLETED,
    FAILED,
    CANCELLED
};

// UI element types
enum class UIElementType {
    BUTTON,
    TEXT_INPUT,
    LABEL,
    PROGRESS_BAR,
    TAB,
    WINDOW,
    DIALOG,
    DOWNLOAD_ITEM
};

// Download information
struct DownloadInfo {
    std::string id;
    std::string url;
    std::string filename;
    std::string localPath;
    size_t totalSize;
    size_t downloadedSize;
    double progress;
    DownloadState state;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point estimatedCompletion;
    double speed; // bytes per second
    std::string errorMessage;
    
    DownloadInfo() : totalSize(0), downloadedSize(0), progress(0.0), state(DownloadState::QUEUED), speed(0.0) {}
};

// Authentication states
enum class AuthState {
    UNAUTHENTICATED,
    AUTHENTICATING,
    AUTHENTICATED,
    EXPIRED,
    LOCKED,
    ERROR
};

// User information
struct UserInfo {
    std::string id;
    std::string email;
    std::string firstName;
    std::string lastName;
    std::string avatar;
    bool emailVerified;
    std::chrono::system_clock::time_point lastLogin;
    std::chrono::system_clock::time_point createdAt;
    
    UserInfo() : emailVerified(false) {}
};

// Authentication token
struct AuthToken {
    std::string token;
    std::string refreshToken;
    std::chrono::system_clock::time_point expiresAt;
    std::chrono::system_clock::time_point refreshExpiresAt;
    bool isValid;
    
    AuthToken() : isValid(false) {}
};

// UI element base class
class UIElement {
public:
    UIElement(int x, int y, int width, int height, UIElementType type);
    virtual ~UIElement() = default;
    
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual bool handleEvent(const SDL_Event& event) = 0;
    virtual void update() = 0;
    
    void setPosition(int x, int y);
    void setSize(int width, int height);
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool isVisible() const { return m_visible; }
    bool isEnabled() const { return m_enabled; }
    UIElementType getType() const { return m_type; }

protected:
    int m_x, m_y, m_width, m_height;
    bool m_visible;
    bool m_enabled;
    bool m_hovered;
    bool m_focused;
    UIElementType m_type;
};

// Button class
class Button : public UIElement {
public:
    Button(int x, int y, int width, int height, const std::string& text);
    ~Button() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setText(const std::string& text);
    void setOnClick(std::function<void()> callback);
    void setColors(SDL_Color normal, SDL_Color hover, SDL_Color pressed);
    
    const std::string& getText() const { return m_text; }

private:
    std::string m_text;
    std::function<void()> m_onClick;
    SDL_Color m_normalColor;
    SDL_Color m_hoverColor;
    SDL_Color m_pressedColor;
    bool m_pressed;
};

// Text input class
class TextInput : public UIElement {
public:
    TextInput(int x, int y, int width, int height, const std::string& placeholder = "");
    ~TextInput() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder);
    void setPasswordMode(bool password);
    void setOnTextChange(std::function<void(const std::string&)> callback);
    void setOnEnter(std::function<void()> callback);
    
    const std::string& getText() const { return m_text; }
    bool isPasswordMode() const { return m_passwordMode; }

private:
    std::string m_text;
    std::string m_placeholder;
    bool m_passwordMode;
    std::function<void(const std::string&)> m_onTextChange;
    std::function<void()> m_onEnter;
    int m_cursorPos;
    bool m_showCursor;
    Uint32 m_cursorBlinkTime;
};

// Progress bar class
class ProgressBar : public UIElement {
public:
    ProgressBar(int x, int y, int width, int height);
    ~ProgressBar() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setProgress(double progress);
    void setText(const std::string& text);
    void setColors(SDL_Color background, SDL_Color foreground, SDL_Color text);
    
    double getProgress() const { return m_progress; }
    const std::string& getText() const { return m_text; }

private:
    double m_progress;
    std::string m_text;
    SDL_Color m_backgroundColor;
    SDL_Color m_foregroundColor;
    SDL_Color m_textColor;
};

// Download item UI class
class DownloadItem : public UIElement {
public:
    DownloadItem(int x, int y, int width, int height, const DownloadInfo& download);
    ~DownloadItem() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setDownload(const DownloadInfo& download);
    void setOnPause(std::function<void(const std::string&)> callback);
    void setOnResume(std::function<void(const std::string&)> callback);
    void setOnCancel(std::function<void(const std::string&)> callback);
    void setOnOpen(std::function<void(const std::string&)> callback);
    
    const DownloadInfo& getDownload() const { return m_download; }
    std::string getDownloadId() const { return m_download.id; }

private:
    DownloadInfo m_download;
    std::function<void(const std::string&)> m_onPause;
    std::function<void(const std::string&)> m_onResume;
    std::function<void(const std::string&)> m_onCancel;
    std::function<void(const std::string&)> m_onOpen;
    
    Button* m_pauseButton;
    Button* m_resumeButton;
    Button* m_cancelButton;
    Button* m_openButton;
    ProgressBar* m_progressBar;
    Label* m_filenameLabel;
    Label* m_speedLabel;
    Label* m_statusLabel;
};

// Main Zepra Core class - unified system
class ZepraCore {
public:
    static ZepraCore& getInstance();
    
    // Initialization
    bool initialize(const std::string& configPath = "config/zepra_config.json");
    void shutdown();
    
    // Main loop
    void run();
    void stop();
    
    // System state
    SystemState getState() const { return m_state; }
    bool isRunning() const { return m_state == SystemState::RUNNING; }
    
    // Authentication
    bool login(const std::string& email, const std::string& password);
    bool loginWith2FA(const std::string& tempToken, const std::string& code);
    bool logout();
    bool checkSession();
    AuthState getAuthState() const { return m_authState; }
    UserInfo getCurrentUser() const { return m_currentUser; }
    bool isAuthenticated() const { return m_authState == AuthState::AUTHENTICATED; }
    
    // Download management
    std::string startDownload(const std::string& url, const std::string& filename = "");
    bool pauseDownload(const std::string& downloadId);
    bool resumeDownload(const std::string& downloadId);
    bool cancelDownload(const std::string& downloadId);
    bool openDownload(const std::string& downloadId);
    std::vector<DownloadInfo> getAllDownloads() const;
    DownloadInfo getDownload(const std::string& downloadId) const;
    
    // UI management
    void showDownloadManager();
    void hideDownloadManager();
    void showLoginDialog();
    void hideLoginDialog();
    void showTwoFactorDialog(const std::string& tempToken);
    void hideTwoFactorDialog();
    
    // Callbacks
    void setOnAuthStateChanged(std::function<void(AuthState, const UserInfo&)> callback);
    void setOnDownloadCompleted(std::function<void(const DownloadInfo&)> callback);
    void setOnDownloadFailed(std::function<void(const DownloadInfo&)> callback);
    
    // Configuration
    void setConfig(const std::string& key, const std::string& value);
    std::string getConfig(const std::string& key, const std::string& defaultValue = "") const;

private:
    ZepraCore();
    ~ZepraCore();
    
    // SDL management
    bool initializeSDL();
    void shutdownSDL();
    
    // Component initialization
    bool initializeComponents();
    bool initializeAuthentication();
    bool initializeDownloadManager();
    bool initializeUI();
    
    // Event handling
    void handleEvents();
    void handleWindowEvent(const SDL_Event& event);
    void handleKeyboardEvent(const SDL_Event& event);
    void handleMouseEvent(const SDL_Event& event);
    
    // Update and render
    void update();
    void render();
    
    // Download management
    void updateDownloads();
    void processDownloadQueue();
    bool downloadFile(const std::string& url, const std::string& localPath, 
                     std::function<void(size_t, size_t)> progressCallback);
    
    // Authentication
    bool verifyToken();
    void clearSession();
    
    // UI rendering
    void renderUI();
    void renderDownloadManager();
    void renderLoginDialog();
    void renderTwoFactorDialog();
    
    // Member variables
    SystemState m_state;
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    bool m_running;
    
    // Authentication
    AuthState m_authState;
    AuthToken m_currentToken;
    UserInfo m_currentUser;
    std::string m_authServerUrl;
    
    // Download management
    std::map<std::string, DownloadInfo> m_downloads;
    std::queue<std::string> m_downloadQueue;
    std::mutex m_downloadMutex;
    std::atomic<bool> m_downloadManagerVisible;
    
    // UI elements
    std::vector<std::unique_ptr<UIElement>> m_uiElements;
    std::unique_ptr<DownloadItem> m_downloadManager;
    std::unique_ptr<TextInput> m_loginEmailInput;
    std::unique_ptr<TextInput> m_loginPasswordInput;
    std::unique_ptr<TextInput> m_twoFactorInput;
    std::unique_ptr<Button> m_loginButton;
    std::unique_ptr<Button> m_twoFactorButton;
    
    // Callbacks
    std::function<void(AuthState, const UserInfo&)> m_onAuthStateChanged;
    std::function<void(const DownloadInfo&)> m_onDownloadCompleted;
    std::function<void(const DownloadInfo&)> m_onDownloadFailed;
    
    // Configuration
    std::map<std::string, std::string> m_config;
    
    // Threading
    mutable std::mutex m_mutex;
    std::atomic<bool> m_initialized;
    
    // Disable copy constructor and assignment
    ZepraCore(const ZepraCore&) = delete;
    ZepraCore& operator=(const ZepraCore&) = delete;
};

// Utility functions
namespace Utils {
    SDL_Color createColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, 
                   int x, int y, SDL_Color color);
    void renderRect(SDL_Renderer* renderer, int x, int y, int width, int height, 
                   SDL_Color color, bool filled = true);
    void renderBorder(SDL_Renderer* renderer, int x, int y, int width, int height, 
                     SDL_Color color, int thickness = 1);
    bool isPointInRect(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight);
    std::string formatFileSize(size_t bytes);
    std::string formatSpeed(double bytesPerSecond);
    std::string formatTime(std::chrono::system_clock::time_point time);
    std::string generateUUID();
    std::string getCurrentTimestamp();
    bool isValidEmail(const std::string& email);
    std::string extractFilenameFromUrl(const std::string& url);
    std::string sanitizeFilename(const std::string& filename);
}

// Colors
namespace Colors {
    const SDL_Color WHITE = {255, 255, 255, 255};
    const SDL_Color BLACK = {0, 0, 0, 255};
    const SDL_Color GRAY = {128, 128, 128, 255};
    const SDL_Color LIGHT_GRAY = {192, 192, 192, 255};
    const SDL_Color DARK_GRAY = {64, 64, 64, 255};
    const SDL_Color BLUE = {0, 122, 255, 255};
    const SDL_Color GREEN = {0, 255, 0, 255};
    const SDL_Color RED = {255, 0, 0, 255};
    const SDL_Color YELLOW = {255, 255, 0, 255};
    const SDL_Color ORANGE = {255, 165, 0, 255};
    const SDL_Color PURPLE = {128, 0, 128, 255};
    const SDL_Color TRANSPARENT = {0, 0, 0, 0};
}

} // namespace ZepraCore

#endif // ZEPRA_CORE_H 