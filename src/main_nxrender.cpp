// ZepraBrowser - Production WebCore Integration
// Full integration with HTML/CSS/JS engine and text rendering

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <unordered_map>

// WebCore includes - MUST come BEFORE X11 to avoid Display/Color conflicts
#include "webcore/page.hpp"
#include "webcore/html_parser.hpp"
#include "webcore/paint_context.hpp"
#include "webcore/render_tree.hpp"

// Namespace alias for WebCore (before X11 pollutes namespace)
namespace wc = Zepra::WebCore;

// X11/OpenGL - after WebCore to avoid macro conflicts
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>

// FreeType for text rendering
#include <ft2build.h>
#include FT_FREETYPE_H

#include "nxrender.h"

// Browser includes
#include "sandbox/sandbox_manager.h"
#include "platform/platform_infrastructure.h"
#include "webcore/navigation_history.hpp"


// ============================================================================
// Font/Text Rendering System
// ============================================================================

struct CharGlyph {
    unsigned int textureID;
    int width, height;
    int bearingX, bearingY;
    int advance;
};

class TextRenderer {
public:
    bool init() {
        if (FT_Init_FreeType(&ft_)) {
            std::cerr << "FreeType init failed" << std::endl;
            return false;
        }
        
        // Try common font paths
        const char* fontPaths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
            "/usr/share/fonts/google-noto/NotoSans-Regular.ttf",
            nullptr
        };
        
        for (int i = 0; fontPaths[i]; i++) {
            if (FT_New_Face(ft_, fontPaths[i], 0, &face_) == 0) {
                std::cout << "Loaded font: " << fontPaths[i] << std::endl;
                FT_Set_Pixel_Sizes(face_, 0, 16);
                loadGlyphs();
                return true;
            }
        }
        
        std::cerr << "No fonts found!" << std::endl;
        return false;
    }
    
    void loadGlyphs() {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        for (unsigned char c = 32; c < 128; c++) {
            if (FT_Load_Char(face_, c, FT_LOAD_RENDER)) continue;
            
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
                face_->glyph->bitmap.width, face_->glyph->bitmap.rows,
                0, GL_ALPHA, GL_UNSIGNED_BYTE, face_->glyph->bitmap.buffer);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            glyphs_[c] = {
                texture,
                (int)face_->glyph->bitmap.width,
                (int)face_->glyph->bitmap.rows,
                face_->glyph->bitmap_left,
                face_->glyph->bitmap_top,
                (int)(face_->glyph->advance.x >> 6)
            };
        }
    }
    
    void setSize(int size) {
        if (size != currentSize_) {
            currentSize_ = size;
            FT_Set_Pixel_Sizes(face_, 0, size);
            // Reload glyphs at new size
            for (auto& [c, g] : glyphs_) {
                glDeleteTextures(1, &g.textureID);
            }
            glyphs_.clear();
            loadGlyphs();
        }
    }
    
    void drawText(const std::string& text, float x, float y, 
                  uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4ub(r, g, b, a);
        
        for (char c : text) {
            if (c < 32 || c >= 128) continue;
            auto it = glyphs_.find(c);
            if (it == glyphs_.end()) continue;
            
            const CharGlyph& g = it->second;
            float xpos = x + g.bearingX;
            float ypos = y - g.bearingY;
            float w = g.width;
            float h = g.height;
            
            glBindTexture(GL_TEXTURE_2D, g.textureID);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(xpos, ypos);
            glTexCoord2f(1, 0); glVertex2f(xpos + w, ypos);
            glTexCoord2f(1, 1); glVertex2f(xpos + w, ypos + h);
            glTexCoord2f(0, 1); glVertex2f(xpos, ypos + h);
            glEnd();
            
            x += g.advance;
        }
        
        glDisable(GL_TEXTURE_2D);
    }
    
    int textWidth(const std::string& text) {
        int width = 0;
        for (char c : text) {
            if (c < 32 || c >= 128) continue;
            auto it = glyphs_.find(c);
            if (it != glyphs_.end()) width += it->second.advance;
        }
        return width;
    }
    
    void cleanup() {
        for (auto& [c, g] : glyphs_) {
            glDeleteTextures(1, &g.textureID);
        }
        glyphs_.clear();
        if (face_) FT_Done_Face(face_);
        if (ft_) FT_Done_FreeType(ft_);
    }
    
