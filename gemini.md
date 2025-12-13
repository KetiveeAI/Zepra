# Zepra Browser - Integration Guide for YOUR Components

## Your Custom Stack

```
┌─────────────────────────────────────────┐
│         Zepra Browser UI (Qt)           │  ← This is what I built
├─────────────────────────────────────────┤
│  YOUR Components (already built):       │
│  • ZepraEngine    (browser engine)      │
│  • ZepraScript    (JS engine)           │
│  • HTML/CSS Parser                      │
│  • ZepraSearch    (search engine)       │
│  • ZepraDevTools  (developer tools)     │
│  • Backend API    (C++ ketivee.com)     │
└─────────────────────────────────────────┘
```

## Project Structure

```
ZepraBrowser/
├── main.cpp                 # Entry point
├── mainwindow.h/.cpp        # UI implementation (THIS)
├── ZepraEngine.h/.cpp       # YOUR browser engine
├── ZepraScript.h/.cpp       # YOUR JS engine
├── ZepraSearchAPI.h/.cpp    # YOUR search API client
├── ZepraDevTools.h/.cpp     # YOUR dev tools
├── HTMLParser.h/.cpp        # YOUR HTML parser
├── CSSParser.h/.cpp         # YOUR CSS parser
└── ZepraBrowser.pro         # Qt project file
```

## Integration Points

### 1️⃣ ZepraEngine Integration

In `BrowserTab::loadPage()`:

```cpp
void BrowserTab::loadPage(const QString &url) {
    // YOUR ZepraEngine loads the page
    engine->loadURL(url);
    
    // ZepraEngine should:
    // 1. Fetch HTML from network
    // 2. Parse with YOUR HTML/CSS parser
    // 3. Execute JS with ZepraScript
    // 4. Render to the widget surface
}
```

Connect signals:
```cpp
// In BrowserTab constructor
engine = new ZepraEngine(this);
jsEngine = new ZepraScript(engine);

connect(engine, &ZepraEngine::loadFinished,
        this, &BrowserTab::onEngineLoadComplete);
connect(engine, &ZepraEngine::titleChanged,
        this, &BrowserTab::onTitleChanged);
connect(engine, &ZepraEngine::iconChanged,
        this, [this](const QIcon &icon) {
            emit iconChanged(icon);
        });

// Render to surface
engine->setRenderTarget(renderWidget->getRenderSurface());
```

### 2️⃣ ZepraScript Integration

Each tab has its own JS engine instance:

```cpp
// In BrowserTab constructor
jsEngine = new ZepraScript(engine);
jsEngine->setGlobalObject("window", engine->getWindowObject());
jsEngine->setGlobalObject("document", engine->getDOMTree());

// Execute scripts
jsEngine->evaluate(scriptCode);
```

### 3️⃣ ZepraSearch Integration

Search queries go to ketivee.com:

```cpp
bool BrowserTab::isSearchQuery(const QString &input) {
    // Space = search query
    if (input.contains(" ")) return true;
    
    // Has protocol = URL
    if (input.startsWith("http")) return false;
    
    // Has dot, no space = likely domain
    if (input.contains(".") && !input.contains(" ")) return false;
    
    return true; // Everything else is search
}

QString BrowserTab::buildSearchUrl(const QString &query) {
    // YOUR ZepraSearch API
    return searchAPI->buildSearchURL(query);
    
    // Or direct:
    // return "https://ketivee.com/search?q=" + query;
}
```

Initialize search API:
```cpp
// In MainWindow constructor
searchAPI = new ZepraSearchAPI("https://ketivee.com/api");
searchAPI->setAPIKey("your_api_key"); // If needed
```

### 4️⃣ ZepraDevTools Integration

Open dev tools with F12 or button:

```cpp
void MainWindow::onDevToolsClicked() {
    if (currentTabIndex >= 0) {
        BrowserTab *tab = tabs[currentTabIndex];
        
        // YOUR ZepraDevTools
        ZepraDevTools *devTools = new ZepraDevTools(this);
        devTools->attachTo(tab->getEngine());
        devTools->show();
        
        // DevTools should inspect:
        // - DOM tree from engine
        // - Network requests
        // - Console logs from ZepraScript
        // - Performance metrics
    }
}
```

### 5️⃣ Network/Backend Integration

Your C++ backend API calls:

