/*
 * ZepraBrowser REOX UI Runtime
 * 
 * Connects the REOX app.rx UI to the browser via NXRender bridge.
 * 
 * Usage:
 *   1. Include this file in zepra_browser.cpp
 *   2. Call rx_browser_init() instead of render loop
 *   3. Call rx_browser_frame() each frame
 *   4. Call rx_browser_handle_mouse/key events
 *   5. Call rx_browser_destroy() on exit
 */

#ifndef REOX_BROWSER_RUNTIME_H
#define REOX_BROWSER_RUNTIME_H

#include "reox_nxrender_bridge.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Browser State
 * ============================================================================ */

typedef struct {
    char url[2048];
    char title[256];
    bool is_loading;
    int tab_id;
} RxTab;

typedef struct {
    RxTab tabs[32];
    int tab_count;
    int active_tab;
    
    char address_text[2048];
    char search_text[256];
    bool address_focused;
    bool search_focused;
    
    bool sidebar_visible;
    bool settings_open;
    bool can_go_back;
    bool can_go_forward;
    
    // Callbacks for browser actions
    void (*on_navigate)(const char* url);
    void (*on_new_tab)(void);
    void (*on_close_tab)(int tab_id);
    void (*on_search)(const char* query);
} RxBrowserState;

static RxBrowserState* rx_browser = NULL;

/* ============================================================================
 * Color Helpers
 * ============================================================================ */

static inline NxColor rx_color(uint32_t hex) {
    return (NxColor){
        (hex >> 16) & 0xFF,
        (hex >> 8) & 0xFF,
        hex & 0xFF,
        255
    };
}

static inline NxColor rx_color_alpha(uint32_t hex, uint8_t alpha) {
    return (NxColor){
        (hex >> 16) & 0xFF,
        (hex >> 8) & 0xFF,
        hex & 0xFF,
        alpha
    };
}

/* Theme colors */
#define RX_BG_PRIMARY     0x0D1117
#define RX_BG_SECONDARY   0x161B22
#define RX_BG_TERTIARY    0x21262D
#define RX_BG_ELEVATED    0x30363D
#define RX_TEXT_PRIMARY   0xF0F6FC
#define RX_TEXT_SECONDARY 0x8B949E
#define RX_TEXT_PLACEHOLDER 0x6E7681
#define RX_ACCENT         0x58A6FF
#define RX_SUCCESS        0x3FB950
#define RX_WARNING        0xD29922
#define RX_ERROR          0xF85149
#define RX_BORDER         0x30363D
#define RX_GRADIENT_START 0xE8B4D8
#define RX_GRADIENT_END   0xA8A0F5

/* ============================================================================
 * UI Component Builders
 * ============================================================================ */

/* Traffic lights (close/minimize/maximize) */
static inline RxNode* rx_build_traffic_lights(void) {
    RxNode* container = rx_node_create(RX_NODE_HSTACK);
    container->gap = 8;
    container->padding = 12;
    
    // Close
    RxNode* close = rx_node_create(RX_NODE_BUTTON);
    close->width = 12;
    close->height = 12;
    close->corner_radius = 6;
    close->background = rx_color(0xED6A5E);
    rx_node_add_child(container, close);
    
    // Minimize
    RxNode* minimize = rx_node_create(RX_NODE_BUTTON);
    minimize->width = 12;
    minimize->height = 12;
    minimize->corner_radius = 6;
    minimize->background = rx_color(0xF4BF4F);
    rx_node_add_child(container, minimize);
    
    // Maximize
    RxNode* maximize = rx_node_create(RX_NODE_BUTTON);
    maximize->width = 12;
    maximize->height = 12;
    maximize->corner_radius = 6;
    maximize->background = rx_color(0x61C554);
    rx_node_add_child(container, maximize);
    
    return container;
}

