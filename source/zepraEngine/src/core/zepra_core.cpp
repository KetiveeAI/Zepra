#include "core/zepra_core.h"
#include <nxhttp.h>
#include <nlohmann/json.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cstring>
#include <filesystem>

// JSON library namespace alias
using json = nlohmann::json;

namespace ZepraCore {

// Singleton instance
ZepraCore& ZepraCore::getInstance() {
    static ZepraCore instance;
    return instance;
}

ZepraCore::ZepraCore() 
    : m_state(SystemState::INITIALIZING)
    , m_window(nullptr)
    , m_renderer(nullptr)
    , m_font(nullptr)
    , m_running(false)
    , m_authState(AuthState::UNAUTHENTICATED)
    , m_downloadManagerVisible(false)
    , m_initialized(false) {
    // nxhttp does not require a persistent handle
    std::cout << "[Zepra] Zepra Core initializing..." << std::endl;
}

ZepraCore::~ZepraCore() {
    shutdown();
}

bool ZepraCore::initialize(const std::string& configPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    std::cout << "[Zepra] Initializing Zepra Core Browser..." << std::endl;
    
    // Load configuration
    if (!loadConfig(configPath)) {
        std::cout << "[Warn] Using default configuration" << std::endl;
    }
    
    // Initialize SDL
    if (!initializeSDL()) {
        std::cerr << "[Error] Failed to initialize SDL" << std::endl;
        return false;
    }
    
    // Initialize components
    if (!initializeComponents()) {
        std::cerr << "[Error] Failed to initialize components" << std::endl;
        return false;
    }
    
    // Initialize authentication
    if (!initializeAuthentication()) {
        std::cerr << "[Error] Failed to initialize authentication" << std::endl;
        return false;
    }
    
    // Initialize download manager
    if (!initializeDownloadManager()) {
        std::cerr << "[Error] Failed to initialize download manager" << std::endl;
        return false;
    }
    
    // Initialize UI
    if (!initializeUI()) {
        std::cerr << "[Error] Failed to initialize UI" << std::endl;
        return false;
    }
    
    m_initialized = true;
    m_state = SystemState::READY;
    
    std::cout << "[OK] Zepra Core Browser initialized successfully!" << std::endl;
    return true;
}

void ZepraCore::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    std::cout << "[Zepra] Shutting down Zepra Core Browser..." << std::endl;
    
    // Stop the main loop
    m_running = false;
    
    // Shutdown components
    shutdownSDL();
    
    m_initialized = false;
    m_state = SystemState::SHUTDOWN;
    
    std::cout << "[OK] Zepra Core Browser shutdown complete" << std::endl;
}

void ZepraCore::run() {
    if (!m_initialized) {
        std::cerr << "[Error] Zepra Core not initialized" << std::endl;
        return;
    }
    
    m_state = SystemState::RUNNING;
    m_running = true;
    
    std::cout << "[Run] Starting Zepra Core Browser main loop..." << std::endl;
    
    // Main loop
    while (m_running) {
        handleEvents();
        update();
        render();
        
        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }
}

void ZepraCore::stop() {
    m_running = false;
    m_state = SystemState::SHUTDOWN;
}

// SDL Management
bool ZepraCore::initializeSDL() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }
    
    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        return false;
    }
    
    // Create window
    m_window = SDL_CreateWindow(
        "Zepra Core Browser",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
    );
    
    if (!m_window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create renderer
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Load font
    m_font = TTF_OpenFont("assets/fonts/Roboto-Regular.ttf", 16);
    if (!m_font) {
        // Try system font as fallback
        m_font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 16);
        if (!m_font) {
            std::cerr << "Could not load font! TTF_Error: " << TTF_GetError() << std::endl;
            return false;
        }
    }
    
    return true;
}