private:
    FT_Library ft_ = nullptr;
    FT_Face face_ = nullptr;
    std::unordered_map<char, CharGlyph> glyphs_;
    int currentSize_ = 16;
};

// ============================================================================
// Global State
// ============================================================================

Display* g_display = nullptr;
Window g_window = 0;
GLXContext g_glContext = nullptr;
Atom g_wmDeleteMessage;
int g_windowWidth = 1280;
int g_windowHeight = 800;
bool g_running = true;
float g_mouseX = 0, g_mouseY = 0;

TextRenderer g_textRenderer;

// Theme colors (matching mockup)
struct {
    uint8_t bgTop[3] = {50, 30, 80};
    uint8_t bgBottom[3] = {25, 15, 50};
    uint8_t tabBar[4] = {35, 25, 55, 255};
    uint8_t addressBar[4] = {45, 35, 70, 200};
    uint8_t accent[3] = {100, 120, 255};
    uint8_t iconGreen[3] = {80, 200, 120};
    uint8_t textPrimary[3] = {230, 230, 245};
    uint8_t textSecondary[3] = {160, 160, 180};
} colors;

// Tab with WebCore Page
struct Tab {
    std::string title;
    std::string url;
    bool active;
    std::unique_ptr<wc::Page> page;
    wc::NavigationHistory history;
    
    Tab(const std::string& url_) : url(url_), active(false) {
        page = std::make_unique<wc::Page>();
        page->setViewport(1200, 600);
        title = "New Tab";
    }
    
    bool canGoBack() const { return history.canGoBack(); }
    bool canGoForward() const { return history.canGoForward(); }
    
    void goBack() {
        std::string prevUrl = history.goBack();
        if (!prevUrl.empty()) {
            url = prevUrl;
            page->loadFromURL(url);
            title = page->title();
        }
    }
    
    void goForward() {
        std::string nextUrl = history.goForward();
        if (!nextUrl.empty()) {
            url = nextUrl;
            page->loadFromURL(url);
            title = page->title();
        }
    }
    
    void load() {
        if (url == "zepra://start" || url.empty()) {
            page->loadHTML(getStartPage(), "zepra://start");
            title = "New Tab - Zepra";
        } else if (url == "zepra://settings") {
            page->loadHTML(getSettingsPage(), "zepra://settings");
            title = "Settings - Zepra";
        } else if (url.find("://") != std::string::npos) {
            page->loadFromURL(url);
            title = page->title();
        } else {
            // Search
            std::string searchUrl = "https://ketiveesearch.com/search?q=" + url;
            page->loadFromURL(searchUrl);
            url = searchUrl;
            title = "Search: " + url;
        }
        // Push to history
        history.push(url, title);
    }
    
    std::string getStartPage() {
        return R"(<!DOCTYPE html>
<html>
<head><title>New Tab - Zepra</title>
<style>
body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    background: linear-gradient(180deg, #321950 0%, #190a32 100%);
    color: #e0e0f0;
    margin: 0;
    height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
}
.logo { font-size: 80px; color: #6478ff; margin-bottom: 30px; }
.search-box {
    width: 500px; height: 50px;
    background: rgba(60, 50, 90, 0.7);
    border: 1px solid rgba(100, 100, 150, 0.3);
    border-radius: 25px;
    padding: 0 20px;
    font-size: 16px;
    color: white;
}
.quick-links { display: flex; gap: 30px; margin-top: 50px; }
.quick-link {
    width: 50px; height: 50px;
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
}
</style>
</head>
<body>
    <div class="logo">Z</div>
    <input type="text" class="search-box" placeholder="Search or enter URL">
    <div class="quick-links">
        <div class="quick-link" style="background: #5080dc;"></div>
        <div class="quick-link" style="background: #8c64c8;"></div>
        <div class="quick-link" style="background: #c878a0;"></div>
        <div class="quick-link" style="background: #dc6464;"></div>
    </div>
</body>
</html>)";
    }
    
