/*
 * WebCore Integration Bridge for Native Browser
 * 
 * Links the native zepra_browser.cpp with WebCore (CSS/DOM) and ZebraScript (JS)
 */

#pragma once

#include <string>
#include <memory>

// Forward declarations (avoid pulling in full headers for faster compile)
namespace Zepra {
    namespace WebCore {
        class HTMLParser;
        class DOMDocument;
        class CSSEngine;
        class StyleResolver;
        class ScriptContext;
    }
    namespace Runtime {
        class VM;
    }
}

/**
 * @brief Simplified bridge for native browser
 */
class WebCoreBridge {
public:
    static WebCoreBridge& instance() {
        static WebCoreBridge inst;
        return inst;
    }
    
    // Initialize WebCore (call once at startup)
    bool initialize();
    
    // Parse HTML content
    bool parseHTML(const std::string& html);
    
    // Get page title
    std::string getPageTitle();
    
    // Extract CSS from parsed document
    void extractStyles();
    
    // Execute JavaScript
    bool executeScript(const std::string& script);
    
    // Get computed style for element
    // Returns CSS properties as map
    struct ComputedStyle {
        uint32_t color = 0x000000;
        uint32_t backgroundColor = 0xFFFFFF;
        float fontSize = 16.0f;
        bool fontBold = false;
        std::string fontFamily = "sans-serif";
    };
    
    // Get text content lines for rendering
    struct TextLine {
        std::string text;
        float x, y;
        ComputedStyle style;
    };
    
    std::vector<TextLine> getTextLines(float viewWidth, float viewHeight);
    
    // Cleanup
    void shutdown();
    
private:
    WebCoreBridge() = default;
    ~WebCoreBridge() { shutdown(); }
    
    // WebCore objects (opaque pointers to avoid header deps)
    void* document_ = nullptr;  // DOMDocument*
    void* cssEngine_ = nullptr; // CSSEngine*
    void* scriptCtx_ = nullptr; // ScriptContext*
    
    bool initialized_ = false;
};

// C-style functions for simpler linking
extern "C" {
    int webcore_init();
    int webcore_parse_html(const char* html);
    const char* webcore_get_title();
    int webcore_exec_js(const char* script);
    void webcore_shutdown();
}
