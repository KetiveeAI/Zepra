# Zepra Browser Advanced Developer Tools

## Overview

The Zepra Browser now includes comprehensive developer tools similar to Chrome DevTools, Firefox Developer Tools, and Safari Web Inspector. This advanced system provides full debugging, profiling, and development capabilities with Node.js integration.

## 🚀 Key Features

### Core Developer Tools
- **Console Panel**: Advanced JavaScript console with filtering and history
- **Elements Panel**: DOM inspector with real-time editing
- **Network Panel**: Request/response monitoring and analysis
- **Performance Panel**: Profiling and performance metrics
- **Sources Panel**: JavaScript debugging with breakpoints
- **Application Panel**: Storage, cache, and service worker inspection
- **Security Panel**: Certificate and permission management
- **Node.js Panel**: Server-side JavaScript execution and module management

### Advanced Browser Engine
- **WebKit Integration**: Full WebKit engine with modern web standards
- **JavaScript Engine**: ES2022 support with async/await and modules
- **Web APIs**: WebGL, WebAudio, WebRTC, Service Workers
- **Security Features**: HTTPS enforcement, CSP, XSS protection
- **Performance Optimization**: Memory management, caching, compression
- **Accessibility**: Screen reader support, high contrast, keyboard navigation

## 🔧 Developer Tools Architecture

### Components

#### 1. DeveloperTools Class
```cpp
class DeveloperTools {
    // Console management
    void log(const String& message, ConsoleLevel level);
    void clearConsole();
    std::vector<ConsoleMessage> getConsoleMessages();
    
    // Network monitoring
    void logNetworkRequest(const NetworkRequest& request);
    void logNetworkResponse(const NetworkResponse& response);
    
    // DOM inspection
    void setDOMTree(std::shared_ptr<DOMNode> root);
    std::shared_ptr<DOMNode> getNodeById(int nodeId);
    
    // JavaScript execution
    String executeJavaScript(const String& script);
    void executeJavaScriptAsync(const String& script);
    
    // Performance monitoring
    void startPerformanceMonitoring();
    PerformanceMetrics getPerformanceMetrics();
    
    // Node.js integration
    void enableNodeIntegration(bool enabled);
    String executeNodeScript(const String& script);
};
```

#### 2. DevToolsManager Class
```cpp
class DevToolsManager {
    std::shared_ptr<DeveloperTools> createTools();
    void destroyTools(std::shared_ptr<DeveloperTools> tools);
    void setGlobalNodeIntegration(bool enabled);
    void startGlobalPerformanceMonitoring();
};
```

#### 3. DevToolsUI Class
```cpp
class DevToolsUI {
    // Panel management
    void showPanel(DevToolsPanel panel);
    void setCurrentPanel(DevToolsPanel panel);
    
    // Console panel
    void updateConsole();
    void executeConsoleCommand(const String& command);
    
    // Elements panel
    void updateDOMTree();
    void highlightElement(int nodeId);
    
    // Network panel
    void updateNetworkLog();
    void exportNetworkLog(const String& filePath);
    
    // Performance panel
    void startPerformanceRecording();
    void exportPerformanceData(const String& filePath);
    
    // Node.js panel
    void updateNodeModules();
    void executeNodeScript(const String& script);
    void installNodePackage(const String& packageName);
};
```

## 🌐 WebKit Engine Features

### Configuration
```cpp
struct WebKitConfig {
    bool enableJavaScript = true;
    bool enableWebGL = true;
    bool enableWebAudio = true;
    bool enableWebRTC = true;
    bool enableServiceWorkers = true;
    bool enableNotifications = true;
    bool enableGeolocation = true;
    bool enableDevTools = true;
    bool enableNodeIntegration = true;
    String userAgent = "Zepra Browser/3.0";
    int maxMemoryUsage = 1024; // MB
    int maxConcurrentRequests = 20;
    bool enableCaching = true;
    bool enableCompression = true;
};
```