    std::string getSettingsPage() {
        return R"(<!DOCTYPE html>
<html>
<head><title>Settings - Zepra</title></head>
<body style="font-family: sans-serif; background: #1a1025; color: #e0e0f0; padding: 40px;">
    <h1>Settings</h1>
    <h2>General</h2>
    <p>Home page: zepra://start</p>
    <h2>Privacy</h2>
    <p>Block third-party cookies: Enabled</p>
    <p>HTTPS-Only mode: Enabled</p>
    <h2>Search</h2>
    <p>Default search engine: KetiveeSearch</p>
</body>
</html>)";
    }
};

std::vector<Tab> g_tabs;
int g_activeTab = 0;
bool g_devToolsOpen = false;
std::string g_addressBarText = "zepra://start";
bool g_addressBarFocused = false;
bool g_settingsOpen = false;

NxTheme* g_theme = nullptr;
std::unique_ptr<zepra::PlatformInfrastructure> g_platform;
std::unique_ptr<zepra::SandboxManager> g_sandboxManager;

// ============================================================================
// Drawing Helpers
// ============================================================================

void drawGradient(float x, float y, float w, float h, uint8_t* c1, uint8_t* c2) {
    glBegin(GL_QUADS);
    glColor3ub(c1[0], c1[1], c1[2]); glVertex2f(x, y); glVertex2f(x+w, y);
    glColor3ub(c2[0], c2[1], c2[2]); glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}

void drawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    glColor4ub(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x+w, y); glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}

void drawRoundedRect(float x, float y, float w, float h, float rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    glColor4ub(r, g, b, a);
    // Center
    glBegin(GL_QUADS);
    glVertex2f(x+rad, y); glVertex2f(x+w-rad, y); glVertex2f(x+w-rad, y+h); glVertex2f(x+rad, y+h);
    glEnd();
    // Left/Right
    glBegin(GL_QUADS);
    glVertex2f(x, y+rad); glVertex2f(x+rad, y+rad); glVertex2f(x+rad, y+h-rad); glVertex2f(x, y+h-rad);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(x+w-rad, y+rad); glVertex2f(x+w, y+rad); glVertex2f(x+w, y+h-rad); glVertex2f(x+w-rad, y+h-rad);
    glEnd();
    
    // Corners
    int seg = 8;
    float step = 1.57079f / seg;
    for (int c = 0; c < 4; c++) {
        float cx = (c == 0 || c == 3) ? x + rad : x + w - rad;
        float cy = (c < 2) ? y + rad : y + h - rad;
        float start = c * 1.57079f;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= seg; i++) {
            float ang = start + i * step;
            glVertex2f(cx + cos(ang) * rad, cy - sin(ang) * rad);
        }
        glEnd();
    }
}

void drawCircle(float cx, float cy, float rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    glColor4ub(r, g, b, a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int j = 0; j <= 24; j++) {
        float ang = j * 6.28318f / 24;
        glVertex2f(cx + cos(ang) * rad, cy + sin(ang) * rad);
    }
    glEnd();
}

void drawLine(float x1, float y1, float x2, float y2, uint8_t r, uint8_t g, uint8_t b, float w) {
    glColor3ub(r, g, b);
    glLineWidth(w);
    glBegin(GL_LINES);
    glVertex2f(x1, y1); glVertex2f(x2, y2);
    glEnd();
    glLineWidth(1);
}

// ============================================================================
// UI Rendering
// ============================================================================

