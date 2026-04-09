// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "event.h"
#include "event_router.h"
#include <vector>
#include <functional>

namespace NXRender {
namespace Input {

enum class FocusOrigin {
    None,
    Mouse,
    Keyboard,
    Programmatic
};

using FocusChangeCallback = std::function<void(EventTarget* oldTarget, EventTarget* newTarget)>;

class FocusManager {
public:
    FocusManager(EventRouter* router);
    ~FocusManager();

    void setFocusedTarget(EventTarget* target, FocusOrigin origin = FocusOrigin::Programmatic);
    EventTarget* focusedTarget() const { return currentFocus_; }

    void clearFocus();
    bool requestFocusNext();
    bool requestFocusPrevious();

    bool isKeyboardFocused() const { return currentOrigin_ == FocusOrigin::Keyboard; }
    bool isFocused(EventTarget* target) const;

    // Focus chain management
    void setFocusChain(const std::vector<EventTarget*>& chain);
    void addToFocusChain(EventTarget* target);
    void removeFromFocusChain(EventTarget* target);
    int focusChainIndex(EventTarget* target) const;
    size_t focusChainSize() const { return focusChain_.size(); }

    // Settings
    void setWrapFocus(bool wrap) { wrapFocus_ = wrap; }
    bool wrapFocus() const { return wrapFocus_; }

    // Listeners
    void addFocusChangeListener(FocusChangeCallback callback);

    bool handleGlobalKeyEvent(const KeyEvent& event);

private:
    EventRouter* router_;
    EventTarget* currentFocus_ = nullptr;
    FocusOrigin currentOrigin_ = FocusOrigin::None;

    std::vector<EventTarget*> focusChain_;
    bool wrapFocus_ = true;
    std::vector<FocusChangeCallback> focusChangeListeners_;

    EventTarget* findNextFocusable(EventTarget* current, bool reverse) const;
};

} // namespace Input
} // namespace NXRender