### Advanced Features
- **Security**: HTTPS enforcement, content security policy, XSS auditor
- **Performance**: Memory management, garbage collection, caching
- **Accessibility**: Screen reader support, keyboard navigation
- **Media**: Camera/microphone access, WebRTC, WebAudio
- **Storage**: LocalStorage, SessionStorage, IndexedDB, Cache API
- **Network**: HTTP/2, compression, custom headers, proxy support

## 🟢 Node.js Integration

### Backend API Routes

#### JavaScript Execution
```javascript
POST /api/devtools/execute
{
    "script": "console.log('Hello from Node.js'); return 'result';",
    "modulesPath": "./node_modules",
    "contextId": "main"
}
```

#### Node.js Script Execution
```javascript
POST /api/devtools/node/execute
{
    "script": "const fs = require('fs'); return fs.readdirSync('.');",
    "modulesPath": "./node_modules",
    "requireModules": [
        {"name": "fs", "path": "fs"},
        {"name": "path", "path": "path"}
    ]
}
```

#### Module Management
```javascript
GET /api/devtools/modules
POST /api/devtools/modules/install
{
    "packageName": "express",
    "version": "4.18.2",
    "modulesPath": "./node_modules"
}
```

#### Performance Monitoring
```javascript
POST /api/devtools/performance/start
POST /api/devtools/performance/stop
```

### C++ Integration
```cpp
// Enable Node.js integration
webKitEngine.enableNodeIntegration(true);
webKitEngine.setNodeModulesPath("./node_modules");

// Execute Node.js script
String result = webKitEngine.executeNodeScript(
    "const fs = require('fs'); "
    "return fs.readdirSync('.').join(', ');"
);
```

## 🎨 Developer Tools UI

### Panel Types
```cpp
enum class DevToolsPanel {
    CONSOLE,      // JavaScript console
    ELEMENTS,     // DOM inspector
    NETWORK,      // Network monitoring
    PERFORMANCE,  // Performance profiling
    SOURCES,      // JavaScript debugging
    APPLICATION,  // Storage and cache
    SECURITY,     // Security information
    NODE_JS,      // Node.js integration
    SETTINGS      // DevTools configuration
};
```

### Console Features
- **Message Filtering**: By level (log, info, warn, error, debug)
- **Source Filtering**: Filter by file or domain
- **Search**: Text search in console messages
- **History**: Command history with up/down navigation
- **Auto-completion**: JavaScript expression completion

### Network Features
- **Request Monitoring**: All HTTP/HTTPS requests
- **Response Analysis**: Status codes, headers, timing
- **Filtering**: By type (XHR, JS, CSS, images, etc.)
- **Export**: HAR format export
- **Timeline**: Request timing visualization

### Performance Features
- **CPU Profiling**: JavaScript execution profiling
- **Memory Profiling**: Memory usage and leaks
- **Network Profiling**: Request timing and bandwidth
- **Rendering Profiling**: Paint and layout timing
- **Export**: Performance data export

## 🔒 Security Features

### Security Policies
```cpp
// Set strict security policy
webKitEngine.setSecurityPolicy("strict");

// Request permissions
webKitEngine.requestPermission("geolocation", "https://example.com");
webKitEngine.requestPermission("notifications", "https://example.com");

// Grant/deny permissions
webKitEngine.grantPermission("camera", "https://example.com");
webKitEngine.denyPermission("microphone", "https://example.com");
```

### Security Information
- **Certificate Inspection**: SSL/TLS certificate details
- **Permission Management**: Site permission controls
- **Content Security Policy**: CSP violation reporting
- **Mixed Content**: Insecure content blocking
- **Cross-Origin**: CORS policy enforcement

## 📊 Performance Monitoring

### Metrics Collection
```cpp
struct PerformanceMetrics {
    double domContentLoaded;
    double loadComplete;
    double firstPaint;
    double firstContentfulPaint;
    double largestContentfulPaint;
    double cumulativeLayoutShift;
    double firstInputDelay;
    int totalBytes;
    int totalRequests;
    double averageResponseTime;
};
```