void renderTabBar() {
    drawRect(0, 40, (float)g_windowWidth, 38, colors.tabBar[0], colors.tabBar[1], colors.tabBar[2], 255);
    
    // Hamburger menu
    drawRoundedRect(12, 48, 28, 24, 6, 50, 40, 70, 200);
    drawLine(18, 54, 34, 54, 180, 180, 200, 2);
    drawLine(18, 60, 34, 60, 180, 180, 200, 2);
    drawLine(18, 66, 34, 66, 180, 180, 200, 2);
    
    float tx = 50;
    for (size_t i = 0; i < g_tabs.size(); i++) {
        float tw = 160, th = 28, ty = 46;
        bool hover = (g_mouseX >= tx && g_mouseX <= tx + tw && g_mouseY >= ty && g_mouseY <= ty + th);
        
        if (g_tabs[i].active) {
            drawRoundedRect(tx, ty, tw, th, 8, 60, 50, 90, 255);
        } else if (hover) {
            drawRoundedRect(tx, ty, tw, th, 8, 50, 40, 75, 200);
        }
        
        // Favicon
        drawCircle(tx + 16, ty + th/2, 6, colors.iconGreen[0], colors.iconGreen[1], colors.iconGreen[2], 255);
        
        // Tab title with text
        g_textRenderer.setSize(13);
        std::string tabTitle = g_tabs[i].title;
        if (tabTitle.length() > 16) tabTitle = tabTitle.substr(0, 14) + "...";
        g_textRenderer.drawText(tabTitle, tx + 28, ty + 18, 
            colors.textPrimary[0], colors.textPrimary[1], colors.textPrimary[2]);
        
        // Close button
        float closeX = tx + tw - 18;
        float closeY = ty + th/2;
        bool closeHover = (g_mouseX >= closeX - 8 && g_mouseX <= closeX + 8 && 
                          g_mouseY >= closeY - 8 && g_mouseY <= closeY + 8);
        if (closeHover) {
            drawCircle(closeX, closeY, 10, 150, 60, 60, 200);
        }
        drawLine(closeX - 4, closeY - 4, closeX + 4, closeY + 4, 200, 200, 220, 2);
        drawLine(closeX + 4, closeY - 4, closeX - 4, closeY + 4, 200, 200, 220, 2);
        
        tx += tw + 8;
    }
    
    // New tab button
    bool newHover = (g_mouseX >= tx && g_mouseX <= tx + 28 && g_mouseY >= 48 && g_mouseY <= 72);
    drawRoundedRect(tx, 48, 28, 24, 6, 50, 40, 70, newHover ? 220 : 150);
    drawLine(tx + 14, 52, tx + 14, 68, 200, 200, 220, 2);
    drawLine(tx + 6, 60, tx + 22, 60, 200, 200, 220, 2);
}

void renderAddressBar() {
    float y = 78, h = 28;
    
    // Back button
    drawRoundedRect(12, y, 28, h, 8, 45, 35, 65, 200);
    drawLine(20, y + h/2, 28, y + h/2 - 6, 180, 180, 200, 2);
    drawLine(20, y + h/2, 28, y + h/2 + 6, 180, 180, 200, 2);
    
    // Forward button
    drawRoundedRect(44, y, 28, h, 8, 45, 35, 65, 200);
    drawLine(64, y + h/2, 56, y + h/2 - 6, 180, 180, 200, 2);
    drawLine(64, y + h/2, 56, y + h/2 + 6, 180, 180, 200, 2);
    
    // URL bar
    float urlX = 80, urlW = g_windowWidth - 140.0f;
    if (g_addressBarFocused) {
        drawRoundedRect(urlX, y, urlW, h, 14, 55, 45, 80, 240);
    } else {
        drawRoundedRect(urlX, y, urlW, h, 14, colors.addressBar[0], colors.addressBar[1], colors.addressBar[2], 200);
    }
    
    // Lock icon
    drawCircle(urlX + 18, y + h/2, 5, colors.iconGreen[0], colors.iconGreen[1], colors.iconGreen[2], 255);
    
    // URL text
    g_textRenderer.setSize(14);
    std::string displayUrl = g_addressBarText;
    if (displayUrl.length() > 60) displayUrl = displayUrl.substr(0, 57) + "...";
    g_textRenderer.drawText(displayUrl, urlX + 32, y + 18, 
        colors.textPrimary[0], colors.textPrimary[1], colors.textPrimary[2]);
    
    // Cursor when focused
    if (g_addressBarFocused) {
        int cursorX = urlX + 32 + g_textRenderer.textWidth(g_addressBarText);
        drawRect(cursorX, y + 6, 2, h - 12, 255, 255, 255, 200);
    }
    
    // Menu button
    float mx = g_windowWidth - 35.0f;
    drawRoundedRect(mx - 5, y, 30, h, 6, 45, 35, 65, 200);
    drawLine(mx, y + 8, mx + 16, y + 8, 180, 180, 200, 2);
    drawLine(mx, y + h/2, mx + 16, y + h/2, 180, 180, 200, 2);
    drawLine(mx, y + h - 8, mx + 16, y + h - 8, 180, 180, 200, 2);
}

// Forward declaration
void renderStartPage(float contentY, float contentH);

