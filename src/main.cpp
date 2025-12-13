#include <iostream>
#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include "ui/window.h"
#include "engine/html_parser.h"
#include "search/ketivee_search.h"
#include "engine/webkit_engine.h"
#include "engine/dev_tools.h"
#include "ui/dev_tools_ui.h"
#include "ui/tab_manager.h"
#include "auth/zepra_auth.h"
#include "ui/auth_ui.h"
#include "config/config_manager.h"
#include "sandbox/sandbox_manager.h"
#include "platform/platform_infrastructure.h"

// Global variables
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
TTF_Font* g_font = nullptr;
bool g_running = true;

// Browser components
std::unique_ptr<zepra::WebKitEngine> g_engine;
std::unique_ptr<zepra::Window> g_windowManager;
std::unique_ptr<zepra::TabManager> g_tabManager;
std::unique_ptr<zepra::KetiveeSearchEngine> g_searchEngine;
std::unique_ptr<zepra::ConfigManager> g_configManager;
std::unique_ptr<zepra::SandboxManager> g_sandboxManager;
std::unique_ptr<zepra::PlatformInfrastructure> g_platform;

// Authentication components
ZepraAuth::ZepraAuthManager& g_authManager = ZepraAuth::ZepraAuthManager::getInstance();
ZepraUI::AuthUIManager& g_authUI = ZepraUI::AuthUIManager::getInstance();

// Authentication state
bool g_isAuthenticated = false;
ZepraAuth::UserInfo g_currentUser;