/* Tab item */
static inline RxNode* rx_build_tab(RxTab* tab, bool is_active) {
    RxNode* container = rx_node_create(RX_NODE_HSTACK);
    container->gap = 8;
    container->padding = 8;
    container->corner_radius = 8;
    container->background = is_active ? rx_color(RX_BG_TERTIARY) : rx_color(RX_BG_SECONDARY);
    
    // Favicon placeholder
    RxNode* favicon = rx_node_create(RX_NODE_TEXT);
    favicon->text = strdup("🌐");
    favicon->foreground = rx_color(RX_TEXT_SECONDARY);
    rx_node_add_child(container, favicon);
    
    // Title
    RxNode* title = rx_node_create(RX_NODE_TEXT);
    title->text = strdup(tab->title[0] ? tab->title : "New Tab");
    title->foreground = is_active ? rx_color(RX_TEXT_PRIMARY) : rx_color(RX_TEXT_SECONDARY);
    rx_node_add_child(container, title);
    
    // Close button
    RxNode* close = rx_node_create(RX_NODE_BUTTON);
    close->text = strdup("×");
    close->foreground = rx_color(RX_TEXT_SECONDARY);
    rx_node_add_child(container, close);
    
    return container;
}

/* Tab bar */
static inline RxNode* rx_build_tab_bar(void) {
    RxNode* container = rx_node_create(RX_NODE_HSTACK);
    container->gap = 2;
    container->background = rx_color(RX_BG_SECONDARY);
    container->height = 40;
    
    // Tabs
    for (int i = 0; i < rx_browser->tab_count; i++) {
        RxNode* tab = rx_build_tab(&rx_browser->tabs[i], i == rx_browser->active_tab);
        rx_node_add_child(container, tab);
    }
    
    // New tab button
    RxNode* plus = rx_node_create(RX_NODE_BUTTON);
    plus->text = strdup("+");
    plus->foreground = rx_color(RX_TEXT_SECONDARY);
    plus->padding = 8;
    rx_node_add_child(container, plus);
    
    // Spacer
    RxNode* spacer = rx_node_create(RX_NODE_SPACER);
    rx_node_add_child(container, spacer);
    
    return container;
}

/* Navigation bar */
static inline RxNode* rx_build_nav_bar(void) {
    RxNode* container = rx_node_create(RX_NODE_HSTACK);
    container->gap = 12;
    container->padding = 8;
    container->background = rx_color(RX_BG_SECONDARY);
    
    // Back
    RxNode* back = rx_node_create(RX_NODE_BUTTON);
    back->text = strdup("←");
    back->foreground = rx_browser->can_go_back ? rx_color(RX_TEXT_PRIMARY) : rx_color(0x484F58);
    rx_node_add_child(container, back);
    
    // Forward
    RxNode* forward = rx_node_create(RX_NODE_BUTTON);
    forward->text = strdup("→");
    forward->foreground = rx_browser->can_go_forward ? rx_color(RX_TEXT_PRIMARY) : rx_color(0x484F58);
    rx_node_add_child(container, forward);
    
    // Refresh
    RxNode* refresh = rx_node_create(RX_NODE_BUTTON);
    refresh->text = strdup("↻");
    refresh->foreground = rx_color(RX_TEXT_PRIMARY);
    rx_node_add_child(container, refresh);
    
    // Address bar
    RxNode* address = rx_node_create(RX_NODE_TEXTFIELD);
    bool is_start = (strcmp(rx_browser->address_text, "zepra://start") == 0 || 
                     rx_browser->address_text[0] == '\0');
    if (is_start && !rx_browser->address_focused) {
        address->text = strdup("Search or enter address");
        address->foreground = rx_color(RX_TEXT_PLACEHOLDER);
    } else {
        address->text = strdup(rx_browser->address_text);
        address->foreground = rx_color(RX_TEXT_PRIMARY);
    }
    address->background = rx_browser->address_focused ? rx_color(RX_BG_ELEVATED) : rx_color(RX_BG_TERTIARY);
    address->corner_radius = 8;
    address->padding = 10;
    rx_node_add_child(container, address);
    
    // Extensions
    RxNode* ext = rx_node_create(RX_NODE_BUTTON);
    ext->text = strdup("🧩");
    ext->foreground = rx_color(RX_TEXT_SECONDARY);
    rx_node_add_child(container, ext);
    
    // Menu
    RxNode* menu = rx_node_create(RX_NODE_BUTTON);
    menu->text = strdup("⋮");
    menu->foreground = rx_color(RX_TEXT_SECONDARY);
    rx_node_add_child(container, menu);
    
    return container;
}

