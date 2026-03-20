/**
 * @file zepra_ui.h
 * @brief ZepraBrowser UI Framework - C++ wrapper for REOX/NXRender
 * 
 * Provides C++ API for declarative UI building using NXRender backend.
 * Replaces SDL dependency with pure OpenGL rendering.
 * 
 * Copyright (c) 2025 KetiveeAI
 */

#ifndef ZEPRA_UI_H
#define ZEPRA_UI_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

// Include REOX bridge (C header)
extern "C" {
#include "../../REOX/reolab/runtime/reox_nxrender_bridge.h"
}

namespace ZepraUI {

// ============================================================================
// Forward Declarations
// ============================================================================

class Widget;
class Container;
class Window;

// ============================================================================
// Types & Constants
// ============================================================================

struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    Color(uint32_t hex) : r((hex >> 16) & 0xFF), g((hex >> 8) & 0xFF), b(hex & 0xFF), a(255) {}
    
    NxColor toNx() const { return {r, g, b, a}; }
    static Color fromNx(NxColor c) { return Color(c.r, c.g, c.b, c.a); }
    
    // Convenient presets
    static Color black() { return Color(0, 0, 0); }
    static Color white() { return Color(255, 255, 255); }
    static Color transparent() { return Color(0, 0, 0, 0); }
    static Color primary() { return Color(88, 166, 255); }   // NeolyxOS blue
    static Color surface() { return Color(30, 30, 30); }
    static Color surfaceLight() { return Color(45, 45, 45); }
};

struct Rect {
    float x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
    
    bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

// ============================================================================
// Widget Base Class
// ============================================================================

class Widget {
public:
    virtual ~Widget() = default;
    
    // Build REOX node tree
    virtual RxNode* build() = 0;
    
    // Common properties
    Widget& background(Color c) { bg_ = c; return *this; }
    Widget& foreground(Color c) { fg_ = c; return *this; }
    Widget& padding(float p) { padding_ = p; return *this; }
    Widget& cornerRadius(float r) { radius_ = r; return *this; }
    Widget& size(float w, float h) { width_ = w; height_ = h; return *this; }
    
protected:
    Color bg_ = Color::transparent();
    Color fg_ = Color::white();
    float padding_ = 0;
    float radius_ = 0;
    float width_ = -1;
    float height_ = -1;
    
    void applyStyle(RxNode* node) {
        if (node) {
            node->background = bg_.toNx();
            node->foreground = fg_.toNx();
            node->padding = padding_;
            node->corner_radius = radius_;
            if (width_ > 0) node->width = width_;
            if (height_ > 0) node->height = height_;
        }
    }
};

// ============================================================================
// Container Widgets (VStack, HStack, ZStack)
// ============================================================================

class VStack : public Widget {
public:
    VStack(float gap = 8.0f) : gap_(gap) {}
    
    VStack& add(std::unique_ptr<Widget> child) {
        children_.push_back(std::move(child));
        return *this;
    }
    
    template<typename T, typename... Args>
    VStack& add(Args&&... args) {
        children_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return *this;
    }
    
    RxNode* build() override {
        RxNode* node = rx_node_create(RX_NODE_VSTACK);
        node->gap = gap_;
        applyStyle(node);
        
        for (auto& child : children_) {
            RxNode* childNode = child->build();
            if (childNode) rx_node_add_child(node, childNode);
        }
        return node;
    }
    
private:
    float gap_;
    std::vector<std::unique_ptr<Widget>> children_;
};

class HStack : public Widget {
public:
    HStack(float gap = 8.0f) : gap_(gap) {}
    
    HStack& add(std::unique_ptr<Widget> child) {
        children_.push_back(std::move(child));
        return *this;
    }
    
    template<typename T, typename... Args>
    HStack& add(Args&&... args) {
        children_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return *this;
    }
    
    RxNode* build() override {
        RxNode* node = rx_node_create(RX_NODE_HSTACK);
        node->gap = gap_;
        applyStyle(node);
        
        for (auto& child : children_) {
            RxNode* childNode = child->build();
            if (childNode) rx_node_add_child(node, childNode);
        }
        return node;
    }
    
private:
    float gap_;
    std::vector<std::unique_ptr<Widget>> children_;
};

class ZStack : public Widget {
public:
    ZStack& add(std::unique_ptr<Widget> child) {
        children_.push_back(std::move(child));
        return *this;
    }
    
    RxNode* build() override {
        RxNode* node = rx_node_create(RX_NODE_ZSTACK);
        applyStyle(node);
        
        for (auto& child : children_) {
            RxNode* childNode = child->build();
            if (childNode) rx_node_add_child(node, childNode);
        }
        return node;
    }
    
private:
    std::vector<std::unique_ptr<Widget>> children_;
};

// ============================================================================
// Basic Widgets
// ============================================================================

class Text : public Widget {
public:
    Text(const std::string& text) : text_(text) {}
    
