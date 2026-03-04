/*
 * ZepraBrowser REOX Integration Example
 * 
 * This file shows how to integrate the REOX UI runtime into zepra_browser.cpp.
 * 
 * USAGE:
 * 
 * 1. Include the runtime header:
 *    #include "reox_browser_runtime.h"
 * 
 * 2. Replace initWindow/render loop with:
 * 
 *    // In main():
 *    rx_browser_init(1280, 800);
 *    
 *    // Set callbacks
 *    rx_browser->on_navigate = [](const char* url) {
 *        // Load URL in WebCore
 *        g_tabs[g_activeTab].url = url;
 *        g_tabs[g_activeTab].load();
 *    };
 *    
 *    while (running) {
 *        // Handle X11 events
 *        while (XPending(display)) {
 *            XEvent e;
 *            XNextEvent(display, &e);
 *            
 *            switch (e.type) {
 *                case MotionNotify:
 *                    rx_browser_mouse_move(e.xmotion.x, e.xmotion.y);
 *                    break;
 *                case ButtonPress:
 *                    rx_browser_mouse_down(e.xbutton.x, e.xbutton.y);
 *                    break;
 *                case ButtonRelease:
 *                    rx_browser_mouse_up(e.xbutton.x, e.xbutton.y);
 *                    break;
 *                case KeyPress: {
 *                    KeySym key = XLookupKeysym(&e.xkey, 0);
 *                    if (key == XK_Return) rx_browser_key_enter();
 *                    else if (key == XK_BackSpace) rx_browser_key_backspace();
 *                    else {
 *                        char buf[32];
 *                        int len = XLookupString(&e.xkey, buf, sizeof(buf), NULL, NULL);
 *                        if (len > 0) rx_browser_key_press(key, buf);
 *                    }
 *                    break;
 *                }
 *                case ConfigureNotify:
 *                    rx_browser_resize(e.xconfigure.width, e.xconfigure.height);
 *                    break;
 *            }
 *        }
 *        
 *        // Render frame
 *        rx_browser_frame();
 *        
 *        // Sleep
 *        std::this_thread::sleep_for(std::chrono::milliseconds(16));
 *    }
 *    
 *    rx_browser_destroy();
 * 
 * 
 * ALTERNATIVE: Hybrid Mode (keep C++ rendering, use REOX for state)
 * 
 * In this mode, you keep the existing C++ OpenGL rendering but use 
 * the REOX state management:
 * 
 *    #include "reox_browser_runtime.h"
 *    
 *    // Use rx_browser state instead of g_addressBarText, etc.
 *    // Example: rx_browser->address_text instead of g_addressBarText
 */

// ============================================================================
// Quick Integration Snippet
// ============================================================================

#ifdef ENABLE_REOX_UI

#include "reox_browser_runtime.h"

// Add to main() after window creation:
void init_reox_browser() {
    rx_browser_init(g_windowWidth, g_windowHeight);
    
    // Connect to WebCore
    rx_browser->on_navigate = [](const char* url) {
        if (!g_tabs.empty()) {
            g_tabs[g_activeTab].url = url;
            g_tabs[g_activeTab].load();
        }
    };
    
    rx_browser->on_new_tab = []() {
        for (auto& t : g_tabs) t.active = false;
        g_tabs.emplace_back("zepra://start");
        g_tabs.back().active = true;
        g_tabs.back().load();
        g_activeTab = g_tabs.size() - 1;
    };
    
    rx_browser->on_search = [](const char* query) {
        std::string url = "https://ketiveesearch.com/search?q=";
        url += query;
        rx_browser_navigate(url.c_str());
    };
}

// Replace render() with:
void render_reox() {
    rx_browser_frame();
}

// Replace handleEvents() mouse/key handling with rx_browser_* calls

#endif // ENABLE_REOX_UI
