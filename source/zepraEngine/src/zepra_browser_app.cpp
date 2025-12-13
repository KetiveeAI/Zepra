/**
 * @file zepra_browser_app.cpp
 * @brief Main Zepra Browser Application
 */

#include "webcore/sdl_gl_context.hpp"
#include "webcore/browser_ui.hpp"
#include "webcore/page.hpp"
#include "webcore/shortcut_manager.hpp"
#include "webcore/context_menu.hpp"
#include "webcore/simple_font.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>

using namespace Zepra::WebCore;

class BrowserApp {
public:
    BrowserApp() : chrome_(1024, 768), zoomLevel_(1.0f) {}
    
    bool init() {
        if (!context_.initialize("Zepra Browser", 1024, 768, RenderMode::Auto)) {
            std::cerr << "Failed to initialize display context" << std::endl;
            return false;
        }
        
        std::cout << "Context initialized, setting up shortcuts..." << std::endl;
        setupShortcuts();
        std::cout << "Shortcuts set up, setting up UI events..." << std::endl;
        setupUIEvents();
        std::cout << "UI events set up, creating initial tab..." << std::endl;
        
        newTab("Welcome", "zepra://start");
        
        std::cout << "Init complete, entering event loop" << std::endl;
        return true;
    }
    
    void run() {
        bool running = true;
        SDL_Event event;
        int frameCount = 0;
        
        shortcuts_.registerShortcut(SDLK_q, KMOD_CTRL, "Quit", [&]() { 
            std::cout << "Quit shortcut triggered" << std::endl;
            running = false; 
        });
        
        std::cout << "Starting event loop..." << std::endl;

        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    std::cout << "SDL_QUIT received" << std::endl;
                    running = false;
                }
                handleEvent(event);
            }
            update();
            render();
            
            frameCount++;
            if (frameCount == 1) {
                std::cout << "First frame rendered!" << std::endl;
            }
            if (frameCount == 10) {
                std::cout << "10 frames rendered, browser running normally..." << std::endl;
            }
            
            SDL_Delay(16);
        }
        std::cout << "Event loop ended after " << frameCount << " frames" << std::endl;
    }

