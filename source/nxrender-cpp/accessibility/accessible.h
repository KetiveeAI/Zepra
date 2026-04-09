// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file accessible.h
 * @brief Accessibility API for NXRender widgets.
 *
 * Provides a WAI-ARIA-compatible accessibility tree that screen readers
 * and assistive technologies can query. Each widget can expose an
 * AccessibleNode with role, name, description, state, and actions.
 */

#pragma once

#include "../nxgfx/primitives.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

namespace NXRender {

class Widget;

/**
 * @brief ARIA roles for widgets.
 */
enum class AccessibleRole : uint8_t {
    None,
    Button,
    Checkbox,
    RadioButton,
    Slider,
    TextBox,
    Label,
    Link,
    List,
    ListItem,
    Menu,
    MenuItem,
    MenuBar,
    Tab,
    TabPanel,
    TabList,
    Tree,
    TreeItem,
    Grid,
    GridCell,
    Row,
    ColumnHeader,
    RowHeader,
    Dialog,
    Alert,
    AlertDialog,
    Tooltip,
    ProgressBar,
    ScrollBar,
    Separator,
    Toolbar,
    StatusBar,
    Group,
    Region,
    Heading,
    Image,
    Document,
    Application,
    Window
};

/**
 * @brief Accessibility states.
 */
struct AccessibleState {
    bool disabled = false;
    bool focused = false;
    bool checked = false;
    bool selected = false;
    bool expanded = false;
    bool readonly_ = false;
    bool required = false;
    bool busy = false;
    bool hidden = false;
    bool pressed = false;
    bool invalid = false;

    // Value range
    bool hasValue = false;
    float valueNow = 0;
    float valueMin = 0;
    float valueMax = 100;
    std::string valueText;
};

/**
 * @brief An action that can be performed on an accessible element.
 */
struct AccessibleAction {
    std::string name;
    std::string description;
    std::function<void()> perform;
};

/**
 * @brief A node in the accessibility tree.
 */
class AccessibleNode {
public:
    AccessibleNode();
    explicit AccessibleNode(Widget* widget);
    ~AccessibleNode();

    // Identity
    Widget* widget() const { return widget_; }
    uint32_t id() const { return id_; }

    // Role
    AccessibleRole role() const { return role_; }
    void setRole(AccessibleRole role) { role_ = role; }
    std::string roleString() const;

    // Name and description
    const std::string& name() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    const std::string& description() const { return description_; }
    void setDescription(const std::string& desc) { description_ = desc; }

    // Value
    const std::string& value() const { return value_; }
    void setValue(const std::string& val) { value_ = val; }

    // State
    const AccessibleState& state() const { return state_; }
    AccessibleState& state() { return state_; }

    // Bounds (screen coordinates)
    Rect bounds() const;

    // Tree
    AccessibleNode* parent() const { return parent_; }
    const std::vector<std::unique_ptr<AccessibleNode>>& children() const { return children_; }
    void addChild(std::unique_ptr<AccessibleNode> child);
    void removeChild(AccessibleNode* child);
    size_t childCount() const { return children_.size(); }

    // Actions
    void addAction(const std::string& name, const std::string& desc, std::function<void()> fn);
    const std::vector<AccessibleAction>& actions() const { return actions_; }
    bool performAction(const std::string& name);
    bool performDefaultAction();

    // Relations
    void setLabelledBy(AccessibleNode* node) { labelledBy_ = node; }
    AccessibleNode* labelledBy() const { return labelledBy_; }

    void setDescribedBy(AccessibleNode* node) { describedBy_ = node; }
    AccessibleNode* describedBy() const { return describedBy_; }

    // Live region
    enum class LivePriority : uint8_t { Off, Polite, Assertive };
    LivePriority livePriority() const { return livePriority_; }
    void setLivePriority(LivePriority p) { livePriority_ = p; }

    // Level (for headings, tree items)
    int level() const { return level_; }
    void setLevel(int l) { level_ = l; }

    // Position in set
    int positionInSet() const { return posInSet_; }
    void setPositionInSet(int pos) { posInSet_ = pos; }
    int setSize() const { return setSize_; }
    void setSetSize(int size) { setSize_ = size; }

private:
    Widget* widget_ = nullptr;
    uint32_t id_ = 0;

    AccessibleRole role_ = AccessibleRole::None;
    std::string name_;
    std::string description_;
    std::string value_;
    AccessibleState state_;

    AccessibleNode* parent_ = nullptr;
    std::vector<std::unique_ptr<AccessibleNode>> children_;
    std::vector<AccessibleAction> actions_;

    AccessibleNode* labelledBy_ = nullptr;
    AccessibleNode* describedBy_ = nullptr;
    LivePriority livePriority_ = LivePriority::Off;
    int level_ = 0;
    int posInSet_ = 0;
    int setSize_ = 0;

    static uint32_t s_nextId;
};

} // namespace NXRender