void ZepraCore::shutdownSDL() {
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

// Component initialization
bool ZepraCore::initializeComponents() {
    try {
        // nxhttp does not require global initialization
        std::cout << "[Zepra] HTTP client (nxhttp) ready" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception during component initialization: " << e.what() << std::endl;
        return false;
    }
}

bool ZepraCore::initializeAuthentication() {
    m_authServerUrl = getConfig("auth.serverUrl", "https://auth.ketivee.com");
    m_authState = AuthState::UNAUTHENTICATED;
    
    // Check existing session
    checkSession();
    
    return true;
}

bool ZepraCore::initializeDownloadManager() {
    // Create downloads directory
    std::filesystem::create_directories("downloads");
    
    return true;
}

bool ZepraCore::initializeUI() {
    // Create UI elements
    m_loginEmailInput = std::make_unique<TextInput>(400, 200, 300, 30, "Email");
    m_loginPasswordInput = std::make_unique<TextInput>(400, 250, 300, 30, "Password");
    m_loginPasswordInput->setPasswordMode(true);
    
    m_loginButton = std::make_unique<Button>(400, 300, 100, 30, "Login");
    m_loginButton->setOnClick([this]() {
        if (m_loginEmailInput && m_loginPasswordInput) {
            login(m_loginEmailInput->getText(), m_loginPasswordInput->getText());
        }
    });
    
    m_twoFactorInput = std::make_unique<TextInput>(400, 200, 300, 30, "2FA Code");
    m_twoFactorButton = std::make_unique<Button>(400, 250, 100, 30, "Verify");
    
    // Add UI elements
    m_uiElements.push_back(std::move(m_loginEmailInput));
    m_uiElements.push_back(std::move(m_loginPasswordInput));
    m_uiElements.push_back(std::move(m_loginButton));
    m_uiElements.push_back(std::move(m_twoFactorInput));
    m_uiElements.push_back(std::move(m_twoFactorButton));
    
    return true;
}

// Event handling
void ZepraCore::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Handle UI events
        for (auto& element : m_uiElements) {
            if (element->handleEvent(event)) {
                break; // Event handled
            }
        }
        
        // Handle general events
        switch (event.type) {
            case SDL_QUIT:
                m_running = false;
                break;
                
            case SDL_KEYDOWN:
                handleKeyboardEvent(event);
                break;
                
            case SDL_WINDOWEVENT:
                handleWindowEvent(event);
                break;
        }
    }
}

void ZepraCore::handleWindowEvent(const SDL_Event& event) {
    switch (event.window.event) {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            // Handle window resize
            break;
    }
}

void ZepraCore::handleKeyboardEvent(const SDL_Event& event) {
    switch (event.key.keysym.sym) {
        case SDLK_F12:
            // Toggle dev tools
            std::cout << "🔧 Dev Tools toggled" << std::endl;
            break;
            
        case SDLK_F5:
            // Refresh current page
            std::cout << "🔄 Page refresh" << std::endl;
            break;
            
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            // Handle Ctrl+ shortcuts
            break;
    }
}

// Update and render
void ZepraCore::update() {
    // Update authentication
    checkSession();
    
    // Update downloads
    updateDownloads();
    
    // Update UI elements
    for (auto& element : m_uiElements) {
        element->update();
    }
}

void ZepraCore::render() {
    // Clear screen
    SDL_SetRenderDrawColor(m_renderer, 240, 240, 240, 255);
    SDL_RenderClear(m_renderer);
    
    // Render UI
    renderUI();
    
    // Present render
    SDL_RenderPresent(m_renderer);
}

void ZepraCore::renderUI() {
    // Render authentication dialogs
    if (m_authState == AuthState::UNAUTHENTICATED) {
        renderLoginDialog();
    }
    
    // Render download manager if visible
    if (m_downloadManagerVisible) {
        renderDownloadManager();
    }
}

void ZepraCore::renderLoginDialog() {
    // Render login dialog background
    Utils::renderRect(m_renderer, 350, 150, 400, 200, Colors::WHITE);
    Utils::renderBorder(m_renderer, 350, 150, 400, 200, Colors::GRAY);
    
    // Render title
    Utils::renderText(m_renderer, m_font, "Zepra Core Browser Login", 400, 170, Colors::BLACK);
    
    // Render UI elements
    for (auto& element : m_uiElements) {
        if (element->isVisible()) {
            element->render(m_renderer);
        }
    }
}

void ZepraCore::renderDownloadManager() {
    // Render download manager background
    Utils::renderRect(m_renderer, 50, 50, 800, 500, Colors::WHITE);
    Utils::renderBorder(m_renderer, 50, 50, 800, 500, Colors::GRAY);
    
    // Render title
    Utils::renderText(m_renderer, m_font, "Download Manager", 70, 70, Colors::BLACK);
    
    // Render download list
    int y = 100;
    for (const auto& download : m_downloads) {
        renderDownloadItem(download.second, 70, y);
        y += 60;
    }
}

