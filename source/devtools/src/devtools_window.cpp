/**
 * @file devtools_window.cpp
 * @brief Zepra DevTools Window - Standalone Implementation
 * 
 * Uses callbacks to connect to engine rather than direct VM access.
 */

// Standalone build - no internal headers

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <string>
#include <vector>

namespace Zepra::DevTools {

// Theme colors
namespace Colors {
    constexpr SDL_Color BG = {18, 15, 28, 255};
    constexpr SDL_Color HEADER = {28, 24, 42, 255};
    constexpr SDL_Color PANEL = {22, 18, 35, 255};
    constexpr SDL_Color TAB = {30, 26, 48, 255};
    constexpr SDL_Color TAB_ACTIVE = {45, 40, 70, 255};
    constexpr SDL_Color TEXT = {240, 240, 250, 255};
    constexpr SDL_Color TEXT_DIM = {145, 140, 170, 255};
    constexpr SDL_Color ACCENT_CYAN = {0, 230, 255, 255};
    constexpr SDL_Color STATUS_OK = {80, 230, 140, 255};
    constexpr SDL_Color STATUS_ERROR = {255, 100, 100, 255};
}

namespace Layout {
    constexpr int HEADER_HEIGHT = 42;
    constexpr int TAB_HEIGHT = 36;
    constexpr int SIDEBAR_WIDTH = 52;
    constexpr int STATUS_HEIGHT = 22;
    constexpr int DEFAULT_WIDTH = 1280;
    constexpr int DEFAULT_HEIGHT = 800;
}

// PanelType enum - local to this file
enum class PanelType {
    Elements = 0, Console, Network, Sources,
    Performance, Application, Security, Settings, COUNT
};

class DevToolsWindowImpl {
public:
    DevToolsWindowImpl() = default;
    ~DevToolsWindowImpl() { shutdown(); }
    
    bool initialize() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
        if (TTF_Init() < 0) return false;
        
        window_ = SDL_CreateWindow("Zepra DevTools",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            Layout::DEFAULT_WIDTH, Layout::DEFAULT_HEIGHT,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!window_) return false;
        
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer_) return false;
        
        loadFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 13);
        return true;
    }
    
    void run() {
        running_ = true;
        while (running_) {
            handleEvents();
            render();
            SDL_Delay(16);
        }
    }
    
    void shutdown() {
        if (font_) { TTF_CloseFont(font_); font_ = nullptr; }
        if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
        if (window_) { SDL_DestroyWindow(window_); window_ = nullptr; }
        TTF_Quit();
        SDL_Quit();
    }
    
