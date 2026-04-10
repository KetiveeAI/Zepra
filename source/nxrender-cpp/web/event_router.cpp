// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "event_router.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace NXRender {
namespace Web {

// ==================================================================
// DOMEventTarget
// ==================================================================

void DOMEventTarget::addEventListener(DOMEventType type, DOMEventHandler handler,
                                       bool capture, bool once, bool passive) {
    listeners_.push_back({type, std::move(handler), capture, once, passive});
}

void DOMEventTarget::removeEventListener(DOMEventType type, bool capture) {
    listeners_.erase(
        std::remove_if(listeners_.begin(), listeners_.end(),
                        [&](const EventListener& l) {
                            return l.type == type && l.capture == capture;
                        }),
        listeners_.end());
}

bool DOMEventTarget::dispatchEvent(DOMEventData& event) {
    for (auto it = listeners_.begin(); it != listeners_.end(); ) {
        if (it->type != event.type) { ++it; continue; }

        // Phase check
        bool shouldFire = false;
        if (event.phase == DOMEventData::Phase::Capturing && it->capture) shouldFire = true;
        if (event.phase == DOMEventData::Phase::AtTarget) shouldFire = true;
        if (event.phase == DOMEventData::Phase::Bubbling && !it->capture) shouldFire = true;

        if (shouldFire) {
            it->handler(event);

            if (it->once) {
                it = listeners_.erase(it);
                continue;
            }
        }

        if (event.immediatePropagationStopped) break;
        ++it;
    }
    return !event.defaultPrevented;
}

bool DOMEventTarget::hasEventListeners(DOMEventType type) const {
    for (const auto& l : listeners_) {
        if (l.type == type) return true;
    }
    return false;
}

// ==================================================================
// EventRouter
// ==================================================================

EventRouter::EventRouter() {}
EventRouter::~EventRouter() {}

static double currentTimeMs() {
    auto now = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return static_cast<double>(ms.count());
}

void EventRouter::handleMouseDown(float x, float y, uint8_t button,
                                    bool ctrl, bool shift, bool alt) {
    if (!root_ || !hitTester_) return;

    mouseDown_ = true;
    pressedButton_ = button;

    auto hit = hitTester_->hitTestFirst(root_, x, y);

    DOMEventData event;
    event.type = DOMEventType::MouseDown;
    event.clientX = x; event.clientY = y;
    event.pageX = x; event.pageY = y;
    event.button = button;
    event.ctrlKey = ctrl; event.shiftKey = shift; event.altKey = alt;
    event.target = hit.node;
    event.timeStamp = currentTimeMs();

    if (hit.node) {
        activeNode_ = hit.node;

        // Focus management
        if (isFocusable(hit.node) && hit.node != focusedNode_) {
            const BoxNode* prevFocused = focusedNode_;
            focusedNode_ = hit.node;

            if (prevFocused) {
                DOMEventData blurEvent;
                blurEvent.type = DOMEventType::Blur;
                blurEvent.target = prevFocused;
                blurEvent.timeStamp = event.timeStamp;
                auto path = buildEventPath(prevFocused);
                dispatchAlongPath(blurEvent, path);
            }

            DOMEventData focusEvent;
            focusEvent.type = DOMEventType::Focus;
            focusEvent.target = focusedNode_;
            focusEvent.timeStamp = event.timeStamp;
            auto path = buildEventPath(focusedNode_);
            dispatchAlongPath(focusEvent, path);
        }

        auto path = buildEventPath(hit.node);
        dispatchAlongPath(event, path);
    }

    prevMouseX_ = x; prevMouseY_ = y;
}

void EventRouter::handleMouseUp(float x, float y, uint8_t button,
                                  bool ctrl, bool shift, bool alt) {
    if (!root_ || !hitTester_) return;

    mouseDown_ = false;
    auto hit = hitTester_->hitTestFirst(root_, x, y);

    DOMEventData event;
    event.type = DOMEventType::MouseUp;
    event.clientX = x; event.clientY = y;
    event.pageX = x; event.pageY = y;
    event.button = button;
    event.ctrlKey = ctrl; event.shiftKey = shift; event.altKey = alt;
    event.target = hit.node;
    event.timeStamp = currentTimeMs();

    if (hit.node) {
        auto path = buildEventPath(hit.node);
        dispatchAlongPath(event, path);

        // Generate click event if mouse up on same node as mouse down
        if (hit.node == activeNode_) {
            DOMEventData clickEvent;
            clickEvent.type = DOMEventType::Click;
            clickEvent.clientX = x; clickEvent.clientY = y;
            clickEvent.pageX = x; clickEvent.pageY = y;
            clickEvent.button = button;
            clickEvent.ctrlKey = ctrl; clickEvent.shiftKey = shift; clickEvent.altKey = alt;
            clickEvent.target = hit.node;
            clickEvent.timeStamp = currentTimeMs();

            // Double-click detection
            double now = clickEvent.timeStamp;
            float distSq = (x - lastClickX_) * (x - lastClickX_) +
                            (y - lastClickY_) * (y - lastClickY_);
            if (now - lastClickTime_ < 300 && distSq < 25) {
                clickCount_++;
            } else {
                clickCount_ = 1;
            }
            lastClickTime_ = now;
            lastClickX_ = x; lastClickY_ = y;

            dispatchAlongPath(clickEvent, path);

            if (clickCount_ == 2) {
                DOMEventData dblEvent;
                dblEvent.type = DOMEventType::DblClick;
                dblEvent.clientX = x; dblEvent.clientY = y;
                dblEvent.target = hit.node;
                dblEvent.timeStamp = currentTimeMs();
                dispatchAlongPath(dblEvent, path);
            }
        }
    }

    activeNode_ = nullptr;
    prevMouseX_ = x; prevMouseY_ = y;
}

void EventRouter::handleMouseMove(float x, float y, bool ctrl, bool shift, bool alt) {
    if (!root_ || !hitTester_) return;

    auto hit = hitTester_->hitTestFirst(root_, x, y);

    // Hover tracking
    updateHover(hit.node, x, y);

    DOMEventData event;
    event.type = DOMEventType::MouseMove;
    event.clientX = x; event.clientY = y;
    event.pageX = x; event.pageY = y;
    event.movementX = x - prevMouseX_;
    event.movementY = y - prevMouseY_;
    event.ctrlKey = ctrl; event.shiftKey = shift; event.altKey = alt;
    event.target = hit.node;
    event.timeStamp = currentTimeMs();
    event.buttons = mouseDown_ ? (1 << pressedButton_) : 0;

    if (hit.node) {
        auto path = buildEventPath(hit.node);
        dispatchAlongPath(event, path);
    }

    prevMouseX_ = x; prevMouseY_ = y;
}

void EventRouter::handleWheel(float x, float y, float deltaX, float deltaY,
                                bool ctrl, bool shift) {
    if (!root_ || !hitTester_) return;

    auto hit = hitTester_->hitTestFirst(root_, x, y);

    DOMEventData event;
    event.type = DOMEventType::Wheel;
    event.clientX = x; event.clientY = y;
    event.deltaX = deltaX; event.deltaY = deltaY;
    event.ctrlKey = ctrl; event.shiftKey = shift;
    event.target = hit.node;
    event.timeStamp = currentTimeMs();

    if (hit.node) {
        auto path = buildEventPath(hit.node);
        dispatchAlongPath(event, path);
    }
}

void EventRouter::handleKeyDown(const std::string& key, const std::string& code,
                                  uint32_t keyCode, bool ctrl, bool shift, bool alt, bool meta) {
    if (!focusedNode_) return;

    // Tab key → focus traversal
    if (key == "Tab") {
        if (shift) moveFocusBackward();
        else moveFocusForward();
        return;
    }

    DOMEventData event;
    event.type = DOMEventType::KeyDown;
    event.key = key; event.code = code; event.keyCode = keyCode;
    event.ctrlKey = ctrl; event.shiftKey = shift; event.altKey = alt; event.metaKey = meta;
    event.target = focusedNode_;
    event.timeStamp = currentTimeMs();

    auto path = buildEventPath(focusedNode_);
    dispatchAlongPath(event, path);
}

void EventRouter::handleKeyUp(const std::string& key, const std::string& code,
                                uint32_t keyCode, bool ctrl, bool shift, bool alt, bool meta) {
    if (!focusedNode_) return;

    DOMEventData event;
    event.type = DOMEventType::KeyUp;
    event.key = key; event.code = code; event.keyCode = keyCode;
    event.ctrlKey = ctrl; event.shiftKey = shift; event.altKey = alt; event.metaKey = meta;
    event.target = focusedNode_;
    event.timeStamp = currentTimeMs();

    auto path = buildEventPath(focusedNode_);
    dispatchAlongPath(event, path);
}

void EventRouter::handleTextInput(const std::string& text) {
    if (!focusedNode_) return;

    DOMEventData event;
    event.type = DOMEventType::Input;
    event.data = text;
    event.inputType = "insertText";
    event.target = focusedNode_;
    event.timeStamp = currentTimeMs();

    auto path = buildEventPath(focusedNode_);
    dispatchAlongPath(event, path);
}

void EventRouter::handleTouchStart(const std::vector<DOMEventData::TouchPoint>& touches) {
    if (!root_ || !hitTester_ || touches.empty()) return;

    auto hit = hitTester_->hitTestFirst(root_, touches[0].clientX, touches[0].clientY);

    DOMEventData event;
    event.type = DOMEventType::TouchStart;
    event.touches = touches;
    event.changedTouches = touches;
    event.target = hit.node;
    event.timeStamp = currentTimeMs();

    if (hit.node) {
        auto path = buildEventPath(hit.node);
        dispatchAlongPath(event, path);
    }
}

void EventRouter::handleTouchMove(const std::vector<DOMEventData::TouchPoint>& touches) {
    if (!root_ || !hitTester_ || touches.empty()) return;

    auto hit = hitTester_->hitTestFirst(root_, touches[0].clientX, touches[0].clientY);

    DOMEventData event;
    event.type = DOMEventType::TouchMove;
    event.touches = touches;
    event.changedTouches = touches;
    event.target = hit.node;
    event.timeStamp = currentTimeMs();

    if (hit.node) {
        auto path = buildEventPath(hit.node);
        dispatchAlongPath(event, path);
    }
}

void EventRouter::handleTouchEnd(const std::vector<DOMEventData::TouchPoint>& touches) {
    if (!root_) return;

    DOMEventData event;
    event.type = DOMEventType::TouchEnd;
    event.changedTouches = touches;
    event.target = hoveredNode_;
    event.timeStamp = currentTimeMs();

    if (hoveredNode_) {
        auto path = buildEventPath(hoveredNode_);
        dispatchAlongPath(event, path);
    }
}

// ==================================================================
// Focus management
// ==================================================================

void EventRouter::moveFocusForward() {
    const BoxNode* next = findNextFocusable(focusedNode_, true);
    if (next && next != focusedNode_) {
        const BoxNode* prev = focusedNode_;
        focusedNode_ = next;

        if (prev) {
            DOMEventData blur;
            blur.type = DOMEventType::Blur;
            blur.target = prev;
            blur.timeStamp = currentTimeMs();
            auto path = buildEventPath(prev);
            dispatchAlongPath(blur, path);
        }

        DOMEventData focus;
        focus.type = DOMEventType::Focus;
        focus.target = focusedNode_;
        focus.timeStamp = currentTimeMs();
        auto path = buildEventPath(focusedNode_);
        dispatchAlongPath(focus, path);
    }
}

void EventRouter::moveFocusBackward() {
    const BoxNode* next = findNextFocusable(focusedNode_, false);
    if (next && next != focusedNode_) {
        const BoxNode* prev = focusedNode_;
        focusedNode_ = next;

        if (prev) {
            DOMEventData blur;
            blur.type = DOMEventType::Blur;
            blur.target = prev;
            blur.timeStamp = currentTimeMs();
            auto path = buildEventPath(prev);
            dispatchAlongPath(blur, path);
        }

        DOMEventData focus;
        focus.type = DOMEventType::Focus;
        focus.target = focusedNode_;
        focus.timeStamp = currentTimeMs();
        auto path = buildEventPath(focusedNode_);
        dispatchAlongPath(focus, path);
    }
}

void EventRouter::setPointerCapture(const BoxNode* node, int32_t pointerId) {
    releasePointerCapture(node, pointerId);
    pointerCaptures_.push_back({pointerId, node});
}

void EventRouter::releasePointerCapture(const BoxNode* node, int32_t pointerId) {
    pointerCaptures_.erase(
        std::remove_if(pointerCaptures_.begin(), pointerCaptures_.end(),
                        [&](const PointerCapture& pc) {
                            return pc.pointerId == pointerId && pc.target == node;
                        }),
        pointerCaptures_.end());
}

bool EventRouter::hasPointerCapture(const BoxNode* node, int32_t pointerId) const {
    for (const auto& pc : pointerCaptures_) {
        if (pc.pointerId == pointerId && pc.target == node) return true;
    }
    return false;
}

void EventRouter::addDelegation(DOMEventType type, DelegationHandler handler) {
    delegations_.push_back({type, std::move(handler)});
}

// ==================================================================
// Event path construction
// ==================================================================

std::vector<const BoxNode*> EventRouter::buildEventPath(const BoxNode* target) {
    std::vector<const BoxNode*> path;
    const BoxNode* node = target;
    while (node) {
        path.push_back(node);
        node = node->parent();
    }
    // Path is target → root, reverse for capture phase
    std::reverse(path.begin(), path.end());
    return path;
}

bool EventRouter::dispatchAlongPath(DOMEventData& event,
                                      const std::vector<const BoxNode*>& path) {
    if (path.empty()) return true;

    // Check delegation handlers first
    for (auto& d : delegations_) {
        if (d.type == event.type) {
            if (d.handler(event.target, event)) return !event.defaultPrevented;
        }
    }

    // Note: In a full implementation, each BoxNode would have its own
    // DOMEventTarget with listeners. Here we dispatch structurally.
    // The actual handler invocation would require mapping BoxNode → JS handler.
    // For now, this infrastructure provides the correct path and phase semantics.

    // Capture phase (root → target-1)
    event.phase = DOMEventData::Phase::Capturing;
    for (size_t i = 0; i < path.size() - 1; i++) {
        event.currentTarget = path[i];
        if (event.propagationStopped) break;
    }

    // At target
    if (!event.propagationStopped) {
        event.phase = DOMEventData::Phase::AtTarget;
        event.currentTarget = path.back();
    }

    // Bubble phase (target+1 → root)
    if (!event.propagationStopped && event.bubbles) {
        event.phase = DOMEventData::Phase::Bubbling;
        for (int i = static_cast<int>(path.size()) - 2; i >= 0; i--) {
            if (event.propagationStopped) break;
            event.currentTarget = path[i];
        }
    }

    return !event.defaultPrevented;
}

// ==================================================================
// Hover management
// ==================================================================

void EventRouter::updateHover(const BoxNode* newHovered, float x, float y) {
    if (newHovered == hoveredNode_) return;

    const BoxNode* oldHovered = hoveredNode_;
    hoveredNode_ = newHovered;

    // MouseLeave on old
    if (oldHovered) {
        DOMEventData leave;
        leave.type = DOMEventType::MouseLeave;
        leave.clientX = x; leave.clientY = y;
        leave.target = oldHovered;
        leave.relatedTarget = newHovered;
        leave.timeStamp = currentTimeMs();
        leave.bubbles = false;
        auto path = buildEventPath(oldHovered);
        dispatchAlongPath(leave, path);
    }

    // MouseEnter on new
    if (newHovered) {
        DOMEventData enter;
        enter.type = DOMEventType::MouseEnter;
        enter.clientX = x; enter.clientY = y;
        enter.target = newHovered;
        enter.relatedTarget = oldHovered;
        enter.timeStamp = currentTimeMs();
        enter.bubbles = false;
        auto path = buildEventPath(newHovered);
        dispatchAlongPath(enter, path);

        updateCursor(newHovered);
    } else {
        currentCursor_ = Cursor::Default;
    }
}

void EventRouter::updateCursor(const BoxNode* node) {
    if (!node) { currentCursor_ = Cursor::Default; return; }

    // Determine cursor from node tag/style
    const std::string& tag = node->tag();
    if (tag == "a" || tag == "button") { currentCursor_ = Cursor::Pointer; return; }
    if (tag == "input" || tag == "textarea") { currentCursor_ = Cursor::Text; return; }

    currentCursor_ = Cursor::Default;
}

bool EventRouter::isFocusable(const BoxNode* node) const {
    if (!node || !node->isVisible() || node->pointerEventsNone()) return false;

    const std::string& tag = node->tag();
    return tag == "input" || tag == "textarea" || tag == "select" ||
           tag == "button" || tag == "a" || tag == "details" ||
           tag == "summary";
}

const BoxNode* EventRouter::findNextFocusable(const BoxNode* from, bool forward) const {
    if (!root_) return nullptr;

    // DFS traversal to find all focusable nodes
    std::vector<const BoxNode*> focusable;
    std::function<void(const BoxNode*)> collect = [&](const BoxNode* node) {
        if (!node) return;
        if (isFocusable(node)) focusable.push_back(node);
        for (size_t i = 0; i < node->childCount(); i++) {
            collect(node->childAt(i));
        }
    };
    collect(root_);

    if (focusable.empty()) return nullptr;

    // Find current position
    auto it = std::find(focusable.begin(), focusable.end(), from);
    if (it == focusable.end()) {
        return forward ? focusable.front() : focusable.back();
    }

    if (forward) {
        ++it;
        return (it == focusable.end()) ? focusable.front() : *it;
    } else {
        if (it == focusable.begin()) return focusable.back();
        --it;
        return *it;
    }
}

} // namespace Web
} // namespace NXRender