void ZepraCore::renderDownloadItem(const DownloadInfo& download, int x, int y) {
    // Render download item background
    Utils::renderRect(m_renderer, x, y, 750, 50, Colors::LIGHT_GRAY);
    
    // Render filename
    Utils::renderText(m_renderer, m_font, download.filename, x + 10, y + 5, Colors::BLACK);
    
    // Render progress bar
    int progressWidth = static_cast<int>(750 * download.progress);
    Utils::renderRect(m_renderer, x + 10, y + 25, progressWidth, 10, Colors::BLUE);
    Utils::renderRect(m_renderer, x + 10, y + 25, 750, 10, Colors::GRAY, false);
    
    // Render status
    std::string status;
    switch (download.state) {
        case DownloadState::QUEUED: status = "Queued"; break;
        case DownloadState::DOWNLOADING: status = "Downloading"; break;
        case DownloadState::PAUSED: status = "Paused"; break;
        case DownloadState::COMPLETED: status = "Completed"; break;
        case DownloadState::FAILED: status = "Failed"; break;
        case DownloadState::CANCELLED: status = "Cancelled"; break;
    }
    
    Utils::renderText(m_renderer, m_font, status, x + 10, y + 40, Colors::BLACK);
    
    // Render progress text
    std::string progressText = Utils::formatFileSize(download.downloadedSize) + " / " + 
                              Utils::formatFileSize(download.totalSize);
    Utils::renderText(m_renderer, m_font, progressText, x + 600, y + 25, Colors::BLACK);
}

// Authentication methods
bool ZepraCore::login(const std::string& email, const std::string& password) {
    std::cout << "🔐 Login attempt for: " << email << std::endl;
    
    // Simulate authentication (replace with actual implementation)
    if (email == "test@ketivee.com" && password == "password") {
        m_authState = AuthState::AUTHENTICATED;
        m_currentUser.email = email;
        m_currentUser.firstName = "Test";
        m_currentUser.lastName = "User";
        
        if (m_onAuthStateChanged) {
            m_onAuthStateChanged(m_authState, m_currentUser);
        }
        
        std::cout << "[OK] Login successful" << std::endl;
        return true;
    }
    
    std::cout << "[Error] Login failed" << std::endl;
    return false;
}

bool ZepraCore::loginWith2FA(const std::string& tempToken, const std::string& code) {
    std::cout << "🔐 2FA verification with code: " << code << std::endl;
    
    // Simulate 2FA verification
    if (code == "123456") {
        m_authState = AuthState::AUTHENTICATED;
        
        if (m_onAuthStateChanged) {
            m_onAuthStateChanged(m_authState, m_currentUser);
        }
        
        std::cout << "[OK] 2FA verification successful" << std::endl;
        return true;
    }
    
    std::cout << "[Error] 2FA verification failed" << std::endl;
    return false;
}

bool ZepraCore::logout() {
    m_authState = AuthState::UNAUTHENTICATED;
    clearSession();
    
    if (m_onAuthStateChanged) {
        m_onAuthStateChanged(m_authState, m_currentUser);
    }
    
    std::cout << "🔐 Logout successful" << std::endl;
    return true;
}

bool ZepraCore::checkSession() {
    // Check if token is still valid
    if (m_authState == AuthState::AUTHENTICATED) {
        // Verify token with server (simplified)
        return true;
    }
    
    return false;
}

void ZepraCore::clearSession() {
    m_currentToken = AuthToken();
    m_currentUser = UserInfo();
    m_authState = AuthState::UNAUTHENTICATED;
}

// Download management
std::string ZepraCore::startDownload(const std::string& url, const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_downloadMutex);
    
    DownloadInfo download;
    download.id = Utils::generateUUID();
    download.url = url;
    download.filename = filename.empty() ? Utils::extractFilenameFromUrl(url) : filename;
    download.localPath = "downloads/" + download.filename;
    download.state = DownloadState::QUEUED;
    download.startTime = std::chrono::system_clock::now();
    
    m_downloads[download.id] = download;
    m_downloadQueue.push(download.id);
    
    std::cout << "📥 Started download: " << download.filename << std::endl;
    
    return download.id;
}

bool ZepraCore::pauseDownload(const std::string& downloadId) {
    std::lock_guard<std::mutex> lock(m_downloadMutex);
    
    auto it = m_downloads.find(downloadId);
    if (it != m_downloads.end()) {
        it->second.state = DownloadState::PAUSED;
        std::cout << "⏸️ Paused download: " << it->second.filename << std::endl;
        return true;
    }
    
    return false;
}

bool ZepraCore::resumeDownload(const std::string& downloadId) {
    std::lock_guard<std::mutex> lock(m_downloadMutex);
    
    auto it = m_downloads.find(downloadId);
    if (it != m_downloads.end()) {
        it->second.state = DownloadState::DOWNLOADING;
        std::cout << "▶️ Resumed download: " << it->second.filename << std::endl;
        return true;
    }
    
    return false;
}

