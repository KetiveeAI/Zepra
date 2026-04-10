// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "web/box/box_tree.h"
#include "web/paint/hit_test.h"
#include <vector>
#include <functional>
#include <string>
#include <cstdint>

namespace NXRender {
namespace Web {

// ==================================================================
// DOM Event types
// ==================================================================

enum class DOMEventType : uint16_t {
    // Mouse
    Click, DblClick, MouseDown, MouseUp, MouseMove,
    MouseEnter, MouseLeave, MouseOver, MouseOut,
    ContextMenu, AuxClick,

    // Keyboard
    KeyDown, KeyUp, KeyPress,

    // Focus
    Focus, Blur, FocusIn, FocusOut,

    // Input
    Input, Change, Submit, Reset,
    BeforeInput, CompositionStart, CompositionUpdate, CompositionEnd,

    // Touch
    TouchStart, TouchMove, TouchEnd, TouchCancel,

    // Pointer
    PointerDown, PointerUp, PointerMove,
    PointerEnter, PointerLeave, PointerOver, PointerOut,
    PointerCancel, GotPointerCapture, LostPointerCapture,

    // Wheel
    Wheel,

    // Drag
    DragStart, Drag, DragEnd, DragEnter, DragLeave, DragOver, Drop,

    // Scroll
    Scroll, ScrollEnd,

    // Animation/Transition
    AnimationStart, AnimationEnd, AnimationIteration, AnimationCancel,
    TransitionStart, TransitionEnd, TransitionRun, TransitionCancel,

    // Resize
    Resize,

    // Custom
    Custom,
};

// ==================================================================
// DOM Event data
// ==================================================================

struct DOMEventData {
    DOMEventType type = DOMEventType::Custom;

    // Mouse/Pointer
    float clientX = 0, clientY = 0;
    float pageX = 0, pageY = 0;
    float screenX = 0, screenY = 0;
    float offsetX = 0, offsetY = 0;
    float movementX = 0, movementY = 0;
    uint8_t button = 0;
    uint16_t buttons = 0;
    int32_t pointerId = 0;
    float pressure = 0;
    float width = 1, height = 1;
    std::string pointerType = "mouse";

    // Keyboard
    std::string key;
    std::string code;
    uint32_t keyCode = 0;
    bool repeat = false;

    // Modifiers
    bool ctrlKey = false;
    bool shiftKey = false;
    bool altKey = false;
    bool metaKey = false;

    // Wheel
    float deltaX = 0, deltaY = 0, deltaZ = 0;
    uint8_t deltaMode = 0;

    // Touch
    struct TouchPoint {
        int32_t identifier = 0;
        float clientX = 0, clientY = 0;
        float pageX = 0, pageY = 0;
        float radiusX = 0, radiusY = 0;
        float force = 0;
    };
    std::vector<TouchPoint> touches;
    std::vector<TouchPoint> changedTouches;
    std::vector<TouchPoint> targetTouches;

    // Input event
    std::string inputType;
    std::string data;

    // Drag
    std::vector<std::string> dataTransferTypes;

    // Bubbling control
    bool bubbles = true;
    bool cancelable = true;
    bool composed = false;
    bool defaultPrevented = false;
    bool propagationStopped = false;
    bool immediatePropagationStopped = false;

    // Target
    const BoxNode* target = nullptr;
    const BoxNode* currentTarget = nullptr;
    const BoxNode* relatedTarget = nullptr;

    // Phase
    enum class Phase { None, Capturing, AtTarget, Bubbling } phase = Phase::None;

    // Timestamp (ms since epoch)
    double timeStamp = 0;

    void preventDefault() { defaultPrevented = true; }
    void stopPropagation() { propagationStopped = true; }
    void stopImmediatePropagation() {
        propagationStopped = true;
        immediatePropagationStopped = true;
    }
};

// ==================================================================
// Event listener
// ==================================================================

using DOMEventHandler = std::function<void(DOMEventData&)>;

struct EventListener {
    DOMEventType type;
    DOMEventHandler handler;
    bool capture = false;
    bool once = false;
    bool passive = false;
};

// ==================================================================
// Event target mixin
// ==================================================================

class DOMEventTarget {
public:
    virtual ~DOMEventTarget() = default;

    void addEventListener(DOMEventType type, DOMEventHandler handler,
                          bool capture = false, bool once = false, bool passive = false);
    void removeEventListener(DOMEventType type, bool capture = false);
    bool dispatchEvent(DOMEventData& event);

