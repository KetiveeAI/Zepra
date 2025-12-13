/**
 * @file devtools_pro.cpp
 * @brief Zepra DevTools Pro - Full-featured with sidebar, tabs, network details
 * 
 * Features:
 * - Left sidebar with icon navigation
 * - Top tabs for sub-panels
 * - Network: Request/Response/Headers/Preview tabs
 * - View page source
 * - Inspect elements with CSS
 * - Real-time data flow visualization
 * 
 * Build: g++ -o devtools_pro devtools_pro.cpp -lSDL2 -lSDL2_ttf -std=c++17
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <map>
#include <algorithm>

// =============================================================================
// Constants
// =============================================================================
const int SIDEBAR_WIDTH = 50;
const int TAB_HEIGHT = 32;
const int HEADER_HEIGHT = 40;
const int STATUS_HEIGHT = 24;

// =============================================================================
// Theme - Zepra Signature (Cyan + Violet)
// =============================================================================
namespace Theme {
    const SDL_Color BG = {18, 14, 28, 255};
    const SDL_Color SIDEBAR = {28, 22, 42, 255};
    const SDL_Color HEADER = {38, 30, 58, 255};
    const SDL_Color PANEL = {22, 18, 35, 255};
    const SDL_Color TABS = {32, 26, 48, 255};
    const SDL_Color INPUT = {35, 28, 52, 255};
    const SDL_Color DIVIDER = {45, 38, 65, 255};
    
    const SDL_Color TEXT = {235, 235, 245, 255};
    const SDL_Color TEXT_DIM = {140, 135, 160, 255};
    const SDL_Color TEXT_MUTED = {95, 90, 115, 255};
    
    const SDL_Color CYAN = {0, 220, 255, 255};
    const SDL_Color VIOLET = {160, 90, 255, 255};
    const SDL_Color PINK = {255, 100, 180, 255};
    const SDL_Color GREEN = {80, 220, 140, 255};
    const SDL_Color YELLOW = {255, 200, 80, 255};
    const SDL_Color RED = {255, 95, 95, 255};
    const SDL_Color BLUE = {100, 160, 255, 255};
    
    const SDL_Color TAB_ACTIVE = {50, 42, 78, 255};
    const SDL_Color TAB_HOVER = {42, 35, 65, 255};
    const SDL_Color SIDEBAR_ACTIVE = {55, 45, 85, 255};
    const SDL_Color SIDEBAR_HOVER = {45, 38, 70, 255};
    
    const SDL_Color CONNECTED = {80, 220, 140, 255};
}

// =============================================================================
// Main Panel Types
// =============================================================================
enum class MainPanel {
    Elements = 0,
    Console,
    Sources,
    Network,
    Performance,
    Memory,
    Application,
    Security,
    COUNT
};

const char* PANEL_ICONS[] = {"🔍", "▶", "📄", "🌐", "⚡", "💾", "📦", "🔒"};
const char* PANEL_NAMES[] = {"Elements", "Console", "Sources", "Network", "Performance", "Memory", "Application", "Security"};

// =============================================================================
// Network Sub-tabs
// =============================================================================
enum class NetworkTab { All, Headers, Preview, Response, Timing };

// =============================================================================
// Network Request Structure
// =============================================================================
struct Request {
    int id;
    std::string url;
    std::string method;
    int status;
    std::string statusText;
    std::string type;
    std::string initiator;
    size_t size;
    double time;
    bool completed;
    bool selected;
    
    // Headers
    std::map<std::string, std::string> requestHeaders;
    std::map<std::string, std::string> responseHeaders;
    
    // Body
    std::string requestBody;
    std::string responseBody;
    
    // Timing
    double dns, connect, ssl, wait, download;
};

// =============================================================================
// Console Message
// =============================================================================
struct ConsoleLine {
    enum Type { LOG, INFO, WARN, ERROR, INPUT, OUTPUT };
    Type type;
    std::string text;
    std::string time;
};

// =============================================================================
// DOM Node
// =============================================================================
struct DOMNode {
    int id;
    std::string tag;
    std::map<std::string, std::string> attrs;
    std::vector<DOMNode> children;
    std::string textContent;
    bool expanded = true;
    bool selected = false;
    
    // Box model
    int margin[4] = {0, 0, 0, 0};
    int border[4] = {0, 0, 0, 0};
    int padding[4] = {0, 0, 0, 0};
    int width = 0, height = 0;
    
    // CSS
    std::map<std::string, std::string> styles;
};

// =============================================================================
// DevTools Application
// =============================================================================
class DevToolsPro {
public:
    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
        if (TTF_Init() < 0) return false;
        
        window_ = SDL_CreateWindow(
            "Zepra DevTools",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width_, height_,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        if (!window_) return false;
        
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer_) return false;
        
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        
        // Load fonts
        const char* paths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
            nullptr
        };
        for (int i = 0; paths[i]; i++) {
            font_ = TTF_OpenFont(paths[i], 13);
            if (font_) {
                fontSmall_ = TTF_OpenFont(paths[i], 11);
                fontBold_ = TTF_OpenFont(paths[i], 13);
                break;
            }
        }
        
        initData();
        return true;
    }
    
    void initData() {
        // Console
        addConsole(ConsoleLine::INFO, "DevTools connected");
        addConsole(ConsoleLine::LOG, "Document loaded: https://example.com");
        addConsole(ConsoleLine::INPUT, "> console.log('Hello')");
        addConsole(ConsoleLine::OUTPUT, "Hello");
        addConsole(ConsoleLine::WARN, "Deprecated API: document.write");
        addConsole(ConsoleLine::ERROR, "TypeError: undefined is not a function");
        
        // Network requests
        Request r1;
        r1.id = 1;
        r1.url = "https://example.com/";
        r1.method = "GET";
        r1.status = 200;
        r1.statusText = "OK";
        r1.type = "document";
        r1.initiator = "Other";
        r1.size = 4520;
        r1.time = 125;
        r1.completed = true;
        r1.requestHeaders = {{"Accept", "text/html"}, {"User-Agent", "Zepra/1.0"}};
        r1.responseHeaders = {{"Content-Type", "text/html"}, {"Content-Length", "4520"}, {"Server", "nginx"}};
        r1.responseBody = "<!DOCTYPE html>\n<html>\n<head>\n  <title>Example</title>\n</head>\n<body>\n  <h1>Hello World</h1>\n  <p>Welcome to Zepra Browser</p>\n</body>\n</html>";
        r1.dns = 5; r1.connect = 15; r1.ssl = 25; r1.wait = 60; r1.download = 20;
        requests_.push_back(r1);
        
        Request r2;
        r2.id = 2;
        r2.url = "https://example.com/style.css";
        r2.method = "GET";
        r2.status = 200;
        r2.statusText = "OK";
        r2.type = "stylesheet";
        r2.size = 2840;
        r2.time = 45;
        r2.completed = true;
        r2.responseHeaders = {{"Content-Type", "text/css"}};
        r2.responseBody = "body { font-family: sans-serif; }\nh1 { color: #333; }\np { line-height: 1.6; }";
        r2.dns = 0; r2.connect = 0; r2.ssl = 0; r2.wait = 30; r2.download = 15;
        requests_.push_back(r2);
        
        Request r3;
        r3.id = 3;
        r3.url = "https://example.com/app.js";
        r3.method = "GET";
        r3.status = 200;
        r3.statusText = "OK";
        r3.type = "script";
        r3.size = 128000;
        r3.time = 350;
        r3.completed = true;
        r3.responseHeaders = {{"Content-Type", "application/javascript"}};
        r3.responseBody = "// app.js\nfunction init() {\n  console.log('App initialized');\n}\n\nwindow.onload = init;";
        r3.dns = 0; r3.connect = 0; r3.ssl = 0; r3.wait = 150; r3.download = 200;
        requests_.push_back(r3);
        
        Request r4;
        r4.id = 4;
        r4.url = "https://api.example.com/users";
        r4.method = "GET";
        r4.status = 200;
        r4.statusText = "OK";
        r4.type = "xhr";
        r4.size = 1240;
        r4.time = 180;
        r4.completed = true;
        r4.responseHeaders = {{"Content-Type", "application/json"}};
        r4.responseBody = "{\n  \"users\": [\n    {\"id\": 1, \"name\": \"Alice\"},\n    {\"id\": 2, \"name\": \"Bob\"}\n  ]\n}";
        requests_.push_back(r4);
        
        Request r5;
        r5.id = 5;
        r5.url = "https://api.example.com/missing";
        r5.method = "GET";
        r5.status = 404;
        r5.statusText = "Not Found";
        r5.type = "xhr";
        r5.size = 128;
        r5.time = 90;
        r5.completed = true;
        r5.responseHeaders = {{"Content-Type", "application/json"}};
        r5.responseBody = "{\"error\": \"Resource not found\"}";
        requests_.push_back(r5);
        
        // DOM
        root_.id = 1;
        root_.tag = "html";
        root_.attrs["lang"] = "en";
        
        DOMNode head;
        head.id = 2;
        head.tag = "head";
        DOMNode title;
        title.id = 3;
        title.tag = "title";
        title.textContent = "Example Page";
        head.children.push_back(title);
        
        DOMNode body;
        body.id = 4;
        body.tag = "body";
        body.attrs["class"] = "main-app";
        body.margin[0] = 8; body.margin[1] = 8; body.margin[2] = 8; body.margin[3] = 8;
        body.padding[0] = 16; body.padding[1] = 16; body.padding[2] = 16; body.padding[3] = 16;
        body.width = 1200; body.height = 800;
        body.styles = {{"display", "block"}, {"font-family", "sans-serif"}, {"color", "#333"}, {"background", "#fff"}};
        
        DOMNode div;
        div.id = 5;
        div.tag = "div";
        div.attrs["id"] = "app";
        div.attrs["class"] = "container";
        div.padding[0] = 20; div.padding[1] = 20; div.padding[2] = 20; div.padding[3] = 20;
        div.width = 800; div.height = 400;
        div.styles = {{"display", "flex"}, {"flex-direction", "column"}};
        
        DOMNode h1;
        h1.id = 6;
        h1.tag = "h1";
        h1.textContent = "Hello World";
        h1.styles = {{"font-size", "24px"}, {"margin", "0 0 16px 0"}};
        
        DOMNode p;
        p.id = 7;
        p.tag = "p";
        p.textContent = "Welcome to Zepra Browser";
        p.styles = {{"font-size", "14px"}, {"line-height", "1.6"}};
        
        div.children.push_back(h1);
        div.children.push_back(p);
        body.children.push_back(div);
        
        root_.children.push_back(head);
        root_.children.push_back(body);
        
        selectedNode_ = &body;
    }
    
    void addConsole(ConsoleLine::Type type, const std::string& text) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        char buf[16];
        strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
        console_.push_back({type, text, buf});
    }
    
    void run() {
        running_ = true;
        while (running_) {
            handleEvents();
            render();
            SDL_Delay(16);
        }
    }
    
    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running_ = false;
                    break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                        width_ = e.window.data1;
                        height_ = e.window.data2;
                    }
                    break;
                case SDL_KEYDOWN:
                    handleKey(e.key);
                    break;
                case SDL_TEXTINPUT:
                    if (panel_ == MainPanel::Console) {
                        input_ += e.text.text;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    handleClick(e.button.x, e.button.y);
                    break;
                case SDL_MOUSEMOTION:
                    mouseX_ = e.motion.x;
                    mouseY_ = e.motion.y;
                    break;
            }
        }
    }
    
    void handleKey(const SDL_KeyboardEvent& key) {
        if (key.keysym.sym == SDLK_ESCAPE) {
            running_ = false;
        } else if (key.keysym.sym == SDLK_RETURN) {
            if (panel_ == MainPanel::Console && !input_.empty()) {
                executeCommand(input_);
                input_.clear();
            }
        } else if (key.keysym.sym == SDLK_BACKSPACE) {
            if (!input_.empty()) input_.pop_back();
        }
        
        // Quick panel switch
        if (key.keysym.sym >= SDLK_1 && key.keysym.sym <= SDLK_8) {
            panel_ = static_cast<MainPanel>(key.keysym.sym - SDLK_1);
        }
    }
    
    void handleClick(int x, int y) {
        // Sidebar click
        if (x < SIDEBAR_WIDTH) {
            int idx = (y - HEADER_HEIGHT) / 45;
            if (idx >= 0 && idx < static_cast<int>(MainPanel::COUNT)) {
                panel_ = static_cast<MainPanel>(idx);
            }
            return;
        }
        
        // Network request list click
        if (panel_ == MainPanel::Network && !detailsOpen_) {
            int listY = HEADER_HEIGHT + TAB_HEIGHT + 35;
            for (size_t i = 0; i < requests_.size(); i++) {
                if (y >= listY && y < listY + 22) {
                    selectedRequest_ = i;
                    detailsOpen_ = true;
                    break;
                }
                listY += 22;
            }
        }
        
        // Network sub-tabs
        if (panel_ == MainPanel::Network && detailsOpen_) {
            int tabY = HEADER_HEIGHT + TAB_HEIGHT;
            if (y >= tabY && y <= tabY + 28) {
                int tabX = SIDEBAR_WIDTH + 10;
                const char* tabs[] = {"Headers", "Preview", "Response", "Timing"};
                for (int i = 0; i < 4; i++) {
                    if (x >= tabX && x < tabX + 80) {
                        netTab_ = static_cast<NetworkTab>(i + 1);
                        break;
                    }
                    tabX += 85;
                }
            }
        }
        
        // Close details button
        if (panel_ == MainPanel::Network && detailsOpen_) {
            if (x >= width_ - 30 && x <= width_ - 10 && y >= HEADER_HEIGHT + TAB_HEIGHT + 5 && y <= HEADER_HEIGHT + TAB_HEIGHT + 25) {
                detailsOpen_ = false;
            }
        }
    }
    
    void executeCommand(const std::string& cmd) {
        addConsole(ConsoleLine::INPUT, "> " + cmd);
        
        if (cmd == "clear") {
            console_.clear();
        } else if (cmd == "help") {
            addConsole(ConsoleLine::INFO, "Commands: clear, help, document.title, location.href");
        } else if (cmd == "document.title") {
            addConsole(ConsoleLine::OUTPUT, "\"Example Page\"");
        } else if (cmd == "location.href") {
            addConsole(ConsoleLine::OUTPUT, "\"https://example.com/\"");
        } else if (cmd.find("console.log") != std::string::npos) {
            size_t s = cmd.find('(') + 1;
            size_t e = cmd.rfind(')');
            if (s < e) {
                std::string arg = cmd.substr(s, e - s);
                if ((arg.front() == '\'' || arg.front() == '"') && arg.size() > 2) {
                    arg = arg.substr(1, arg.size() - 2);
                }
                addConsole(ConsoleLine::LOG, arg);
            }
        } else {
            addConsole(ConsoleLine::OUTPUT, "undefined");
        }
    }
    
    void render() {
        setColor(Theme::BG);
        SDL_RenderClear(renderer_);
        
        renderHeader();
        renderSidebar();
        renderMainPanel();
        renderStatusBar();
        
        SDL_RenderPresent(renderer_);
    }
    
    void renderHeader() {
        SDL_Rect h = {0, 0, width_, HEADER_HEIGHT};
        setColor(Theme::HEADER);
        SDL_RenderFillRect(renderer_, &h);
        
        // Title
        text("◉ Zepra DevTools", SIDEBAR_WIDTH + 15, 12, Theme::CYAN);
        
        // Connected
        int cx = width_ - 110;
        fillCircle(cx, 20, 5, Theme::CONNECTED);
        text("Connected", cx + 12, 12, Theme::TEXT_DIM, true);
    }
    
    void renderSidebar() {
        SDL_Rect s = {0, HEADER_HEIGHT, SIDEBAR_WIDTH, height_ - HEADER_HEIGHT - STATUS_HEIGHT};
        setColor(Theme::SIDEBAR);
        SDL_RenderFillRect(renderer_, &s);
        
        int y = HEADER_HEIGHT + 8;
        for (int i = 0; i < static_cast<int>(MainPanel::COUNT); i++) {
            SDL_Rect item = {0, y, SIDEBAR_WIDTH, 40};
            
            bool active = static_cast<MainPanel>(i) == panel_;
            bool hover = mouseX_ < SIDEBAR_WIDTH && mouseY_ >= y && mouseY_ < y + 40;
            
            if (active) {
                setColor(Theme::SIDEBAR_ACTIVE);
                SDL_RenderFillRect(renderer_, &item);
                // Left accent
                SDL_Rect acc = {0, y, 3, 40};
                setColor(Theme::CYAN);
                SDL_RenderFillRect(renderer_, &acc);
            } else if (hover) {
                setColor(Theme::SIDEBAR_HOVER);
                SDL_RenderFillRect(renderer_, &item);
            }
            
            // Icon (just first letter for now)
            char icon[3] = {PANEL_NAMES[i][0], PANEL_NAMES[i][1], '\0'};
            text(icon, 15, y + 12, active ? Theme::CYAN : Theme::TEXT_DIM);
            
            y += 45;
        }
    }
    
    void renderMainPanel() {
        int px = SIDEBAR_WIDTH;
        int py = HEADER_HEIGHT;
        int pw = width_ - SIDEBAR_WIDTH;
        int ph = height_ - HEADER_HEIGHT - STATUS_HEIGHT;
        
        SDL_Rect p = {px, py, pw, ph};
        setColor(Theme::PANEL);
        SDL_RenderFillRect(renderer_, &p);
        
        // Panel tabs
        SDL_Rect tabs = {px, py, pw, TAB_HEIGHT};
        setColor(Theme::TABS);
        SDL_RenderFillRect(renderer_, &tabs);
        
        // Active tab indicator
        text(PANEL_NAMES[static_cast<int>(panel_)], px + 15, py + 8, Theme::TEXT);
        
        if (panel_ == MainPanel::Network && detailsOpen_) {
            // Sub-tabs for network details
            const char* subTabs[] = {"Headers", "Preview", "Response", "Timing"};
            int tx = px + 120;
            for (int i = 0; i < 4; i++) {
                SDL_Rect tab = {tx, py + 3, 75, 26};
                bool active = static_cast<int>(netTab_) == i + 1;
                if (active) {
                    setColor(Theme::TAB_ACTIVE);
                    SDL_RenderFillRect(renderer_, &tab);
                }
                text(subTabs[i], tx + 10, py + 8, active ? Theme::CYAN : Theme::TEXT_DIM, true);
                tx += 85;
            }
            
            // Close button
            text("✕", pw + px - 25, py + 8, Theme::RED);
        }
        
        // Panel content
        int contentY = py + TAB_HEIGHT + 10;
        
        switch (panel_) {
            case MainPanel::Elements: renderElements(px + 10, contentY); break;
            case MainPanel::Console: renderConsole(px + 10, contentY); break;
            case MainPanel::Network: renderNetwork(px + 10, contentY); break;
            case MainPanel::Sources: renderSources(px + 10, contentY); break;
            case MainPanel::Performance: renderPerformance(px + 10, contentY); break;
            case MainPanel::Memory: renderMemory(px + 10, contentY); break;
            case MainPanel::Application: renderApplication(px + 10, contentY); break;
            case MainPanel::Security: renderSecurity(px + 10, contentY); break;
            default: break;
        }
    }
    
    void renderElements(int x, int y) {
        int splitX = x + (width_ - SIDEBAR_WIDTH) / 2;
        
        // DOM Tree
        text("DOM", x, y, Theme::CYAN);
        y += 25;
        renderNode(root_, x, y, 0);
        
        // Divider
        SDL_Rect div = {splitX - 5, HEADER_HEIGHT + TAB_HEIGHT, 1, height_ - HEADER_HEIGHT - TAB_HEIGHT - STATUS_HEIGHT};
        setColor(Theme::DIVIDER);
        SDL_RenderFillRect(renderer_, &div);
        
        // Styles panel
        text("Styles", splitX + 10, HEADER_HEIGHT + TAB_HEIGHT + 10, Theme::VIOLET);
        
        if (selectedNode_) {
            int sy = HEADER_HEIGHT + TAB_HEIGHT + 40;
            
            // Selector
            std::string sel = selectedNode_->tag;
            if (selectedNode_->attrs.count("class")) sel += "." + selectedNode_->attrs.at("class");
            if (selectedNode_->attrs.count("id")) sel += "#" + selectedNode_->attrs.at("id");
            
            text(sel + " {", splitX + 15, sy, Theme::TEXT);
            sy += 20;
            
            for (const auto& [prop, val] : selectedNode_->styles) {
                text("  " + prop + ":", splitX + 20, sy, Theme::CYAN, true);
                text(val + ";", splitX + 140, sy, Theme::TEXT, true);
                sy += 18;
            }
            
            text("}", splitX + 15, sy, Theme::TEXT);
            sy += 30;
            
            // Box Model
            text("Box Model", splitX + 10, sy, Theme::VIOLET);
            sy += 25;
            renderBoxModel(splitX + 30, sy);
        }
    }
    
    void renderNode(const DOMNode& node, int x, int& y, int depth) {
        if (y > height_ - 50) return;
        
        std::string prefix = node.children.empty() ? "  " : (node.expanded ? "▼ " : "▶ ");
        std::string line = prefix + "<" + node.tag;
        
        for (const auto& [k, v] : node.attrs) {
            line += " " + k + "=\"" + v + "\"";
        }
        
        if (!node.textContent.empty()) {
            line += ">" + node.textContent + "</" + node.tag + ">";
        } else if (node.children.empty()) {
            line += " />";
        } else {
            line += ">";
        }
        
        SDL_Color col = (&node == selectedNode_) ? Theme::CYAN : Theme::TEXT;
        text(line, x + depth * 16, y, col, true);
        y += 18;
        
        if (node.expanded) {
            for (const auto& child : node.children) {
                renderNode(child, x, y, depth + 1);
            }
            if (!node.children.empty() && node.textContent.empty()) {
                text("</" + node.tag + ">", x + depth * 16, y, Theme::TEXT_DIM, true);
                y += 18;
            }
        }
    }
    
    void renderBoxModel(int x, int y) {
        if (!selectedNode_) return;
        
        int w = 180, h = 130;
        
        // Margin
        SDL_Rect m = {x, y, w, h};
        SDL_SetRenderDrawColor(renderer_, 255, 180, 100, 80);
        SDL_RenderFillRect(renderer_, &m);
        text("margin", x + 5, y + 3, Theme::TEXT_MUTED, true);
        
        // Border
        SDL_Rect b = {x + 15, y + 20, w - 30, h - 40};
        SDL_SetRenderDrawColor(renderer_, 255, 220, 120, 80);
        SDL_RenderFillRect(renderer_, &b);
        text("border", x + 20, y + 23, Theme::TEXT_MUTED, true);
        
        // Padding
        SDL_Rect p = {x + 30, y + 40, w - 60, h - 80};
        SDL_SetRenderDrawColor(renderer_, 140, 200, 100, 80);
        SDL_RenderFillRect(renderer_, &p);
        text("padding", x + 35, y + 43, Theme::TEXT_MUTED, true);
        
        // Content
        SDL_Rect c = {x + 50, y + 55, w - 100, h - 110};
        SDL_SetRenderDrawColor(renderer_, 100, 180, 255, 80);
        SDL_RenderFillRect(renderer_, &c);
        
        std::string size = std::to_string(selectedNode_->width) + "×" + std::to_string(selectedNode_->height);
        text(size, x + 55, y + 58, Theme::TEXT, true);
    }
    
    void renderConsole(int x, int y) {
        // Filter buttons
        text("🔍", x, y, Theme::TEXT_DIM);
        
        const char* filters[] = {"All", "Errors", "Warnings", "Info", "Logs"};
        int fx = x + 30;
        for (int i = 0; i < 5; i++) {
            SDL_Rect btn = {fx, y - 3, 55, 20};
            setColor(Theme::INPUT);
            SDL_RenderFillRect(renderer_, &btn);
            text(filters[i], fx + 5, y, Theme::TEXT_DIM, true);
            fx += 60;
        }
        
        y += 30;
        
        // Messages
        int maxY = height_ - STATUS_HEIGHT - 60;
        int start = std::max(0, (int)console_.size() - ((maxY - y) / 18));
        
        for (size_t i = start; i < console_.size() && y < maxY; i++) {
            const auto& msg = console_[i];
            
            SDL_Color col;
            std::string prefix;
            switch (msg.type) {
                case ConsoleLine::LOG: col = Theme::TEXT; break;
                case ConsoleLine::INFO: col = Theme::BLUE; prefix = "ℹ "; break;
                case ConsoleLine::WARN: col = Theme::YELLOW; prefix = "⚠ "; break;
                case ConsoleLine::ERROR: col = Theme::RED; prefix = "✖ "; break;
                case ConsoleLine::INPUT: col = Theme::CYAN; break;
                case ConsoleLine::OUTPUT: col = Theme::VIOLET; prefix = "← "; break;
            }
            
            text(msg.time, x, y, Theme::TEXT_MUTED, true);
            text(prefix + msg.text, x + 70, y, col);
            y += 18;
        }
        
        // Input
        int iy = height_ - STATUS_HEIGHT - 45;
        SDL_Rect inp = {x - 5, iy, width_ - SIDEBAR_WIDTH - 20, 32};
        setColor(Theme::INPUT);
        SDL_RenderFillRect(renderer_, &inp);
        setColor(Theme::VIOLET);
        SDL_RenderDrawRect(renderer_, &inp);
        
        text("> " + input_ + "_", x + 5, iy + 8, Theme::TEXT);
    }
    
    void renderNetwork(int x, int y) {
        if (detailsOpen_ && selectedRequest_ < requests_.size()) {
            renderNetworkDetails(x, y);
            return;
        }
        
        // Header row
        text("Name", x, y, Theme::TEXT_DIM);
        text("Status", x + 250, y, Theme::TEXT_DIM);
        text("Type", x + 320, y, Theme::TEXT_DIM);
        text("Size", x + 400, y, Theme::TEXT_DIM);
        text("Time", x + 480, y, Theme::TEXT_DIM);
        text("Waterfall", x + 560, y, Theme::TEXT_DIM);
        
        y += 25;
        
        // Divider
        SDL_Rect div = {x - 5, y, width_ - SIDEBAR_WIDTH - 20, 1};
        setColor(Theme::DIVIDER);
        SDL_RenderFillRect(renderer_, &div);
        y += 8;
        
        // Requests
        double maxTime = 0;
        for (const auto& r : requests_) maxTime = std::max(maxTime, r.time);
        
        for (size_t i = 0; i < requests_.size(); i++) {
            const auto& r = requests_[i];
            
            bool hover = mouseY_ >= y && mouseY_ < y + 22 && mouseX_ > SIDEBAR_WIDTH;
            if (hover) {
                SDL_Rect row = {x - 5, y - 2, width_ - SIDEBAR_WIDTH - 20, 22};
                setColor(Theme::TAB_HOVER);
                SDL_RenderFillRect(renderer_, &row);
            }
            
            // Name (truncated)
            std::string name = r.url;
            size_t lastSlash = name.rfind('/');
            if (lastSlash != std::string::npos && lastSlash < name.size() - 1) {
                name = name.substr(lastSlash + 1);
            }
            if (name.empty()) name = r.url;
            if (name.size() > 28) name = name.substr(0, 25) + "...";
            text(name, x, y, Theme::TEXT);
            
            // Status
            SDL_Color statusCol = r.status < 400 ? Theme::GREEN : Theme::RED;
            text(std::to_string(r.status), x + 250, y, statusCol);
            
            // Type
            text(r.type, x + 320, y, Theme::TEXT_DIM);
            
            // Size
            std::string size = r.size > 1024 ? std::to_string(r.size / 1024) + " KB" : std::to_string(r.size) + " B";
            text(size, x + 400, y, Theme::TEXT);
            
            // Time
            text(std::to_string((int)r.time) + " ms", x + 480, y, Theme::TEXT);
            
            // Waterfall
            int barWidth = (int)((r.time / maxTime) * 150);
            SDL_Rect bar = {x + 560, y + 3, barWidth, 12};
            setColor({(Uint8)(Theme::CYAN.r / 2 + Theme::VIOLET.r / 2),
                      (Uint8)(Theme::CYAN.g / 2 + Theme::VIOLET.g / 2),
                      (Uint8)(Theme::CYAN.b / 2 + Theme::VIOLET.b / 2), 200});
            SDL_RenderFillRect(renderer_, &bar);
            
            y += 22;
        }
        
        // Summary
        y = height_ - STATUS_HEIGHT - 30;
        size_t totalSize = 0;
        double totalTime = 0;
        for (const auto& r : requests_) {
            totalSize += r.size;
            totalTime += r.time;
        }
        std::string summary = std::to_string(requests_.size()) + " requests | " +
                              std::to_string(totalSize / 1024) + " KB | " +
                              std::to_string((int)totalTime) + " ms";
        text(summary, x, y, Theme::TEXT_DIM);
    }
    
    void renderNetworkDetails(int x, int y) {
        const Request& r = requests_[selectedRequest_];
        
        // Request URL
        text(r.method + " " + r.url, x, y, Theme::TEXT);
        text(std::to_string(r.status) + " " + r.statusText, x + 400, y, r.status < 400 ? Theme::GREEN : Theme::RED);
        y += 30;
        
        switch (netTab_) {
            case NetworkTab::Headers:
                renderHeaders(x, y, r);
                break;
            case NetworkTab::Preview:
            case NetworkTab::Response:
                renderResponse(x, y, r);
                break;
            case NetworkTab::Timing:
                renderTiming(x, y, r);
                break;
            default:
                renderHeaders(x, y, r);
                break;
        }
    }
    
    void renderHeaders(int x, int y, const Request& r) {
        text("Request Headers", x, y, Theme::VIOLET);
        y += 22;
        
        for (const auto& [k, v] : r.requestHeaders) {
            text(k + ":", x + 10, y, Theme::CYAN, true);
            text(v, x + 150, y, Theme::TEXT, true);
            y += 18;
        }
        
        y += 15;
        text("Response Headers", x, y, Theme::VIOLET);
        y += 22;
        
        for (const auto& [k, v] : r.responseHeaders) {
            text(k + ":", x + 10, y, Theme::CYAN, true);
            text(v, x + 150, y, Theme::TEXT, true);
            y += 18;
        }
    }
    
    void renderResponse(int x, int y, const Request& r) {
        text("Response Body (" + std::to_string(r.size) + " bytes)", x, y, Theme::VIOLET);
        y += 25;
        
        // Show response body
        std::istringstream ss(r.responseBody);
        std::string line;
        int lineNum = 1;
        while (std::getline(ss, line) && y < height_ - STATUS_HEIGHT - 20) {
            text(std::to_string(lineNum), x, y, Theme::TEXT_MUTED, true);
            text(line, x + 35, y, Theme::TEXT);
            y += 16;
            lineNum++;
        }
    }
    
    void renderTiming(int x, int y, const Request& r) {
        text("Timing Breakdown", x, y, Theme::VIOLET);
        y += 30;
        
        double total = r.dns + r.connect + r.ssl + r.wait + r.download;
        
        struct Phase { std::string name; double ms; SDL_Color col; };
        std::vector<Phase> phases = {
            {"DNS Lookup", r.dns, {100, 150, 255, 200}},
            {"Connection", r.connect, {255, 180, 100, 200}},
            {"SSL/TLS", r.ssl, {200, 100, 255, 200}},
            {"Waiting (TTFB)", r.wait, {100, 255, 150, 200}},
            {"Content Download", r.download, {100, 200, 255, 200}}
        };
        
        int barX = x + 150;
        int barMaxW = 300;
        
        for (const auto& p : phases) {
            text(p.name, x, y, Theme::TEXT);
            
            int barW = total > 0 ? (int)((p.ms / total) * barMaxW) : 0;
            SDL_Rect bar = {barX, y + 2, std::max(barW, 2), 14};
            SDL_SetRenderDrawColor(renderer_, p.col.r, p.col.g, p.col.b, p.col.a);
            SDL_RenderFillRect(renderer_, &bar);
            
            text(std::to_string((int)p.ms) + " ms", barX + barW + 10, y, Theme::TEXT_DIM, true);
            
            y += 28;
        }
        
        y += 10;
        text("Total: " + std::to_string((int)r.time) + " ms", x, y, Theme::CYAN);
    }
    
    void renderSources(int x, int y) {
        // File tree
        text("📁 Sources", x, y, Theme::CYAN);
        y += 25;
        
        std::vector<std::string> files = {"index.html", "app.js", "style.css", "utils.js"};
        for (const auto& f : files) {
            text("  " + f, x, y, Theme::TEXT);
            y += 20;
        }
        
        // Code view
        int codeX = x + 180;
        text("app.js", codeX, HEADER_HEIGHT + TAB_HEIGHT + 10, Theme::VIOLET);
        
        int cy = HEADER_HEIGHT + TAB_HEIGHT + 35;
        std::vector<std::string> code = {
            "function init() {",
            "  console.log('App initialized');",
            "  setupEventListeners();",
            "}",
            "",
            "function setupEventListeners() {",
            "  document.body.addEventListener('click', onClick);",
            "}",
            "",
            "window.onload = init;"
        };
        
        for (size_t i = 0; i < code.size(); i++) {
            text(std::to_string(i + 1), codeX, cy, Theme::TEXT_MUTED, true);
            text(code[i], codeX + 35, cy, Theme::TEXT);
            cy += 18;
        }
    }
    
    void renderPerformance(int x, int y) {
        text("Performance Metrics", x, y, Theme::CYAN);
        y += 30;
        
        text("FPS: 60", x, y, Theme::GREEN);
        text("CPU: 8%", x + 100, y, Theme::GREEN);
        text("Memory: 42 MB", x + 200, y, Theme::YELLOW);
        text("Heap: 28 MB", x + 340, y, Theme::TEXT);
        
        y += 40;
        
        // Timeline
        int barX = x;
        int barW = width_ - SIDEBAR_WIDTH - 40;
        int barH = 80;
        
        SDL_Rect bg = {barX, y, barW, barH};
        setColor(Theme::INPUT);
        SDL_RenderFillRect(renderer_, &bg);
        
        srand(42);
        for (int i = 0; i < barW; i += 10) {
            int h = rand() % 60 + 5;
            SDL_Rect bar = {barX + i, y + barH - h, 8, h};
            
            if (h > 50) setColor(Theme::RED);
            else if (h > 30) setColor(Theme::YELLOW);
            else setColor(Theme::CYAN);
            
            SDL_RenderFillRect(renderer_, &bar);
        }
    }
    
    void renderMemory(int x, int y) {
        text("Heap Snapshot", x, y, Theme::CYAN);
        y += 30;
        
        text("[Take Snapshot]", x, y, Theme::VIOLET);
        y += 30;
        
        text("Snapshots:", x, y, Theme::TEXT);
        y += 25;
        
        text("  Snapshot 1  -  12.4 MB  -  10:42:15", x, y, Theme::TEXT);
        y += 22;
        text("  Snapshot 2  -  15.8 MB  -  10:43:22  (+3.4 MB)", x, y, Theme::YELLOW);
    }
    
    void renderApplication(int x, int y) {
        text("Storage", x, y, Theme::CYAN);
        y += 25;
        
        std::vector<std::string> items = {"LocalStorage", "SessionStorage", "Cookies", "IndexedDB", "Cache Storage"};
        for (const auto& i : items) {
            text("• " + i, x + 10, y, Theme::TEXT);
            y += 22;
        }
        
        // Content
        int cx = x + 200;
        text("LocalStorage (3 items)", cx, HEADER_HEIGHT + TAB_HEIGHT + 10, Theme::VIOLET);
        
        int cy = HEADER_HEIGHT + TAB_HEIGHT + 40;
        std::vector<std::pair<std::string, std::string>> data = {
            {"theme", "\"dark\""},
            {"user_id", "\"abc123\""},
            {"settings", "{...}"}
        };
        
        text("Key", cx, cy, Theme::TEXT_DIM);
        text("Value", cx + 150, cy, Theme::TEXT_DIM);
        cy += 25;
        
        for (const auto& [k, v] : data) {
            text(k, cx, cy, Theme::TEXT);
            text(v, cx + 150, cy, Theme::CYAN);
            cy += 22;
        }
    }
    
    void renderSecurity(int x, int y) {
        text("Security Overview", x, y, Theme::CYAN);
        y += 30;
        
        fillCircle(x + 8, y + 7, 6, Theme::GREEN);
        text("Connection is secure (HTTPS)", x + 22, y, Theme::GREEN);
        y += 35;
        
        text("Certificate", x, y, Theme::VIOLET);
        y += 25;
        text("  Issued to: example.com", x, y, Theme::TEXT);
        y += 20;
        text("  Issued by: Let's Encrypt", x, y, Theme::TEXT);
        y += 20;
        text("  Valid: Jan 2025 - Jan 2026", x, y, Theme::TEXT);
    }
    
    void renderStatusBar() {
        SDL_Rect s = {0, height_ - STATUS_HEIGHT, width_, STATUS_HEIGHT};
        setColor({25, 20, 38, 255});
        SDL_RenderFillRect(renderer_, &s);
        
        text("1-8: Panels | Tab: Cycle | ESC: Close", 10, height_ - 17, Theme::TEXT_MUTED, true);
        text("https://example.com", width_ - 200, height_ - 17, Theme::TEXT_DIM, true);
    }
    
    // =========================================================================
    // Helpers
    // =========================================================================
    
    void setColor(SDL_Color c) {
        SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    }
    
    void fillCircle(int cx, int cy, int r, SDL_Color col) {
        setColor(col);
        for (int y = -r; y <= r; y++) {
            for (int x = -r; x <= r; x++) {
                if (x*x + y*y <= r*r) {
                    SDL_RenderDrawPoint(renderer_, cx + x, cy + y);
                }
            }
        }
    }
    
    void text(const std::string& t, int x, int y, SDL_Color col, bool small = false) {
        if (!font_) {
            setColor(col);
            for (size_t i = 0; i < t.size() && i < 80; i++) {
                if (t[i] != ' ') {
                    SDL_Rect r = {x + (int)i * 7, y, 5, 11};
                    SDL_RenderFillRect(renderer_, &r);
                }
            }
            return;
        }
        
        TTF_Font* f = small ? (fontSmall_ ? fontSmall_ : font_) : font_;
        SDL_Surface* surf = TTF_RenderUTF8_Blended(f, t.c_str(), col);
        if (!surf) return;
        
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
        if (!tex) { SDL_FreeSurface(surf); return; }
        
        SDL_Rect dst = {x, y, surf->w, surf->h};
        SDL_RenderCopy(renderer_, tex, nullptr, &dst);
        
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
    
    void cleanup() {
        if (fontSmall_) TTF_CloseFont(fontSmall_);
        if (fontBold_) TTF_CloseFont(fontBold_);
        if (font_) TTF_CloseFont(font_);
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        TTF_Quit();
        SDL_Quit();
    }
    
private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* font_ = nullptr;
    TTF_Font* fontSmall_ = nullptr;
    TTF_Font* fontBold_ = nullptr;
    
    int width_ = 1280;
    int height_ = 720;
    int mouseX_ = 0, mouseY_ = 0;
    bool running_ = false;
    
    MainPanel panel_ = MainPanel::Network;
    NetworkTab netTab_ = NetworkTab::Headers;
    
    std::vector<ConsoleLine> console_;
    std::string input_;
    
    std::vector<Request> requests_;
    size_t selectedRequest_ = 0;
    bool detailsOpen_ = false;
    
    DOMNode root_;
    DOMNode* selectedNode_ = nullptr;
};

// =============================================================================
// Main
// =============================================================================
int main() {
    std::cout << "╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║         ZEPRA DEVTOOLS PRO                        ║\n";
    std::cout << "╠═══════════════════════════════════════════════════╣\n";
    std::cout << "║  • Sidebar navigation                             ║\n";
    std::cout << "║  • Network: Headers/Preview/Response/Timing       ║\n";
    std::cout << "║  • Elements: DOM + CSS + Box Model                ║\n";
    std::cout << "║  • Console: REPL with commands                    ║\n";
    std::cout << "║  • Click requests to see details                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n\n";
    
    DevToolsPro devtools;
    
    if (!devtools.init()) {
        std::cerr << "Failed to initialize\n";
        return 1;
    }
    
    std::cout << "Window opened! 🚀\n";
    
    devtools.run();
    devtools.cleanup();
    
    return 0;
}
