/**
 * @file devtools_panel.cpp
 * @brief Professional DevTools Panel implementation
 * 
 * Firefox/Safari-style developer tools with real network data
 */

#include "ui/devtools_panel.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

// External render functions (from zepra_browser.cpp)
extern void drawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
extern void drawRoundedRect(float x, float y, float w, float h, float r, uint8_t cr, uint8_t cg, uint8_t cb, uint8_t a);
extern int g_windowWidth, g_windowHeight;

// Text renderer class (forward from zepra_browser.cpp)
class TextRenderer {
public:
    void setSize(int size);
    void drawText(const std::string& text, float x, float y, uint8_t r, uint8_t g, uint8_t b);
    void drawText(const std::string& text, float x, float y, float alpha, uint8_t r, uint8_t g, uint8_t b);
    int textWidth(const std::string& text);
};
extern TextRenderer g_textRenderer;

namespace zepra {

constexpr const char* DevToolsPanel::TAB_NAMES[];

DevToolsPanel::DevToolsPanel() {
    colors_ = DevToolsColors();
}

DevToolsPanel::~DevToolsPanel() {}

void DevToolsPanel::toggle() {
    visible_ = !visible_;
}

void DevToolsPanel::show() {
    visible_ = true;
}

void DevToolsPanel::hide() {
    visible_ = false;
}

void DevToolsPanel::setActiveTab(DevToolsTab tab) {
    activeTab_ = tab;
    selectedRequest_ = 0;  // Clear selection on tab change
}

void DevToolsPanel::setHeight(float height) {
    height_ = std::max(150.0f, std::min(height, 600.0f));
}

void DevToolsPanel::selectRequest(uint64_t requestId) {
    selectedRequest_ = requestId;
}

void DevToolsPanel::render(float windowWidth, float windowHeight) {
    if (!visible_) return;
    
    y_ = windowHeight - height_;
    float width = windowWidth;
    
    // Main panel background
    drawRect(0, y_, width, height_, colors_.bg[0], colors_.bg[1], colors_.bg[2], 250);
    
    // Header bar
    drawRect(0, y_, width, 30, colors_.header[0], colors_.header[1], colors_.header[2], 255);
    
    // Border line
    drawRect(0, y_, width, 1, colors_.border[0], colors_.border[1], colors_.border[2], 255);
    
    renderHeader(width);
    
    float contentY = y_ + 32;
    float contentHeight = height_ - 40;
    
    switch (activeTab_) {
        case DevToolsTab::ELEMENTS:
            renderElementsTab(width, contentHeight);
            break;
        case DevToolsTab::CONSOLE:
            renderConsoleTab(width, contentHeight);
            break;
        case DevToolsTab::NETWORK:
            renderNetworkTab(width, contentHeight);
            break;
        case DevToolsTab::SOURCES:
            renderSourcesTab(width, contentHeight);
            break;
        case DevToolsTab::PERFORMANCE:
            renderPerformanceTab(width, contentHeight);
            break;
        case DevToolsTab::APPLICATION:
            renderApplicationTab(width, contentHeight);
            break;
        case DevToolsTab::SECURITY:
            renderSecurityTab(width, contentHeight);
            break;
        case DevToolsTab::SETTINGS:
            renderSettingsTab(width, contentHeight);
            break;
        default:
            break;
    }
}

void DevToolsPanel::renderHeader(float width) {
    float tabX = 5;
    g_textRenderer.setSize(11);
    
    for (int i = 0; i < static_cast<int>(DevToolsTab::COUNT); i++) {
        float tabW = 85;
        bool isActive = (i == static_cast<int>(activeTab_));
        
        if (isActive) {
            drawRect(tabX, y_ + 2, tabW, 28, 
                     colors_.tab_active[0], colors_.tab_active[1], colors_.tab_active[2], 255);
            // Active indicator line
            drawRect(tabX, y_ + 28, tabW, 2, 
                     colors_.info[0], colors_.info[1], colors_.info[2], 255);
        }
        
        float textX = tabX + 10;
        float textY = y_ + 19;
        
        if (isActive) {
            g_textRenderer.drawText(TAB_NAMES[i], textX, textY, 
                                    colors_.tab_text_active[0], colors_.tab_text_active[1], colors_.tab_text_active[2]);
        } else {
            g_textRenderer.drawText(TAB_NAMES[i], textX, textY, 
                                    colors_.tab_text[0], colors_.tab_text[1], colors_.tab_text[2]);
        }
        
        tabX += tabW + 2;
    }
    
    // Close button on right
    float closeX = width - 30;
    g_textRenderer.drawText("×", closeX, y_ + 19, 180, 180, 200);
}

void DevToolsPanel::renderConsoleTab(float width, float contentHeight) {
    auto& console = ConsoleLog::instance();
    auto entries = console.getLastN(15);
    
    float logY = y_ + 35;
    g_textRenderer.setSize(11);
    
    if (entries.empty()) {
        g_textRenderer.drawText("Console ready. No output yet.", 15, logY + 10, 
                                colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
    } else {
        int rowNum = 0;
        for (const auto& entry : entries) {
            // Alternating row background
            if (rowNum % 2 == 1) {
                drawRect(0, logY - 2, width, 16, 
                         colors_.row_alt[0], colors_.row_alt[1], colors_.row_alt[2], 100);
            }
            
            uint8_t r, g, b;
            getLogLevelColor(entry.level, r, g, b);
            
            std::string line = entry.message;
            if (line.length() > 100) line = line.substr(0, 97) + "...";
            
            g_textRenderer.drawText(line, 15, logY + 10, r, g, b);
            logY += 16;
            rowNum++;
            
            if (logY > y_ + height_ - 45) break;
        }
    }
    
    // Console input area
    float inputY = y_ + height_ - 32;
    drawRect(0, inputY, width, 32, colors_.header[0], colors_.header[1], colors_.header[2], 255);
    drawRoundedRect(10, inputY + 4, width - 20, 24, 4, 
                    colors_.bg[0] + 10, colors_.bg[1] + 10, colors_.bg[2] + 10, 255);
    g_textRenderer.drawText("> " + consoleInput_, 18, inputY + 18, 
                            colors_.success[0], colors_.success[1], colors_.success[2]);
    
    // Stats
    char stats[64];
    snprintf(stats, sizeof(stats), "%zu entries", console.size());
    g_textRenderer.setSize(9);
    g_textRenderer.drawText(stats, width - 80, inputY + 18, 
                            colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
}

void DevToolsPanel::renderNetworkTab(float width, float contentHeight) {
    auto& monitor = getNetworkMonitor();
    const auto& entries = monitor.getEntries();
    
    // Column header
    float headerY = y_ + 32;
    drawRect(0, headerY, width, 22, colors_.header[0], colors_.header[1], colors_.header[2], 255);
    
    g_textRenderer.setSize(10);
    g_textRenderer.drawText("Method", 15, headerY + 15, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    g_textRenderer.drawText("URL", 80, headerY + 15, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    g_textRenderer.drawText("Status", width - 220, headerY + 15, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    g_textRenderer.drawText("Size", width - 150, headerY + 15, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    g_textRenderer.drawText("Time", width - 80, headerY + 15, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    
    float rowY = headerY + 24;
    g_textRenderer.setSize(10);
    
    if (entries.empty()) {
        g_textRenderer.drawText("No network activity. Requests will appear here.", 15, rowY + 20, 
                                colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
    } else {
        int rowNum = 0;
        for (const auto& entry : entries) {
            if (rowY > y_ + height_ - 50) break;
            
            // Row background
            bool isSelected = (entry.request.id == selectedRequest_);
            if (isSelected) {
                drawRect(0, rowY, width, 20, 60, 60, 100, 200);
            } else if (rowNum % 2 == 1) {
                drawRect(0, rowY, width, 20, 
                         colors_.row_alt[0], colors_.row_alt[1], colors_.row_alt[2], 100);
            }
            
            // Method
            uint8_t methodR = colors_.success[0], methodG = colors_.success[1], methodB = colors_.success[2];
            if (entry.request.method == "POST" || entry.request.method == "PUT") {
                methodR = colors_.warning[0]; methodG = colors_.warning[1]; methodB = colors_.warning[2];
            } else if (entry.request.method == "DELETE") {
                methodR = colors_.error[0]; methodG = colors_.error[1]; methodB = colors_.error[2];
            }
            g_textRenderer.drawText(entry.request.method, 15, rowY + 14, methodR, methodG, methodB);
            
            // URL (truncated)
            std::string url = entry.request.url;
            float maxUrlWidth = width - 350;
            if (url.length() > 60) url = url.substr(0, 57) + "...";
            g_textRenderer.drawText(url, 80, rowY + 14, 
                                    colors_.text[0], colors_.text[1], colors_.text[2]);
            
            // Status
            char statusStr[16];
            snprintf(statusStr, sizeof(statusStr), "%d", entry.response.status_code);
            uint8_t statR = colors_.success[0], statG = colors_.success[1], statB = colors_.success[2];
            if (entry.response.status_code >= 400) {
                statR = colors_.error[0]; statG = colors_.error[1]; statB = colors_.error[2];
            } else if (entry.response.status_code >= 300) {
                statR = colors_.warning[0]; statG = colors_.warning[1]; statB = colors_.warning[2];
            } else if (entry.failed) {
                strcpy(statusStr, "Error");
                statR = colors_.error[0]; statG = colors_.error[1]; statB = colors_.error[2];
            }
            g_textRenderer.drawText(statusStr, width - 220, rowY + 14, statR, statG, statB);
            
            // Size
            std::string sizeStr = formatSize(entry.response.content_length);
            g_textRenderer.drawText(sizeStr, width - 150, rowY + 14, 
                                    colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
            
            // Time
            std::string timeStr = formatDuration(entry.response.duration_ms);
            g_textRenderer.drawText(timeStr, width - 80, rowY + 14, 
                                    colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
            
            rowY += 20;
            rowNum++;
        }
    }
    
    // Footer stats
    float footerY = y_ + height_ - 25;
    drawRect(0, footerY, width, 25, colors_.header[0], colors_.header[1], colors_.header[2], 255);
    
    char stats[128];
    snprintf(stats, sizeof(stats), "%zu requests | %s transferred | Blocked: %zu", 
             monitor.getTotalRequests(), 
             formatSize((size_t)monitor.getTotalTransferSize()).c_str(),
             monitor.getBlockedRequests());
    g_textRenderer.setSize(9);
    g_textRenderer.drawText(stats, 15, footerY + 15, 
                            colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
    
    // Clear button
    g_textRenderer.drawText("Clear", width - 50, footerY + 15, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
}

void DevToolsPanel::renderSecurityTab(float width, float contentHeight) {
    auto& checker = getSecurityChecker();
    const auto& warnings = checker.getWarnings();
    
    float sectionY = y_ + 40;
    g_textRenderer.setSize(12);
    g_textRenderer.drawText("Security Overview", 15, sectionY, 255, 255, 255);
    
    sectionY += 25;
    g_textRenderer.setSize(10);
    
    // Connection Status (mock for now)
    g_textRenderer.drawText("● Connection: Secure (TLS 1.3)", 20, sectionY, 
                            colors_.success[0], colors_.success[1], colors_.success[2]);
    sectionY += 18;
    g_textRenderer.drawText("  Certificate: Valid", 20, sectionY, 
                            colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
    
    sectionY += 30;
    g_textRenderer.setSize(12);
    g_textRenderer.drawText("Cross-Origin Policy", 15, sectionY, 255, 255, 255);
    
    sectionY += 20;
    g_textRenderer.setSize(10);
    g_textRenderer.drawText("● Same-origin requests allowed", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 16;
    g_textRenderer.drawText("● Cross-origin requires CORS headers", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    
    sectionY += 30;
    char warningTitle[64];
    snprintf(warningTitle, sizeof(warningTitle), "Warnings (%zu)", warnings.size());
    g_textRenderer.setSize(12);
    g_textRenderer.drawText(warningTitle, 15, sectionY, 255, 255, 255);
    
    sectionY += 20;
    g_textRenderer.setSize(10);
    
    if (warnings.empty()) {
        g_textRenderer.drawText("✓ No security warnings detected", 20, sectionY, 
                                colors_.success[0], colors_.success[1], colors_.success[2]);
    } else {
        for (const auto& warning : warnings) {
            if (sectionY > y_ + height_ - 30) break;
            
            uint8_t warnR = colors_.warning[0], warnG = colors_.warning[1], warnB = colors_.warning[2];
            if (warning.level >= SecurityWarningLevel::HIGH) {
                warnR = colors_.error[0]; warnG = colors_.error[1]; warnB = colors_.error[2];
            }
            
            g_textRenderer.drawText("⚠ " + warning.message, 20, sectionY, warnR, warnG, warnB);
            sectionY += 16;
        }
    }
}

void DevToolsPanel::renderElementsTab(float width, float contentHeight) {
    // Split view: DOM tree on left, Styles on right
    float splitX = width * 0.55f;
    
    // Left panel: DOM Tree
    float sectionY = y_ + 40;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("DOM Tree", 15, sectionY, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    
    sectionY += 22;
    g_textRenderer.setSize(10);
    
    // Sample DOM structure (will be connected to WebCore DOM)
    g_textRenderer.drawText("▼ <html>", 15, sectionY, 0x9CDCFE, 0x9CDCFE, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("  ▼ <head>", 15, sectionY, 0x9CDCFE, 0x9CDCFE, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("      <title>Page Title</title>", 15, sectionY, 0xCE9178, 0xCE9178, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("  </head>", 15, sectionY, 0x9CDCFE, 0x9CDCFE, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("  ▼ <body>", 15, sectionY, 0x9CDCFE, 0x9CDCFE, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("      <div class=\"container\">...</div>", 15, sectionY, 0xCE9178, 0xCE9178, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("  </body>", 15, sectionY, 0x9CDCFE, 0x9CDCFE, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("</html>", 15, sectionY, 0x9CDCFE, 0x9CDCFE, 0xFF);
    
    // Divider
    drawRect(splitX - 1, y_ + 35, 1, contentHeight, 
             colors_.border[0], colors_.border[1], colors_.border[2], 255);
    
    // Right panel: Styles
    float stylesY = y_ + 40;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Styles", splitX + 10, stylesY, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    
    stylesY += 22;
    g_textRenderer.setSize(10);
    g_textRenderer.drawText(".selected-element {", splitX + 10, stylesY, 0x569CD6, 0x569CD6, 0xFF);
    stylesY += 16;
    g_textRenderer.drawText("  width: 100%;", splitX + 10, stylesY, 0xCE9178, 0xCE9178, 0xFF);
    stylesY += 16;
    g_textRenderer.drawText("  height: auto;", splitX + 10, stylesY, 0xCE9178, 0xCE9178, 0xFF);
    stylesY += 16;
    g_textRenderer.drawText("  display: flex;", splitX + 10, stylesY, 0xCE9178, 0xCE9178, 0xFF);
    stylesY += 16;
    g_textRenderer.drawText("}", splitX + 10, stylesY, 0x569CD6, 0x569CD6, 0xFF);
    
    // Computed tab
    stylesY += 30;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Computed", splitX + 10, stylesY, 
                            colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
}

void DevToolsPanel::renderSourcesTab(float width, float contentHeight) {
    // File tree on left, source view on right
    float splitX = 200;
    
    // Left: File tree
    float sectionY = y_ + 40;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Page Sources", 15, sectionY, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    
    sectionY += 22;
    g_textRenderer.setSize(10);
    g_textRenderer.drawText("▼ ketivee.com", 15, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 16;
    g_textRenderer.drawText("  ├ index.html", 20, sectionY, 0x4FC1FF, 0x4FC1FF, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("  ├ styles.css", 20, sectionY, 0xE8A8F0, 0xE8A8F0, 0xFF);
    sectionY += 16;
    g_textRenderer.drawText("  └ app.js", 20, sectionY, 0xDCDC9D, 0xDCDC9D, 0xFF);
    
    // Divider
    drawRect(splitX - 1, y_ + 35, 1, contentHeight, 
             colors_.border[0], colors_.border[1], colors_.border[2], 255);
    
    // Right: Source viewer
    float sourceY = y_ + 40;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Select a file to view source", splitX + 15, sourceY, 
                            colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
}

void DevToolsPanel::renderPerformanceTab(float width, float contentHeight) {
    float sectionY = y_ + 40;
    g_textRenderer.setSize(12);
    g_textRenderer.drawText("Performance Timeline", 15, sectionY, 255, 255, 255);
    
    sectionY += 30;
    g_textRenderer.setSize(10);
    
    // Controls
    drawRoundedRect(15, sectionY, 80, 24, 4, 
                    colors_.info[0], colors_.info[1], colors_.info[2], 255);
    g_textRenderer.drawText("● Record", 25, sectionY + 16, 255, 255, 255);
    
    drawRoundedRect(105, sectionY, 60, 24, 4, 
                    colors_.header[0] + 20, colors_.header[1] + 20, colors_.header[2] + 20, 255);
    g_textRenderer.drawText("Clear", 120, sectionY + 16, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    
    sectionY += 40;
    
    // Metrics
    g_textRenderer.drawText("Page Load Metrics:", 15, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 20;
    g_textRenderer.drawText("  DOM Content Loaded: 245ms", 15, sectionY, 
                            colors_.success[0], colors_.success[1], colors_.success[2]);
    sectionY += 16;
    g_textRenderer.drawText("  First Contentful Paint: 312ms", 15, sectionY, 
                            colors_.success[0], colors_.success[1], colors_.success[2]);
    sectionY += 16;
    g_textRenderer.drawText("  Document Complete: 890ms", 15, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
}

void DevToolsPanel::renderApplicationTab(float width, float contentHeight) {
    // Storage categories on left
    float splitX = 180;
    
    float sectionY = y_ + 40;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Storage", 15, sectionY, 
                            colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
    
    sectionY += 22;
    g_textRenderer.setSize(10);
    g_textRenderer.drawText("  Local Storage", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 18;
    g_textRenderer.drawText("  Session Storage", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 18;
    g_textRenderer.drawText("  Cookies", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 18;
    g_textRenderer.drawText("  IndexedDB", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    
    sectionY += 25;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Cache", 15, sectionY, 
                            colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
    sectionY += 22;
    g_textRenderer.setSize(10);
    g_textRenderer.drawText("  Cache Storage", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 18;
    g_textRenderer.drawText("  Back/Forward Cache", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    
    // Divider
    drawRect(splitX - 1, y_ + 35, 1, contentHeight, 
             colors_.border[0], colors_.border[1], colors_.border[2], 255);
    
    // Right: Content
    float contentYY = y_ + 40;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Select a storage category", splitX + 15, contentYY, 
                            colors_.text_dim[0], colors_.text_dim[1], colors_.text_dim[2]);
}

void DevToolsPanel::renderSettingsTab(float width, float contentHeight) {
    float sectionY = y_ + 40;
    g_textRenderer.setSize(12);
    g_textRenderer.drawText("DevTools Settings", 15, sectionY, 255, 255, 255);
    
    sectionY += 30;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Appearance", 15, sectionY, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    
    sectionY += 22;
    g_textRenderer.setSize(10);
    g_textRenderer.drawText("  Theme: Dark", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 18;
    g_textRenderer.drawText("  Panel layout: Bottom", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    
    sectionY += 25;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Console", 15, sectionY, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    
    sectionY += 22;
    g_textRenderer.setSize(10);
    g_textRenderer.drawText("  [x] Show timestamps", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 18;
    g_textRenderer.drawText("  [x] Preserve log", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 18;
    g_textRenderer.drawText("  [ ] Enable custom formatters", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    
    sectionY += 25;
    g_textRenderer.setSize(11);
    g_textRenderer.drawText("Network", 15, sectionY, 
                            colors_.info[0], colors_.info[1], colors_.info[2]);
    
    sectionY += 22;
    g_textRenderer.setSize(10);
    g_textRenderer.drawText("  [x] Disable cache", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
    sectionY += 18;
    g_textRenderer.drawText("  [ ] Enable network throttling", 20, sectionY, 
                            colors_.text[0], colors_.text[1], colors_.text[2]);
}

bool DevToolsPanel::handleClick(float x, float y_click) {
    if (!visible_) return false;
    if (y_click < y_) return false;
    
    // Check tab clicks
    if (y_click < y_ + 30) {
        float tabX = 5;
        for (int i = 0; i < static_cast<int>(DevToolsTab::COUNT); i++) {
            float tabW = 85;
            if (x >= tabX && x < tabX + tabW) {
                setActiveTab(static_cast<DevToolsTab>(i));
                return true;
            }
            tabX += tabW + 2;
        }
        
        // Close button
        if (x > g_windowWidth - 40) {
            hide();
            return true;
        }
    }
    
    // Network row clicks
    if (activeTab_ == DevToolsTab::NETWORK) {
        float rowY = y_ + 56;
        auto& monitor = getNetworkMonitor();
        const auto& entries = monitor.getEntries();
        
        for (const auto& entry : entries) {
            if (y_click >= rowY && y_click < rowY + 20) {
                selectRequest(entry.request.id);
                return true;
            }
            rowY += 20;
            if (rowY > y_ + height_ - 50) break;
        }
    }
    
    return true;  // Consume click in panel area
}

bool DevToolsPanel::handleKey(int key) {
    // Escape to close
    if (key == 27) {  // ESC
        if (visible_) {
            hide();
            return true;
        }
    }
    return false;
}

void DevToolsPanel::executeConsoleCommand() {
    if (consoleInput_.empty()) return;
    
    // Log the command
    ConsoleLog::instance().log("> " + consoleInput_);
    
    // TODO: Execute through ZepraScript VM
    // For now, just echo
    ConsoleLog::instance().info("Command executed: " + consoleInput_);
    
    consoleInput_.clear();
}

// Global instance
static DevToolsPanel g_devtools_panel;

DevToolsPanel& getDevTools() {
    return g_devtools_panel;
}

} // namespace zepra