    RxNode* build() override {
        RxNode* node = rx_node_create(RX_NODE_TEXT);
        node->text = strdup(text_.c_str());
        applyStyle(node);
        return node;
    }
    
private:
    std::string text_;
};

class Button : public Widget {
public:
    using Callback = std::function<void()>;
    
    Button(const std::string& label) : label_(label) {}
    
    Button& onClick(Callback cb) { onClick_ = cb; return *this; }
    
    RxNode* build() override {
        RxNode* node = rx_node_create(RX_NODE_BUTTON);
        node->text = strdup(label_.c_str());
        node->corner_radius = radius_ > 0 ? radius_ : 6.0f;
        applyStyle(node);
        
        // Store callback via closure-safe static map
        if (onClick_) {
            node->on_click = [](RxNode* n) {
                // Callback invocation via node ID lookup
                (void)n;
            };
            callbacks_[node->id] = onClick_;
        }
        return node;
    }
    
    static void dispatchClick(uint64_t nodeId) {
        auto it = callbacks_.find(nodeId);
        if (it != callbacks_.end()) it->second();
    }
    
private:
    std::string label_;
    Callback onClick_;
    static inline std::unordered_map<uint64_t, Callback> callbacks_;
};

class TextField : public Widget {
public:
    using ChangeCallback = std::function<void(const std::string&)>;
    
    TextField(std::string* binding = nullptr) : binding_(binding) {}
    
    TextField& placeholder(const std::string& p) { placeholder_ = p; return *this; }
    TextField& onChange(ChangeCallback cb) { onChange_ = cb; return *this; }
    
    RxNode* build() override {
        RxNode* node = rx_node_create(RX_NODE_TEXTFIELD);
        node->text = strdup(binding_ ? binding_->c_str() : placeholder_.c_str());
        node->corner_radius = 8.0f;
        node->background = Color::surfaceLight().toNx();
        applyStyle(node);
        return node;
    }
    
private:
    std::string* binding_ = nullptr;
    std::string placeholder_;
    ChangeCallback onChange_;
};

class Spacer : public Widget {
public:
    RxNode* build() override {
        return rx_node_create(RX_NODE_SPACER);
    }
};

class Divider : public Widget {
public:
    RxNode* build() override {
        RxNode* node = rx_node_create(RX_NODE_DIVIDER);
        node->height = 1;
        return node;
    }
};

// ============================================================================
// Window Management
// ============================================================================

class Window {
public:
    Window(uint32_t width, uint32_t height, const std::string& title)
        : width_(width), height_(height), title_(title) {}
    
    ~Window() {
        if (initialized_) rx_bridge_destroy();
    }
    
    bool init() {
        rx_bridge_init(width_, height_);
        initialized_ = true;
        return initialized_;
    }
    
    void setContent(std::unique_ptr<Widget> root) {
        if (!initialized_) return;
        
        if (rx_bridge->root) rx_node_destroy(rx_bridge->root);
        
        RxNode* rootNode = root->build();
        rootNode->x = 0;
        rootNode->y = 0;
        rootNode->width = static_cast<float>(width_);
        rootNode->height = static_cast<float>(height_);
        rootNode->state |= RX_STATE_DIRTY;
        
        rx_bridge->root = rootNode;
        rx_bridge->needs_redraw = true;
    }
    
    void handleMouseMove(float x, float y) { rx_handle_mouse_move(x, y); }
    void handleMouseDown(float x, float y) { rx_handle_mouse_down(x, y); }
    void handleMouseUp(float x, float y) { rx_handle_mouse_up(x, y); }
    
    void frame() { rx_frame(); }
    
    void resize(uint32_t w, uint32_t h) {
        width_ = w;
        height_ = h;
        if (rx_bridge && rx_bridge->gpu) {
            nx_gpu_resize(rx_bridge->gpu, w, h);
        }
        if (rx_bridge && rx_bridge->root) {
            rx_bridge->root->width = static_cast<float>(w);
            rx_bridge->root->height = static_cast<float>(h);
            rx_bridge->root->state |= RX_STATE_DIRTY;
            rx_bridge->needs_redraw = true;
        }
    }
    
    bool needsRedraw() const { return rx_bridge && rx_bridge->needs_redraw; }
    
private:
    uint32_t width_, height_;
    std::string title_;
    bool initialized_ = false;
};

// ============================================================================
// Declarative Macros (SwiftUI-like)
// ============================================================================

#define ZEPRA_VSTACK(...) std::make_unique<ZepraUI::VStack>(__VA_ARGS__)
#define ZEPRA_HSTACK(...) std::make_unique<ZepraUI::HStack>(__VA_ARGS__)
#define ZEPRA_TEXT(t) std::make_unique<ZepraUI::Text>(t)
#define ZEPRA_BUTTON(label) std::make_unique<ZepraUI::Button>(label)
#define ZEPRA_TEXTFIELD() std::make_unique<ZepraUI::TextField>()
#define ZEPRA_SPACER() std::make_unique<ZepraUI::Spacer>()
#define ZEPRA_DIVIDER() std::make_unique<ZepraUI::Divider>()

} // namespace ZepraUI

#endif // ZEPRA_UI_H
