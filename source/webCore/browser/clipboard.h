// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * clipboard.h - Native X11 Clipboard Implementation for ZepraBrowser
 * 
 * Provides copy/paste functionality without external dependencies (xclip).
 * Uses X11 selections (CLIPBOARD and PRIMARY) directly.
 */

#ifndef ZEPRA_CLIPBOARD_H
#define ZEPRA_CLIPBOARD_H

#include <string>
#include <cstring>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace ZepraBrowser {

class Clipboard {
public:
    static Clipboard& instance() {
        static Clipboard inst;
        return inst;
    }
    
    // Initialize with X display
    void init(Display* display, Window window) {
        display_ = display;
        window_ = window;
        
        if (display_) {
            // Get atoms for clipboard operations
            clipboard_ = XInternAtom(display_, "CLIPBOARD", False);
            targets_ = XInternAtom(display_, "TARGETS", False);
            utf8_ = XInternAtom(display_, "UTF8_STRING", False);
            xselData_ = XInternAtom(display_, "XSEL_DATA", False);
        }
    }
    
    // Copy text to clipboard
    bool copy(const std::string& text) {
        if (!display_ || !window_) return false;
        
        copiedText_ = text;
        
        // Take ownership of CLIPBOARD selection
        XSetSelectionOwner(display_, clipboard_, window_, CurrentTime);
        
        // Also set PRIMARY selection (middle-click paste)
        XSetSelectionOwner(display_, XA_PRIMARY, window_, CurrentTime);
        
        XFlush(display_);
        return true;
    }
    
    // Paste from clipboard (synchronous - may block briefly)
    std::string paste() {
        if (!display_ || !window_) return "";
        
        // Check if we own the clipboard
        Window owner = XGetSelectionOwner(display_, clipboard_);
        if (owner == window_) {
            return copiedText_;  // We own it, return our copy
        }
        
        if (owner == None) {
            return "";  // No clipboard content
        }
        
        // Request clipboard content
        XConvertSelection(display_, clipboard_, utf8_, xselData_, window_, CurrentTime);
        XFlush(display_);
        
        // Wait for SelectionNotify event (with timeout)
        XEvent event;
        for (int i = 0; i < 50; i++) {  // 500ms timeout
            if (XCheckTypedWindowEvent(display_, window_, SelectionNotify, &event)) {
                if (event.xselection.selection == clipboard_ && 
                    event.xselection.property != None) {
                    return getSelectionData(event.xselection.property);
                }
                return "";  // Selection failed
            }
            usleep(10000);  // 10ms sleep
        }
        
        return "";  // Timeout
    }
    
    // Handle X11 SelectionRequest events (when others want our clipboard)
    void handleSelectionRequest(XSelectionRequestEvent& event) {
        if (!display_) return;
        
        XSelectionEvent response;
        response.type = SelectionNotify;
        response.display = event.display;
        response.requestor = event.requestor;
        response.selection = event.selection;
        response.target = event.target;
        response.time = event.time;
        response.property = None;
        
        if (event.target == targets_) {
            // Respond with supported targets
            Atom targets[] = { targets_, utf8_, XA_STRING };
            XChangeProperty(display_, event.requestor, event.property,
                           XA_ATOM, 32, PropModeReplace,
                           (unsigned char*)targets, 3);
            response.property = event.property;
        }
        else if (event.target == utf8_ || event.target == XA_STRING) {
            // Respond with clipboard text
            XChangeProperty(display_, event.requestor, event.property,
                           event.target, 8, PropModeReplace,
                           (unsigned char*)copiedText_.c_str(), copiedText_.length());
            response.property = event.property;
        }
        
        XSendEvent(display_, event.requestor, False, 0, (XEvent*)&response);
        XFlush(display_);
    }
    
    // Get copied text (for internal use)
    const std::string& getCopiedText() const { return copiedText_; }
    
private:
    Clipboard() = default;
    
    std::string getSelectionData(Atom property) {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned char* data = nullptr;
        
        if (XGetWindowProperty(display_, window_, property, 0, ~0L, True,
                              AnyPropertyType, &actual_type, &actual_format,
                              &nitems, &bytes_after, &data) == Success) {
            if (data && nitems > 0) {
                std::string result((char*)data, nitems);
                XFree(data);
                return result;
            }
            if (data) XFree(data);
        }
        return "";
    }
    
    Display* display_ = nullptr;
    Window window_ = 0;
    Atom clipboard_ = 0;
    Atom targets_ = 0;
    Atom utf8_ = 0;
    Atom xselData_ = 0;
    std::string copiedText_;
};

} // namespace ZepraBrowser

#endif // ZEPRA_CLIPBOARD_H
