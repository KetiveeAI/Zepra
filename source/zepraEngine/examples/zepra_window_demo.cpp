/**
 * @file zepra_window_demo.cpp
 * @brief Zepra Browser window demo using SDL2 (no TTF)
 */

#include <SDL2/SDL.h>
#include <iostream>

// Colors
const SDL_Color BG_COLOR = {30, 30, 35, 255};
const SDL_Color TOOLBAR_BG = {45, 45, 50, 255};
const SDL_Color URL_BAR_BG = {60, 60, 65, 255};
const SDL_Color ACCENT_COLOR = {100, 150, 255, 255};
const SDL_Color CONTENT_BG = {255, 255, 255, 255};
const SDL_Color TAB_COLOR = {50, 50, 55, 255};
const SDL_Color TAB_ACTIVE = {70, 70, 75, 255};

class ZepraBrowser {
public:
    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        window = SDL_CreateWindow(
            "Zepra Browser - Powered by ZepraScript",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1200, 800,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        
        if (!window) return false;
        
        renderer = SDL_CreateRenderer(window, -1, 
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        
        return renderer != nullptr;
    }
    
    void run() {
        bool running = true;
        SDL_Event event;
        
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
            }
            render();
            SDL_Delay(16);
        }
        
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    
private:
    void drawRect(int x, int y, int w, int h, SDL_Color c) {
        SDL_Rect r = {x, y, w, h};
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        SDL_RenderFillRect(renderer, &r);
    }
    
    void drawButton(int x, int y, int w, int h) {
        drawRect(x, y, w, h, URL_BAR_BG);
        // Draw icon placeholder
        SDL_Rect icon = {x + w/2 - 5, y + h/2 - 3, 10, 6};
        SDL_SetRenderDrawColor(renderer, 180, 180, 185, 255);
        SDL_RenderFillRect(renderer, &icon);
    }
    
    void render() {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        
        // Background
        SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, 255);
        SDL_RenderClear(renderer);
        
        // Toolbar
        drawRect(0, 0, w, 50, TOOLBAR_BG);
        
        // Nav buttons (back, forward, refresh, home)
        int btnX = 10;
        for (int i = 0; i < 4; i++) {
            drawButton(btnX, 10, 30, 30);
            btnX += 35;
        }
        
        // URL bar
        drawRect(btnX + 10, 10, w - btnX - 60, 30, URL_BAR_BG);
        // URL text placeholder
        drawRect(btnX + 20, 18, 200, 14, {100, 100, 105, 255});
        
        // Menu button
        drawButton(w - 40, 10, 30, 30);
        
        // Tab bar
        drawRect(0, 50, w, 35, TAB_COLOR);
        
        // Active tab
        drawRect(5, 55, 180, 28, TAB_ACTIVE);
        // Tab title placeholder
        drawRect(15, 65, 80, 8, {150, 150, 155, 255});
        // Close button
        drawRect(165, 63, 12, 12, {180, 80, 80, 255});
        
        // New tab button
        drawRect(190, 57, 24, 24, URL_BAR_BG);
        // Plus icon
        drawRect(199, 65, 6, 2, {150, 150, 155, 255});
        drawRect(201, 63, 2, 6, {150, 150, 155, 255});
        
        // Content area (white page)
        drawRect(0, 85, w, h - 115, CONTENT_BG);
        
        // "Zepra" logo placeholder
        int logoW = 200, logoH = 50;
        drawRect(w/2 - logoW/2, 140, logoW, logoH, ACCENT_COLOR);
        
        // Search box
        SDL_Rect searchBox = {w/2 - 280, 220, 560, 50};
        SDL_SetRenderDrawColor(renderer, 245, 245, 248, 255);
        SDL_RenderFillRect(renderer, &searchBox);
        SDL_SetRenderDrawColor(renderer, 200, 200, 205, 255);
        SDL_RenderDrawRect(renderer, &searchBox);
        
        // Search placeholder text
        drawRect(w/2 - 80, 238, 160, 14, {200, 200, 205, 255});
        
        // Search icon
        drawRect(w/2 - 260, 235, 20, 20, {150, 150, 155, 255});
        
        // Quick links row
        int linkX = w/2 - 200;
        for (int i = 0; i < 4; i++) {
            // Link icon
            drawRect(linkX, 300, 80, 80, {240, 240, 245, 255});
            drawRect(linkX + 25, 320, 30, 30, {200, 200, 210, 255});
            // Link title
            drawRect(linkX + 20, 365, 40, 8, {180, 180, 185, 255});
            linkX += 100;
        }
        
        // Status bar
        drawRect(0, h - 30, w, 30, TOOLBAR_BG);
        // Status text placeholder
        drawRect(10, h - 22, 300, 12, {80, 80, 85, 255});
        
        SDL_RenderPresent(renderer);
    }
    
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
};

int main(int, char**) {
    ZepraBrowser browser;
    if (browser.init()) {
        browser.run();
    }
    return 0;
}