/* Start page */
static inline RxNode* rx_build_start_page(void) {
    RxNode* container = rx_node_create(RX_NODE_VSTACK);
    container->gap = 32;
    container->padding = 40;
    
    // Gradient background will be rendered separately
    
    // Logo section
    RxNode* logo_section = rx_node_create(RX_NODE_VSTACK);
    logo_section->gap = 16;
    
    // Logo circle with Z
    RxNode* logo = rx_node_create(RX_NODE_BUTTON);
    logo->width = 120;
    logo->height = 120;
    logo->corner_radius = 60;
    logo->background = rx_color(0x6B5B95);
    logo->text = strdup("Z");
    logo->foreground = rx_color(0xFFFFFF);
    rx_node_add_child(logo_section, logo);
    
    // "Zepra" text
    RxNode* brand = rx_node_create(RX_NODE_TEXT);
    brand->text = strdup("Zepra");
    brand->foreground = rx_color(0x4A4063);
    rx_node_add_child(logo_section, brand);
    
    rx_node_add_child(container, logo_section);
    
    // Search box
    RxNode* search_container = rx_node_create(RX_NODE_HSTACK);
    search_container->gap = 12;
    search_container->padding = 16;
    search_container->corner_radius = 28;
    search_container->background = rx_color(0xFFFFFF);
    search_container->width = 600;
    
    // Search icon
    RxNode* search_icon = rx_node_create(RX_NODE_TEXT);
    search_icon->text = strdup("🔍");
    rx_node_add_child(search_container, search_icon);
    
    // Search input
    RxNode* search_input = rx_node_create(RX_NODE_TEXTFIELD);
    if (rx_browser->search_text[0] == '\0' && !rx_browser->search_focused) {
        search_input->text = strdup("Search the web...");
        search_input->foreground = rx_color(RX_TEXT_PLACEHOLDER);
    } else {
        search_input->text = strdup(rx_browser->search_text);
        search_input->foreground = rx_color(0x1F2328);
    }
    rx_node_add_child(search_container, search_input);
    
    // Mic icon
    RxNode* mic_icon = rx_node_create(RX_NODE_TEXT);
    mic_icon->text = strdup("🎤");
    rx_node_add_child(search_container, mic_icon);
    
    rx_node_add_child(container, search_container);
    
    // Quick links
    RxNode* quick_links = rx_node_create(RX_NODE_HSTACK);
    quick_links->gap = 20;
    
    const char* icons[] = {"📧", "📰", "🎬", "📸"};
    const char* labels[] = {"Mail", "News", "Videos", "Images"};
    uint32_t colors[] = {0x5080DC, 0x8C64C8, 0xC878A0, 0xDC6464};
    
    for (int i = 0; i < 4; i++) {
        RxNode* link = rx_node_create(RX_NODE_VSTACK);
        link->gap = 8;
        
        RxNode* icon = rx_node_create(RX_NODE_BUTTON);
        icon->width = 50;
        icon->height = 50;
        icon->corner_radius = 25;
        icon->background = rx_color(colors[i]);
        icon->text = strdup(icons[i]);
        rx_node_add_child(link, icon);
        
        RxNode* label = rx_node_create(RX_NODE_TEXT);
        label->text = strdup(labels[i]);
        label->foreground = rx_color(0x4A4063);
        rx_node_add_child(link, label);
        
        rx_node_add_child(quick_links, link);
    }
    
    rx_node_add_child(container, quick_links);
    
    return container;
}