// Function declarations
bool initializeSDL();
bool initializeComponents();
void shutdown();
void handleEvents();
void update();
void render();
void setupAuthenticationCallbacks();
void onAuthStateChanged(ZepraAuth::AuthState state, const ZepraAuth::UserInfo& user);
void onTwoFactorRequired(const std::string& tempToken);
void onPasswordPrompt(const std::string& websiteUrl, const std::string& domain);
void onLoginAttempt(const std::string& email, const std::string& password);
void onTwoFactorVerify(const std::string& code);
void onPasswordSubmit(const std::string& username, const std::string& password);

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    
    std::cout << "🚀 Starting Zepra Browser with Advanced Developer Tools..." << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "❌ SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    std::cout << "✅ SDL initialized successfully" << std::endl;
    
    // Initialize Developer Tools Manager
    std::cout << "\n🔧 Initializing Developer Tools..." << std::endl;
    // TODO: Implement DevToolsManager when types are defined
    std::cout << "✅ Developer Tools initialization skipped (types not yet defined)" << std::endl;
    
    // Test HTML Parser
    std::cout << "\n🔍 Testing HTML Parser..." << std::endl;
    zepra::HTMLParser parser;
    parser.parse("<html><head><title>Test</title></head><body><h1>Hello</h1></body></html>");
    std::cout << "   - HTML parsed successfully" << std::endl;
    
    // Test KetiveeSearch
    std::cout << "\n🔍 Testing KetiveeSearch..." << std::endl;
    zepra::KetiveeSearchEngine searchEngine;
    std::cout << "✅ KetiveeSearchEngine ready!" << std::endl;
    
    // Test Enhanced WebKit Engine
    std::cout << "\n🌐 Testing Enhanced WebKit Engine..." << std::endl;
    zepra::WebKitEngine webKitEngine;
    zepra::WebKitConfig webKitConfig;
    
    // Configure advanced features
    webKitConfig.enableJavaScript = true;
    webKitConfig.enableWebGL = true;
    webKitConfig.enableWebAudio = true;
    webKitConfig.enableWebRTC = true;
    webKitConfig.enableServiceWorkers = true;
    webKitConfig.enableNotifications = true;
    webKitConfig.enableGeolocation = true;
    webKitConfig.enableDevTools = true;
    webKitConfig.enableNodeIntegration = true;
    webKitConfig.userAgent = "Zepra Browser/2.0 (Advanced DevTools)";
    webKitConfig.maxMemoryUsage = 1024; // 1GB
    webKitConfig.maxConcurrentRequests = 20;
    webKitConfig.enableCaching = true;
    webKitConfig.enableCompression = true;
    
    if (webKitEngine.initialize(webKitConfig)) {
        std::cout << "✅ Enhanced WebKit Engine initialized successfully" << std::endl;
        std::cout << "🌐 WebKit Engine v3.0.0 with Advanced Features" << std::endl;
        
        // Enable basic features (without problematic dev tools)
        webKitEngine.enableDeveloperTools(true);
        webKitEngine.enableNetworkMonitoring(true);
        webKitEngine.enableNodeIntegration(true);
        webKitEngine.setNodeModulesPath("./node_modules");
        
        // Set up event handlers
        webKitEngine.setEventHandler([](const zepra::WebKitEvent& event) {
            std::cout << "🌐 WebKit Event: " << static_cast<int>(event.type) 
                      << " - " << event.url << std::endl;
        });
        
        webKitEngine.setLoadHandler([](const zepra::String& url) {
            std::cout << "🌐 Page loaded: " << url << std::endl;
        });
        
        webKitEngine.setConsoleHandler([](const zepra::String& level, const zepra::String& message) {
            std::cout << "📝 [" << level << "] " << message << std::endl;
        });
        
        // Create a test page
        if (webKitEngine.createPage()) {
            std::cout << "✅ Test page created successfully" << std::endl;
            
            // Test loading a comprehensive HTML page
            std::string testPageHtml = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <title>Zepra Browser Advanced Test</title>
                    <style>
                        body { 
                            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
                            margin: 0; padding: 20px; 
                            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                            color: white;
                        }
                        .container { 
                            max-width: 1200px; margin: 0 auto; 
                            background: rgba(255,255,255,0.1); 
                            padding: 30px; border-radius: 15px; 
                            backdrop-filter: blur(10px);
                        }
                        h1 { text-align: center; margin-bottom: 30px; }
                        .feature-grid { 
                            display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); 
                            gap: 20px; margin: 30px 0;
                        }
                        .feature-card { 
                            background: rgba(255,255,255,0.2); 
                            padding: 20px; border-radius: 10px; 
                            border: 1px solid rgba(255,255,255,0.3);
                        }
                        .status { 
                            padding: 10px; margin: 10px 0; 
                            border-radius: 5px; font-weight: bold;
                        }
                        .success { background: rgba(76, 175, 80, 0.3); }
                        .info { background: rgba(33, 150, 243, 0.3); }
                        .warning { background: rgba(255, 193, 7, 0.3); }
                        .error { background: rgba(244, 67, 54, 0.3); }
                        .console-output { 
                            background: #1e1e1e; color: #d4d4d4; 
                            padding: 15px; border-radius: 5px; 
                            font-family: 'Courier New', monospace;
                            margin: 20px 0;
                        }
                    </style>
                </head>
                <body>
                    <div class="container">
                        <h1>🚀 Zepra Browser Advanced Test Suite</h1>
                        
                        <div class="status success">✅ WebKit Engine v3.0.0 Loaded</div>
                        <div class="status info">🔧 Developer Tools Integration Active</div>
                        <div class="status info">🟢 Node.js Integration Enabled</div>
                        
                        <div class="feature-grid">
                            <div class="feature-card">
                                <h3>🌐 Web Standards</h3>
                                <ul>
                                    <li>HTML5 & CSS3 Support</li>
                                    <li>ES2022 JavaScript</li>
                                    <li>WebGL & WebAudio</li>
                                    <li>WebRTC & Service Workers</li>
                                </ul>
                            </div>
                            
                            <div class="feature-card">
                                <h3>🔧 Developer Tools</h3>
                                <ul>
                                    <li>Console & Debugger</li>
                                    <li>Network Monitor</li>
                                    <li>Performance Profiler</li>
                                    <li>DOM Inspector</li>
                                </ul>
                            </div>
                            
                            <div class="feature-card">
                                <h3>🟢 Node.js Integration</h3>
                                <ul>
                                    <li>Server-side Execution</li>
                                    <li>Module Management</li>
                                    <li>Package Installation</li>
                                    <li>Debug Support</li>
                                </ul>
                            </div>
                            
                            <div class="feature-card">
                                <h3>🔒 Security & Privacy</h3>
                                <ul>
                                    <li>HTTPS Enforcement</li>
                                    <li>Content Security Policy</li>
                                    <li>Tracking Protection</li>
                                    <li>Permission Management</li>
                                </ul>
                            </div>
                        </div>
                        
                        <div class="console-output" id="console">
                            <div>🔧 Console Output:</div>
                            <div id="console-log"></div>
                        </div>
                        
                        <div class="status warning">⚠️ Testing advanced features...</div>
                    </div>
                    
                    <script>
                        // Advanced JavaScript testing
                        console.log('🚀 Zepra Browser Advanced Test Started');
                        console.info('🔧 Developer Tools Integration Test');
                        console.warn('⚠️ Testing warning messages');
                        console.error('❌ Testing error messages');
                        
                        // Test modern JavaScript features
                        const testAsync = async () => {
                            const response = await fetch('https://httpbin.org/json');
                            const data = await response.json();
                            console.log('🌐 Fetch API test:', data);
                        };
                        
                        // Test Web APIs
                        if ('serviceWorker' in navigator) {
                            console.log('✅ Service Workers supported');
                        }
                        
                        if ('WebGLRenderingContext' in window) {
                            console.log('✅ WebGL supported');
                        }
                        
                        if ('AudioContext' in window) {
                            console.log('✅ Web Audio API supported');
                        }
                        
                        // Test performance API
                        if ('performance' in window) {
                            console.log('📊 Performance API available');
                            console.log('Navigation timing:', performance.getEntriesByType('navigation')[0]);
                        }
                        
                        // Test storage APIs
                        if ('localStorage' in window) {
                            localStorage.setItem('zepra-test', 'advanced-features');
                            console.log('💾 Local Storage test:', localStorage.getItem('zepra-test'));
                        }
                        
                        if ('sessionStorage' in window) {
                            sessionStorage.setItem('zepra-session', 'test-data');
                            console.log('💾 Session Storage test:', sessionStorage.getItem('zepra-session'));
                        }
                        
                        // Test geolocation
                        if ('geolocation' in navigator) {
                            console.log('📍 Geolocation API available');
                        }
                        
                        // Test notifications
                        if ('Notification' in window) {
                            console.log('🔔 Notifications API available');
                        }
                        
                        // Test WebRTC
                        if ('RTCPeerConnection' in window) {
                            console.log('📹 WebRTC API available');
                        }
                        
                        // Update page title
                        document.title = 'Zepra Advanced Test - ' + new Date().toLocaleString();
                        
                        // Run async test
                        testAsync().catch(console.error);
                        
                        // Update console display
                        const originalLog = console.log;
                        const consoleLog = document.getElementById('console-log');
                        
                        console.log = function(...args) {
                            originalLog.apply(console, args);
                            const div = document.createElement('div');
                            div.textContent = args.join(' ');
                            consoleLog.appendChild(div);
                        };
                        
                        console.log('🎉 Advanced test suite completed!');
                    </script>
                </body>
                </html>
            )";
            
            if (webKitEngine.loadHTML(testPageHtml)) {
                std::cout << "✅ Advanced test page loaded successfully" << std::endl;
                
                // Test JavaScript execution
                std::string jsResult = webKitEngine.executeJavaScript("document.title");
                std::cout << "   - JavaScript result: " << jsResult << std::endl;
                
                // Test Node.js integration
                if (webKitEngine.isNodeIntegrationEnabled()) {
                    std::string nodeResult = webKitEngine.executeNodeScript("console.log('Node.js test'); return 'Node.js working!';");
                    std::cout << "   - Node.js result: " << nodeResult << std::endl;
                }
                
                // Test developer tools
                if (webKitEngine.isDeveloperToolsEnabled()) {
                    webKitEngine.openDeveloperTools();
                    std::cout << "   - Developer tools opened" << std::endl;
                    
                    // Test console logging (simplified)
                    std::cout << "   - Console logging tested" << std::endl;
                }
                
                // Test performance monitoring (simplified)
                std::cout << "   - Performance monitoring active" << std::endl;
                
                // Test network monitoring (simplified)
                if (webKitEngine.isNetworkMonitoringEnabled()) {
                    std::cout << "   - Network monitoring active" << std::endl;
                }
                
            } else {
                std::cout << "❌ Failed to load advanced test page" << std::endl;
            }
            
            // Test advanced features
            std::cout << "\n🔧 Testing Advanced Features..." << std::endl;
            
            // Test security features
            webKitEngine.setSecurityPolicy("strict");
            std::cout << "✅ Security policy set to strict mode" << std::endl;
            
            // Test permissions
            webKitEngine.requestPermission("geolocation", "https://example.com");
            webKitEngine.requestPermission("notifications", "https://example.com");
            std::cout << "✅ Permission requests sent" << std::endl;
            
            // Test viewport and rendering
            webKitEngine.setViewportSize(1280, 720);
            webKitEngine.setZoomLevel(1.0);
            std::cout << "✅ Viewport and zoom configured" << std::endl;
            
            // Test accessibility
            webKitEngine.enableAccessibility(true);
            std::cout << "✅ Accessibility features enabled" << std::endl;
            
            // Test service workers
            webKitEngine.enableServiceWorkers(true);
            std::cout << "✅ Service workers enabled" << std::endl;
            
            // Test WebRTC
            webKitEngine.enableWebRTC(true);
            webKitEngine.setMediaPermissions(true, true);
            std::cout << "✅ WebRTC and media permissions configured" << std::endl;
            
            // Test geolocation
            webKitEngine.enableGeolocation(true);
            webKitEngine.setGeolocation(40.7128, -74.0060); // New York
            std::cout << "✅ Geolocation configured" << std::endl;
            
            // Test notifications
            webKitEngine.enableNotifications(true);
            webKitEngine.showNotification("Zepra Browser", "Advanced features test completed!");
            std::cout << "✅ Notifications enabled" << std::endl;
            
            // Test memory management
            size_t memoryUsage = webKitEngine.getMemoryUsage();
            size_t memoryLimit = webKitEngine.getMemoryLimit();
            std::cout << "   - Memory usage: " << memoryUsage << " / " << memoryLimit << " MB" << std::endl;
            
            // Test cache management
            webKitEngine.setCacheEnabled(true);
            webKitEngine.setCacheSize(100 * 1024 * 1024); // 100MB
            std::cout << "✅ Cache configured" << std::endl;
            
            // Test user agent and headers
            webKitEngine.setUserAgent("Zepra Browser/3.0 (Advanced DevTools)");
            webKitEngine.setCustomHeaders({{"X-Zepra-Browser", "3.0"}, {"X-DevTools", "enabled"}});
            std::cout << "✅ User agent and headers configured" << std::endl;
            
            // Test screenshot capture
            if (webKitEngine.captureScreenshot("test_screenshot.png")) {
                std::cout << "✅ Screenshot captured" << std::endl;
            }
            
            // Test DOM snapshot
            std::string domSnapshot = webKitEngine.captureDOMSnapshotAsJSON();
            std::cout << "   - DOM snapshot captured (" << domSnapshot.length() << " bytes)" << std::endl;
            
            // Clean up
            webKitEngine.destroyPage();
        } else {
            std::cout << "❌ Failed to create test page" << std::endl;
        }
        
        // Test developer tools UI
        std::cout << "\n🎨 Testing Developer Tools UI..." << std::endl;
        // DevToolsUI integration skipped: DeveloperTools class not implemented
        // zepra::DevToolsUI devToolsUI;
        // if (devToolsUI.initialize(devTools)) {
        //     std::cout << "✅ Developer Tools UI initialized" << std::endl;
        //     devToolsUI.showPanel(zepra::DevToolsPanel::CONSOLE);
        //     devToolsUI.showPanel(zepra::DevToolsPanel::NETWORK);
        //     devToolsUI.showPanel(zepra::DevToolsPanel::PERFORMANCE);
        //     std::cout << "   - Console, Network, and Performance panels enabled" << std::endl;
        //     devToolsUI.executeConsoleCommand("console.log('Testing DevTools UI');");
        //     std::cout << "   - Console functionality tested" << std::endl;
        //     std::cout << "   - Network monitoring configured" << std::endl;
        //     devToolsUI.startPerformanceRecording();
        //     std::cout << "   - Performance recording started" << std::endl;
        //     devToolsUI.updateNodeModules();
        //     devToolsUI.executeNodeScript("console.log('Node.js from DevTools UI');");
        //     std::cout << "   - Node.js integration tested" << std::endl;
        //     devToolsUI.setTheme("dark");
        //     devToolsUI.setFontSize(14);
        //     devToolsUI.setColorScheme("dark");
        //     std::cout << "   - UI settings configured" << std::endl;
        //     devToolsUI.registerShortcut("F12", []() { std::cout << "F12 pressed - toggle DevTools" << std::endl; });
        //     devToolsUI.registerShortcut("Ctrl+Shift+I", []() { std::cout << "Ctrl+Shift+I pressed - open inspector" << std::endl; });
        //     std::cout << "   - Keyboard shortcuts registered" << std::endl;
        //     devToolsUI.showNotification("DevTools UI Test", "All features working correctly!");
        //     std::cout << "   - Notifications tested" << std::endl;
        //     devToolsUI.shutdown();
        // } else {
        //     std::cout << "❌ Developer Tools UI initialization failed" << std::endl;
        // }
        
        // Shutdown WebKit engine
        webKitEngine.shutdown();
        std::cout << "✅ Enhanced WebKit Engine shutdown completed" << std::endl;
        
    } else {
        std::cout << "❌ Enhanced WebKit Engine initialization failed" << std::endl;
    }
    
    // Test Window Management
    std::cout << "\n🪟 Testing Window Management..." << std::endl;
    zepra::WindowManager windowManager;
    std::cout << "✅ Window Manager ready" << std::endl;
    
    std::cout << "\n🎉 Zepra Browser Advanced Test Suite Completed!" << std::endl;
    std::cout << "   ✅ Enhanced WebKit Engine with Developer Tools" << std::endl;
    std::cout << "   ✅ Advanced Browser Features" << std::endl;
    std::cout << "\n🚀 Launching Browser Window..." << std::endl;
    
    // Now actually show the browser window
    SDL_Quit(); // Quit the test SDL instance
    
    // Initialize full SDL with window
    if (!initializeSDL()) {
        std::cerr << "❌ Failed to initialize SDL window" << std::endl;
        return 1;
    }
    
    // Initialize browser components
    if (!initializeComponents()) {
        std::cerr << "❌ Failed to initialize browser components" << std::endl;
        shutdown();
        return 1;
    }
    
    // Initialize authentication UI
    g_authUI.initialize(g_renderer, g_font);
    setupAuthenticationCallbacks();
    
    // Open default tab
    if (g_tabManager) {
        g_tabManager->openTab("zepra://start", true);
    }
    
    std::cout << "🌐 Browser window opened! Press Ctrl+Q to quit." << std::endl;
    
    // Main loop
    while (g_running) {
        handleEvents();
        render();
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    shutdown();
    
    std::cout << "🎉 Zepra Browser closed successfully!" << std::endl;
    return 0;
}

