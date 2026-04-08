// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file clipboard.h
 * @brief X11 clipboard integration for CLIPBOARD and PRIMARY selections.
 */

#pragma once

#include <string>
#include <functional>
#include <cstdint>

namespace NXRender {
namespace Input {

enum class ClipboardSelection {
    Clipboard,    // CLIPBOARD (Ctrl+C/V)
    Primary       // PRIMARY (middle-click paste on X11)
};

/**
 * @brief X11 clipboard manager.
 *
 * Provides read/write access to X11 CLIPBOARD and PRIMARY selections.
 * Async: writing is synchronous (we own the selection), reading requires
 * an X11 event roundtrip handled via processEvents().
 */
class Clipboard {
public:
    static Clipboard& instance();

    /**
     * @brief Initialize clipboard with the X11 display and window.
     * Must be called after window creation.
     */
    bool init(void* display, unsigned long window);

    /**
     * @brief Shutdown and release clipboard ownership.
     */
    void shutdown();

    /**
     * @brief Write text to a selection.
     * Takes ownership of the selection. Data is served via SelectionRequest events.
     */
    void setText(const std::string& text, ClipboardSelection sel = ClipboardSelection::Clipboard);

    /**
     * @brief Read text from a selection (async).
     * Initiates a ConvertSelection request. The callback is invoked when
     * the selection data is received via a SelectionNotify event.
     */
    void getText(std::function<void(const std::string&)> callback,
                 ClipboardSelection sel = ClipboardSelection::Clipboard);

    /**
     * @brief Get the most recently written text (from our own writes).
     * Synchronous. Only returns data if we own the selection.
     */
    const std::string& ownedText(ClipboardSelection sel = ClipboardSelection::Clipboard) const;

    /**
     * @brief Check if we currently own a selection.
     */
    bool ownsSelection(ClipboardSelection sel = ClipboardSelection::Clipboard) const;

    /**
     * @brief Process an X11 SelectionRequest event (serve our clipboard data to requestors).
     * Called from the event loop when we receive a SelectionRequest.
     */
    void handleSelectionRequest(void* event);

    /**
     * @brief Process an X11 SelectionNotify event (received clipboard data from owner).
     * Called from the event loop when we receive a SelectionNotify.
     */
    void handleSelectionNotify(void* event);

    /**
     * @brief Check if we lost selection ownership.
     */
    void handleSelectionClear(void* event);

private:
    Clipboard() = default;
    Clipboard(const Clipboard&) = delete;
    Clipboard& operator=(const Clipboard&) = delete;

    void* display_ = nullptr;
    unsigned long window_ = 0;

    std::string clipboardText_;
    std::string primaryText_;
    bool ownsClipboard_ = false;
    bool ownsPrimary_ = false;

    std::function<void(const std::string&)> pendingCallback_;
    ClipboardSelection pendingSelection_ = ClipboardSelection::Clipboard;

    // X11 atoms (stored as unsigned long to avoid Xlib header dependency)
    unsigned long atomClipboard_ = 0;
    unsigned long atomTargets_ = 0;
    unsigned long atomUtf8_ = 0;
    unsigned long atomNxClip_ = 0;
};

} // namespace Input
} // namespace NXRender