### Monitoring Features
- **Real-time Metrics**: Live performance data
- **Historical Data**: Performance trends over time
- **Threshold Alerts**: Performance degradation warnings
- **Export Reports**: Performance analysis reports
- **Comparison**: Performance comparison between versions

## 🎯 Usage Examples

### Basic Developer Tools Setup
```cpp
#include "engine/dev_tools.h"
#include "ui/dev_tools_ui.h"

// Initialize developer tools
zepra::DevToolsManager devToolsManager;
auto devTools = devToolsManager.createTools();

// Configure tools
devTools->enableNodeIntegration(true);
devTools->enableNetworkMonitoring(true);
devTools->startPerformanceMonitoring();

// Set up callbacks
devTools->setConsoleCallback([](const zepra::ConsoleMessage& msg) {
    std::cout << "Console: " << msg.message << std::endl;
});

devTools->setPerformanceCallback([](const zepra::PerformanceMetrics& metrics) {
    std::cout << "Performance: " << metrics.totalRequests << " requests" << std::endl;
});
```

### WebKit Engine with Developer Tools
```cpp
#include "engine/webkit_engine.h"

// Initialize WebKit engine
zepra::WebKitEngine webKitEngine;
zepra::WebKitConfig config;

config.enableJavaScript = true;
config.enableWebGL = true;
config.enableDevTools = true;
config.enableNodeIntegration = true;
config.userAgent = "Zepra Browser/3.0";

if (webKitEngine.initialize(config)) {
    // Integrate developer tools
    webKitEngine.setDeveloperTools(devTools);
    webKitEngine.enableDeveloperTools(true);
    webKitEngine.enableNetworkMonitoring(true);
    
    // Set up event handlers
    webKitEngine.setEventHandler([](const zepra::WebKitEvent& event) {
        std::cout << "WebKit Event: " << event.url << std::endl;
    });
    
    // Create and load page
    webKitEngine.createPage();
    webKitEngine.loadHTML("<html><body><h1>Hello World</h1></body></html>");
    
    // Execute JavaScript
    String result = webKitEngine.executeJavaScript("document.title");
    
    // Open developer tools
    webKitEngine.openDeveloperTools();
}
```

### Developer Tools UI
```cpp
#include "ui/dev_tools_ui.h"

// Initialize UI
zepra::DevToolsUI devToolsUI;
devToolsUI.initialize(devTools);

// Show panels
devToolsUI.showPanel(zepra::DevToolsPanel::CONSOLE);
devToolsUI.showPanel(zepra::DevToolsPanel::NETWORK);
devToolsUI.showPanel(zepra::DevToolsPanel::PERFORMANCE);

// Configure console
zepra::ConsoleFilter filter;
filter.showLog = true;
filter.showError = true;
filter.searchText = "error";
devToolsUI.setConsoleFilter(filter);

// Execute console command
devToolsUI.executeConsoleCommand("console.log('Hello from DevTools UI');");

// Start performance recording
devToolsUI.startPerformanceRecording();

// Export data
devToolsUI.exportNetworkLog("network_log.har");
devToolsUI.exportPerformanceData("performance_data.json");
```

## 🚀 Advanced Features

### Node.js Module Management
```cpp
// Update available modules
devToolsUI.updateNodeModules();

// Install package
devToolsUI.installNodePackage("express", "4.18.2");
devToolsUI.installNodePackage("lodash", "4.17.21");

// Execute Node.js script
devToolsUI.executeNodeScript(R"(
    const express = require('express');
    const app = express();
    
    app.get('/', (req, res) => {
        res.json({ message: 'Hello from Node.js!' });
    });
    
    console.log('Express app created');
    return 'Server ready on port 3000';
)");
```