void renderPageContent() {
    float contentY = 115;
    float contentH = g_windowHeight - contentY - (g_devToolsOpen ? 220.0f : 0);
    
    if (g_tabs.empty()) return;
    
    Tab& tab = g_tabs[g_activeTab];
    wc::Page* page = tab.page.get();
    
    // Clip to content area
    glEnable(GL_SCISSOR_TEST);
    glScissor(10, (int)(g_windowHeight - contentY - contentH), g_windowWidth - 20, (int)contentH);
    
    // Content background
    drawRect(10, contentY, g_windowWidth - 20.0f, contentH, 25, 20, 45, 255);
    
    if (page && page->renderTree()) {
        // Render the page via DisplayList
        wc::DisplayList displayList;
        page->paint(displayList);
        
        float scrollY = page->scrollY();
        
        for (const auto& cmd : displayList.commands()) {
            float x = cmd.rect.x + 10;
            float y = cmd.rect.y + contentY - scrollY;
            float w = cmd.rect.width;
            float h = cmd.rect.height;
            
            if (y + h < contentY || y > contentY + contentH) continue;
            
            switch (cmd.type) {
                case wc::PaintCommandType::FillRect:
                    drawRect(x, y, w, h, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
                    break;
                    
                case wc::PaintCommandType::FillRoundedRect:
                    drawRoundedRect(x, y, w, h, cmd.radius, 
                        cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
                    break;
                    
                case wc::PaintCommandType::StrokeRect:
                    drawLine(x, y, x + w, y, cmd.color.r, cmd.color.g, cmd.color.b, cmd.number);
                    drawLine(x + w, y, x + w, y + h, cmd.color.r, cmd.color.g, cmd.color.b, cmd.number);
                    drawLine(x + w, y + h, x, y + h, cmd.color.r, cmd.color.g, cmd.color.b, cmd.number);
                    drawLine(x, y + h, x, y, cmd.color.r, cmd.color.g, cmd.color.b, cmd.number);
                    break;
                    
                case wc::PaintCommandType::DrawText:
                    g_textRenderer.setSize((int)cmd.number);
                    g_textRenderer.drawText(cmd.text, x, y + cmd.number,
                        cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
                    break;
                    
                case wc::PaintCommandType::DrawGradient:
                    // Linear gradient
                    {
                        glBegin(GL_QUADS);
                        glColor4ub(cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
                        glVertex2f(x, y); glVertex2f(x+w, y);
                        glColor4ub(cmd.color2.r, cmd.color2.g, cmd.color2.b, cmd.color.a);
                        glVertex2f(x+w, y+h); glVertex2f(x, y+h);
                        glEnd();
                    }
                    break;
                    
                default:
                    break;
            }
        }
    } else {
        // Loading state or start page rendering
        renderStartPage(contentY, contentH);
    }
    
    glDisable(GL_SCISSOR_TEST);
}

void renderStartPage(float contentY, float contentH) {
    float centerX = g_windowWidth / 2.0f;
    float centerY = contentY + contentH / 2.0f - 50;
    
    // Logo background
    float logoSize = 70;
    drawRoundedRect(centerX - logoSize/2, centerY - logoSize, logoSize, logoSize, 15, 50, 40, 80, 180);
    
    // Z letter
    g_textRenderer.setSize(52);
    g_textRenderer.drawText("Z", centerX - 18, centerY - 20, 
        colors.accent[0], colors.accent[1], colors.accent[2]);
    
    // Search bar
    float searchW = 450;
    float searchH = 42;
    float searchX = centerX - searchW/2;
    float searchY = centerY + 20;
    drawRoundedRect(searchX, searchY, searchW, searchH, 21, 55, 45, 85, 220);
    
    // Search icon
    drawCircle(searchX + 25, searchY + searchH/2, 8, 120, 120, 150, 255);
    drawLine(searchX + 31, searchY + searchH/2 + 6, searchX + 38, searchY + searchH/2 + 13, 120, 120, 150, 2);
    
    // Search placeholder
    g_textRenderer.setSize(15);
    g_textRenderer.drawText("Search or enter URL", searchX + 50, searchY + 26, 140, 140, 160);
    
    // Quick links
    float qy = searchY + 80;
    float qSpacing = 70;
    float qStart = centerX - qSpacing * 1.5f;
    uint8_t qColors[4][3] = {{80, 130, 220}, {140, 100, 200}, {200, 120, 160}, {220, 100, 100}};
    
    for (int i = 0; i < 4; i++) {
        drawCircle(qStart + i * qSpacing, qy, 22, qColors[i][0], qColors[i][1], qColors[i][2], 255);
    }
}

void renderDevTools() {
    if (!g_devToolsOpen) return;
    
    float y = g_windowHeight - 220.0f;
    drawRect(0, y, (float)g_windowWidth, 220, 30, 25, 45, 250);
    drawRect(0, y, (float)g_windowWidth, 32, 40, 35, 60, 255);
    
    // DevTools tabs
    const char* tabs[] = {"Elements", "Console", "Network", "Sources", "Performance"};
    float tx = 10;
    g_textRenderer.setSize(12);
    for (int i = 0; i < 5; i++) {
        float tw = 80;
        bool active = (i == 1); // Console selected
        if (active) drawRect(tx, y, tw, 32, 50, 45, 75, 255);
        g_textRenderer.drawText(tabs[i], tx + 10, y + 20, 
            active ? 255 : 180, active ? 255 : 180, active ? 255 : 200);
        tx += tw + 5;
    }
    
    // Console output
    g_textRenderer.setSize(13);
    float consoleY = y + 45;
    
    // Log entries
    drawRect(10, consoleY, g_windowWidth - 20.0f, 24, 35, 30, 55, 200);
    drawCircle(22, consoleY + 12, 5, 80, 180, 255, 255); // Info icon
    g_textRenderer.drawText("[Info] Page loaded successfully", 35, consoleY + 17, 180, 220, 255);
    consoleY += 28;
    
    drawRect(10, consoleY, g_windowWidth - 20.0f, 24, 35, 30, 55, 200);
    drawCircle(22, consoleY + 12, 5, 255, 200, 80, 255); // Warning icon
    g_textRenderer.drawText("[Warn] Mixed content detected", 35, consoleY + 17, 255, 220, 120);
    consoleY += 28;
    
    // Console input
    drawRoundedRect(10, y + 185, g_windowWidth - 20.0f, 28, 6, 45, 40, 70, 255);
    g_textRenderer.drawText("> ", 18, y + 204, 100, 200, 120);
}

void renderSettings() {
    if (!g_settingsOpen) return;
    
    float w = 280, h = 350;
    float x = g_windowWidth - w - 20;
    float y = 110;
    
    drawRoundedRect(x, y, w, h, 12, 40, 35, 65, 245);
    
    g_textRenderer.setSize(14);
    g_textRenderer.drawText("Settings", x + 20, y + 30, 255, 255, 255);
    
    // Settings items
    const char* items[] = {"New Tab", "New Window", "History", "Downloads", "Bookmarks", "Extensions"};
    float iy = y + 50;
    g_textRenderer.setSize(13);
    for (int i = 0; i < 6; i++) {
        bool hover = (g_mouseX >= x && g_mouseX <= x + w && g_mouseY >= iy && g_mouseY <= iy + 32);
        if (hover) drawRect(x, iy, w, 32, 50, 45, 75, 200);
        g_textRenderer.drawText(items[i], x + 20, iy + 22, 200, 200, 220);
        iy += 32;
    }
    
    // Sign in button
    drawRoundedRect(x + 20, y + h - 50, w - 40, 36, 8, 
        colors.accent[0], colors.accent[1], colors.accent[2], 255);
    g_textRenderer.drawText("Sign in to Zepra", x + 60, y + h - 28, 255, 255, 255);
}

// ============================================================================
// Event Handling
// ============================================================================

void handleEvents() {
    while (XPending(g_display)) {
        XEvent e;
        XNextEvent(g_display, &e);
        
        switch (e.type) {
            case ClientMessage:
                if ((Atom)e.xclient.data.l[0] == g_wmDeleteMessage) g_running = false;
                break;
                
            case ConfigureNotify:
                if (e.xconfigure.width != g_windowWidth || e.xconfigure.height != g_windowHeight) {
                    g_windowWidth = e.xconfigure.width;
                    g_windowHeight = e.xconfigure.height;
                    glViewport(0, 0, g_windowWidth, g_windowHeight);
                    glMatrixMode(GL_PROJECTION);
                    glLoadIdentity();
                    glOrtho(0, g_windowWidth, g_windowHeight, 0, -1, 1);
                    glMatrixMode(GL_MODELVIEW);
                    
                    for (auto& tab : g_tabs) {
                        if (tab.page) tab.page->setViewport(g_windowWidth - 20.0f, g_windowHeight - 130.0f);
                    }
                }
                break;
                
            case MotionNotify:
                g_mouseX = (float)e.xmotion.x;
                g_mouseY = (float)e.xmotion.y;
                break;
                
            case ButtonPress:
                if (e.xbutton.button == 1) {
                    float mx = (float)e.xbutton.x, my = (float)e.xbutton.y;
                    
                    // Close settings if clicking outside
                    if (g_settingsOpen && mx < g_windowWidth - 300) {
                        g_settingsOpen = false;
                    }
                    
                    // Menu button
                    if (mx >= g_windowWidth - 40 && my >= 78 && my <= 106) {
                        g_settingsOpen = !g_settingsOpen;
                    }
                    
                    // Address bar click
                    else if (my >= 78 && my <= 106 && mx >= 80 && mx <= g_windowWidth - 60) {
                        g_addressBarFocused = true;
                    } else {
                        g_addressBarFocused = false;
                    }
                    
                    // Tab bar clicks
                    if (my >= 40 && my <= 75) {
                        float tx = 50;
                        for (size_t i = 0; i < g_tabs.size(); i++) {
                            float tw = 160;
                            if (mx >= tx && mx <= tx + tw) {
                                if (mx >= tx + tw - 25) {
                                    // Close tab
                                    if (g_tabs.size() > 1) {
                                        g_tabs.erase(g_tabs.begin() + i);
                                        if (g_activeTab >= (int)g_tabs.size()) g_activeTab = g_tabs.size() - 1;
                                        g_tabs[g_activeTab].active = true;
                                        g_addressBarText = g_tabs[g_activeTab].url;
                                    }
                                } else {
                                    g_activeTab = i;
                                    for (auto& t : g_tabs) t.active = false;
                                    g_tabs[i].active = true;
                                    g_addressBarText = g_tabs[i].url;
                                }
                                break;
                            }
                            tx += tw + 8;
                        }
                        
                        // New tab
                        if (mx >= tx && mx <= tx + 30) {
                            for (auto& t : g_tabs) t.active = false;
                            g_tabs.emplace_back("zepra://start");
                            g_tabs.back().active = true;
                            g_tabs.back().load();
                            g_activeTab = g_tabs.size() - 1;
                            g_addressBarText = "zepra://start";
                        }
                    }
                }
                
                // Scroll
                if (e.xbutton.button == 4 || e.xbutton.button == 5) {
                    if (!g_tabs.empty() && g_tabs[g_activeTab].page) {
                        float delta = (e.xbutton.button == 4) ? -40.0f : 40.0f;
                        g_tabs[g_activeTab].page->scrollBy(0, delta);
                    }
                }
                break;
                
            case KeyPress: {
                KeySym key = XLookupKeysym(&e.xkey, 0);
                
                if (key == XK_Escape) {
                    if (g_settingsOpen) g_settingsOpen = false;
                    else if (g_addressBarFocused) g_addressBarFocused = false;
                    else g_running = false;
                }
                if (key == XK_F12) g_devToolsOpen = !g_devToolsOpen;
                if (key == XK_F5 && !g_tabs.empty()) {
                    g_tabs[g_activeTab].load();
                }
                
                // Address bar input
                if (g_addressBarFocused) {
                    if (key == XK_Return) {
                        g_addressBarFocused = false;
                        if (!g_tabs.empty()) {
                            g_tabs[g_activeTab].url = g_addressBarText;
                            g_tabs[g_activeTab].load();
                        }
                    } else if (key == XK_BackSpace) {
                        if (!g_addressBarText.empty()) g_addressBarText.pop_back();
                    } else {
                        char buf[32];
                        KeySym ks;
                        int len = XLookupString(&e.xkey, buf, sizeof(buf), &ks, nullptr);
                        if (len > 0 && buf[0] >= 32) {
                            g_addressBarText += std::string(buf, len);
                        }
                    }
                }
                
                // Ctrl shortcuts
                if (e.xkey.state & ControlMask) {
                    if (key == XK_t) {
                        for (auto& t : g_tabs) t.active = false;
                        g_tabs.emplace_back("zepra://start");
                        g_tabs.back().active = true;
                        g_tabs.back().load();
                        g_activeTab = g_tabs.size() - 1;
                        g_addressBarText = "zepra://start";
                    }
                    if (key == XK_w && g_tabs.size() > 1) {
                        g_tabs.erase(g_tabs.begin() + g_activeTab);
                        if (g_activeTab >= (int)g_tabs.size()) g_activeTab = g_tabs.size() - 1;
                        g_tabs[g_activeTab].active = true;
                        g_addressBarText = g_tabs[g_activeTab].url;
                    }
                    if (key == XK_l) g_addressBarFocused = true;
                }
                
                // Alt+Arrow: Back/Forward navigation
                if (e.xkey.state & Mod1Mask) {
                    if (key == XK_Left && !g_tabs.empty()) {
                        g_tabs[g_activeTab].goBack();
                        g_addressBarText = g_tabs[g_activeTab].url;
                    }
                    if (key == XK_Right && !g_tabs.empty()) {
                        g_tabs[g_activeTab].goForward();
                        g_addressBarText = g_tabs[g_activeTab].url;
                    }
                }

                break;
            }
        }
    }
}

// ============================================================================
// Main
// ============================================================================

bool initWindow() {
    g_display = XOpenDisplay(nullptr);
    if (!g_display) return false;
    
    int screen = DefaultScreen(g_display);
    int attribs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    XVisualInfo* vi = glXChooseVisual(g_display, screen, attribs);
    if (!vi) return false;
    
    Colormap cmap = XCreateColormap(g_display, RootWindow(g_display, screen), vi->visual, AllocNone);
    XSetWindowAttributes swa = {};
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
    
    g_window = XCreateWindow(g_display, RootWindow(g_display, screen), 0, 0, 
        g_windowWidth, g_windowHeight, 0, vi->depth, InputOutput, vi->visual, 
        CWColormap | CWEventMask, &swa);
    
    XStoreName(g_display, g_window, "Zepra Browser");
    g_wmDeleteMessage = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_display, g_window, &g_wmDeleteMessage, 1);
    
    g_glContext = glXCreateContext(g_display, vi, nullptr, GL_TRUE);
    XFree(vi);
    XMapWindow(g_display, g_window);
    glXMakeCurrent(g_display, g_window, g_glContext);
    
    glViewport(0, 0, g_windowWidth, g_windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, g_windowWidth, g_windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    
    return true;
}

void shutdown() {
    g_textRenderer.cleanup();
    g_tabs.clear();
    g_sandboxManager.reset();
    g_platform.reset();
    if (g_theme) nx_theme_destroy(g_theme);
    if (g_glContext) {
        glXMakeCurrent(g_display, None, nullptr);
        glXDestroyContext(g_display, g_glContext);
    }
    if (g_window) XDestroyWindow(g_display, g_window);
    if (g_display) XCloseDisplay(g_display);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawGradient(0, 0, (float)g_windowWidth, (float)g_windowHeight, colors.bgTop, colors.bgBottom);
    renderTabBar();
    renderAddressBar();
    renderPageContent();
    renderDevTools();
    renderSettings();
    glXSwapBuffers(g_display, g_window);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    std::cout << "Starting Zepra Browser..." << std::endl;
    std::cout << "Version: " << nx_version() << std::endl;
    
    NxSystemInfo* sys = nx_detect_system();
    if (sys) {
        std::cout << "GPU: " << sys->gpu_name << std::endl;
        nx_free_system_info(sys);
    }
    
    if (!initWindow()) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }
    
    if (!g_textRenderer.init()) {
        std::cerr << "Text rendering not available" << std::endl;
    }
    
    g_theme = nx_theme_dark();
    g_platform = std::make_unique<zepra::PlatformInfrastructure>();
    g_sandboxManager = std::make_unique<zepra::SandboxManager>();
    
    // Create initial tab
    g_tabs.emplace_back("zepra://start");
    g_tabs.back().active = true;
    g_tabs.back().load();
    
    std::cout << "Browser ready!" << std::endl;
    
    while (g_running) {
        handleEvents();
        render();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    shutdown();
    return 0;
}