    bool hasEventListeners(DOMEventType type) const;

protected:
    std::vector<EventListener> listeners_;
};

// ==================================================================
// Event router — dispatches platform events through the box tree
// ==================================================================

class EventRouter {
public:
    EventRouter();
    ~EventRouter();

    void setRootBox(const BoxNode* root) { root_ = root; }
    void setHitTestEngine(HitTestEngine* engine) { hitTester_ = engine; }

    // Platform event entry points
    void handleMouseDown(float x, float y, uint8_t button, bool ctrl, bool shift, bool alt);
    void handleMouseUp(float x, float y, uint8_t button, bool ctrl, bool shift, bool alt);
    void handleMouseMove(float x, float y, bool ctrl, bool shift, bool alt);
    void handleWheel(float x, float y, float deltaX, float deltaY, bool ctrl, bool shift);
    void handleKeyDown(const std::string& key, const std::string& code,
                       uint32_t keyCode, bool ctrl, bool shift, bool alt, bool meta);
    void handleKeyUp(const std::string& key, const std::string& code,
                     uint32_t keyCode, bool ctrl, bool shift, bool alt, bool meta);
    void handleTextInput(const std::string& text);
    void handleTouchStart(const std::vector<DOMEventData::TouchPoint>& touches);
    void handleTouchMove(const std::vector<DOMEventData::TouchPoint>& touches);
    void handleTouchEnd(const std::vector<DOMEventData::TouchPoint>& touches);

    // Focus management
    void setFocusedNode(const BoxNode* node) { focusedNode_ = node; }
    const BoxNode* focusedNode() const { return focusedNode_; }
    void moveFocusForward();
    void moveFocusBackward();

    // Hover tracking
    const BoxNode* hoveredNode() const { return hoveredNode_; }

    // Pointer capture
    void setPointerCapture(const BoxNode* node, int32_t pointerId);
    void releasePointerCapture(const BoxNode* node, int32_t pointerId);
    bool hasPointerCapture(const BoxNode* node, int32_t pointerId) const;

    // Event delegation
    using DelegationHandler = std::function<bool(const BoxNode*, DOMEventData&)>;
    void addDelegation(DOMEventType type, DelegationHandler handler);

    // Cursor management
    enum class Cursor : uint8_t {
        Auto, Default, Pointer, Crosshair, Text, Move,
        NotAllowed, Wait, Progress, Help, Grab, Grabbing,
        NResize, SResize, EResize, WResize,
        NEResize, NWResize, SEResize, SWResize,
        ColResize, RowResize, None,
    };
    Cursor currentCursor() const { return currentCursor_; }

private:
    const BoxNode* root_ = nullptr;
    HitTestEngine* hitTester_ = nullptr;
    const BoxNode* focusedNode_ = nullptr;
    const BoxNode* hoveredNode_ = nullptr;
    const BoxNode* activeNode_ = nullptr;
    Cursor currentCursor_ = Cursor::Default;

    // Pointer capture map
    struct PointerCapture {
        int32_t pointerId;
        const BoxNode* target;
    };
    std::vector<PointerCapture> pointerCaptures_;

    // Delegation handlers
    struct DelegationEntry {
        DOMEventType type;
        DelegationHandler handler;
    };
    std::vector<DelegationEntry> delegations_;

    // Previous mouse position for movement delta
    float prevMouseX_ = 0, prevMouseY_ = 0;
    bool mouseDown_ = false;
    uint8_t pressedButton_ = 0;

    // Click tracking
    double lastClickTime_ = 0;
    float lastClickX_ = 0, lastClickY_ = 0;
    int clickCount_ = 0;

    // Build event path (capture → target → bubble)
    std::vector<const BoxNode*> buildEventPath(const BoxNode* target);

    // Dispatch event along path
    bool dispatchAlongPath(DOMEventData& event,
                           const std::vector<const BoxNode*>& path);

    // Fire enter/leave events for hover transitions
    void updateHover(const BoxNode* newHovered, float x, float y);

    // Update cursor from node style
    void updateCursor(const BoxNode* node);

    // Check if node is focusable
    bool isFocusable(const BoxNode* node) const;

    // Find next/previous focusable node
    const BoxNode* findNextFocusable(const BoxNode* from, bool forward) const;
};

} // namespace Web
} // namespace NXRender
