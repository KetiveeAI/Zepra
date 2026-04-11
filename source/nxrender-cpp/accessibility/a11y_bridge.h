// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "accessible.h"
#include "accessibility_tree.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace NXRender {

// ==================================================================
// AT-SPI bridge — connects AccessibilityTree to Linux AT-SPI2
// ==================================================================

class ATSPIBridge {
public:
    ATSPIBridge();
    ~ATSPIBridge();

    bool connect();
    void disconnect();
    bool isConnected() const { return connected_; }

    void setRoot(AccessibleNode* root);

    // Push updates to AT-SPI
    void notifyFocusChanged(AccessibleNode* node);
    void notifyValueChanged(AccessibleNode* node);
    void notifyStateChanged(AccessibleNode* node, const std::string& stateName, bool value);
    void notifyChildrenChanged(AccessibleNode* parent);
    void notifyTextChanged(AccessibleNode* node, int offset, int addedCount, int removedCount);
    void announce(const std::string& text, AccessibleNode::LivePriority priority);

    // AT-SPI action delegation
    using ActionHandler = std::function<bool(AccessibleNode*, const std::string&)>;
    void setActionHandler(ActionHandler handler) { actionHandler_ = handler; }

private:
    bool connected_ = false;
    AccessibleNode* root_ = nullptr;
    ActionHandler actionHandler_;

    // D-Bus connection state (opaque — would be atspi_* handles)
    struct DBusState;
    std::unique_ptr<DBusState> dbus_;

    void registerObject(AccessibleNode* node);
    void unregisterObject(AccessibleNode* node);
    std::string pathForNode(AccessibleNode* node) const;
};

// ==================================================================
// ARIA attribute resolver — maps HTML ARIA attrs to AccessibleNode props
// ==================================================================

struct ARIAAttribute {
    std::string name;
    std::string value;
};

class ARIAResolver {
public:
    static AccessibleRole roleFromString(const std::string& role);
    static std::string roleToString(AccessibleRole role);

    static void applyAttributes(AccessibleNode* node,
                                 const std::vector<ARIAAttribute>& attrs);

    // Implicit ARIA role from HTML tag
    static AccessibleRole implicitRole(const std::string& tagName,
                                        const std::string& type = "");

    // Resolve accessible name from ARIA attributes + DOM content
    struct NameResult {
        std::string name;
        enum class Source { None, AriaLabel, AriaLabelledBy, Title, Contents, Alt, Placeholder } source;
    };
    static NameResult computeName(const std::string& ariaLabel,
                                   const std::string& ariaLabelledBy,
                                   const std::string& title,
                                   const std::string& alt,
                                   const std::string& placeholder,
                                   const std::string& textContent);

    // Resolve accessible description
    static std::string computeDescription(const std::string& ariaDescribedBy,
                                            const std::string& title);

    // Validate ARIA role + state combinations
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> warnings;
    };
    static ValidationResult validate(AccessibleRole role,
                                      const AccessibleState& state,
                                      const std::vector<ARIAAttribute>& attrs);

    // ARIA required states for a role
    static std::vector<std::string> requiredStates(AccessibleRole role);
    static std::vector<std::string> supportedStates(AccessibleRole role);
};

// ==================================================================
// Keyboard navigation map — tab order management
// ==================================================================

class KeyboardNavigationMap {
public:
    struct NavItem {
        AccessibleNode* node = nullptr;
        int tabIndex = 0;
        bool focusable = true;
        bool tabbable = true; // tabindex >= 0
    };

    void rebuild(AccessibleNode* root);

    AccessibleNode* next(AccessibleNode* current) const;
    AccessibleNode* previous(AccessibleNode* current) const;
    AccessibleNode* first() const;
    AccessibleNode* last() const;

    // Arrow key navigation within groups (radio, menu, etc.)
    AccessibleNode* nextInGroup(AccessibleNode* current) const;
    AccessibleNode* previousInGroup(AccessibleNode* current) const;

    // Focus trap (for modals/dialogs)
    void pushFocusTrap(AccessibleNode* container);
    void popFocusTrap();
    bool inFocusTrap() const { return !focusTraps_.empty(); }

    const std::vector<NavItem>& items() const { return items_; }

private:
    std::vector<NavItem> items_;
    std::vector<AccessibleNode*> focusTraps_;

    void collect(AccessibleNode* node);
    void sortByTabIndex();
    bool isInCurrentTrap(AccessibleNode* node) const;
};

// ==================================================================
// Reduced motion / high contrast preferences
// ==================================================================

struct AccessibilityPreferences {
    bool prefersReducedMotion = false;
    bool prefersReducedTransparency = false;
    bool prefersHighContrast = false;
    bool prefersColorScheme = false; // true = dark
    float fontScaleFactor = 1.0f;
    bool forcedColors = false;

    using ChangeCallback = std::function<void(const AccessibilityPreferences&)>;

    static AccessibilityPreferences query();
    static void onChange(ChangeCallback cb);
};

// ==================================================================
// Screen reader output buffer
// ==================================================================

class ScreenReaderOutput {
public:
    struct Utterance {
        std::string text;
        enum class Priority { Low, Medium, High, Alert } priority = Priority::Medium;
        bool interrupt = false;
        double timestamp = 0;
    };

    void speak(const std::string& text,
               Utterance::Priority priority = Utterance::Priority::Medium,
               bool interrupt = false);
    void clear();
    void flush();

    bool hasPending() const { return !queue_.empty(); }
    Utterance dequeue();

    // Virtual cursor position announcement
    void announcePosition(AccessibleNode* node);
    void announceRole(AccessibleNode* node);
    void announceState(AccessibleNode* node);
    void announceValue(AccessibleNode* node);

    // Full node description
    std::string describeNode(AccessibleNode* node) const;

private:
    std::vector<Utterance> queue_;
    double lastUtteranceTime_ = 0;
};

} // namespace NXRender
