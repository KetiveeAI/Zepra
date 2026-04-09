// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "focus_manager.h"
#include <algorithm>
#include <vector>

namespace NXRender {
namespace Input {

FocusManager::FocusManager(EventRouter* router) : router_(router) {}

FocusManager::~FocusManager() {
    clearFocus();
}

// ==================================================================
// Focus set/clear
// ==================================================================

void FocusManager::clearFocus() {
    if (!currentFocus_) return;

    EventTarget* oldFocus = currentFocus_;
    currentFocus_ = nullptr;
    currentOrigin_ = FocusOrigin::None;

    EventModifiers emptyMods;
    Event blurEvent(EventType::FocusOut, false);
    blurEvent.setTarget(oldFocus);
    blurEvent.setPhase(EventPhase::AtTarget);
    blurEvent.setCurrentTarget(oldFocus);

    if (router_) {
        router_->dispatchEvent(oldFocus, blurEvent);
    }

    // Notify listeners
    for (auto& listener : focusChangeListeners_) {
        if (listener) listener(oldFocus, nullptr);
    }
}

void FocusManager::setFocusedTarget(EventTarget* target, FocusOrigin origin) {
    if (currentFocus_ == target) {
        currentOrigin_ = origin;
        return;
    }

    EventTarget* oldFocus = currentFocus_;
    clearFocus();

    if (target) {
        currentFocus_ = target;
        currentOrigin_ = origin;

        EventModifiers emptyMods;
        Event focusEvent(EventType::FocusIn, false);

        if (router_) {
            router_->dispatchEvent(target, focusEvent);
        }

        // Notify listeners
        for (auto& listener : focusChangeListeners_) {
            if (listener) listener(oldFocus, target);
        }
    }
}

// ==================================================================
// Focus chain management
// ==================================================================

void FocusManager::setFocusChain(const std::vector<EventTarget*>& chain) {
    focusChain_ = chain;
}

void FocusManager::addToFocusChain(EventTarget* target) {
    if (!target) return;
    // Avoid duplicates
    for (const auto& t : focusChain_) {
        if (t == target) return;
    }
    focusChain_.push_back(target);
}

void FocusManager::removeFromFocusChain(EventTarget* target) {
    focusChain_.erase(
        std::remove(focusChain_.begin(), focusChain_.end(), target),
        focusChain_.end()
    );
    if (currentFocus_ == target) {
        clearFocus();
    }
}

// ==================================================================
// Focus traversal
// ==================================================================

EventTarget* FocusManager::findNextFocusable(EventTarget* current, bool reverse) const {
    if (focusChain_.empty()) return nullptr;

    // Find current position in chain
    int currentIdx = -1;
    for (size_t i = 0; i < focusChain_.size(); i++) {
        if (focusChain_[i] == current) {
            currentIdx = static_cast<int>(i);
            break;
        }
    }

    if (currentIdx < 0) {
        // Not in chain — return first/last depending on direction
        return reverse ? focusChain_.back() : focusChain_.front();
    }

    if (reverse) {
        // Move backward, wrapping
        int nextIdx = currentIdx - 1;
        if (nextIdx < 0) {
            if (wrapFocus_) {
                nextIdx = static_cast<int>(focusChain_.size()) - 1;
            } else {
                return nullptr;
            }
        }
        return focusChain_[nextIdx];
    } else {
        // Move forward, wrapping
        int nextIdx = currentIdx + 1;
        if (nextIdx >= static_cast<int>(focusChain_.size())) {
            if (wrapFocus_) {
                nextIdx = 0;
            } else {
                return nullptr;
            }
        }
        return focusChain_[nextIdx];
    }
}

bool FocusManager::requestFocusNext() {
    EventTarget* next = findNextFocusable(currentFocus_, false);
    if (next) {
        setFocusedTarget(next, FocusOrigin::Keyboard);
        return true;
    }
    return false;
}

bool FocusManager::requestFocusPrevious() {
    EventTarget* prev = findNextFocusable(currentFocus_, true);
    if (prev) {
        setFocusedTarget(prev, FocusOrigin::Keyboard);
        return true;
    }
    return false;
}

// ==================================================================
// Global key handling
// ==================================================================

bool FocusManager::handleGlobalKeyEvent(const KeyEvent& event) {
    if (event.type() == EventType::KeyDown && event.keyString() == "Tab") {
        if (event.modifiers().shift) {
            requestFocusPrevious();
        } else {
            requestFocusNext();
        }
        return true;
    }

    // Escape clears focus
    if (event.type() == EventType::KeyDown && event.keyString() == "Escape") {
        clearFocus();
        return true;
    }

    return false;
}

// ==================================================================
// Focus query
// ==================================================================

bool FocusManager::isFocused(EventTarget* target) const {
    return currentFocus_ == target && target != nullptr;
}

int FocusManager::focusChainIndex(EventTarget* target) const {
    for (size_t i = 0; i < focusChain_.size(); i++) {
        if (focusChain_[i] == target) return static_cast<int>(i);
    }
    return -1;
}

// ==================================================================
// Listener management
// ==================================================================

void FocusManager::addFocusChangeListener(FocusChangeCallback callback) {
    focusChangeListeners_.push_back(std::move(callback));
}

} // namespace Input
} // namespace NXRender