private:
    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running_ = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running_ = false;
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
                width_ = e.window.data1;
                height_ = e.window.data2;
            }
        }
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer_, Colors::BG.r, Colors::BG.g, Colors::BG.b, 255);
        SDL_RenderClear(renderer_);
        renderHeader();
        renderTabs();
        renderSidebar();
        renderMainPanel();
        renderStatusBar();
        SDL_RenderPresent(renderer_);
    }
    
    void renderHeader() {
        drawRect(0, 0, width_, Layout::HEADER_HEIGHT, Colors::HEADER);
        drawText("ZEPRA DEVTOOLS", 15, 12, Colors::ACCENT_CYAN);
        drawCircle(width_ - 120, 20, 5, connected_ ? Colors::STATUS_OK : Colors::STATUS_ERROR);
        drawText(connected_ ? "Connected" : "Standalone", width_ - 105, 12, Colors::TEXT_DIM);
    }
    
    void renderTabs() {
        drawRect(Layout::SIDEBAR_WIDTH, Layout::HEADER_HEIGHT,
                 width_ - Layout::SIDEBAR_WIDTH, Layout::TAB_HEIGHT, Colors::TAB);
        const char* names[] = {"Elements", "Console", "Network", "Sources", 
                               "Performance", "Application", "Security", "Settings"};
        int x = Layout::SIDEBAR_WIDTH + 10;
        for (int i = 0; i < 8; ++i) {
            bool active = (currentPanel_ == static_cast<PanelType>(i));
            if (active) {
                drawRect(x, Layout::HEADER_HEIGHT + 4, 85, Layout::TAB_HEIGHT - 6, Colors::TAB_ACTIVE);
            }
            drawText(names[i], x + 8, Layout::HEADER_HEIGHT + 10, active ? Colors::TEXT : Colors::TEXT_DIM);
            x += 95;
        }
    }
    
    void renderSidebar() {
        int y = Layout::HEADER_HEIGHT;
        int h = height_ - y - Layout::STATUS_HEIGHT;
        drawRect(0, y, Layout::SIDEBAR_WIDTH, h, Colors::HEADER);
        const char* icons[] = {"[E]", "[C]", "[N]", "[S]", "[P]", "[A]", "[+]", "[*]"};
        int itemY = y + Layout::TAB_HEIGHT + 10;
        for (int i = 0; i < 8; ++i) {
            bool active = (currentPanel_ == static_cast<PanelType>(i));
            if (active) {
                drawRect(0, itemY - 2, Layout::SIDEBAR_WIDTH, 36, Colors::TAB_ACTIVE);
            }
            drawText(icons[i], 14, itemY + 8, active ? Colors::ACCENT_CYAN : Colors::TEXT_DIM);
            itemY += 42;
        }
    }
    
    void renderMainPanel() {
        int x = Layout::SIDEBAR_WIDTH;
        int y = Layout::HEADER_HEIGHT + Layout::TAB_HEIGHT;
        drawRect(x, y, width_ - x, height_ - y - Layout::STATUS_HEIGHT, Colors::PANEL);
        
        const char* panelNames[] = {"Elements", "Console", "Network", "Sources", 
                                    "Performance", "Application", "Security", "Settings"};
        int idx = static_cast<int>(currentPanel_);
        drawText(panelNames[idx], x + 20, y + 20, Colors::TEXT);
        drawText("Panel content - connect to browser for live data", x + 20, y + 50, Colors::TEXT_DIM);
    }
    
    void renderStatusBar() {
        int y = height_ - Layout::STATUS_HEIGHT;
        drawRect(0, y, width_, Layout::STATUS_HEIGHT, Colors::HEADER);
        drawText("Standalone Mode | 1-8: Switch panels | ESC: Close", 10, y + 4, Colors::TEXT_DIM);
    }
    
    void drawRect(int x, int y, int w, int h, SDL_Color c) {
        SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
        SDL_Rect r = {x, y, w, h};
        SDL_RenderFillRect(renderer_, &r);
    }
    
    void drawText(const std::string& text, int x, int y, SDL_Color c) {
        if (!font_ || text.empty()) return;
        SDL_Surface* s = TTF_RenderUTF8_Blended(font_, text.c_str(), c);
        if (!s) return;
        SDL_Texture* t = SDL_CreateTextureFromSurface(renderer_, s);
        SDL_Rect dst = {x, y, s->w, s->h};
        SDL_RenderCopy(renderer_, t, nullptr, &dst);
        SDL_DestroyTexture(t);
        SDL_FreeSurface(s);
    }
    
    void drawCircle(int cx, int cy, int r, SDL_Color c) {
        SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (dx*dx + dy*dy <= r*r)
                    SDL_RenderDrawPoint(renderer_, cx + dx, cy + dy);
    }
    
    void loadFont(const char* path, int size) {
        font_ = TTF_OpenFont(path, size);
    }
    
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* font_ = nullptr;
    int width_ = Layout::DEFAULT_WIDTH;
    int height_ = Layout::DEFAULT_HEIGHT;
    bool running_ = false;
    bool connected_ = false;
    PanelType currentPanel_ = PanelType::Console;
};

} // namespace Zepra::DevTools

int main(int argc, char* argv[]) {
    std::cout << "Zepra DevTools - Standalone Mode" << std::endl;
    
    Zepra::DevTools::DevToolsWindowImpl window;
    if (!window.initialize()) {
        std::cerr << "Failed to initialize DevTools" << std::endl;
        return 1;
    }
    window.run();
    return 0;
}
