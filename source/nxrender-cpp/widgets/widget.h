// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file widget.h
 * @brief Base Widget class for NXRender UI
 */

#pragma once

#include "../nxgfx/primitives.h"
#include "../nxgfx/color.h"
#include "../input/events.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace NXRender {

class GpuContext;
class Theme;

using WidgetId = uint64_t;

/**
 * @brief Widget state flags
 */
struct WidgetState {
    bool visible = true;
    bool enabled = true;
    bool focused = false;
    bool hovered = false;
    bool pressed = false;
};

/**
 * @brief Event handling result
 */
enum class EventResult {
    Ignored,      // Event not handled
    Handled,      // Event handled, no redraw
    NeedsRedraw   // Event handled, needs redraw
};

/**
 * @brief Base Widget class
 */
class Widget {
public:
    Widget();
    virtual ~Widget();
    
    // Identification
    WidgetId id() const { return id_; }
    
    // Bounds
    const Rect& bounds() const { return bounds_; }
    void setBounds(const Rect& bounds);
    void setPosition(float x, float y);
    void setSize(float width, float height);
    
    // State
    const WidgetState& state() const { return state_; }
    bool isVisible() const { return state_.visible; }
    bool isEnabled() const { return state_.enabled; }
    bool isFocused() const { return state_.focused; }
    bool isHovered() const { return state_.hovered; }
    bool isPressed() const { return state_.pressed; }
    
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    void setFocused(bool focused);
    
    // Padding & Margin
    EdgeInsets padding() const { return padding_; }
    EdgeInsets margin() const { return margin_; }
    void setPadding(const EdgeInsets& padding) { padding_ = padding; }
    void setMargin(const EdgeInsets& margin) { margin_ = margin; }
    
    // Background & Border
    Color backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }
    
    // ==========================================================================
    // Rendering
    // ==========================================================================
    
    virtual void render(GpuContext* ctx);
    
    // ==========================================================================
    // Layout
    // ==========================================================================
    
    virtual Size measure(const Size& available);
    virtual void layout();
    
    // ==========================================================================
    // Events
    // ==========================================================================
    
    virtual EventResult handleEvent(const Event& event);
    virtual EventResult onMouseDown(float x, float y, MouseButton button);
    virtual EventResult onMouseUp(float x, float y, MouseButton button);
    virtual EventResult onMouseMove(float x, float y);
    virtual EventResult onMouseEnter();
    virtual EventResult onMouseLeave();
    virtual EventResult onKeyDown(KeyCode key, Modifiers mods);
    virtual EventResult onKeyUp(KeyCode key, Modifiers mods);
    virtual EventResult onTextInput(const std::string& text);
    virtual EventResult onFocus();
    virtual EventResult onBlur();
    
    // ==========================================================================
    // Children
    // ==========================================================================
    
    void addChild(std::unique_ptr<Widget> child);
    void removeChild(Widget* child);
    void clearChildren();
    const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }
    Widget* parent() const { return parent_; }
    
    // Find child at point
    Widget* hitTest(float x, float y);
    
protected:
    void renderChildren(GpuContext* ctx);
    void invalidate(); // Request redraw
    
    WidgetId id_;
    Rect bounds_;
    WidgetState state_;
    EdgeInsets padding_;
    EdgeInsets margin_;
    Color backgroundColor_ = Color::transparent();
    Widget* parent_ = nullptr;
    std::vector<std::unique_ptr<Widget>> children_;
    
private:
    static WidgetId nextId_;
};

} // namespace NXRender