### Network Monitoring
```cpp
// Enable network monitoring
webKitEngine.enableNetworkMonitoring(true);

// Get network data
auto requests = webKitEngine.getNetworkRequests();
auto responses = webKitEngine.getNetworkResponses();

// Configure network filter
zepra::NetworkFilter filter;
filter.showXHR = true;
filter.showJS = true;
filter.statusFilter = "200,404";
devToolsUI.setNetworkFilter(filter);

// Export network log
devToolsUI.exportNetworkLog("network_analysis.har");
```

### Performance Profiling
```cpp
// Start performance monitoring
webKitEngine.startPerformanceMonitoring();

// Get performance metrics
auto metrics = webKitEngine.getPerformanceMetrics();
std::cout << "Load time: " << metrics.loadComplete << "ms" << std::endl;
std::cout << "First paint: " << metrics.firstPaint << "ms" << std::endl;

// Export performance data
devToolsUI.exportPerformanceData("performance_profile.json");
```

## 🔧 Configuration

### Environment Variables
```bash
# Node.js backend
PORT=6329
NODE_ENV=development
FRONTEND_URL=http://localhost:3000

# Browser settings
ZEPRA_DEVTOOLS_ENABLED=true
ZEPRA_NODE_INTEGRATION=true
ZEPRA_NETWORK_MONITORING=true
ZEPRA_PERFORMANCE_MONITORING=true
```

### Settings File
```json
{
    "devTools": {
        "enabled": true,
        "nodeIntegration": true,
        "networkMonitoring": true,
        "performanceMonitoring": true,
        "preserveLog": true,
        "showTimestamps": true
    },
    "ui": {
        "theme": "dark",
        "fontSize": 14,
        "colorScheme": "dark",
        "layout": "default",
        "autoRefresh": true,
        "refreshInterval": 1000
    },
    "webkit": {
        "enableJavaScript": true,
        "enableWebGL": true,
        "enableDevTools": true,
        "enableNodeIntegration": true,
        "maxMemoryUsage": 1024,
        "enableCaching": true
    }
}
```

## 🎯 Keyboard Shortcuts

### Default Shortcuts
- `F12` - Toggle Developer Tools
- `Ctrl+Shift+I` - Open Inspector
- `Ctrl+Shift+J` - Open Console
- `Ctrl+Shift+C` - Inspect Element
- `Ctrl+Shift+N` - Open Network Panel
- `Ctrl+Shift+P` - Open Performance Panel
- `Ctrl+R` - Reload page
- `Ctrl+Shift+R` - Hard reload
- `Ctrl+U` - View page source
- `Ctrl+Shift+M` - Toggle device mode

### Custom Shortcuts
```cpp
// Register custom shortcuts
devToolsUI.registerShortcut("Ctrl+Shift+Z", []() {
    std::cout << "Custom shortcut executed" << std::endl;
});

devToolsUI.registerShortcut("F5", []() {
    webKitEngine.reload();
});
```

## 📈 Performance Optimization

### Memory Management
```cpp
// Set memory limits
webKitEngine.setMemoryLimit(1024 * 1024 * 1024); // 1GB

// Monitor memory usage
size_t usage = webKitEngine.getMemoryUsage();
size_t limit = webKitEngine.getMemoryLimit();

// Garbage collection
webKitEngine.garbageCollect();

// Clear caches
webKitEngine.clearCache();
webKitEngine.clearMemoryCache();
```

### Caching Strategy
```cpp
// Enable caching
webKitEngine.setCacheEnabled(true);
webKitEngine.setCacheSize(100 * 1024 * 1024); // 100MB

// Configure cache policies
webKitEngine.setCustomHeaders({
    {"Cache-Control", "max-age=3600"},
    {"ETag", "zepra-browser-v3.0"}
});
```

## 🔍 Debugging Features

### Console Debugging
```cpp
// Log different levels
devTools->log("Info message", zepra::ConsoleLevel::INFO);
devTools->log("Warning message", zepra::ConsoleLevel::WARN);
devTools->log("Error message", zepra::ConsoleLevel::ERROR);

// Log with source information
devTools->log("Debug message", "app.js", 42, 10);

// Clear console
devTools->clearConsole();
```