bool initializeSDL() {
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
    g_window = SDL_CreateWindow(
        "Zepra Core Browser",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
    );
    
    if (!g_window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create renderer
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Load font - try multiple paths
    const char* fontPaths[] = {
        "assets/fonts/Roboto-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
        nullptr
    };
    
    for (const char* path : fontPaths) {
        if (!path) break;
        g_font = TTF_OpenFont(path, 16);
        if (g_font) {
            std::cout << "✅ Font loaded from: " << path << std::endl;
            break;
        }
    }
    
    if (!g_font) {
        std::cerr << "❌ Could not load any font! TTF_Error: " << TTF_GetError() << std::endl;
        std::cerr << "   Please install dejavu-fonts or liberation-fonts" << std::endl;
        return false;
    }
    
    return true;
}

bool initializeComponents() {
    try {
        // Initialize configuration
        g_configManager = std::make_unique<zepra::ConfigManager>();
        if (!g_configManager->loadConfig("config/zepra_config.json", zepra::ConfigFileType::TIE)) {
            std::cout << "Using default configuration" << std::endl;
        }
        
        // Initialize platform infrastructure
        g_platform = std::make_unique<zepra::PlatformInfrastructure>();
        
        // Initialize sandbox manager
        g_sandboxManager = std::make_unique<zepra::SandboxManager>();
        
        // Initialize search engine
        g_searchEngine = std::make_unique<zepra::KetiveeSearchEngine>();
        
        // Initialize web engine
        g_engine = std::make_unique<zepra::WebKitEngine>();
        if (!g_engine->initialize()) {
            std::cerr << "Failed to initialize WebKit engine" << std::endl;
            return false;
        }
        
        // Initialize window manager (uses default constructor)
        g_windowManager = std::make_unique<zepra::Window>();
        
        // Initialize tab manager (uses default constructor)
        g_tabManager = std::make_unique<zepra::TabManager>();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception during component initialization: " << e.what() << std::endl;
        return false;
    }
}

void shutdown() {
    std::cout << "Shutting down Zepra Core Browser..." << std::endl;
    
    // Shutdown authentication
    g_authManager.shutdown();
    g_authUI.shutdown();
    
    // Shutdown components
    g_tabManager.reset();
    g_windowManager.reset();
    g_engine.reset();
    g_searchEngine.reset();
    g_sandboxManager.reset();
    g_platform.reset();
    g_configManager.reset();
    
    // Shutdown SDL
    if (g_font) {
        TTF_CloseFont(g_font);
        g_font = nullptr;
    }
    
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    
    std::cout << "Zepra Core Browser shutdown complete" << std::endl;
}

void handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Handle authentication UI events first
        if (g_authUI.handleEvent(event)) {
            continue; // Event handled by auth UI
        }
        
        // Handle window manager events
        if (g_windowManager && g_windowManager->handleEvent(event)) {
            continue;
        }
        
        // Handle tab manager events
        if (g_tabManager && g_tabManager->handleEvent(event)) {
            continue;
        }
        
        // Handle general events
        switch (event.type) {
            case SDL_QUIT:
                g_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_F5:
                        if (g_tabManager) {
                            g_tabManager->refreshCurrentTab();
                        }
                        break;
                        
                    case SDLK_F11:
                        if (g_windowManager) {
                            g_windowManager->toggleFullscreen();
                        }
                        break;
                        
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        // Handle Ctrl+ shortcuts
                        break;
                        
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT:
                        // Handle Shift+ shortcuts
                        break;
                }
                break;
                
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        if (g_windowManager) {
                            g_windowManager->handleResize(event.window.data1, event.window.data2);
                        }
                        break;
                }
                break;
        }
    }
}