bool ZepraCore::cancelDownload(const std::string& downloadId) {
    std::lock_guard<std::mutex> lock(m_downloadMutex);
    
    auto it = m_downloads.find(downloadId);
    if (it != m_downloads.end()) {
        it->second.state = DownloadState::CANCELLED;
        std::cout << "[Error] Cancelled download: " << it->second.filename << std::endl;
        return true;
    }
    
    return false;
}

bool ZepraCore::openDownload(const std::string& downloadId) {
    std::lock_guard<std::mutex> lock(m_downloadMutex);
    
    auto it = m_downloads.find(downloadId);
    if (it != m_downloads.end() && it->second.state == DownloadState::COMPLETED) {
        std::cout << "📂 Opening download: " << it->second.filename << std::endl;
        // Open file with system default application
        return true;
    }
    
    return false;
}

std::vector<DownloadInfo> ZepraCore::getAllDownloads() const {
    std::lock_guard<std::mutex> lock(m_downloadMutex);
    
    std::vector<DownloadInfo> result;
    for (const auto& pair : m_downloads) {
        result.push_back(pair.second);
    }
    
    return result;
}

DownloadInfo ZepraCore::getDownload(const std::string& downloadId) const {
    std::lock_guard<std::mutex> lock(m_downloadMutex);
    
    auto it = m_downloads.find(downloadId);
    if (it != m_downloads.end()) {
        return it->second;
    }
    
    return DownloadInfo();
}

void ZepraCore::updateDownloads() {
    std::lock_guard<std::mutex> lock(m_downloadMutex);
    
    // Process download queue
    processDownloadQueue();
    
    // Update download progress
    for (auto& pair : m_downloads) {
        DownloadInfo& download = pair.second;
        
        if (download.state == DownloadState::DOWNLOADING) {
            // Update progress (simplified)
            if (download.downloadedSize < download.totalSize) {
                download.downloadedSize += 1024; // Simulate download progress
                download.progress = static_cast<double>(download.downloadedSize) / download.totalSize;
                
                if (download.downloadedSize >= download.totalSize) {
                    download.state = DownloadState::COMPLETED;
                    download.progress = 1.0;
                    
                    if (m_onDownloadCompleted) {
                        m_onDownloadCompleted(download);
                    }
                    
                    std::cout << "[OK] Download completed: " << download.filename << std::endl;
                }
            }
        }
    }
}

void ZepraCore::processDownloadQueue() {
    while (!m_downloadQueue.empty()) {
        std::string downloadId = m_downloadQueue.front();
        m_downloadQueue.pop();
        
        auto it = m_downloads.find(downloadId);
        if (it != m_downloads.end()) {
            DownloadInfo& download = it->second;
            download.state = DownloadState::DOWNLOADING;
            
            // Start actual download (simplified)
            download.totalSize = 1024 * 1024; // 1MB for demo
            download.downloadedSize = 0;
            download.progress = 0.0;
        }
    }
}

bool ZepraCore::downloadFile(const std::string& url, const std::string& localPath, 
                             std::function<void(size_t, size_t)> progressCallback) {
    try {
        nx::HttpClient client;
        auto response = client.get(url);
        
        if (!response.ok()) {
            std::cerr << "Download failed: HTTP " << response.status() << std::endl;
            return false;
        }
        
        // Write response body to file
        std::ofstream file(localPath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << localPath << std::endl;
            return false;
        }
        
        std::string body = response.body();
        file.write(body.data(), body.size());
        file.close();
        
        if (progressCallback) {
            progressCallback(body.size(), body.size());
        }
        
        return true;
    } catch (const nx::HttpException& e) {
        std::cerr << "Download error: " << e.what() << std::endl;
        return false;
    }
}

// UI management
void ZepraCore::showDownloadManager() {
    m_downloadManagerVisible = true;
    std::cout << "📥 Download manager shown" << std::endl;
}

void ZepraCore::hideDownloadManager() {
    m_downloadManagerVisible = false;
    std::cout << "📥 Download manager hidden" << std::endl;
}

void ZepraCore::showLoginDialog() {
    // Login dialog is always shown when not authenticated
    std::cout << "🔐 Login dialog shown" << std::endl;
}

void ZepraCore::hideLoginDialog() {
    // Login dialog is hidden when authenticated
    std::cout << "🔐 Login dialog hidden" << std::endl;
}

void ZepraCore::showTwoFactorDialog(const std::string& tempToken) {
    std::cout << "🔐 2FA dialog shown" << std::endl;
}

void ZepraCore::hideTwoFactorDialog() {
    std::cout << "🔐 2FA dialog hidden" << std::endl;
}