```cpp
// Example API client structure
class ZepraSearchAPI : public QObject {
public:
    ZepraSearchAPI(const QString &apiUrl);
    
    // Search
    void search(const QString &query);
    QString buildSearchURL(const QString &query);
    
    // Other API calls
    void getUserData();
    void saveBookmark(const QString &url);
    
signals:
    void searchResults(const QJsonArray &results);
    void apiError(const QString &error);
};
```

## Build Instructions

### Prerequisites
```bash
# Install Qt
sudo apt-get install qt6-base-dev

# Your existing codebase
# ZepraEngine, ZepraScript, parsers, etc.
```

### CMakeLists.txt (Recommended)

```cmake
cmake_minimum_required(VERSION 3.16)
project(ZepraBrowser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)

add_executable(ZepraBrowser
    main.cpp
    mainwindow.cpp
    
    # YOUR components
    ZepraEngine.cpp
    ZepraScript.cpp
    ZepraSearchAPI.cpp
    ZepraDevTools.cpp
    HTMLParser.cpp
    CSSParser.cpp
)

target_link_libraries(ZepraBrowser
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)
```

### Build

```bash
mkdir build && cd build
cmake ..
make
./ZepraBrowser
```

## What YOUR Components Need to Provide

### ZepraEngine.h
```cpp
class ZepraEngine : public QObject {
    Q_OBJECT
public:
    explicit ZepraEngine(QObject *parent = nullptr);
    
    // Load URL
    void loadURL(const QString &url);
    
    // Render target
    void setRenderTarget(QWidget *target);
    
    // Access
    QObject* getWindowObject();
    QObject* getDOMTree();
    
signals:
    void loadStarted();
    void loadProgress(int percent);
    void loadFinished(bool success);
    void titleChanged(const QString &title);
    void iconChanged(const QIcon &icon);
    void urlChanged(const QString &url);
};
```

### ZepraScript.h
```cpp
class ZepraScript : public QObject {
    Q_OBJECT
public:
    explicit ZepraScript(ZepraEngine *engine);
    
    // Execute JavaScript
    QVariant evaluate(const QString &code);
    
    // Global objects
    void setGlobalObject(const QString &name, QObject *obj);
    
    // Console
    void log(const QString &message);
    
signals:
    void consoleOutput(const QString &message);
    void scriptError(const QString &error, int line);
};
```

### ZepraSearchAPI.h
```cpp
class ZepraSearchAPI : public QObject {
    Q_OBJECT
public:
    explicit ZepraSearchAPI(const QString &apiBaseUrl);
    
    QString buildSearchURL(const QString &query);
    void performSearch(const QString &query);
    
signals:
    void searchComplete(const QJsonArray &results);
};
```

### ZepraDevTools.h
```cpp
class ZepraDevTools : public QWidget {
    Q_OBJECT
public:
    explicit ZepraDevTools(QWidget *parent = nullptr);
    
    void attachTo(ZepraEngine *engine);
    void detach();
    
    // Panels
    void showElementsPanel();
    void showConsolePanel();
    void showNetworkPanel();
    void showPerformancePanel();
};
```

## UI Features Included

✅ Chromium-style tab bar at top
✅ New tab button (+)
✅ URL/search bar (detects URLs vs searches)
✅ Navigation buttons (back, forward, refresh, home)
✅ DevTools button (⚙)
✅ Tab history (per tab)
✅ Multiple tabs
✅ Dark theme matching your design
✅ Default to ketivee.com

## Testing

### Without Your Engine (Current State)
```bash
./ZepraBrowser
# Opens with test UI
# All buttons work
# Tabs work
# URL bar detects search vs URL
```

### With Your Engine Connected
```cpp
// In BrowserTab::loadPage(), uncomment:
engine->loadURL(url);

// Build and run:
# Should now render actual pages with YOUR engine
```

## Default Behavior

- 🏠 **Home**: `https://ketivee.com`
- 🔍 **Search**: Queries go to `ketivee.com/search?q=...`
- 🆕 **New Tab**: Opens `ketivee.com`
- ⚙️ **DevTools**: F12 or button opens YOUR ZepraDevTools

## Next Steps

1. ✅ **Test UI** - Run now, all UI works
2. 🔌 **Connect ZepraEngine** - Load actual pages
3. 🔌 **Connect ZepraScript** - Execute JavaScript
4. 🔌 **Connect ZepraSearch** - Wire up search API
5. 🔌 **Connect ZepraDevTools** - Add inspection
6. 🎨 **Polish** - Add loading indicators, favorites, etc.

The UI shell is complete and ready for your components!