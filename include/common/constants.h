#pragma once

#include "types.h"

namespace zepra {

// Browser Constants
constexpr const char* BROWSER_NAME = "Zepra Browser";
constexpr const char* BROWSER_VERSION = "1.0.0";
constexpr const char* BROWSER_USER_AGENT = "Zepra/1.0.0 (Ketivee OS)";

// Window Constants
constexpr int DEFAULT_WINDOW_WIDTH = 1200;
constexpr int DEFAULT_WINDOW_HEIGHT = 800;
constexpr int MIN_WINDOW_WIDTH = 400;
constexpr int MIN_WINDOW_HEIGHT = 300;
constexpr int MAX_WINDOW_WIDTH = 3840;
constexpr int MAX_WINDOW_HEIGHT = 2160;

// UI Constants
constexpr int TOOLBAR_HEIGHT = 40;
constexpr int TAB_HEIGHT = 32;
constexpr int TAB_BAR_HEIGHT = 32;
constexpr int ADDRESS_BAR_HEIGHT = 30;
constexpr int BOOKMARKS_BAR_HEIGHT = 24;
constexpr int STATUS_BAR_HEIGHT = 20;
constexpr int SCROLLBAR_WIDTH = 16;
constexpr int BORDER_WIDTH = 1;

// Rendering Constants
constexpr float DEFAULT_FONT_SIZE = 16.0f;
constexpr float DEFAULT_LINE_HEIGHT = 1.2f;
constexpr int MAX_TEXTURE_SIZE = 4096;
constexpr int DEFAULT_DPI = 96;

// Network Constants
constexpr int DEFAULT_TIMEOUT_MS = 30000;
constexpr int MAX_REDIRECTS = 10;
constexpr int MAX_CONNECTIONS = 10;
constexpr int BUFFER_SIZE = 8192;

// Search Constants
constexpr const char* KETIVEE_SEARCH_URL = "http://localhost:8080";
constexpr const char* SEARCH_API_ENDPOINT = "http://localhost:8080/api/search";
constexpr const char* MULTIMODAL_API_ENDPOINT = "http://localhost:8080/api/multimodal";
constexpr const char* DEFAULT_SEARCH_ENGINE = "ZepraSearch";
constexpr int MAX_SEARCH_RESULTS = 20;
constexpr int SEARCH_DELAY_MS = 300;

// Cache Constants
constexpr int MAX_CACHE_SIZE_MB = 100;
constexpr int MAX_HISTORY_ENTRIES = 1000;
constexpr int MAX_BOOKMARKS = 1000;
constexpr int MAX_COOKIES = 1000;

// Security Constants
constexpr const char* ALLOWED_PROTOCOLS[] = {"http:", "https:", "file:", "data:"};
constexpr const char* BLOCKED_PROTOCOLS[] = {"javascript:", "vbscript:", "data:text/html"};
constexpr int MAX_SCRIPT_EXECUTION_TIME_MS = 5000;

// Performance Constants
constexpr int MAX_DOM_NODES = 10000;
constexpr int MAX_CSS_RULES = 1000;
constexpr int MAX_LAYOUT_ITERATIONS = 10;
constexpr float LAYOUT_EPSILON = 0.1f;

// File Paths
constexpr const char* CONFIG_DIR = ".zepra";
constexpr const char* CACHE_DIR = "cache";
constexpr const char* COOKIES_FILE = "cookies.json";
constexpr const char* HISTORY_FILE = "history.json";
constexpr const char* BOOKMARKS_FILE = "bookmarks.json";
constexpr const char* SETTINGS_FILE = "settings.json";

// Default URLs
constexpr const char* DEFAULT_HOME_PAGE = "https://ketivee.com";
constexpr const char* DEFAULT_NEW_TAB_PAGE = "about:newtab";
constexpr const char* DEFAULT_ERROR_PAGE = "about:error";
constexpr const char* DEFAULT_BLANK_PAGE = "about:blank";

// Color Schemes
namespace colors {
    // Light Theme
    constexpr Color BACKGROUND = Color(255, 255, 255);
    constexpr Color FOREGROUND = Color(0, 0, 0);
    constexpr Color PRIMARY = Color(0, 120, 215);
    constexpr Color SECONDARY = Color(240, 240, 240);
    constexpr Color BORDER = Color(200, 200, 200);
    constexpr Color HIGHLIGHT = Color(0, 120, 215, 50);
    constexpr Color ERROR_COLOR = Color(215, 0, 0);
    constexpr Color SUCCESS = Color(0, 128, 0);
    constexpr Color WARNING = Color(255, 165, 0);
    
    // Dark Theme
    constexpr Color DARK_BACKGROUND = Color(32, 32, 32);
    constexpr Color DARK_FOREGROUND = Color(255, 255, 255);
    constexpr Color DARK_PRIMARY = Color(0, 120, 215);
    constexpr Color DARK_SECONDARY = Color(48, 48, 48);
    constexpr Color DARK_BORDER = Color(64, 64, 64);
    constexpr Color DARK_HIGHLIGHT = Color(0, 120, 215, 50);
}

// Font Constants
namespace fonts {
    constexpr const char* DEFAULT_FAMILY = "Segoe UI, Arial, sans-serif";
    constexpr const char* MONOSPACE_FAMILY = "Consolas, Monaco, monospace";
    constexpr const char* SERIF_FAMILY = "Times New Roman, serif";
    
    constexpr float SMALL_SIZE = 12.0f;
    constexpr float NORMAL_SIZE = 14.0f;
    constexpr float LARGE_SIZE = 16.0f;
    constexpr float EXTRA_LARGE_SIZE = 18.0f;
}

// Animation Constants
constexpr int ANIMATION_DURATION_MS = 200;
constexpr float ANIMATION_EASING = 0.25f;
constexpr int FPS = 60;
constexpr int FRAME_TIME_MS = 1000 / FPS;

// Memory Constants
constexpr int MAX_MEMORY_USAGE_MB = 512;
constexpr int GARBAGE_COLLECTION_INTERVAL_MS = 5000;
constexpr float MEMORY_PRESSURE_THRESHOLD = 0.8f;

// Debug Constants
constexpr bool DEBUG_MODE = false;
constexpr bool VERBOSE_LOGGING = false;
constexpr const char* LOG_FILE = "zepra.log";
constexpr int MAX_LOG_SIZE_MB = 10;

} // namespace zepra 