### Network Debugging
```cpp
// Monitor network requests
devTools->logNetworkRequest({
    .url = "https://api.example.com/data",
    .method = "POST",
    .headers = "Content-Type: application/json",
    .postData = "{\"key\":\"value\"}"
});

// Monitor network responses
devTools->logNetworkResponse({
    .url = "https://api.example.com/data",
    .statusCode = 200,
    .statusText = "OK",
    .headers = "Content-Type: application/json",
    .body = "{\"result\":\"success\"}"
});
```

## 🎨 UI Customization

### Themes
```cpp
// Available themes
devToolsUI.setTheme("light");   // Light theme
devToolsUI.setTheme("dark");    // Dark theme
devToolsUI.setTheme("auto");    // System theme
```

### Layout
```cpp
// Available layouts
devToolsUI.setLayout("default");     // Default layout
devToolsUI.setLayout("compact");     // Compact layout
devToolsUI.setLayout("expanded");    // Expanded layout
devToolsUI.setLayout("custom");      // Custom layout
```

### Font and Colors
```cpp
// Font settings
devToolsUI.setFontSize(12);          // Small
devToolsUI.setFontSize(14);          // Medium
devToolsUI.setFontSize(16);          // Large

// Color schemes
devToolsUI.setColorScheme("light");  // Light colors
devToolsUI.setColorScheme("dark");   // Dark colors
devToolsUI.setColorScheme("high-contrast"); // High contrast
```

## 🚀 Getting Started

### 1. Build the Project
```bash
# Clone the repository
git clone https://github.com/ketivee/zepra-browser.git
cd zepra-browser

# Build with CMake
mkdir build && cd build
cmake ..
make -j$(nproc)

# Or use the build script
./build.sh
```

### 2. Start the Backend
```bash
# Install Node.js dependencies
cd ketiveeserchengin/backend
npm install

# Start the server
npm start
# or
node server.js
```

### 3. Run the Browser
```bash
# From the build directory
./bin/zepra

# Or use the startup script
./start_browser.bat
```

### 4. Open Developer Tools
- Press `F12` to open developer tools
- Use `Ctrl+Shift+I` for inspector
- Use `Ctrl+Shift+J` for console
- Use `Ctrl+Shift+N` for network panel

## 📚 API Reference

### DeveloperTools API
See `include/engine/dev_tools.h` for complete API documentation.

### WebKit Engine API
See `include/engine/webkit_engine.h` for complete API documentation.

### DevTools UI API
See `include/ui/dev_tools_ui.h` for complete API documentation.

### Node.js Backend API
See `ketiveeserchengin/backend/routes/devtools.js` for complete API documentation.

## 🤝 Contributing

### Development Setup
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new features
5. Submit a pull request

### Code Style
- Follow the existing C++ coding style
- Use meaningful variable and function names
- Add comments for complex logic
- Include error handling
- Write unit tests for new features

### Testing
```bash
# Run unit tests
make test

# Run integration tests
./test/integration_test.sh

# Run performance tests
./test/performance_test.sh
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- WebKit team for the excellent browser engine
- Node.js team for the JavaScript runtime
- Chrome DevTools team for inspiration
- Firefox Developer Tools team for ideas
- Safari Web Inspector team for concepts

## 📞 Support

- **Documentation**: [https://zepra.ketivee.com/docs](https://zepra.ketivee.com/docs)
- **Issues**: [https://github.com/ketivee/zepra-browser/issues](https://github.com/ketivee/zepra-browser/issues)
- **Discussions**: [https://github.com/ketivee/zepra-browser/discussions](https://github.com/ketivee/zepra-browser/discussions)
- **Email**: support@ketivee.org

---

**Zepra Browser** - Advanced Developer Tools for Modern Web Development 