/* ============================================================================
 * Main UI Builder
 * ============================================================================ */

static inline RxNode* rx_build_browser_ui(float width, float height) {
    RxNode* root = rx_node_create(RX_NODE_VSTACK);
    root->x = 0;
    root->y = 0;
    root->width = width;
    root->height = height;
    root->gap = 0;
    root->background = rx_color(RX_BG_PRIMARY);
    
    // Header (traffic lights + tabs)
    RxNode* header = rx_node_create(RX_NODE_HSTACK);
    header->background = rx_color(RX_BG_SECONDARY);
    header->height = 53;
    
    rx_node_add_child(header, rx_build_traffic_lights());
    rx_node_add_child(header, rx_build_tab_bar());
    rx_node_add_child(root, header);
    
    // Navigation bar
    RxNode* nav = rx_build_nav_bar();
    rx_node_add_child(root, nav);
    
    // Content area
    RxNode* content = rx_node_create(RX_NODE_ZSTACK);
    content->background = rx_color(RX_BG_PRIMARY);
    
    // Check if start page
    bool is_start = (rx_browser->tab_count == 0 ||
                    strcmp(rx_browser->tabs[rx_browser->active_tab].url, "zepra://start") == 0 ||
                    rx_browser->tabs[rx_browser->active_tab].url[0] == '\0');
    
    if (is_start) {
        rx_node_add_child(content, rx_build_start_page());
    }
    // Else: WebView content (rendered by WebCore separately)
    
    rx_node_add_child(root, content);
    
    return root;
}

/* ============================================================================
 * Browser Runtime API
 * ============================================================================ */

static inline void rx_browser_init(uint32_t width, uint32_t height) {
    // Init bridge
    rx_bridge_init(width, height);
    
    // Init browser state
    rx_browser = (RxBrowserState*)calloc(1, sizeof(RxBrowserState));
    
    // Create initial tab
    rx_browser->tab_count = 1;
    rx_browser->active_tab = 0;
    strcpy(rx_browser->tabs[0].url, "zepra://start");
    strcpy(rx_browser->tabs[0].title, "New Tab");
    strcpy(rx_browser->address_text, "zepra://start");
    
    // Build initial UI
    rx_bridge->root = rx_build_browser_ui((float)width, (float)height);
    rx_bridge->needs_redraw = true;
}

static inline void rx_browser_destroy(void) {
    if (rx_browser) {
        free(rx_browser);
        rx_browser = NULL;
    }
    rx_bridge_destroy();
}

static inline void rx_browser_rebuild(void) {
    if (!rx_bridge) return;
    
    float w = rx_bridge->root ? rx_bridge->root->width : 1280;
    float h = rx_bridge->root ? rx_bridge->root->height : 800;
    
    if (rx_bridge->root) {
        rx_node_destroy(rx_bridge->root);
    }
    
    rx_bridge->root = rx_build_browser_ui(w, h);
    rx_bridge->needs_redraw = true;
}

static inline void rx_browser_frame(void) {
    rx_frame();
}

static inline void rx_browser_resize(uint32_t width, uint32_t height) {
    if (!rx_bridge) return;
    nx_gpu_resize(rx_bridge->gpu, width, height);
    rx_browser_rebuild();
}

/* Navigate to URL */
static inline void rx_browser_navigate(const char* url) {
    if (!rx_browser) return;
    
    RxTab* tab = &rx_browser->tabs[rx_browser->active_tab];
    strncpy(tab->url, url, sizeof(tab->url) - 1);
    strncpy(rx_browser->address_text, url, sizeof(rx_browser->address_text) - 1);
    tab->is_loading = true;
    
    if (rx_browser->on_navigate) {
        rx_browser->on_navigate(url);
    }
    
    rx_browser_rebuild();
}

