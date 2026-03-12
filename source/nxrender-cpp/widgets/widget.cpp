// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file widget.cpp
 * @brief Base Widget implementation
 */

#include "widgets/widget.h"
#include "nxgfx/context.h"

namespace NXRender {

WidgetId Widget::nextId_ = 1;

Widget::Widget() : id_(nextId_++) {}

Widget::~Widget() = default;

void Widget::setBounds(const Rect& bounds) {
    bounds_ = bounds;
}

void Widget::setPosition(float x, float y) {
    bounds_.x = x;
    bounds_.y = y;
}

void Widget::setSize(float width, float height) {
    bounds_.width = width;
    bounds_.height = height;
}

void Widget::setVisible(bool visible) {
    state_.visible = visible;
}

void Widget::setEnabled(bool enabled) {
    state_.enabled = enabled;
}

void Widget::setFocused(bool focused) {
    state_.focused = focused;
}

void Widget::render(GpuContext* ctx) {
    if (!state_.visible) return;
    
    // Draw background if set
    if (backgroundColor_.a > 0) {
        ctx->fillRect(bounds_, backgroundColor_);
    }
    
    // Render children
    renderChildren(ctx);
}

void Widget::renderChildren(GpuContext* ctx) {
    for (auto& child : children_) {
        if (child->isVisible()) {
            child->render(ctx);
        }
    }
}

Size Widget::measure(const Size& available) {
    // Default: use current bounds size
    return Size(bounds_.width, bounds_.height);
}

void Widget::layout() {
    // Default: do nothing
}

EventResult Widget::handleEvent(const Event& event) {
    if (!state_.enabled) return EventResult::Ignored;
    
    switch (event.type) {
        case EventType::MouseDown:
            return onMouseDown(event.mouse.x, event.mouse.y, event.mouse.button);
        case EventType::MouseUp:
            return onMouseUp(event.mouse.x, event.mouse.y, event.mouse.button);
        case EventType::MouseMove:
            return onMouseMove(event.mouse.x, event.mouse.y);
        case EventType::MouseEnter:
            return onMouseEnter();
        case EventType::MouseLeave:
            return onMouseLeave();
        case EventType::KeyDown:
            return onKeyDown(event.key.key, event.key.modifiers);
        case EventType::KeyUp:
            return onKeyUp(event.key.key, event.key.modifiers);
        case EventType::TextInput:
            return onTextInput(event.textInput);
        case EventType::Focus:
            return onFocus();
        case EventType::Blur:
            return onBlur();
        default:
            return EventResult::Ignored;
    }
}

EventResult Widget::onMouseDown(float x, float y, MouseButton button) {
    (void)x; (void)y; (void)button;
    return EventResult::Ignored;
}

EventResult Widget::onMouseUp(float x, float y, MouseButton button) {
    (void)x; (void)y; (void)button;
    return EventResult::Ignored;
}

EventResult Widget::onMouseMove(float x, float y) {
    (void)x; (void)y;
    return EventResult::Ignored;
}

EventResult Widget::onMouseEnter() {
    state_.hovered = true;
    return EventResult::NeedsRedraw;
}

EventResult Widget::onMouseLeave() {
    state_.hovered = false;
    return EventResult::NeedsRedraw;
}

EventResult Widget::onKeyDown(KeyCode key, Modifiers mods) {
    (void)key; (void)mods;
    return EventResult::Ignored;
}

EventResult Widget::onKeyUp(KeyCode key, Modifiers mods) {
    (void)key; (void)mods;
    return EventResult::Ignored;
}

EventResult Widget::onTextInput(const std::string& text) {
    (void)text;
    return EventResult::Ignored;
}

EventResult Widget::onFocus() {
    state_.focused = true;
    return EventResult::NeedsRedraw;
}

EventResult Widget::onBlur() {
    state_.focused = false;
    return EventResult::NeedsRedraw;
}

void Widget::addChild(std::unique_ptr<Widget> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
}

void Widget::removeChild(Widget* child) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const auto& ptr) { return ptr.get() == child; });
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

void Widget::clearChildren() {
    for (auto& child : children_) {
        child->parent_ = nullptr;
    }
    children_.clear();
}

Widget* Widget::hitTest(float x, float y) {
    if (!state_.visible || !bounds_.contains(x, y)) {
        return nullptr;
    }
    
    // Check children in reverse order (top to bottom)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (Widget* hit = (*it)->hitTest(x, y)) {
            return hit;
        }
    }
    
    return this;
}

void Widget::invalidate() {
    // Would notify compositor of damage
}

} // namespace NXRender