// Callbacks
void ZepraCore::setOnAuthStateChanged(std::function<void(AuthState, const UserInfo&)> callback) {
    m_onAuthStateChanged = callback;
}

void ZepraCore::setOnDownloadCompleted(std::function<void(const DownloadInfo&)> callback) {
    m_onDownloadCompleted = callback;
}

void ZepraCore::setOnDownloadFailed(std::function<void(const DownloadInfo&)> callback) {
    m_onDownloadFailed = callback;
}

// Configuration
void ZepraCore::setConfig(const std::string& key, const std::string& value) {
    m_config[key] = value;
}

std::string ZepraCore::getConfig(const std::string& key, const std::string& defaultValue) const {
    auto it = m_config.find(key);
    return it != m_config.end() ? it->second : defaultValue;
}

bool ZepraCore::loadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        // Parse JSON configuration
        json config;
        file >> config;
        
        // Load configuration values
        if (config.contains("auth")) {
            auto& auth = config["auth"];
            if (auth.contains("serverUrl")) {
                m_config["auth.serverUrl"] = auth["serverUrl"].get<std::string>();
            }
            if (auth.contains("timeout")) {
                m_config["auth.timeout"] = std::to_string(auth["timeout"].get<int>());
            }
        }
        
        if (config.contains("downloads")) {
            auto& downloads = config["downloads"];
            if (downloads.contains("defaultDirectory")) {
                m_config["downloads.defaultDirectory"] = downloads["defaultDirectory"].get<std::string>();
            }
            if (downloads.contains("maxConcurrent")) {
                m_config["downloads.maxConcurrent"] = std::to_string(downloads["maxConcurrent"].get<int>());
            }
        }
        
        if (config.contains("ui")) {
            auto& ui = config["ui"];
            if (ui.contains("theme")) {
                m_config["ui.theme"] = ui["theme"].get<std::string>();
            }
            if (ui.contains("fontSize")) {
                m_config["ui.fontSize"] = std::to_string(ui["fontSize"].get<int>());
            }
        }
        
        return true;
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

// Utility functions implementation
namespace Utils {
    
SDL_Color createColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return {r, g, b, a};
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, 
               int x, int y, SDL_Color color) {
    if (!font) return;
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect dest = {x, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, nullptr, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

void renderRect(SDL_Renderer* renderer, int x, int y, int width, int height, 
               SDL_Color color, bool filled) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, width, height};
    
    if (filled) {
        SDL_RenderFillRect(renderer, &rect);
    } else {
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void renderBorder(SDL_Renderer* renderer, int x, int y, int width, int height, 
                 SDL_Color color, int thickness) {
    for (int i = 0; i < thickness; i++) {
        SDL_Rect rect = {x + i, y + i, width - 2 * i, height - 2 * i};
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

bool isPointInRect(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight) {
    return x >= rectX && x < rectX + rectWidth && y >= rectY && y < rectY + rectHeight;
}

std::string formatFileSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return ss.str();
}

std::string formatSpeed(double bytesPerSecond) {
    return formatFileSize(static_cast<size_t>(bytesPerSecond)) + "/s";
}

std::string formatTime(std::chrono::system_clock::time_point time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    return ss.str();
}

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex = "0123456789abcdef";
    
    std::string uuid;
    uuid.reserve(36);
    
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid += '-';
        } else {
            uuid += hex[dis(gen)];
        }
    }
    
    return uuid;
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

bool isValidEmail(const std::string& email) {
    size_t atPos = email.find('@');
    if (atPos == std::string::npos || atPos == 0) {
        return false;
    }
    
    size_t dotPos = email.find('.', atPos);
    if (dotPos == std::string::npos || dotPos == atPos + 1) {
        return false;
    }
    
    return true;
}

std::string extractFilenameFromUrl(const std::string& url) {
    size_t lastSlash = url.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash < url.length() - 1) {
        std::string filename = url.substr(lastSlash + 1);
        size_t queryPos = filename.find('?');
        if (queryPos != std::string::npos) {
            filename = filename.substr(0, queryPos);
        }
        return sanitizeFilename(filename);
    }
    return "download";
}

std::string sanitizeFilename(const std::string& filename) {
    std::string sanitized = filename;
    const std::string invalidChars = "<>:\"|?*\\/";
    
    for (char c : invalidChars) {
        size_t pos = sanitized.find(c);
        while (pos != std::string::npos) {
            sanitized[pos] = '_';
            pos = sanitized.find(c, pos + 1);
        }
    }
    
    return sanitized;
}

} // namespace Utils

} // namespace ZepraCore 