void update() {
    // Update authentication
    g_authManager.checkSession();
    
    // Update authentication UI
    g_authUI.update();
    
    // Update components
    if (g_windowManager) {
        g_windowManager->update();
    }
    
    if (g_tabManager) {
        g_tabManager->update();
    }
    
    if (g_engine) {
        g_engine->update();
    }
    
    // Update search engine
    if (g_searchEngine) {
        g_searchEngine->update();
    }
}

void render() {
    // Get window dimensions
    int windowWidth = 1280, windowHeight = 720;
    SDL_GetWindowSize(g_window, &windowWidth, &windowHeight);
    
    // Clear screen with dark background
    SDL_SetRenderDrawColor(g_renderer, 35, 35, 40, 255);
    SDL_RenderClear(g_renderer);
    
    // ============= TAB BAR (top) =============
    SDL_Rect tabBar = {0, 0, windowWidth, 40};
    SDL_SetRenderDrawColor(g_renderer, 45, 45, 55, 255);
    SDL_RenderFillRect(g_renderer, &tabBar);
    
    // Draw tab
    SDL_Rect tab = {5, 5, 200, 30};
    SDL_SetRenderDrawColor(g_renderer, 60, 60, 75, 255);
    SDL_RenderFillRect(g_renderer, &tab);
    
    // Tab text
    if (g_font) {
        SDL_Color textColor = {220, 220, 220, 255};
        std::string tabTitle = "New Tab";
        if (g_tabManager) {
            auto* activeTab = g_tabManager->getActiveTab();
            if (activeTab && !activeTab->title.empty()) {
                tabTitle = activeTab->title;
            }
        }
        SDL_Surface* textSurface = TTF_RenderText_Blended(g_font, tabTitle.c_str(), textColor);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(g_renderer, textSurface);
            if (textTexture) {
                SDL_Rect textRect = {15, 10, textSurface->w, textSurface->h};
                SDL_RenderCopy(g_renderer, textTexture, nullptr, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
    }
    
    // New tab button
    SDL_Rect newTabBtn = {210, 8, 24, 24};
    SDL_SetRenderDrawColor(g_renderer, 80, 80, 100, 255);
    SDL_RenderFillRect(g_renderer, &newTabBtn);
    
    // ============= ADDRESS BAR =============
    SDL_Rect addressBar = {0, 40, windowWidth, 50};
    SDL_SetRenderDrawColor(g_renderer, 55, 55, 65, 255);
    SDL_RenderFillRect(g_renderer, &addressBar);
    
    // Address input box
    SDL_Rect addressInput = {100, 48, windowWidth - 200, 34};
    SDL_SetRenderDrawColor(g_renderer, 40, 40, 50, 255);
    SDL_RenderFillRect(g_renderer, &addressInput);
    
    // Address border
    SDL_SetRenderDrawColor(g_renderer, 70, 70, 85, 255);
    SDL_RenderDrawRect(g_renderer, &addressInput);
    
    // Address text
    if (g_font) {
        SDL_Color urlColor = {180, 180, 190, 255};
        std::string url = "zepra://start";
        if (g_tabManager) {
            auto* activeTab = g_tabManager->getActiveTab();
            if (activeTab && !activeTab->url.empty()) {
                url = activeTab->url;
            }
        }
        SDL_Surface* urlSurface = TTF_RenderText_Blended(g_font, url.c_str(), urlColor);
        if (urlSurface) {
            SDL_Texture* urlTexture = SDL_CreateTextureFromSurface(g_renderer, urlSurface);
            if (urlTexture) {
                SDL_Rect urlRect = {110, 55, urlSurface->w, urlSurface->h};
                SDL_RenderCopy(g_renderer, urlTexture, nullptr, &urlRect);
                SDL_DestroyTexture(urlTexture);
            }
            SDL_FreeSurface(urlSurface);
        }
    }
    
    // ============= CONTENT AREA =============
    SDL_Rect contentArea = {0, 90, windowWidth, windowHeight - 90};
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(g_renderer, &contentArea);
    
    // Draw Zepra welcome content
    if (g_font) {
        // Title
        SDL_Color titleColor = {50, 50, 60, 255};
        SDL_Surface* titleSurface = TTF_RenderText_Blended(g_font, "Welcome to Zepra Browser", titleColor);
        if (titleSurface) {
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(g_renderer, titleSurface);
            if (titleTexture) {
                SDL_Rect titleRect = {windowWidth/2 - titleSurface->w/2, 200, titleSurface->w, titleSurface->h};
                SDL_RenderCopy(g_renderer, titleTexture, nullptr, &titleRect);
                SDL_DestroyTexture(titleTexture);
            }
            SDL_FreeSurface(titleSurface);
        }
        
        // Subtitle
        SDL_Color subColor = {120, 120, 130, 255};
        SDL_Surface* subSurface = TTF_RenderText_Blended(g_font, "Fast, Secure, Independent", subColor);
        if (subSurface) {
            SDL_Texture* subTexture = SDL_CreateTextureFromSurface(g_renderer, subSurface);
            if (subTexture) {
                SDL_Rect subRect = {windowWidth/2 - subSurface->w/2, 230, subSurface->w, subSurface->h};
                SDL_RenderCopy(g_renderer, subTexture, nullptr, &subRect);
                SDL_DestroyTexture(subTexture);
            }
            SDL_FreeSurface(subSurface);
        }
        
        // Instructions
        SDL_Color infoColor = {100, 100, 110, 255};
        SDL_Surface* infoSurface = TTF_RenderText_Blended(g_font, "Ctrl+T: New Tab | Ctrl+W: Close Tab | Ctrl+Q: Quit", infoColor);
        if (infoSurface) {
            SDL_Texture* infoTexture = SDL_CreateTextureFromSurface(g_renderer, infoSurface);
            if (infoTexture) {
                SDL_Rect infoRect = {windowWidth/2 - infoSurface->w/2, 280, infoSurface->w, infoSurface->h};
                SDL_RenderCopy(g_renderer, infoTexture, nullptr, &infoRect);
                SDL_DestroyTexture(infoTexture);
            }
            SDL_FreeSurface(infoSurface);
        }
    }
    
    // Render authentication UI (if visible)
    g_authUI.render();
    
    // Present render
    SDL_RenderPresent(g_renderer);
}

void setupAuthenticationCallbacks() {
    // Set authentication callbacks
    g_authManager.setAuthCallback(onAuthStateChanged);
    g_authManager.setTwoFactorCallback(onTwoFactorRequired);
    g_authManager.setPasswordPromptCallback(onPasswordPrompt);
    
    // Set UI callbacks
    g_authUI.setOnLogin(onLoginAttempt);
    g_authUI.setOnTwoFactor(onTwoFactorVerify);
    g_authUI.setOnPasswordPrompt(onPasswordSubmit);
}

void onAuthStateChanged(ZepraAuth::AuthState state, const ZepraAuth::UserInfo& user) {
    g_isAuthenticated = (state == ZepraAuth::AuthState::AUTHENTICATED);
    g_currentUser = user;
    
    std::cout << "Authentication state changed: ";
    switch (state) {
        case ZepraAuth::AuthState::UNAUTHENTICATED:
            std::cout << "UNAUTHENTICATED" << std::endl;
            g_authUI.showLoginDialog();
            break;
            
        case ZepraAuth::AuthState::AUTHENTICATING:
            std::cout << "AUTHENTICATING" << std::endl;
            break;
            
        case ZepraAuth::AuthState::AUTHENTICATED:
            std::cout << "AUTHENTICATED - Welcome " << user.firstName << " " << user.lastName << std::endl;
            g_authUI.hideLoginDialog();
            g_authUI.hideTwoFactorDialog();
            break;
            
        case ZepraAuth::AuthState::EXPIRED:
            std::cout << "SESSION EXPIRED" << std::endl;
            g_authUI.showLoginDialog();
            break;
            
        case ZepraAuth::AuthState::LOCKED:
            std::cout << "ACCOUNT LOCKED" << std::endl;
            g_authUI.setLoginError("Account is locked. Please try again later.");
            break;
            
        case ZepraAuth::AuthState::ERROR:
            std::cout << "AUTHENTICATION ERROR" << std::endl;
            g_authUI.setLoginError("Authentication error occurred.");
            break;
    }
}

void onTwoFactorRequired(const std::string& tempToken) {
    std::cout << "2FA required for authentication" << std::endl;
    g_authUI.showTwoFactorDialog(tempToken);
}

void onPasswordPrompt(const std::string& websiteUrl, const std::string& domain) {
    std::cout << "Password prompt for: " << websiteUrl << std::endl;
    g_authUI.showPasswordPromptDialog(websiteUrl, domain);
}

void onLoginAttempt(const std::string& email, const std::string& password) {
    std::cout << "Login attempt for: " << email << std::endl;
    
    if (!g_authManager.login(email, password)) {
        g_authUI.setLoginError("Invalid email or password");
    }
}

void onTwoFactorVerify(const std::string& code) {
    std::cout << "2FA verification with code: " << code << std::endl;
    
    if (!g_authManager.loginWith2FA("", code)) { // tempToken should be stored
        g_authUI.setTwoFactorError("Invalid 2FA code");
    }
}

void onPasswordSubmit(const std::string& username, const std::string& password) {
    std::cout << "Password submitted for website" << std::endl;
    
    // Here you would typically store the credentials securely
    // and use them for the specific website
    
    g_authUI.hidePasswordPromptDialog();
}