/* New tab */
static inline void rx_browser_new_tab(void) {
    if (!rx_browser || rx_browser->tab_count >= 32) return;
    
    int idx = rx_browser->tab_count++;
    rx_browser->active_tab = idx;
    strcpy(rx_browser->tabs[idx].url, "zepra://start");
    strcpy(rx_browser->tabs[idx].title, "New Tab");
    strcpy(rx_browser->address_text, "zepra://start");
    
    if (rx_browser->on_new_tab) {
        rx_browser->on_new_tab();
    }
    
    rx_browser_rebuild();
}

/* Close tab */
static inline void rx_browser_close_tab(int tab_id) {
    if (!rx_browser || rx_browser->tab_count <= 1) return;
    
    // Shift tabs
    for (int i = tab_id; i < rx_browser->tab_count - 1; i++) {
        rx_browser->tabs[i] = rx_browser->tabs[i + 1];
    }
    rx_browser->tab_count--;
    
    if (rx_browser->active_tab >= rx_browser->tab_count) {
        rx_browser->active_tab = rx_browser->tab_count - 1;
    }
    
    strcpy(rx_browser->address_text, rx_browser->tabs[rx_browser->active_tab].url);
    
    if (rx_browser->on_close_tab) {
        rx_browser->on_close_tab(tab_id);
    }
    
    rx_browser_rebuild();
}

/* Search */
static inline void rx_browser_search(const char* query) {
    if (!rx_browser) return;
    
    strncpy(rx_browser->search_text, query, sizeof(rx_browser->search_text) - 1);
    
    // Build search URL
    char url[2048];
    snprintf(url, sizeof(url), "https://ketiveesearch.com/search?q=%s", query);
    
    if (rx_browser->on_search) {
        rx_browser->on_search(query);
    }
    
    rx_browser_navigate(url);
}

/* Input handlers */
static inline void rx_browser_mouse_move(float x, float y) {
    rx_handle_mouse_move(x, y);
}

static inline void rx_browser_mouse_down(float x, float y) {
    rx_handle_mouse_down(x, y);
    
    // Check focus states
    if (rx_bridge && rx_bridge->focused) {
        if (rx_bridge->focused->type == RX_NODE_TEXTFIELD) {
            // Determine which text field
            // (In real impl, use node IDs)
        }
    }
}

static inline void rx_browser_mouse_up(float x, float y) {
    rx_handle_mouse_up(x, y);
}

static inline void rx_browser_key_press(uint32_t keysym, const char* text) {
    if (!rx_browser) return;
    
    // Handle text input to focused field
    if (rx_browser->address_focused && text && text[0]) {
        size_t len = strlen(rx_browser->address_text);
        if (len < sizeof(rx_browser->address_text) - 1) {
            strcat(rx_browser->address_text, text);
            rx_browser_rebuild();
        }
    } else if (rx_browser->search_focused && text && text[0]) {
        size_t len = strlen(rx_browser->search_text);
        if (len < sizeof(rx_browser->search_text) - 1) {
            strcat(rx_browser->search_text, text);
            rx_browser_rebuild();
        }
    }
}

static inline void rx_browser_key_backspace(void) {
    if (!rx_browser) return;
    
    if (rx_browser->address_focused) {
        size_t len = strlen(rx_browser->address_text);
        if (len > 0) {
            rx_browser->address_text[len - 1] = '\0';
            rx_browser_rebuild();
        }
    } else if (rx_browser->search_focused) {
        size_t len = strlen(rx_browser->search_text);
        if (len > 0) {
            rx_browser->search_text[len - 1] = '\0';
            rx_browser_rebuild();
        }
    }
}

static inline void rx_browser_key_enter(void) {
    if (!rx_browser) return;
    
    if (rx_browser->address_focused) {
        rx_browser->address_focused = false;
        rx_browser_navigate(rx_browser->address_text);
    } else if (rx_browser->search_focused) {
        rx_browser->search_focused = false;
        rx_browser_search(rx_browser->search_text);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* REOX_BROWSER_RUNTIME_H */