private:
    void setupShortcuts() {
        shortcuts_.registerShortcut(SDLK_t, KMOD_CTRL, "New Tab", [this]() {
            newTab("New Tab", "about:blank");
        });
        
        shortcuts_.registerShortcut(SDLK_w, KMOD_CTRL, "Close Tab", [this]() {
            closeTab(chrome_.tabBar()->activeTab());
        });
         
        shortcuts_.registerShortcut(SDLK_TAB, KMOD_CTRL, "Next Tab", [this]() {
            int count = chrome_.tabBar()->tabCount();
            if (count > 0) {
                int next = (chrome_.tabBar()->activeTab() + 1) % count;
                chrome_.tabBar()->setActiveTab(next);
            }
        });
        
        shortcuts_.registerShortcut(SDLK_l, KMOD_CTRL, "Focus Address", [this]() {
            chrome_.navigationBar()->addressBar()->setFocused(true);
        });
        
        shortcuts_.registerShortcut(SDLK_r, KMOD_CTRL, "Refresh", [this]() {
            if (activePage_) activePage_->loadFromURL(activePage_->url());
        });

        shortcuts_.registerShortcut(SDLK_h, KMOD_ALT, "Home", [this]() {
             navigate("zepra://start");
        });
        
        shortcuts_.registerShortcut(SDLK_F12, KMOD_NONE, "DevTools", [this]() {
            std::cout << "DevTools toggled (F12)" << std::endl;
        });
        
        // Zoom shortcuts
        shortcuts_.registerShortcut(SDLK_PLUS, KMOD_CTRL, "Zoom In", [this]() { zoomIn(); });
        shortcuts_.registerShortcut(SDLK_EQUALS, KMOD_CTRL, "Zoom In", [this]() { zoomIn(); });
        shortcuts_.registerShortcut(SDLK_MINUS, KMOD_CTRL, "Zoom Out", [this]() { zoomOut(); });
        shortcuts_.registerShortcut(SDLK_0, KMOD_CTRL, "Reset Zoom", [this]() { resetZoom(); });
        
        // View Source
        shortcuts_.registerShortcut(SDLK_u, KMOD_CTRL, "View Source", [this]() {
            std::cout << "View Source (Ctrl+U)" << std::endl;
        });
    }
    
    void setupUIEvents() {
        auto* nav = chrome_.navigationBar();
        auto* tabs = chrome_.tabBar();
        auto* sidebar = chrome_.sidebar();
        
        nav->setOnNavigate([this](const std::string& url) { navigate(url); });
        nav->setOnBack([this]() { std::cout << "Back clicked" << std::endl; });
        nav->setOnForward([this]() { std::cout << "Forward clicked" << std::endl; });
        nav->setOnRefresh([this]() { if (activePage_) activePage_->loadFromURL(activePage_->url()); });
        nav->setOnHome([this]() { navigate("zepra://start"); });
        nav->setOnMenu([this]() { std::cout << "Menu clicked" << std::endl; });
        
        tabs->setOnTabSwitch([this](int index) { switchTab(index); });
        tabs->setOnNewTab([this]() { newTab("New Tab", "about:blank"); });
        tabs->setOnCloseTab([this](int index) { closeTab(index); });
        
        // Sidebar click handlers
        sidebar->setOnItemClick([this](const std::string& id) {
            std::cout << "Sidebar: " << id << " clicked" << std::endl;
            if (id == "home") {
                navigate("zepra://start");
            } else if (id == "settings") {
                navigate("zepra://settings");
            } else if (id == "search") {
                chrome_.navigationBar()->addressBar()->setFocused(true);
            } else if (id == "calendar") {
                navigate("zepra://calendar");
            } else if (id == "lab") {
                navigate("zepra://lab");
            }
        });
    }
    
    void handleEvent(const SDL_Event& event) {
        // Context menu takes priority
        if (contextMenu_.isVisible()) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (contextMenu_.handleMouseDown(event.button.x, event.button.y, event.button.button)) {
                    return;
                }
            }
            if (event.type == SDL_MOUSEMOTION) {
                contextMenu_.handleMouseMove(event.motion.x, event.motion.y);
            }
        }
        
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            // Right-click opens context menu
            if (event.button.button == SDL_BUTTON_RIGHT) {
                showContextMenu(event.button.x, event.button.y);
                return;
            }
            
            chrome_.handleMouseDown(event.button.x, event.button.y, event.button.button);
            if (activePage_) {
                auto area = chrome_.contentArea();
                if (event.button.y > area.y) {
                    activePage_->hitTest(event.button.x, event.button.y);
                }
            }
        }
        else if (event.type == SDL_MOUSEWHEEL) {
            // Ctrl+Scroll = Zoom
            if (SDL_GetModState() & KMOD_CTRL) {
                if (event.wheel.y > 0) {
                    zoomIn();
                } else if (event.wheel.y < 0) {
                    zoomOut();
                }
            } else {
                // Normal scroll
                if (activePage_) {
                    // Page scrolling - adjust Y scroll offset
                    int scrollAmount = event.wheel.y * 40;
                    std::cout << "Scroll: " << scrollAmount << "px" << std::endl;
                }
            }
        } 
        else if (event.type == SDL_MOUSEBUTTONUP) {
            chrome_.handleMouseUp(event.button.x, event.button.y, event.button.button);
        }
        else if (event.type == SDL_MOUSEMOTION) {
            chrome_.handleMouseMove(event.motion.x, event.motion.y);
        }
        else if (event.type == SDL_KEYDOWN) {
            if (shortcuts_.handleKeyDown(event.key.keysym.sym, event.key.keysym.mod)) return;
            
            bool shift = event.key.keysym.mod & KMOD_SHIFT;
            bool ctrl = event.key.keysym.mod & KMOD_CTRL;
            bool alt = event.key.keysym.mod & KMOD_ALT;
            
            chrome_.handleKeyDown(event.key.keysym.sym, shift, ctrl, alt);
        }
        else if (event.type == SDL_TEXTINPUT) {
            chrome_.handleTextInput(event.text.text);
        }
        else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            resize(event.window.data1, event.window.data2);
        }
    }
    
    void update() {}
    
    void render() {
        context_.beginFrame();
        
        DisplayList displayList;
        chrome_.paint(displayList);
        
        if (activePage_) {
            Rect area = chrome_.contentArea();
            PaintContext ctx(displayList);
            ctx.pushClip(area);
            
            // Check if this is the home page (zepra://start or zepra://newtab)
            if (activePage_->url() == "zepra://start" || activePage_->url() == "zepra://newtab") {
                // === FIGMA DESIGN: Gradient Background (pink → purple → blue) ===
                // Draw gradient in horizontal bands
                int numBands = 50;
                float bandHeight = area.height / numBands;
                
                for (int i = 0; i < numBands; i++) {
                    float t = static_cast<float>(i) / numBands;
                    Color bandColor;
                    
                    if (t < 0.5f) {
                        // Top to middle (pink → purple)
                        float localT = t * 2.0f;
                        bandColor.r = 218 + static_cast<uint8_t>((201 - 218) * localT);  // #DAADC1 → #C9A4D5
                        bandColor.g = 173 + static_cast<uint8_t>((164 - 173) * localT);
                        bandColor.b = 193 + static_cast<uint8_t>((213 - 193) * localT);
                        bandColor.a = 255;
                    } else {
                        // Middle to bottom (purple → blue)
                        float localT = (t - 0.5f) * 2.0f;
                        bandColor.r = 201 + static_cast<uint8_t>((145 - 201) * localT);  // #C9A4D5 → #9191D5
                        bandColor.g = 164 + static_cast<uint8_t>((145 - 164) * localT);
                        bandColor.b = 213 + static_cast<uint8_t>((213 - 213) * localT);
                        bandColor.a = 255;
                    }
                    
                    Rect bandRect = {area.x, area.y + i * bandHeight, area.width, bandHeight + 1};
                    ctx.fillRect(bandRect, bandColor);
                }
                
                // === FIGMA DESIGN: Centered Zepra "Z" Logo ===
                // Draw stylized Z logo in center-upper area
                float logoSize = 80.0f;
                float logoX = area.x + (area.width - logoSize) / 2;
                float logoY = area.y + area.height * 0.35f - logoSize / 2;
                
                // Logo background (white with slight transparency)
                Color logoColor = {255, 255, 255, 220};
                
                // Draw "Z" letter (simplified version - full SVG would be better)
                float zWidth = logoSize * 0.7f;
                float zHeight = logoSize * 0.8f;
                float cx = logoX + logoSize / 2;
                float cy = logoY + logoSize / 2;
                
                // Top bar of Z
                ctx.fillRect({cx - zWidth/2, cy - zHeight/2, zWidth, 8}, logoColor);
                // Diagonal of Z
                for (int d = 0; d < 8; d++) {
                    float dx = (static_cast<float>(d) / 8.0f) * zWidth;
                    float dy = (static_cast<float>(d) / 8.0f) * (zHeight - 16);
                    ctx.fillRect({cx - zWidth/2 + dx - 4, cy - zHeight/2 + 8 + dy, 12, 8}, logoColor);
                }
                // Bottom bar of Z
                ctx.fillRect({cx - zWidth/2, cy + zHeight/2 - 8, zWidth, 8}, logoColor);
                
                // === GLASSMORPHISM: AI Prompt Bar with frosted glass effect ===
                // Centered glass rounded bar (matching provided SVG: #C2C2C2 @ 70%)
                float barWidth = 500.0f;
                float barHeight = 50.0f;
                float barX = area.x + (area.width - barWidth) / 2;
                float barY = area.y + area.height * 0.6f;
                
                // Glass background (frosted effect)
                ctx.fillRect({barX, barY, barWidth, barHeight}, {194, 194, 194, 178});  // #C2C2C2 @ 70%
                
                // Top highlight (simulates light reflection)
                ctx.fillRect({barX, barY, barWidth, 2}, {255, 255, 255, 100});
                
                // Subtle inner glow
                ctx.fillRect({barX + 1, barY + 1, barWidth - 2, 1}, {255, 255, 255, 60});
                
                // Left icon area (clipboard icon placeholder)
                float iconSize = 24.0f;
                float iconY = barY + (barHeight - iconSize) / 2;
                ctx.fillRect({barX + 15, iconY, iconSize, iconSize}, {255, 255, 255, 150});
                
                // Second icon (sparkle/star placeholder)
                ctx.fillRect({barX + 50, iconY, iconSize, iconSize}, {255, 255, 255, 150});
                
                // Prompt text (white with opacity like in SVG)
                std::string promptText = "Let's give your dream life. What you create today?..";
                float promptWidth = SimpleFont::getTextWidth(promptText, 14);
                float promptX = barX + (barWidth - promptWidth) / 2;
                float promptY = barY + (barHeight - 14) / 2;
                SimpleFont::drawText(ctx, promptText, promptX, promptY, {255, 255, 255, 230}, 14);
                
                // Right side icons (voice and another icon)
                ctx.fillRect({barX + barWidth - 70, iconY, iconSize, iconSize}, {255, 255, 255, 150});
                ctx.fillRect({barX + barWidth - 40, iconY, iconSize, iconSize}, {255, 255, 255, 150});
                
            } else {
                // Normal page rendering
                ctx.fillRect(area, {255, 255, 255, 255});
                activePage_->paint(displayList);
            }
            
            ctx.popClip();
        }
        
        if (context_.renderBackend()) {
            context_.renderBackend()->executeDisplayList(displayList);
        }
        context_.endFrame();
    }
    
    void resize(int width, int height) {
        context_.resize(width, height);
        chrome_.resize(width, height);
        if (activePage_) {
            auto area = chrome_.contentArea();
            activePage_->setViewport(area.width, area.height);
        }
    }
    
    void newTab(const std::string& title, const std::string& url) {
        std::cout << "Creating new tab: " << title << " -> " << url << std::endl;
        auto page = std::make_unique<Page>();
        std::cout << "Page created, setting viewport..." << std::endl;
        page->setViewport(chrome_.contentArea().width, chrome_.contentArea().height);
        std::cout << "Viewport set, loading URL..." << std::endl;
        // Use loadFromURL() for all URLs - ResourceLoader handles zepra:// URLs
        page->loadFromURL(url);
        std::cout << "Page loaded, adding tab..." << std::endl;
        pages_.push_back(std::move(page));
        activePage_ = pages_.back().get();
        int index = chrome_.tabBar()->addTab(title, url);
        chrome_.tabBar()->setActiveTab(index);
        std::cout << "Tab created successfully" << std::endl;
    }
    
    void closeTab(int index) {
        if (index < 0 || index >= static_cast<int>(pages_.size())) return;
        
        pages_.erase(pages_.begin() + index);
        chrome_.tabBar()->removeTab(index);
        
        int count = pages_.size();
        if (count == 0) {
            newTab("New Tab", "about:blank");
        } else {
            int newIndex = std::min(index, count - 1);
            chrome_.tabBar()->setActiveTab(newIndex);
        }
    }
    
    void switchTab(int index) {
        if (index < 0 || index >= static_cast<int>(pages_.size())) {
            activePage_ = nullptr;
            return;
        }
        activePage_ = pages_[index].get();
        if (activePage_) chrome_.navigationBar()->setURL(activePage_->url());
    }
    
    void navigate(const std::string& url) {
        if (!activePage_) return;
        
        chrome_.navigationBar()->setLoading(true);
        chrome_.navigationBar()->setURL(url);
        
        // Use loadFromURL() for all URLs - ResourceLoader handles zepra:// URLs
        activePage_->loadFromURL(url);
        
        chrome_.navigationBar()->setLoading(false);
    }
    
    // ---- End of navigate() - inline HTML removed, now uses ResourceLoader ----
    void showContextMenu(int x, int y) {
        contextMenu_.setHasSelection(false);  // TODO: get from page
        contextMenu_.setCanGoBack(false);     // TODO: get from navigation
        contextMenu_.setCanGoForward(false);
        contextMenu_.setPageUrl(activePage_ ? activePage_->url() : "");
        
        contextMenu_.setOnCopy([this]() {
            std::cout << "Copy action" << std::endl;
        });
        
        contextMenu_.setOnPaste([this]() {
            std::cout << "Paste action" << std::endl;
        });
        
        contextMenu_.setOnSearch([this](const std::string& text) {
            navigate("https://ketivee.com/search?q=" + text);
        });
        
        contextMenu_.setOnViewSource([this]() {
            std::cout << "View Source" << std::endl;
        });
        
        contextMenu_.setOnInspect([this]() {
            std::cout << "Inspect Element" << std::endl;
        });
        
        contextMenu_.setOnReload([this]() {
            if (activePage_) activePage_->loadFromURL(activePage_->url());
        });
        
        contextMenu_.show(x, y);
    }
    
    void zoomIn() {
        zoomLevel_ = std::min(zoomLevel_ + 0.1f, 3.0f);
        applyZoom();
    }
    
    void zoomOut() {
        zoomLevel_ = std::max(zoomLevel_ - 0.1f, 0.25f);
        applyZoom();
    }
    
    void resetZoom() {
        zoomLevel_ = 1.0f;
        applyZoom();
    }
    
    void applyZoom() {
        std::cout << "Zoom: " << (int)(zoomLevel_ * 100) << "%" << std::endl;
        // TODO: Apply zoom to page rendering
    }
    
    SDLGLContext context_;
    BrowserChrome chrome_;
    ShortcutManager shortcuts_;
    ContextMenu contextMenu_;
    std::vector<std::unique_ptr<Page>> pages_;
    Page* activePage_ = nullptr;
    float zoomLevel_ = 1.0f;
};

int main(int argc, char** argv) {
    BrowserApp app;
    if (app.init()) app.run();
    SDL_Quit();
    return 0;
}
