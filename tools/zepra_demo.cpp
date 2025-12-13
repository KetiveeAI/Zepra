/**
 * @file zepra_demo.cpp
 * @brief Zepra Browser - Full featured browser with real font rendering
 */

#include <iostream>
#include <unordered_map>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// ZepraScript includes
#include "zeprascript/config.hpp"

// WebCore includes
#include "webcore/dom.hpp"
#include "webcore/html_parser.hpp"
#include "webcore/css_parser.hpp"
#include "webcore/render_tree.hpp"
#include "webcore/layout_engine.hpp"
#include "webcore/paint_context.hpp"
#include "webcore/browser_ui.hpp"
#include "webcore/page.hpp"
#include "webcore/theme.hpp"

using namespace Zepra;

// ============================================================================
// Internal Pages HTML
// ============================================================================

const char* NEW_TAB_HTML = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>New Tab - Zepra Browser</title>
</head>
<body>
    <h1>Zepra</h1>
    <p>Fast. Private. Beautiful.</p>
    
    <h2>Quick Links</h2>
    <p>Ketivee - ketivee.com</p>
    <p>News - vartta.ketivee.com</p>
    <p>Mail - mail.ketivee.com</p>
    <p>Settings - zepra://settings</p>
    
    <h2>Getting Started</h2>
    <p>Type a URL or search query in the address bar above.</p>
    <p>Press Ctrl+T for new tab, Ctrl+W to close tab.</p>
    
    <script>
        console.log("Welcome to Zepra Browser!");
        console.log("ZepraScript Engine v1.1 Active");
        console.log("Math check: 2 + 2 = " + (2+2));
        
        try {
            let content = readFile("gemini.md");
            if (content) {
                console.log("Successfully read gemini.md: " + content);
            } else {
                console.log("Could not read gemini.md (might not exist)");
            }
        } catch(e) {
            console.log("Error reading file: " + e);
        }
    </script>
    
    <p>Powered by Ketivee</p>
</body>
</html>
)HTML";

const char* SETTINGS_HTML = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Settings - Zepra Browser</title>
</head>
<body>
    <h1>Settings</h1>
    
    <h2>Appearance</h2>
    <p>Theme: Purple (Default)</p>
    <p>Font Size: Medium</p>
    
    <h2>Privacy and Security</h2>
    <p>Clear browsing data</p>
    <p>Block trackers: Enabled</p>
    
    <h2>Search Engine</h2>
    <p>Default: Ketivee Search</p>
    
    <h2>About</h2>
    <p>Zepra Browser v1.0.0</p>
    <p>Built by Ketivee</p>
    <p>ZepraScript Engine - WebCore Renderer</p>
</body>
</html>
)HTML";

const char* BOOKMARKS_HTML = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Bookmarks - Zepra Browser</title>
</head>
<body>
    <h1>Bookmarks</h1>
    
    <h3>Ketivee</h3>
    <p>https://ketivee.com</p>
    
    <h3>Ketivee Search</h3>
    <p>https://search.ketivee.com</p>
    
    <h3>Vartta News</h3>
    <p>https://vartta.ketivee.com</p>
    
    <h3>Ketivee Mail</h3>
    <p>https://mail.ketivee.com</p>
</body>
</html>
)";

// ============================================================================
// Font Manager
// ============================================================================

class FontManager {
public:
    static FontManager& instance() {
        static FontManager inst;
        return inst;
    }
    
    bool init() {
        if (TTF_Init() < 0) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
            return false;
        }
        
        // Try common font paths
        const char* fontPaths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            nullptr
        };
        
        for (int i = 0; fontPaths[i]; i++) {
            defaultFont_ = TTF_OpenFont(fontPaths[i], 14);
            if (defaultFont_) {
                fontPath_ = fontPaths[i];
                std::cout << "Font loaded: " << fontPath_ << std::endl;
                break;
            }
        }
        
        if (!defaultFont_) {
            std::cerr << "Could not load any font!" << std::endl;
            return false;
        }
        
        return true;
    }
    
    void shutdown() {
        for (auto& [key, font] : fonts_) {
            if (font) TTF_CloseFont(font);
        }
        fonts_.clear();
        if (defaultFont_) TTF_CloseFont(defaultFont_);
        defaultFont_ = nullptr;
        TTF_Quit();
    }
    
    TTF_Font* getFont(int size, bool bold = false) {
        int key = size * 10 + (bold ? 1 : 0);
        auto it = fonts_.find(key);
        if (it != fonts_.end()) return it->second;
        
        TTF_Font* font = TTF_OpenFont(fontPath_.c_str(), size);
        if (font && bold) {
            TTF_SetFontStyle(font, TTF_STYLE_BOLD);
        }
        fonts_[key] = font ? font : defaultFont_;
        return fonts_[key];
    }
    
    TTF_Font* defaultFont() { return defaultFont_; }
    
