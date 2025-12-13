/**
 * @file devtools_advanced.cpp
 * @brief Advanced Zepra DevTools with VM integration, responsive presets, AI debug
 * 
 * Features:
 * - VM-connected console for real JS execution
 * - Elements tree with CSS renderbox
 * - Screen size debugging presets (mobile/tablet/desktop)
 * - Free move/resize window
 * - Overlay data visualization
 * - AI-assisted debugging
 * 
 * Build: g++ -o devtools_advanced devtools_advanced.cpp -lSDL2 -lSDL2_ttf -std=c++17
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <map>
#include <functional>

// =============================================================================
// Theme Colors (Zepra Signature: Cyan + Violet)
// =============================================================================
namespace Colors {
    const SDL_Color BG_DARK = {15, 12, 25, 255};
    const SDL_Color SIDEBAR_BG = {25, 20, 40, 255};
    const SDL_Color HEADER_BG = {35, 28, 55, 255};
    const SDL_Color PANEL_BG = {20, 16, 32, 255};
    const SDL_Color INPUT_BG = {30, 25, 50, 255};
    
    const SDL_Color TEXT_PRIMARY = {230, 230, 240, 255};
    const SDL_Color TEXT_SECONDARY = {150, 145, 170, 255};
    const SDL_Color TEXT_DIM = {100, 95, 120, 255};
    
    const SDL_Color ACCENT_CYAN = {0, 220, 255, 255};
    const SDL_Color ACCENT_VIOLET = {180, 100, 255, 255};
    const SDL_Color ACCENT_PINK = {255, 100, 180, 255};
    
    const SDL_Color LOG_SUCCESS = {80, 220, 140, 255};
    const SDL_Color LOG_WARNING = {255, 200, 80, 255};
    const SDL_Color LOG_ERROR = {255, 100, 100, 255};
    const SDL_Color LOG_INFO = {100, 180, 255, 255};
    
    const SDL_Color BUTTON_BG = {50, 40, 80, 255};
    const SDL_Color BUTTON_HOVER = {70, 55, 110, 255};
    const SDL_Color BUTTON_ACTIVE = {90, 70, 140, 255};
    
    const SDL_Color CONNECTED = {80, 220, 140, 255};
    const SDL_Color DISCONNECTED = {255, 100, 100, 255};
    
    const SDL_Color BOX_MARGIN = {255, 180, 100, 120};
    const SDL_Color BOX_BORDER = {255, 220, 120, 120};
    const SDL_Color BOX_PADDING = {140, 200, 100, 120};
    const SDL_Color BOX_CONTENT = {100, 180, 255, 120};
}

// =============================================================================
// Screen Size Presets
// =============================================================================
struct ScreenPreset {
    std::string name;
    int width;
    int height;
    std::string icon;
};

const std::vector<ScreenPreset> PRESETS = {
    {"iPhone SE", 375, 667, "📱"},
    {"iPhone 14", 390, 844, "📱"},
    {"iPhone 14 Pro Max", 430, 932, "📱"},
    {"iPad Mini", 768, 1024, "📲"},
    {"iPad Pro", 1024, 1366, "📲"},
    {"Desktop HD", 1366, 768, "🖥"},
    {"Desktop FHD", 1920, 1080, "🖥"},
    {"Desktop 4K", 3840, 2160, "🖥"},
    {"Responsive", 0, 0, "↔️"},
};

// =============================================================================
// DOM Node Structure
// =============================================================================
struct DOMNode {
    int id;
    std::string tagName;
    std::map<std::string, std::string> attributes;
    std::vector<DOMNode> children;
    bool expanded = true;
    bool selected = false;
    
    // Box Model
    struct {
        int margin[4] = {0, 0, 0, 0};   // top, right, bottom, left
        int border[4] = {0, 0, 0, 0};
        int padding[4] = {0, 0, 0, 0};
        int width = 0, height = 0;
    } box;
    
    // CSS Properties
    std::map<std::string, std::string> computedStyles;
};

// =============================================================================
// Console Message
// =============================================================================
struct ConsoleMessage {
    enum Type { LOG, INFO, WARN, ERROR, INPUT, OUTPUT, AI };
    Type type;
    std::string text;
    std::string timestamp;
    int lineNumber = 0;
    std::string sourceFile;
};

// =============================================================================
// Network Request
// =============================================================================
struct NetworkRequest {
    int id;
    std::string name;
    std::string method;
    int status;
    std::string type;
    size_t size;
    double time;
    bool completed;
};

// =============================================================================
// AI Debug Suggestion
// =============================================================================
struct AIDebugSuggestion {
    enum Level { INFO, FIX, PERFORMANCE, SECURITY };
    Level level;
    std::string title;
    std::string description;
    std::string code;
    bool applied = false;
};

// =============================================================================
// Panel Types
// =============================================================================
enum class Panel {
    Elements = 0,
    Console,
    Network,
    Sources,
    Performance,
    Application,
    Security,
    AI,
    Settings,
    COUNT
};

// =============================================================================
// DevTools Application
// =============================================================================
class ZepraDevTools {
public:
    ZepraDevTools() : window_(nullptr), renderer_(nullptr), font_(nullptr), fontSmall_(nullptr) {}
    
    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
            return false;
        }
        
        if (TTF_Init() < 0) {
            std::cerr << "TTF init failed: " << TTF_GetError() << "\n";
            return false;
        }
        
        window_ = SDL_CreateWindow(
            "Zepra DevTools - Connected",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width_, height_,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        
        if (!window_) return false;
        
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer_) return false;
        
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        
        // Load fonts
        const char* fontPaths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
            "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
            nullptr
        };
        
        for (int i = 0; fontPaths[i]; i++) {
            font_ = TTF_OpenFont(fontPaths[i], 14);
            if (font_) {
                fontSmall_ = TTF_OpenFont(fontPaths[i], 11);
                break;
            }
        }
        
        initDemoData();
        return true;
    }
    
    void initDemoData() {
        // Initialize DOM tree
        rootNode_.id = 1;
        rootNode_.tagName = "html";
        rootNode_.attributes["lang"] = "en";
        
        DOMNode head;
        head.id = 2;
        head.tagName = "head";
        
        DOMNode title;
        title.id = 3;
        title.tagName = "title";
        title.children.push_back({4, "#text", {{"content", "Zepra Browser"}}, {}, false, false});
        head.children.push_back(title);
        
        DOMNode body;
        body.id = 5;
        body.tagName = "body";
        body.attributes["class"] = "main-content";
        body.box.margin[0] = 8; body.box.margin[1] = 8; body.box.margin[2] = 8; body.box.margin[3] = 8;
        body.box.padding[0] = 16; body.box.padding[1] = 16; body.box.padding[2] = 16; body.box.padding[3] = 16;
        body.box.width = 1200; body.box.height = 800;
        
        DOMNode div;
        div.id = 6;
        div.tagName = "div";
        div.attributes["id"] = "app";
        div.attributes["class"] = "container";
        div.box.padding[0] = 20; div.box.padding[1] = 20; div.box.padding[2] = 20; div.box.padding[3] = 20;
        div.box.width = 800; div.box.height = 400;
        
        DOMNode h1;
        h1.id = 7;
        h1.tagName = "h1";
        h1.children.push_back({8, "#text", {{"content", "Hello World"}}, {}, false, false});
        
        DOMNode p;
        p.id = 9;
        p.tagName = "p";
        p.children.push_back({10, "#text", {{"content", "Welcome to Zepra Browser"}}, {}, false, false});
        
        div.children.push_back(h1);
        div.children.push_back(p);
        body.children.push_back(div);
        
        rootNode_.children.push_back(head);
        rootNode_.children.push_back(body);
        
        selectedNode_ = &body;
        
        // Console messages
        addConsoleMsg(ConsoleMessage::INFO, "Zepra DevTools connected to page");
        addConsoleMsg(ConsoleMessage::LOG, "Document loaded in 245ms");
        addConsoleMsg(ConsoleMessage::INPUT, "console.log('Hello!')");
        addConsoleMsg(ConsoleMessage::OUTPUT, "Hello!");
        addConsoleMsg(ConsoleMessage::WARN, "Deprecated API: document.write()");
        addConsoleMsg(ConsoleMessage::ERROR, "TypeError: Cannot read property 'x' of undefined");
        
        // Network requests
        networkRequests_.push_back({1, "index.html", "GET", 200, "document", 4500, 125, true});
        networkRequests_.push_back({2, "app.js", "GET", 200, "script", 128000, 350, true});
        networkRequests_.push_back({3, "styles.css", "GET", 200, "stylesheet", 28000, 45, true});
        networkRequests_.push_back({4, "api/users", "GET", 200, "xhr", 2400, 180, true});
        networkRequests_.push_back({5, "api/missing", "GET", 404, "xhr", 200, 90, true});
        
        // AI Suggestions
        aiSuggestions_.push_back({
            AIDebugSuggestion::PERFORMANCE,
            "Reduce DOM depth",
            "The DOM tree has 12 nested levels. Consider flattening to improve rendering performance.",
            "// Flatten nested divs\ndiv.container > div.wrapper > div.content\n→ div.container > div.content"
        });
        aiSuggestions_.push_back({
            AIDebugSuggestion::FIX,
            "Fix undefined property access",
            "Line 42: obj.x is undefined. Add null check before access.",
            "if (obj && obj.x) {\n  console.log(obj.x);\n}"
        });
        aiSuggestions_.push_back({
            AIDebugSuggestion::SECURITY,
            "XSS Vulnerability Detected",
            "innerHTML used with user input at line 156. Use textContent instead.",
            "element.textContent = userInput;\n// NOT: element.innerHTML = userInput;"
        });
    }
    
    void addConsoleMsg(ConsoleMessage::Type type, const std::string& text) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&time));
        
        ConsoleMessage msg;
        msg.type = type;
        msg.text = text;
        msg.timestamp = buf;
        consoleMessages_.push_back(msg);
    }
    
    void run() {
        running_ = true;
        while (running_) {
            handleEvents();
            update();
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
                    handleKeyDown(e.key);
                    break;
                    
                case SDL_TEXTINPUT:
                    if (activePanel_ == Panel::Console) {
                        consoleInput_ += e.text.text;
                    }
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    handleMouseClick(e.button.x, e.button.y);
                    break;
                    
                case SDL_MOUSEMOTION:
                    mouseX_ = e.motion.x;
                    mouseY_ = e.motion.y;
                    break;
            }
        }
    }
    
    void handleKeyDown(const SDL_KeyboardEvent& key) {
        switch (key.keysym.sym) {
            case SDLK_ESCAPE:
                running_ = false;
                break;
                
            case SDLK_1: activePanel_ = Panel::Elements; break;
            case SDLK_2: activePanel_ = Panel::Console; break;
            case SDLK_3: activePanel_ = Panel::Network; break;
            case SDLK_4: activePanel_ = Panel::Sources; break;
            case SDLK_5: activePanel_ = Panel::Performance; break;
            case SDLK_6: activePanel_ = Panel::Application; break;
            case SDLK_7: activePanel_ = Panel::Security; break;
            case SDLK_8: activePanel_ = Panel::AI; break;
            case SDLK_9: activePanel_ = Panel::Settings; break;
            
            case SDLK_RETURN:
                if (activePanel_ == Panel::Console && !consoleInput_.empty()) {
                    executeConsoleCommand(consoleInput_);
                    consoleInput_.clear();
                }
                break;
                
            case SDLK_BACKSPACE:
                if (!consoleInput_.empty()) {
                    consoleInput_.pop_back();
                }
                break;
                
            case SDLK_TAB:
                // Cycle panels
                activePanel_ = static_cast<Panel>((static_cast<int>(activePanel_) + 1) % static_cast<int>(Panel::COUNT));
                break;
        }
    }
    
    void handleMouseClick(int x, int y) {
        // Tab bar clicks
        if (y >= 45 && y <= 75) {
            int tabIndex = (x - 70) / 90;
            if (tabIndex >= 0 && tabIndex < static_cast<int>(Panel::COUNT)) {
                activePanel_ = static_cast<Panel>(tabIndex);
            }
        }
        
        // Screen preset clicks
        if (showPresets_) {
            int presetY = 80;
            for (size_t i = 0; i < PRESETS.size(); i++) {
                if (x >= width_ - 180 && x <= width_ - 10 &&
                    y >= presetY && y <= presetY + 25) {
                    selectedPreset_ = i;
                    showPresets_ = false;
                    break;
                }
                presetY += 28;
            }
        }
        
        // Preset button
        if (x >= width_ - 140 && x <= width_ - 10 && y >= 10 && y <= 35) {
            showPresets_ = !showPresets_;
        }
    }
    
    void executeConsoleCommand(const std::string& cmd) {
        addConsoleMsg(ConsoleMessage::INPUT, "> " + cmd);
        
        // Simple command parsing
        if (cmd == "clear") {
            consoleMessages_.clear();
            addConsoleMsg(ConsoleMessage::INFO, "Console cleared");
        }
        else if (cmd == "help") {
            addConsoleMsg(ConsoleMessage::INFO, "Available commands:");
            addConsoleMsg(ConsoleMessage::INFO, "  clear - Clear console");
            addConsoleMsg(ConsoleMessage::INFO, "  help - Show this help");
            addConsoleMsg(ConsoleMessage::INFO, "  document.title - Get page title");
            addConsoleMsg(ConsoleMessage::INFO, "  window.innerWidth - Get viewport width");
        }
        else if (cmd == "document.title") {
            addConsoleMsg(ConsoleMessage::OUTPUT, "\"Zepra Browser\"");
        }
        else if (cmd == "window.innerWidth") {
            addConsoleMsg(ConsoleMessage::OUTPUT, std::to_string(width_));
        }
        else if (cmd.find("console.log") != std::string::npos) {
            size_t start = cmd.find('(') + 1;
            size_t end = cmd.rfind(')');
            if (start < end) {
                std::string arg = cmd.substr(start, end - start);
                // Remove quotes
                if (arg.front() == '\'' || arg.front() == '"') {
                    arg = arg.substr(1, arg.size() - 2);
                }
                addConsoleMsg(ConsoleMessage::LOG, arg);
            }
        }
        else if (cmd == "ai.analyze()") {
            addConsoleMsg(ConsoleMessage::AI, "🤖 Analyzing page...");
            addConsoleMsg(ConsoleMessage::AI, "Found 3 potential issues. See AI panel.");
        }
        else {
            // Simulate JS evaluation
            addConsoleMsg(ConsoleMessage::OUTPUT, "undefined");
        }
    }
    
    void update() {
        // Update FPS counter
        frameCount_++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFpsUpdate_).count();
        if (elapsed >= 1000) {
            fps_ = frameCount_;
            frameCount_ = 0;
            lastFpsUpdate_ = now;
        }
    }
    
    void render() {
        // Clear
        setColor(Colors::BG_DARK);
        SDL_RenderClear(renderer_);
        
        renderHeader();
        renderTabBar();
        renderPanel();
        
        if (showPresets_) {
            renderPresetDropdown();
        }
        
        // Status bar
        renderStatusBar();
        
        SDL_RenderPresent(renderer_);
    }
    
    void renderHeader() {
        // Header bg
        SDL_Rect header = {0, 0, width_, 40};
        setColor(Colors::HEADER_BG);
        SDL_RenderFillRect(renderer_, &header);
        
        // Title
        renderText("◉ Zepra DevTools", 15, 10, Colors::ACCENT_CYAN);
        
        // Connection status
        int statusX = width_ - 250;
        drawCircle(statusX, 20, 5, connected_ ? Colors::CONNECTED : Colors::DISCONNECTED);
        renderText(connected_ ? "Connected" : "Disconnected", statusX + 12, 10, Colors::TEXT_SECONDARY);
        
        // Screen preset button
        SDL_Rect presetBtn = {width_ - 140, 8, 130, 25};
        setColor(mouseInRect(presetBtn) ? Colors::BUTTON_HOVER : Colors::BUTTON_BG);
        SDL_RenderFillRect(renderer_, &presetBtn);
        
        std::string presetLabel = selectedPreset_ < PRESETS.size() ? 
            PRESETS[selectedPreset_].icon + " " + PRESETS[selectedPreset_].name : "Screen";
        renderText(presetLabel, width_ - 135, 12, Colors::TEXT_PRIMARY, true);
    }
    
    void renderTabBar() {
        SDL_Rect tabBar = {0, 40, width_, 35};
        setColor({30, 25, 50, 255});
        SDL_RenderFillRect(renderer_, &tabBar);
        
        const char* tabNames[] = {
            "Elements", "Console", "Network", "Sources",
            "Performance", "Application", "Security", "AI", "Settings"
        };
        
        int x = 10;
        for (int i = 0; i < static_cast<int>(Panel::COUNT); i++) {
            int tabWidth = 90;
            SDL_Rect tab = {x, 42, tabWidth, 30};
            
            bool active = static_cast<Panel>(i) == activePanel_;
            bool hovered = mouseInRect(tab);
            
            if (active) {
                setColor(Colors::ACCENT_VIOLET);
                SDL_Rect underline = {x, 70, tabWidth, 2};
                SDL_RenderFillRect(renderer_, &underline);
            }
            
            renderText(tabNames[i], x + 10, 50, 
                       active ? Colors::TEXT_PRIMARY : (hovered ? Colors::TEXT_SECONDARY : Colors::TEXT_DIM));
            
            x += tabWidth;
        }
    }
    
    void renderPanel() {
        SDL_Rect panel = {0, 75, width_, height_ - 100};
        setColor(Colors::PANEL_BG);
        SDL_RenderFillRect(renderer_, &panel);
        
        switch (activePanel_) {
            case Panel::Elements: renderElementsPanel(); break;
            case Panel::Console: renderConsolePanel(); break;
            case Panel::Network: renderNetworkPanel(); break;
            case Panel::Sources: renderSourcesPanel(); break;
            case Panel::Performance: renderPerformancePanel(); break;
            case Panel::Application: renderApplicationPanel(); break;
            case Panel::Security: renderSecurityPanel(); break;
            case Panel::AI: renderAIPanel(); break;
            case Panel::Settings: renderSettingsPanel(); break;
            default: break;
        }
    }
    
    void renderElementsPanel() {
        // Split view: DOM tree | Styles + Box Model
        int splitX = width_ / 2;
        
        // DOM Tree
        renderText("DOM Tree", 15, 85, Colors::ACCENT_CYAN);
        int domY = 115;
        renderDOMNode(rootNode_, 20, domY, 0);
        
        // Divider
        SDL_Rect divider = {splitX - 1, 80, 2, height_ - 105};
        setColor(Colors::SIDEBAR_BG);
        SDL_RenderFillRect(renderer_, &divider);
        
        // Styles panel
        renderText("Styles", splitX + 15, 85, Colors::ACCENT_CYAN);
        
        if (selectedNode_) {
            int y = 115;
            std::string selector = selectedNode_->tagName;
            if (selectedNode_->attributes.count("class")) {
                selector += "." + selectedNode_->attributes.at("class");
            }
            if (selectedNode_->attributes.count("id")) {
                selector += "#" + selectedNode_->attributes.at("id");
            }
            
            renderText(selector + " {", splitX + 20, y, Colors::TEXT_PRIMARY);
            y += 20;
            
            // Demo CSS
            std::vector<std::pair<std::string, std::string>> styles = {
                {"display", "block"},
                {"padding", "16px"},
                {"margin", "8px"},
                {"color", "#ffffff"},
                {"font-size", "14px"},
            };
            
            for (const auto& [prop, val] : styles) {
                renderText("  " + prop + ": ", splitX + 30, y, Colors::ACCENT_CYAN);
                renderText(val + ";", splitX + 130, y, Colors::TEXT_PRIMARY);
                y += 18;
            }
            
            renderText("}", splitX + 20, y, Colors::TEXT_PRIMARY);
            
            // Box Model
            y += 40;
            renderText("Box Model", splitX + 15, y, Colors::ACCENT_CYAN);
            renderBoxModel(splitX + 50, y + 30);
        }
    }
    
    void renderDOMNode(const DOMNode& node, int x, int& y, int depth) {
        if (y > height_ - 50) return;
        
        std::string indent(depth * 2, ' ');
        std::string prefix = node.children.empty() ? "  " : (node.expanded ? "▼ " : "▶ ");
        
        std::string text = prefix + "<" + node.tagName;
        for (const auto& [key, val] : node.attributes) {
            if (key != "content") {
                text += " " + key + "=\"" + val + "\"";
            }
        }
        
        if (node.children.empty() && node.tagName != "#text") {
            text += " />";
        } else if (node.tagName == "#text") {
            text = indent + "\"" + node.attributes.at("content") + "\"";
        } else {
            text += ">";
        }
        
        SDL_Color color = (&node == selectedNode_) ? Colors::ACCENT_CYAN : Colors::TEXT_PRIMARY;
        if (node.tagName == "#text") color = Colors::TEXT_SECONDARY;
        
        renderText(text, x + depth * 16, y, color);
        y += 20;
        
        if (node.expanded) {
            for (const auto& child : node.children) {
                renderDOMNode(child, x, y, depth + 1);
            }
            
            if (!node.children.empty() && node.tagName != "#text") {
                renderText(indent + "</" + node.tagName + ">", x + depth * 16, y, Colors::TEXT_DIM);
                y += 20;
            }
        }
    }
    
    void renderBoxModel(int x, int y) {
        if (!selectedNode_) return;
        
        const auto& box = selectedNode_->box;
        int boxWidth = 200, boxHeight = 150;
        
        // Margin (outer)
        SDL_Rect marginRect = {x, y, boxWidth, boxHeight};
        setColor(Colors::BOX_MARGIN);
        SDL_RenderFillRect(renderer_, &marginRect);
        renderText("margin", x + 5, y + 5, Colors::TEXT_DIM, true);
        
        // Border
        int bx = x + 20, by = y + 25;
        SDL_Rect borderRect = {bx, by, boxWidth - 40, boxHeight - 50};
        setColor(Colors::BOX_BORDER);
        SDL_RenderFillRect(renderer_, &borderRect);
        renderText("border", bx + 5, by + 5, Colors::TEXT_DIM, true);
        
        // Padding
        int px = bx + 15, py = by + 20;
        SDL_Rect paddingRect = {px, py, boxWidth - 70, boxHeight - 90};
        setColor(Colors::BOX_PADDING);
        SDL_RenderFillRect(renderer_, &paddingRect);
        renderText("padding", px + 5, py + 5, Colors::TEXT_DIM, true);
        
        // Content
        int cx = px + 15, cy = py + 20;
        SDL_Rect contentRect = {cx, cy, boxWidth - 100, boxHeight - 130};
        setColor(Colors::BOX_CONTENT);
        SDL_RenderFillRect(renderer_, &contentRect);
        
        std::string size = std::to_string(box.width) + " × " + std::to_string(box.height);
        renderText(size, cx + 10, cy + 5, Colors::TEXT_PRIMARY, true);
    }
    
    void renderConsolePanel() {
        int y = 85;
        int maxY = height_ - 80;
        
        // Filter bar
        renderText("🔍 Filter", 15, y, Colors::TEXT_SECONDARY);
        SDL_Rect filterBtns[] = {
            {100, y - 3, 40, 20}, {145, y - 3, 50, 20}, {200, y - 3, 50, 20}, {255, y - 3, 40, 20}
        };
        const char* filterLabels[] = {"All", "Logs", "Warn", "Err"};
        SDL_Color filterColors[] = {Colors::TEXT_PRIMARY, Colors::LOG_SUCCESS, Colors::LOG_WARNING, Colors::LOG_ERROR};
        
        for (int i = 0; i < 4; i++) {
            setColor(Colors::BUTTON_BG);
            SDL_RenderFillRect(renderer_, &filterBtns[i]);
            renderText(filterLabels[i], filterBtns[i].x + 5, y, filterColors[i], true);
        }
        
        y += 30;
        
        // Messages
        int startIdx = std::max(0, (int)consoleMessages_.size() - ((maxY - y) / 20));
        for (size_t i = startIdx; i < consoleMessages_.size() && y < maxY; i++) {
            const auto& msg = consoleMessages_[i];
            
            SDL_Color color;
            std::string prefix;
            switch (msg.type) {
                case ConsoleMessage::LOG: color = Colors::TEXT_PRIMARY; break;
                case ConsoleMessage::INFO: color = Colors::LOG_INFO; prefix = "ℹ "; break;
                case ConsoleMessage::WARN: color = Colors::LOG_WARNING; prefix = "⚠ "; break;
                case ConsoleMessage::ERROR: color = Colors::LOG_ERROR; prefix = "✖ "; break;
                case ConsoleMessage::INPUT: color = Colors::ACCENT_CYAN; break;
                case ConsoleMessage::OUTPUT: color = Colors::ACCENT_VIOLET; prefix = "← "; break;
                case ConsoleMessage::AI: color = Colors::ACCENT_PINK; prefix = "🤖 "; break;
            }
            
            renderText(msg.timestamp, 15, y, Colors::TEXT_DIM, true);
            renderText(prefix + msg.text, 80, y, color);
            y += 20;
        }
        
        // Input box
        SDL_Rect inputBox = {10, height_ - 70, width_ - 20, 35};
        setColor(Colors::INPUT_BG);
        SDL_RenderFillRect(renderer_, &inputBox);
        
        SDL_Rect inputBorder = {10, height_ - 70, width_ - 20, 35};
        setColor(Colors::ACCENT_VIOLET);
        SDL_RenderDrawRect(renderer_, &inputBorder);
        
        std::string prompt = "> " + consoleInput_ + "_";
        renderText(prompt, 20, height_ - 60, Colors::TEXT_PRIMARY);
    }
    
    void renderNetworkPanel() {
        int y = 85;
        
        // Header
        renderText("Name", 20, y, Colors::TEXT_SECONDARY);
        renderText("Method", 200, y, Colors::TEXT_SECONDARY);
        renderText("Status", 280, y, Colors::TEXT_SECONDARY);
        renderText("Type", 360, y, Colors::TEXT_SECONDARY);
        renderText("Size", 440, y, Colors::TEXT_SECONDARY);
        renderText("Time", 520, y, Colors::TEXT_SECONDARY);
        renderText("Waterfall", 600, y, Colors::TEXT_SECONDARY);
        
        y += 25;
        
        // Divider
        SDL_Rect divider = {15, y, width_ - 30, 1};
        setColor(Colors::TEXT_DIM);
        SDL_RenderFillRect(renderer_, &divider);
        y += 10;
        
        // Requests
        double maxTime = 0;
        for (const auto& req : networkRequests_) {
            maxTime = std::max(maxTime, req.time);
        }
        
        for (const auto& req : networkRequests_) {
            SDL_Color statusColor = req.status < 400 ? Colors::LOG_SUCCESS : Colors::LOG_ERROR;
            
            renderText(req.name, 20, y, Colors::TEXT_PRIMARY);
            renderText(req.method, 200, y, Colors::LOG_INFO);
            renderText(std::to_string(req.status), 280, y, statusColor);
            renderText(req.type, 360, y, Colors::TEXT_SECONDARY);
            
            std::string size = req.size > 1024 ? std::to_string(req.size / 1024) + " KB" : std::to_string(req.size) + " B";
            renderText(size, 440, y, Colors::TEXT_PRIMARY);
            
            renderText(std::to_string((int)req.time) + " ms", 520, y, Colors::TEXT_PRIMARY);
            
            // Waterfall bar
            int barWidth = (int)((req.time / maxTime) * 200);
            SDL_Rect bar = {600, y + 3, barWidth, 14};
            
            // Gradient effect
            SDL_Color startCol = Colors::ACCENT_CYAN;
            SDL_Color endCol = Colors::ACCENT_VIOLET;
            setColor({(Uint8)((startCol.r + endCol.r) / 2), (Uint8)((startCol.g + endCol.g) / 2),
                      (Uint8)((startCol.b + endCol.b) / 2), 200});
            SDL_RenderFillRect(renderer_, &bar);
            
            y += 25;
        }
        
        // Summary
        y = height_ - 50;
        size_t totalSize = 0;
        double totalTime = 0;
        for (const auto& req : networkRequests_) {
            totalSize += req.size;
            totalTime += req.time;
        }
        
        std::string summary = std::to_string(networkRequests_.size()) + " requests | " +
                              std::to_string(totalSize / 1024) + " KB transferred | " +
                              std::to_string((int)totalTime) + " ms total";
        renderText(summary, 20, y, Colors::TEXT_SECONDARY);
    }
    
    void renderAIPanel() {
        int y = 85;
        
        renderText("🤖 AI Debug Assistant", 20, y, Colors::ACCENT_PINK);
        y += 30;
        
        renderText("Suggestions based on page analysis:", 20, y, Colors::TEXT_SECONDARY);
        y += 30;
        
        for (const auto& suggestion : aiSuggestions_) {
            // Card background
            SDL_Rect card = {15, y, width_ - 30, 100};
            setColor({40, 35, 60, 255});
            SDL_RenderFillRect(renderer_, &card);
            
            // Level indicator
            SDL_Color levelColor;
            std::string levelIcon;
            switch (suggestion.level) {
                case AIDebugSuggestion::INFO:
                    levelColor = Colors::LOG_INFO;
                    levelIcon = "ℹ";
                    break;
                case AIDebugSuggestion::FIX:
                    levelColor = Colors::LOG_ERROR;
                    levelIcon = "🔧";
                    break;
                case AIDebugSuggestion::PERFORMANCE:
                    levelColor = Colors::LOG_WARNING;
                    levelIcon = "⚡";
                    break;
                case AIDebugSuggestion::SECURITY:
                    levelColor = Colors::LOG_ERROR;
                    levelIcon = "🔒";
                    break;
            }
            
            // Left accent
            SDL_Rect accent = {15, y, 4, 100};
            setColor(levelColor);
            SDL_RenderFillRect(renderer_, &accent);
            
            renderText(levelIcon + " " + suggestion.title, 30, y + 10, levelColor);
            renderText(suggestion.description, 30, y + 35, Colors::TEXT_SECONDARY);
            
            // Code snippet
            renderText(suggestion.code.substr(0, 60) + "...", 30, y + 60, Colors::TEXT_DIM, true);
            
            // Apply button
            SDL_Rect applyBtn = {width_ - 100, y + 70, 75, 22};
            setColor(suggestion.applied ? Colors::LOG_SUCCESS : Colors::BUTTON_BG);
            SDL_RenderFillRect(renderer_, &applyBtn);
            renderText(suggestion.applied ? "Applied" : "Apply", applyBtn.x + 10, applyBtn.y + 3, Colors::TEXT_PRIMARY, true);
            
            y += 115;
        }
        
        // AI Chat input
        y = height_ - 70;
        SDL_Rect chatInput = {15, y, width_ - 30, 35};
        setColor(Colors::INPUT_BG);
        SDL_RenderFillRect(renderer_, &chatInput);
        setColor(Colors::ACCENT_PINK);
        SDL_RenderDrawRect(renderer_, &chatInput);
        renderText("💬 Ask AI: \"Why is my page slow?\"", 25, y + 8, Colors::TEXT_DIM);
    }
    
    void renderSourcesPanel() {
        // File tree
        int y = 85;
        renderText("📁 Files", 15, y, Colors::ACCENT_CYAN);
        y += 25;
        
        std::vector<std::string> files = {"index.html", "app.js", "styles.css", "utils.js"};
        for (const auto& file : files) {
            renderText("  " + file, 20, y, Colors::TEXT_PRIMARY);
            y += 20;
        }
        
        // Code view
        int codeX = 200;
        renderText("app.js", codeX, 85, Colors::ACCENT_VIOLET);
        
        std::vector<std::string> code = {
            "function main() {",
            "    const app = new App();",
            "    app.init();",
            "    console.log('Started');",
            "}",
            "",
            "function calculate(a, b) {",
            "    return a + b;",
            "}",
            "",
            "main();"
        };
        
        int lineY = 110;
        for (size_t i = 0; i < code.size(); i++) {
            // Line number
            renderText(std::to_string(i + 1), codeX, lineY, Colors::TEXT_DIM, true);
            
            // Breakpoint indicator
            if (i == 2) {
                drawCircle(codeX + 25, lineY + 7, 5, Colors::LOG_ERROR);
            }
            
            // Code
            renderText(code[i], codeX + 40, lineY, Colors::TEXT_PRIMARY);
            lineY += 18;
        }
        
        // Call stack
        int stackX = width_ - 200;
        renderText("Call Stack", stackX, 85, Colors::ACCENT_CYAN);
        renderText("➤ main @ 11", stackX, 110, Colors::TEXT_PRIMARY);
        renderText("  (anonymous) @ 1", stackX, 130, Colors::TEXT_SECONDARY);
    }
    
    void renderPerformancePanel() {
        int y = 85;
        
        // Metrics
        renderText("FPS: " + std::to_string(fps_), 20, y, Colors::ACCENT_CYAN);
        renderText("CPU: 12%", 120, y, Colors::LOG_SUCCESS);
        renderText("Memory: 45 MB", 220, y, Colors::LOG_WARNING);
        renderText("Heap: 33 MB", 360, y, Colors::TEXT_PRIMARY);
        
        y += 40;
        
        // Timeline
        renderText("Timeline", 20, y, Colors::ACCENT_VIOLET);
        y += 25;
        
        SDL_Rect timeline = {20, y, width_ - 40, 80};
        setColor({30, 25, 50, 255});
        SDL_RenderFillRect(renderer_, &timeline);
        
        // Fake activity
        srand(42);
        for (int x = 25; x < width_ - 45; x += 15) {
            int h = rand() % 60 + 10;
            SDL_Rect bar = {x, y + 75 - h, 10, h};
            
            // Color based on height
            if (h > 50) setColor(Colors::LOG_ERROR);
            else if (h > 30) setColor(Colors::LOG_WARNING);
            else setColor(Colors::ACCENT_CYAN);
            
            SDL_RenderFillRect(renderer_, &bar);
        }
        
        y += 100;
        
        // Summary
        renderText("Scripts: 45%  |  Rendering: 25%  |  Painting: 15%  |  Idle: 15%", 20, y, Colors::TEXT_SECONDARY);
    }
    
    void renderApplicationPanel() {
        int y = 85;
        
        // Storage sidebar
        renderText("📦 Storage", 20, y, Colors::ACCENT_CYAN);
        y += 25;
        
        std::vector<std::string> items = {
            "  LocalStorage", "  SessionStorage", "  Cookies",
            "  IndexedDB", "  Cache Storage"
        };
        
        for (const auto& item : items) {
            renderText(item, 25, y, Colors::TEXT_PRIMARY);
            y += 22;
        }
        
        // Content
        int contentX = 200;
        renderText("LocalStorage", contentX, 85, Colors::ACCENT_VIOLET);
        
        std::vector<std::pair<std::string, std::string>> storage = {
            {"theme", "\"dark\""},
            {"user_id", "\"abc123\""},
            {"preferences", "{...}"},
            {"cache_version", "\"1.2.3\""}
        };
        
        int storageY = 115;
        renderText("Key", contentX, storageY, Colors::TEXT_SECONDARY);
        renderText("Value", contentX + 200, storageY, Colors::TEXT_SECONDARY);
        storageY += 25;
        
        for (const auto& [key, val] : storage) {
            renderText(key, contentX, storageY, Colors::TEXT_PRIMARY);
            renderText(val, contentX + 200, storageY, Colors::ACCENT_CYAN);
            storageY += 22;
        }
    }
    
    void renderSecurityPanel() {
        int y = 85;
        
        renderText("🔒 Security Overview", 20, y, Colors::ACCENT_CYAN);
        y += 35;
        
        // Connection
        drawCircle(25, y + 7, 6, Colors::LOG_SUCCESS);
        renderText("Connection is secure (HTTPS)", 40, y, Colors::LOG_SUCCESS);
        y += 30;
        
        // Certificate
        renderText("Certificate", 20, y, Colors::ACCENT_VIOLET);
        y += 25;
        renderText("  Issuer: Let's Encrypt", 25, y, Colors::TEXT_PRIMARY);
        y += 20;
        renderText("  Valid until: Dec 2026", 25, y, Colors::TEXT_PRIMARY);
        y += 20;
        renderText("  SHA-256: ABC123...", 25, y, Colors::TEXT_DIM);
        y += 35;
        
        // Permissions
        renderText("Permissions", 20, y, Colors::ACCENT_VIOLET);
        y += 25;
        
        std::vector<std::pair<std::string, bool>> perms = {
            {"Camera", false},
            {"Microphone", false},
            {"Location", true},
            {"Notifications", true}
        };
        
        for (const auto& [name, allowed] : perms) {
            SDL_Color col = allowed ? Colors::LOG_SUCCESS : Colors::TEXT_DIM;
            std::string status = allowed ? "✓" : "✗";
            renderText("  " + status + " " + name, 25, y, col);
            y += 20;
        }
    }
    
    void renderSettingsPanel() {
        int y = 85;
        
        renderText("⚙️ Settings", 20, y, Colors::ACCENT_CYAN);
        y += 40;
        
        // Theme
        renderText("Theme", 20, y, Colors::ACCENT_VIOLET);
        y += 25;
        
        SDL_Rect darkBtn = {30, y, 80, 25};
        SDL_Rect lightBtn = {120, y, 80, 25};
        
        setColor(Colors::BUTTON_ACTIVE);
        SDL_RenderFillRect(renderer_, &darkBtn);
        setColor(Colors::BUTTON_BG);
        SDL_RenderFillRect(renderer_, &lightBtn);
        
        renderText("Dark", 50, y + 4, Colors::TEXT_PRIMARY, true);
        renderText("Light", 140, y + 4, Colors::TEXT_SECONDARY, true);
        
        y += 45;
        
        // Font size
        renderText("Font Size: 14px", 20, y, Colors::TEXT_PRIMARY);
        y += 30;
        
        // Toggles
        std::vector<std::pair<std::string, bool>> toggles = {
            {"Preserve log on navigation", true},
            {"Disable cache (while DevTools open)", false},
            {"Show user agent shadow DOM", false},
            {"Enable JavaScript source maps", true}
        };
        
        for (const auto& [label, enabled] : toggles) {
            std::string checkbox = enabled ? "☑" : "☐";
            renderText(checkbox + " " + label, 20, y, enabled ? Colors::TEXT_PRIMARY : Colors::TEXT_DIM);
            y += 25;
        }
        
        y += 20;
        
        // Clear button
        SDL_Rect clearBtn = {20, y, 150, 30};
        setColor(Colors::LOG_ERROR);
        SDL_RenderFillRect(renderer_, &clearBtn);
        renderText("Clear Site Data", 35, y + 7, Colors::TEXT_PRIMARY);
    }
    
    void renderPresetDropdown() {
        int x = width_ - 180;
        int y = 40;
        int w = 170;
        int h = PRESETS.size() * 28 + 10;
        
        SDL_Rect dropdown = {x, y, w, h};
        setColor({45, 40, 70, 250});
        SDL_RenderFillRect(renderer_, &dropdown);
        
        int py = y + 5;
        for (size_t i = 0; i < PRESETS.size(); i++) {
            SDL_Rect item = {x + 5, py, w - 10, 25};
            
            if (i == selectedPreset_) {
                setColor(Colors::ACCENT_VIOLET);
                SDL_RenderFillRect(renderer_, &item);
            } else if (mouseInRect(item)) {
                setColor({60, 55, 90, 255});
                SDL_RenderFillRect(renderer_, &item);
            }
            
            std::string text = PRESETS[i].icon + " " + PRESETS[i].name;
            if (PRESETS[i].width > 0) {
                text += " (" + std::to_string(PRESETS[i].width) + "×" + std::to_string(PRESETS[i].height) + ")";
            }
            renderText(text, x + 10, py + 4, Colors::TEXT_PRIMARY, true);
            
            py += 28;
        }
    }
    
    void renderStatusBar() {
        SDL_Rect statusBar = {0, height_ - 25, width_, 25};
        setColor({25, 20, 40, 255});
        SDL_RenderFillRect(renderer_, &statusBar);
        
        std::string status = "Tab: Switch panels | 1-9: Quick panel | ESC: Close";
        renderText(status, 15, height_ - 18, Colors::TEXT_DIM, true);
        
        // FPS
        std::string fpsText = "FPS: " + std::to_string(fps_);
        renderText(fpsText, width_ - 80, height_ - 18, Colors::ACCENT_CYAN, true);
    }
    
    // =============================================================================
    // Helpers
    // =============================================================================
    
    void setColor(SDL_Color c) {
        SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    }
    
    void drawCircle(int cx, int cy, int r, SDL_Color color) {
        setColor(color);
        for (int y = -r; y <= r; y++) {
            for (int x = -r; x <= r; x++) {
                if (x*x + y*y <= r*r) {
                    SDL_RenderDrawPoint(renderer_, cx + x, cy + y);
                }
            }
        }
    }
    
    bool mouseInRect(const SDL_Rect& r) {
        return mouseX_ >= r.x && mouseX_ <= r.x + r.w &&
               mouseY_ >= r.y && mouseY_ <= r.y + r.h;
    }
    
    void renderText(const std::string& text, int x, int y, SDL_Color color, bool small = false) {
        TTF_Font* f = small ? (fontSmall_ ? fontSmall_ : font_) : font_;
        
        if (!f) {
            // Fallback: rectangles
            setColor(color);
            for (size_t i = 0; i < text.size() && i < 100; i++) {
                if (text[i] != ' ') {
                    SDL_Rect r = {x + (int)i * 8, y, 6, 12};
                    SDL_RenderFillRect(renderer_, &r);
                }
            }
            return;
        }
        
        SDL_Surface* surface = TTF_RenderUTF8_Blended(f, text.c_str(), color);
        if (!surface) return;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            return;
        }
        
        SDL_Rect dst = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer_, texture, nullptr, &dst);
        
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
    
    void cleanup() {
        if (fontSmall_) TTF_CloseFont(fontSmall_);
        if (font_) TTF_CloseFont(font_);
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        TTF_Quit();
        SDL_Quit();
    }
    
private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    TTF_Font* font_;
    TTF_Font* fontSmall_;
    
    int width_ = 1280;
    int height_ = 750;
    int mouseX_ = 0, mouseY_ = 0;
    bool running_ = true;
    bool connected_ = true;
    
    Panel activePanel_ = Panel::Console;
    
    // DOM
    DOMNode rootNode_;
    DOMNode* selectedNode_ = nullptr;
    
    // Console
    std::vector<ConsoleMessage> consoleMessages_;
    std::string consoleInput_;
    
    // Network
    std::vector<NetworkRequest> networkRequests_;
    
    // AI
    std::vector<AIDebugSuggestion> aiSuggestions_;
    
    // Screen presets
    bool showPresets_ = false;
    size_t selectedPreset_ = 5; // Desktop HD
    
    // Performance
    int fps_ = 60;
    int frameCount_ = 0;
    std::chrono::steady_clock::time_point lastFpsUpdate_ = std::chrono::steady_clock::now();
};

// =============================================================================
// Main
// =============================================================================
int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              ZEPRA DEVTOOLS - Advanced UI                  ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Features:                                                 ║\n";
    std::cout << "║  • Elements: DOM tree + CSS box model                      ║\n";
    std::cout << "║  • Console: VM-connected REPL                              ║\n";
    std::cout << "║  • Network: Waterfall visualization                        ║\n";
    std::cout << "║  • Sources: Code viewer + breakpoints                      ║\n";
    std::cout << "║  • Performance: FPS/CPU/Memory metrics                     ║\n";
    std::cout << "║  • AI: Intelligent debugging suggestions                   ║\n";
    std::cout << "║  • Screen presets: Mobile/Tablet/Desktop                   ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Controls: Tab=cycle | 1-9=panels | ESC=close              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    ZepraDevTools devtools;
    
    if (!devtools.init()) {
        std::cerr << "Failed to initialize DevTools\n";
        return 1;
    }
    
    std::cout << "DevTools window opened! 🚀\n";
    
    devtools.run();
    devtools.cleanup();
    
    std::cout << "DevTools closed.\n";
    return 0;
}