private:
    FontManager() = default;
    TTF_Font* defaultFont_ = nullptr;
    std::string fontPath_;
    std::unordered_map<int, TTF_Font*> fonts_;
};

// ============================================================================
// SDL Text Renderer
// ============================================================================

class SDLRenderer {
public:
    SDLRenderer(SDL_Renderer* renderer) : renderer_(renderer) {}
    
    void render(const WebCore::DisplayList& displayList) {
        for (const auto& cmd : displayList.commands()) {
            switch (cmd.type) {
                case WebCore::PaintCommandType::FillRect:
                    SDL_SetRenderDrawColor(renderer_, 
                        cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
                    {
                        SDL_FRect rect = {cmd.rect.x, cmd.rect.y, cmd.rect.width, cmd.rect.height};
                        SDL_RenderFillRectF(renderer_, &rect);
                    }
                    break;
                    
                case WebCore::PaintCommandType::StrokeRect:
                    SDL_SetRenderDrawColor(renderer_, 
                        cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
                    {
                        SDL_FRect rect = {cmd.rect.x, cmd.rect.y, cmd.rect.width, cmd.rect.height};
                        SDL_RenderDrawRectF(renderer_, &rect);
                    }
                    break;
                    
                case WebCore::PaintCommandType::DrawText:
                    renderText(cmd.text, cmd.rect.x, cmd.rect.y, 
                              cmd.color, cmd.number > 0 ? static_cast<int>(cmd.number) : 14);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    void renderText(const std::string& text, float x, float y, 
                    WebCore::Color color, int fontSize) {
        if (text.empty()) return;
        
        TTF_Font* font = FontManager::instance().getFont(fontSize);
        if (!font) return;
        
        SDL_Color sdlColor = {color.r, color.g, color.b, color.a};
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), sdlColor);
        if (!surface) return;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        if (texture) {
            SDL_FRect dst = {x, y, static_cast<float>(surface->w), static_cast<float>(surface->h)};
            SDL_RenderCopyF(renderer_, texture, nullptr, &dst);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
    
private:
    SDL_Renderer* renderer_;
};

// ============================================================================
// Browser State
// ============================================================================

struct TabState {
    std::unique_ptr<WebCore::Page> page;
    std::string url;
    std::string title;
};

class BrowserState {
public:
    std::vector<TabState> tabs;
    int activeTab = -1;
    
    void addTab(const std::string& url = "zepra://newtab") {
        TabState state;
        state.page = std::make_unique<WebCore::Page>();
        state.url = url;
        state.title = "Loading...";
        
        loadPage(state, url);
        
        tabs.push_back(std::move(state));
        activeTab = tabs.size() - 1;
    }
    
    void loadPage(TabState& state, const std::string& url) {
        state.url = url;
        
        if (url == "zepra://newtab" || url.empty()) {
            state.page->loadHTML(NEW_TAB_HTML, "zepra://newtab");
            state.title = "New Tab";
        } else if (url == "zepra://settings") {
            state.page->loadHTML(SETTINGS_HTML, "zepra://settings");
            state.title = "Settings";
        } else if (url == "zepra://bookmarks") {
            state.page->loadHTML(BOOKMARKS_HTML, "zepra://bookmarks");
            state.title = "Bookmarks";
        } else if (url == "zepra://bookmarks") {
            state.page->loadHTML(BOOKMARKS_HTML, "zepra://bookmarks");
            state.title = "Bookmarks";
        } else if (url.find("javascript:") == 0) {
            std::string script = url.substr(11);
            auto result = state.page->scriptContext()->evaluate(script);
            std::cout << "JS Result: " << (result.success ? result.value : result.error) << std::endl;
            // Stay on current page but maybe show alert?
            // For now, treat as no-nav
        } else if (url.find("zepra://search?q=") == 0) {
            std::string query = url.substr(17);
            std::string html = R"(
<!DOCTYPE html>
<html>
<head><title>Search: )" + query + R"(</title></head>
<body>
<h1>Search Results</h1>
<h2>)" + query + R"(</h2>

<h3>Ketivee - )" + query + R"(</h3>
<p>Find information about )" + query + R"( on Ketivee Search.</p>

<h3>Wikipedia - )" + query + R"(</h3>
<p>Learn more from the free encyclopedia.</p>

<h3>News - )" + query + R"(</h3>
<p>Latest news and updates.</p>

<p>Powered by Ketivee Search</p>
</body>
</html>
)";
            state.page->loadHTML(html, url);
            state.title = "Search: " + query;
        } else {
            // Load external page using ResourceLoader
            std::cout << "Loading: " << url << std::endl;
            state.page->loadFromURL(url);
            state.title = url;
            
            // Auto-dump for test-css (hack for testing)
            if (url.find("test_css.html") != std::string::npos) {
                 if (state.page->renderTree()) state.page->renderTree()->dump();
            }
        }
    }
    
    void navigate(const std::string& input) {
        if (activeTab < 0 || activeTab >= (int)tabs.size()) return;
        
        std::string url = input;
        
        if (input.find("://") == std::string::npos && 
            (input.find('.') == std::string::npos || input.find(' ') != std::string::npos)) {
            url = "zepra://search?q=" + input;
        } else if (input.find("://") == std::string::npos) {
            url = "https://" + input;
        }
        
        loadPage(tabs[activeTab], url);
    }
    
    TabState* currentTab() {
        if (activeTab >= 0 && activeTab < (int)tabs.size()) {
            return &tabs[activeTab];
        }
        return nullptr;
    }
    
    void closeTab(int index) {
        if (index < 0 || index >= (int)tabs.size() || tabs.size() <= 1) return;
        tabs.erase(tabs.begin() + index);
        if (activeTab >= (int)tabs.size()) {
            activeTab = tabs.size() - 1;
        }
    }
};

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    std::string startUrl = "zepra://newtab";
    bool autoDump = false;
    
    if (argc > 1) {
        startUrl = argv[1];
        if (startUrl.find("://") == std::string::npos) {
             startUrl = "file://" + startUrl;
        }
    }
    if (argc > 2 && std::string(argv[2]) == "--dump") {
        autoDump = true;
    }
    
    std::cout << "=== Zepra Browser ===" << std::endl;
    std::cout << "Version: " << ZEPRA_VERSION << std::endl;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    if (!FontManager::instance().init()) {
        std::cerr << "Font initialization failed" << std::endl;
        SDL_Quit();
        return 1;
    }
    
    const int WINDOW_WIDTH = 1200;
    const int WINDOW_HEIGHT = 800;
    
    SDL_Window* window = SDL_CreateWindow(
        "Zepra Browser",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        FontManager::instance().shutdown();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!renderer) {
        SDL_DestroyWindow(window);
        FontManager::instance().shutdown();
        SDL_Quit();
        return 1;
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_StartTextInput();
    
    BrowserState browser;
    WebCore::BrowserChrome chrome(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    browser.addTab(startUrl);
    chrome.tabBar()->addTab(browser.tabs[0].title, browser.tabs[0].url);
    chrome.navigationBar()->setURL("zepra://newtab");

    
    chrome.tabBar()->setOnNewTab([&]() {
        browser.addTab();
        chrome.tabBar()->addTab(browser.currentTab()->title, browser.currentTab()->url);
        chrome.tabBar()->setActiveTab(browser.activeTab);
        chrome.navigationBar()->setURL(browser.currentTab()->url);
    });
    
    chrome.tabBar()->setOnTabSwitch([&](int index) {
        browser.activeTab = index;
        if (auto* tab = browser.currentTab()) {
            chrome.navigationBar()->setURL(tab->url);
        }
    });
    
    chrome.tabBar()->setOnCloseTab([&](int index) {
        browser.closeTab(index);
        if (auto* tab = browser.currentTab()) {
            chrome.navigationBar()->setURL(tab->url);
        }
    });
    
    chrome.navigationBar()->setOnNavigate([&](const std::string& input) {
        browser.navigate(input);
        if (auto* tab = browser.currentTab()) {
            chrome.navigationBar()->setURL(tab->url);
            if (auto* uiTab = chrome.tabBar()->getTab(browser.activeTab)) {
                uiTab->title = tab->title;
                uiTab->url = tab->url;
            }
        }
    });
    
    chrome.navigationBar()->setOnRefresh([&]() {
        if (auto* tab = browser.currentTab()) {
            browser.loadPage(*tab, tab->url);
        }
    });
    
    SDLRenderer sdlRenderer(renderer);
    
    std::cout << "\n=== Browser Ready ===" << std::endl;
    std::cout << "Type in address bar and press Enter" << std::endl;
    std::cout << "Try: zepra://settings, zepra://bookmarks" << std::endl;
    std::cout << "Or type a search query!" << std::endl;
    std::cout << "Ctrl+T = New Tab, Ctrl+W = Close Tab" << std::endl;
    std::cout << "ESC = Exit\n" << std::endl;
    
    bool running = true;
    SDL_Event event;
    int currentWidth = WINDOW_WIDTH;
    int currentHeight = WINDOW_HEIGHT;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                    
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else {
                        bool shift = event.key.keysym.mod & KMOD_SHIFT;
                        bool ctrl = event.key.keysym.mod & KMOD_CTRL;
                        bool alt = event.key.keysym.mod & KMOD_ALT;
                        
                        if (ctrl && event.key.keysym.sym == SDLK_t) {
                            browser.addTab();
                            chrome.tabBar()->addTab(browser.currentTab()->title, browser.currentTab()->url);
                            chrome.tabBar()->setActiveTab(browser.activeTab);
                            chrome.navigationBar()->setURL(browser.currentTab()->url);
                        } else if (ctrl && event.key.keysym.sym == SDLK_w) {
                            if (browser.tabs.size() > 1) {
                                int idx = browser.activeTab;
                                browser.closeTab(idx);
                                chrome.tabBar()->removeTab(idx);
                                chrome.tabBar()->setActiveTab(browser.activeTab);
                                if (auto* tab = browser.currentTab()) {
                                    chrome.navigationBar()->setURL(tab->url);
                                }
                            }
                        } else if (event.key.keysym.sym == SDLK_F12) {
                            if (auto* tab = browser.currentTab()) {
                                if (tab->page->renderTree()) {
                                    std::cout << "\n=== Render Tree Dump ===\n";
                                    tab->page->renderTree()->dump();
                                    std::cout << "========================\n";
                                }
                            }
                        } else {
                            chrome.handleKeyDown(event.key.keysym.sym, shift, ctrl, alt);
                        }
                    }
                    break;
                    
                case SDL_TEXTINPUT:
                    chrome.handleTextInput(event.text.text);
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    chrome.handleMouseDown(
                        static_cast<float>(event.button.x),
                        static_cast<float>(event.button.y),
                        event.button.button
                    );
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    chrome.handleMouseUp(
                        static_cast<float>(event.button.x),
                        static_cast<float>(event.button.y),
                        event.button.button
                    );
                    
                    // Dispatch Click to DOM
                    if (auto* tab = browser.currentTab()) {
                        if (auto* renderNode = tab->page->hitTest(event.button.x, event.button.y)) {
                            if (auto* domNode = renderNode->domNode()) {
                                std::cout << "Hit DOM Node: " << domNode->nodeName() << "\n";
                                WebCore::MouseEvent clickEvt("click", event.button.x, event.button.y, event.button.button);
                                domNode->dispatchEvent(clickEvt);
                            }
                        }
                    }
                    break;
                    
                case SDL_MOUSEMOTION:
                    chrome.handleMouseMove(
                        static_cast<float>(event.motion.x),
                        static_cast<float>(event.motion.y)
                    );
                    break;
                    
                case SDL_MOUSEWHEEL:
                    if (auto* tab = browser.currentTab()) {
                        tab->page->scrollBy(0, -event.wheel.y * 40);
                    }
                    break;
                    
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        currentWidth = event.window.data1;
                        currentHeight = event.window.data2;
                        chrome.resize(currentWidth, currentHeight);
                    }
                    break;
            }
        }
        
        // Clear with white background
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        // Paint browser chrome
        WebCore::DisplayList chromeList;
        chrome.paint(chromeList);
        sdlRenderer.render(chromeList);
        
        // Paint current tab
        if (auto* tab = browser.currentTab()) {
            WebCore::Rect contentArea = chrome.contentArea();
            
            SDL_Rect clipRect = {
                static_cast<int>(contentArea.x),
                static_cast<int>(contentArea.y),
                static_cast<int>(contentArea.width),
                static_cast<int>(contentArea.height)
            };
            SDL_RenderSetClipRect(renderer, &clipRect);
            
            tab->page->setViewport(contentArea.width, contentArea.height);
            tab->page->layout();
            
            WebCore::DisplayList pageList;
            if (tab->page->renderTree()) {
                auto& box = const_cast<WebCore::BoxModel&>(tab->page->renderTree()->boxModel());
                float origX = box.contentBox.x;
                float origY = box.contentBox.y;
                box.contentBox.x = contentArea.x - tab->page->scrollX();
                box.contentBox.y = contentArea.y - tab->page->scrollY();
                
                WebCore::PaintContext ctx(pageList);
                tab->page->renderTree()->paint(ctx);
                
                if (autoDump) {
                     std::cout << "\n=== Auto Dump ===\n";
                     tab->page->renderTree()->dump();
                     running = false;
                }
                
                box.contentBox.x = origX;
                box.contentBox.y = origY;
            }
            
            sdlRenderer.render(pageList);
            SDL_RenderSetClipRect(renderer, nullptr);
        }
        
        SDL_RenderPresent(renderer);
    }
    
    SDL_StopTextInput();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    FontManager::instance().shutdown();
    SDL_Quit();
    
    std::cout << "\n=== Zepra Browser Closed ===" << std::endl;
    
    return 0;
}
