// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "a11y_bridge.h"
#include <algorithm>
#include <chrono>
#include <sstream>

namespace NXRender {

// ==================================================================
// ATSPIBridge
// ==================================================================

struct ATSPIBridge::DBusState {
    bool registered = false;
    std::unordered_map<uint32_t, std::string> objectPaths;
};

ATSPIBridge::ATSPIBridge() : dbus_(std::make_unique<DBusState>()) {}
ATSPIBridge::~ATSPIBridge() { disconnect(); }

bool ATSPIBridge::connect() {
    // Would open D-Bus session connection and register with AT-SPI2 registry
    // For now: mark connected for structural correctness
    connected_ = true;
    return true;
}

void ATSPIBridge::disconnect() {
    if (!connected_) return;
    dbus_->objectPaths.clear();
    dbus_->registered = false;
    connected_ = false;
}

void ATSPIBridge::setRoot(AccessibleNode* root) {
    root_ = root;
    if (connected_ && root) registerObject(root);
}

void ATSPIBridge::notifyFocusChanged(AccessibleNode* node) {
    if (!connected_ || !node) return;
    // Emit org.a]11y.atspi.Event.Focus:focused
    (void)pathForNode(node);
}

void ATSPIBridge::notifyValueChanged(AccessibleNode* node) {
    if (!connected_ || !node) return;
    // Emit org.a11y.atspi.Event.Object:property-change:accessible-value
}

void ATSPIBridge::notifyStateChanged(AccessibleNode* node, const std::string& stateName,
                                      bool value) {
    if (!connected_ || !node) return;
    (void)stateName; (void)value;
    // Emit org.a11y.atspi.Event.Object:state-changed:<stateName>
}

void ATSPIBridge::notifyChildrenChanged(AccessibleNode* parent) {
    if (!connected_ || !parent) return;
    // Emit org.a11y.atspi.Event.Object:children-changed
}

void ATSPIBridge::notifyTextChanged(AccessibleNode* node, int offset,
                                      int addedCount, int removedCount) {
    if (!connected_ || !node) return;
    (void)offset; (void)addedCount; (void)removedCount;
    // Emit org.a11y.atspi.Event.Object:text-changed
}

void ATSPIBridge::announce(const std::string& text, AccessibleNode::LivePriority priority) {
    if (!connected_) return;
    (void)text; (void)priority;
    // Emit Announcement event via AT-SPI
}

void ATSPIBridge::registerObject(AccessibleNode* node) {
    if (!node) return;
    dbus_->objectPaths[node->id()] = pathForNode(node);
    for (size_t i = 0; i < node->childCount(); i++) {
        registerObject(node->children()[i].get());
    }
}

void ATSPIBridge::unregisterObject(AccessibleNode* node) {
    if (!node) return;
    dbus_->objectPaths.erase(node->id());
}

std::string ATSPIBridge::pathForNode(AccessibleNode* node) const {
    return "/org/nxrender/accessible/" + std::to_string(node->id());
}

// ==================================================================
// ARIAResolver
// ==================================================================

AccessibleRole ARIAResolver::roleFromString(const std::string& role) {
    static const std::unordered_map<std::string, AccessibleRole> map = {
        {"none", AccessibleRole::None},
        {"presentation", AccessibleRole::None},
        {"button", AccessibleRole::Button},
        {"checkbox", AccessibleRole::Checkbox},
        {"radio", AccessibleRole::RadioButton},
        {"slider", AccessibleRole::Slider},
        {"textbox", AccessibleRole::TextBox},
        {"label", AccessibleRole::Label},
        {"link", AccessibleRole::Link},
        {"list", AccessibleRole::List},
        {"listitem", AccessibleRole::ListItem},
        {"menu", AccessibleRole::Menu},
        {"menuitem", AccessibleRole::MenuItem},
        {"menubar", AccessibleRole::MenuBar},
        {"tab", AccessibleRole::Tab},
        {"tabpanel", AccessibleRole::TabPanel},
        {"tablist", AccessibleRole::TabList},
        {"tree", AccessibleRole::Tree},
        {"treeitem", AccessibleRole::TreeItem},
        {"grid", AccessibleRole::Grid},
        {"gridcell", AccessibleRole::GridCell},
        {"row", AccessibleRole::Row},
        {"columnheader", AccessibleRole::ColumnHeader},
        {"rowheader", AccessibleRole::RowHeader},
        {"dialog", AccessibleRole::Dialog},
        {"alert", AccessibleRole::Alert},
        {"alertdialog", AccessibleRole::AlertDialog},
        {"tooltip", AccessibleRole::Tooltip},
        {"progressbar", AccessibleRole::ProgressBar},
        {"scrollbar", AccessibleRole::ScrollBar},
        {"separator", AccessibleRole::Separator},
        {"toolbar", AccessibleRole::Toolbar},
        {"status", AccessibleRole::StatusBar},
        {"group", AccessibleRole::Group},
        {"region", AccessibleRole::Region},
        {"heading", AccessibleRole::Heading},
        {"img", AccessibleRole::Image},
        {"document", AccessibleRole::Document},
        {"application", AccessibleRole::Application},
        {"window", AccessibleRole::Window},
    };
    auto it = map.find(role);
    return (it != map.end()) ? it->second : AccessibleRole::None;
}

std::string ARIAResolver::roleToString(AccessibleRole role) {
    switch (role) {
        case AccessibleRole::None: return "none";
        case AccessibleRole::Button: return "button";
        case AccessibleRole::Checkbox: return "checkbox";
        case AccessibleRole::RadioButton: return "radio";
        case AccessibleRole::Slider: return "slider";
        case AccessibleRole::TextBox: return "textbox";
        case AccessibleRole::Label: return "label";
        case AccessibleRole::Link: return "link";
        case AccessibleRole::List: return "list";
        case AccessibleRole::ListItem: return "listitem";
        case AccessibleRole::Menu: return "menu";
        case AccessibleRole::MenuItem: return "menuitem";
        case AccessibleRole::MenuBar: return "menubar";
        case AccessibleRole::Tab: return "tab";
        case AccessibleRole::TabPanel: return "tabpanel";
        case AccessibleRole::TabList: return "tablist";
        case AccessibleRole::Tree: return "tree";
        case AccessibleRole::TreeItem: return "treeitem";
        case AccessibleRole::Grid: return "grid";
        case AccessibleRole::GridCell: return "gridcell";
        case AccessibleRole::Row: return "row";
        case AccessibleRole::ColumnHeader: return "columnheader";
        case AccessibleRole::RowHeader: return "rowheader";
        case AccessibleRole::Dialog: return "dialog";
        case AccessibleRole::Alert: return "alert";
        case AccessibleRole::AlertDialog: return "alertdialog";
        case AccessibleRole::Tooltip: return "tooltip";
        case AccessibleRole::ProgressBar: return "progressbar";
        case AccessibleRole::ScrollBar: return "scrollbar";
        case AccessibleRole::Separator: return "separator";
        case AccessibleRole::Toolbar: return "toolbar";
        case AccessibleRole::StatusBar: return "status";
        case AccessibleRole::Group: return "group";
        case AccessibleRole::Region: return "region";
        case AccessibleRole::Heading: return "heading";
        case AccessibleRole::Image: return "img";
        case AccessibleRole::Document: return "document";
        case AccessibleRole::Application: return "application";
        case AccessibleRole::Window: return "window";
    }
    return "none";
}

void ARIAResolver::applyAttributes(AccessibleNode* node,
                                     const std::vector<ARIAAttribute>& attrs) {
    if (!node) return;

    for (const auto& attr : attrs) {
        if (attr.name == "role") {
            node->setRole(roleFromString(attr.value));
        } else if (attr.name == "aria-label") {
            node->setName(attr.value);
        } else if (attr.name == "aria-description" || attr.name == "aria-describedby") {
            node->setDescription(attr.value);
        } else if (attr.name == "aria-valuenow") {
            node->state().hasValue = true;
            node->state().valueNow = std::strtof(attr.value.c_str(), nullptr);
        } else if (attr.name == "aria-valuemin") {
            node->state().valueMin = std::strtof(attr.value.c_str(), nullptr);
        } else if (attr.name == "aria-valuemax") {
            node->state().valueMax = std::strtof(attr.value.c_str(), nullptr);
        } else if (attr.name == "aria-valuetext") {
            node->state().valueText = attr.value;
        } else if (attr.name == "aria-checked") {
            node->state().checked = (attr.value == "true");
        } else if (attr.name == "aria-selected") {
            node->state().selected = (attr.value == "true");
        } else if (attr.name == "aria-expanded") {
            node->state().expanded = (attr.value == "true");
        } else if (attr.name == "aria-disabled") {
            node->state().disabled = (attr.value == "true");
        } else if (attr.name == "aria-readonly") {
            node->state().readonly_ = (attr.value == "true");
        } else if (attr.name == "aria-required") {
            node->state().required = (attr.value == "true");
        } else if (attr.name == "aria-busy") {
            node->state().busy = (attr.value == "true");
        } else if (attr.name == "aria-hidden") {
            node->state().hidden = (attr.value == "true");
        } else if (attr.name == "aria-pressed") {
            node->state().pressed = (attr.value == "true");
        } else if (attr.name == "aria-invalid") {
            node->state().invalid = (attr.value == "true");
        } else if (attr.name == "aria-live") {
            if (attr.value == "assertive") node->setLivePriority(AccessibleNode::LivePriority::Assertive);
            else if (attr.value == "polite") node->setLivePriority(AccessibleNode::LivePriority::Polite);
            else node->setLivePriority(AccessibleNode::LivePriority::Off);
        } else if (attr.name == "aria-level") {
            node->setLevel(std::atoi(attr.value.c_str()));
        } else if (attr.name == "aria-posinset") {
            node->setPositionInSet(std::atoi(attr.value.c_str()));
        } else if (attr.name == "aria-setsize") {
            node->setSetSize(std::atoi(attr.value.c_str()));
        }
    }
}

AccessibleRole ARIAResolver::implicitRole(const std::string& tag, const std::string& type) {
    if (tag == "button" || tag == "summary") return AccessibleRole::Button;
    if (tag == "a" && !type.empty()) return AccessibleRole::Link;
    if (tag == "a") return AccessibleRole::Link;
    if (tag == "input") {
        if (type == "checkbox") return AccessibleRole::Checkbox;
        if (type == "radio") return AccessibleRole::RadioButton;
        if (type == "range") return AccessibleRole::Slider;
        if (type == "button" || type == "submit" || type == "reset") return AccessibleRole::Button;
        if (type == "image") return AccessibleRole::Image;
        return AccessibleRole::TextBox;
    }
    if (tag == "textarea") return AccessibleRole::TextBox;
    if (tag == "select") return AccessibleRole::List;
    if (tag == "option") return AccessibleRole::ListItem;
    if (tag == "img") return AccessibleRole::Image;
    if (tag == "ul" || tag == "ol") return AccessibleRole::List;
    if (tag == "li") return AccessibleRole::ListItem;
    if (tag == "nav") return AccessibleRole::Region;
    if (tag == "main") return AccessibleRole::Region;
    if (tag == "aside") return AccessibleRole::Region;
    if (tag == "header") return AccessibleRole::Region;
    if (tag == "footer") return AccessibleRole::Region;
    if (tag == "section") return AccessibleRole::Region;
    if (tag == "article") return AccessibleRole::Document;
    if (tag == "dialog") return AccessibleRole::Dialog;
    if (tag == "table") return AccessibleRole::Grid;
    if (tag == "tr") return AccessibleRole::Row;
    if (tag == "th") return AccessibleRole::ColumnHeader;
    if (tag == "td") return AccessibleRole::GridCell;
    if (tag == "progress") return AccessibleRole::ProgressBar;
    if (tag == "meter") return AccessibleRole::ProgressBar;
    if (tag == "h1" || tag == "h2" || tag == "h3" ||
        tag == "h4" || tag == "h5" || tag == "h6") return AccessibleRole::Heading;
    if (tag == "form") return AccessibleRole::Group;
    if (tag == "fieldset") return AccessibleRole::Group;
    if (tag == "details") return AccessibleRole::Group;
    if (tag == "menu") return AccessibleRole::Menu;
    if (tag == "output") return AccessibleRole::StatusBar;
    return AccessibleRole::None;
}

ARIAResolver::NameResult ARIAResolver::computeName(
    const std::string& ariaLabel,
    const std::string& ariaLabelledBy,
    const std::string& title,
    const std::string& alt,
    const std::string& placeholder,
    const std::string& textContent) {

    NameResult result;
    // Priority: aria-labelledby > aria-label > alt > title > placeholder > contents
    if (!ariaLabelledBy.empty()) {
        result.name = ariaLabelledBy;
        result.source = NameResult::Source::AriaLabelledBy;
    } else if (!ariaLabel.empty()) {
        result.name = ariaLabel;
        result.source = NameResult::Source::AriaLabel;
    } else if (!alt.empty()) {
        result.name = alt;
        result.source = NameResult::Source::Alt;
    } else if (!title.empty()) {
        result.name = title;
        result.source = NameResult::Source::Title;
    } else if (!placeholder.empty()) {
        result.name = placeholder;
        result.source = NameResult::Source::Placeholder;
    } else if (!textContent.empty()) {
        result.name = textContent;
        result.source = NameResult::Source::Contents;
    } else {
        result.source = NameResult::Source::None;
    }
    return result;
}

std::string ARIAResolver::computeDescription(const std::string& ariaDescribedBy,
                                               const std::string& title) {
    if (!ariaDescribedBy.empty()) return ariaDescribedBy;
    return title;
}

ARIAResolver::ValidationResult ARIAResolver::validate(
    AccessibleRole role,
    const AccessibleState& state,
    const std::vector<ARIAAttribute>& attrs) {

    ValidationResult result;
    (void)state;

    auto required = requiredStates(role);
    for (const auto& req : required) {
        bool found = false;
        for (const auto& attr : attrs) {
            if (attr.name == req) { found = true; break; }
        }
        if (!found) {
            result.warnings.push_back("Missing required attribute: " + req);
        }
    }

    if (!result.warnings.empty()) result.valid = false;
    return result;
}

std::vector<std::string> ARIAResolver::requiredStates(AccessibleRole role) {
    switch (role) {
        case AccessibleRole::Checkbox: return {"aria-checked"};
        case AccessibleRole::RadioButton: return {"aria-checked"};
        case AccessibleRole::Slider: return {"aria-valuenow", "aria-valuemin", "aria-valuemax"};
        case AccessibleRole::ScrollBar: return {"aria-valuenow", "aria-valuemin", "aria-valuemax"};
        default: return {};
    }
}

std::vector<std::string> ARIAResolver::supportedStates(AccessibleRole role) {
    std::vector<std::string> common = {
        "aria-disabled", "aria-hidden", "aria-label", "aria-describedby"
    };
    switch (role) {
        case AccessibleRole::Button:
            common.push_back("aria-pressed");
            common.push_back("aria-expanded");
            break;
        case AccessibleRole::Checkbox:
        case AccessibleRole::RadioButton:
            common.push_back("aria-checked");
            common.push_back("aria-required");
            break;
        case AccessibleRole::TextBox:
            common.push_back("aria-readonly");
            common.push_back("aria-required");
            common.push_back("aria-invalid");
            common.push_back("aria-multiline");
            common.push_back("aria-placeholder");
            break;
        case AccessibleRole::Tree:
        case AccessibleRole::TreeItem:
            common.push_back("aria-expanded");
            common.push_back("aria-selected");
            break;
        case AccessibleRole::Tab:
            common.push_back("aria-selected");
            break;
        case AccessibleRole::Menu:
        case AccessibleRole::MenuItem:
            common.push_back("aria-expanded");
            break;
        case AccessibleRole::Dialog:
        case AccessibleRole::AlertDialog:
            common.push_back("aria-modal");
            break;
        default: break;
    }
    return common;
}

// ==================================================================
// KeyboardNavigationMap
// ==================================================================

void KeyboardNavigationMap::rebuild(AccessibleNode* root) {
    items_.clear();
    if (root) collect(root);
    sortByTabIndex();
}

void KeyboardNavigationMap::collect(AccessibleNode* node) {
    if (!node || node->state().hidden) return;

    bool focusable = false;
    switch (node->role()) {
        case AccessibleRole::Button:
        case AccessibleRole::Checkbox:
        case AccessibleRole::RadioButton:
        case AccessibleRole::Slider:
        case AccessibleRole::TextBox:
        case AccessibleRole::Link:
        case AccessibleRole::Tab:
        case AccessibleRole::MenuItem:
        case AccessibleRole::TreeItem:
        case AccessibleRole::GridCell:
            focusable = !node->state().disabled;
            break;
        default:
            break;
    }

    if (focusable) {
        items_.push_back({node, 0, true, true});
    }

    for (size_t i = 0; i < node->childCount(); i++) {
        collect(node->children()[i].get());
    }
}

void KeyboardNavigationMap::sortByTabIndex() {
    std::stable_sort(items_.begin(), items_.end(),
                      [](const NavItem& a, const NavItem& b) {
                          if (a.tabIndex == 0 && b.tabIndex == 0) return false;
                          if (a.tabIndex == 0) return false;
                          if (b.tabIndex == 0) return true;
                          return a.tabIndex < b.tabIndex;
                      });
}

AccessibleNode* KeyboardNavigationMap::next(AccessibleNode* current) const {
    if (items_.empty()) return nullptr;
    if (!current) return items_.front().node;

    for (size_t i = 0; i < items_.size(); i++) {
        if (items_[i].node == current) {
            size_t next = (i + 1) % items_.size();
            if (inFocusTrap() && !isInCurrentTrap(items_[next].node)) {
                // Wrap within trap
                for (size_t j = 0; j < items_.size(); j++) {
                    if (isInCurrentTrap(items_[j].node)) return items_[j].node;
                }
            }
            return items_[next].node;
        }
    }
    return items_.front().node;
}

AccessibleNode* KeyboardNavigationMap::previous(AccessibleNode* current) const {
    if (items_.empty()) return nullptr;
    if (!current) return items_.back().node;

    for (size_t i = 0; i < items_.size(); i++) {
        if (items_[i].node == current) {
            size_t prev = (i == 0) ? items_.size() - 1 : i - 1;
            if (inFocusTrap() && !isInCurrentTrap(items_[prev].node)) {
                for (int j = static_cast<int>(items_.size()) - 1; j >= 0; j--) {
                    if (isInCurrentTrap(items_[j].node)) return items_[j].node;
                }
            }
            return items_[prev].node;
        }
    }
    return items_.back().node;
}

AccessibleNode* KeyboardNavigationMap::first() const {
    return items_.empty() ? nullptr : items_.front().node;
}

AccessibleNode* KeyboardNavigationMap::last() const {
    return items_.empty() ? nullptr : items_.back().node;
}

AccessibleNode* KeyboardNavigationMap::nextInGroup(AccessibleNode* current) const {
    if (!current || !current->parent()) return nullptr;
    auto* parent = current->parent();
    const auto& siblings = parent->children();

    for (size_t i = 0; i < siblings.size(); i++) {
        if (siblings[i].get() == current) {
            for (size_t j = i + 1; j < siblings.size(); j++) {
                if (!siblings[j]->state().disabled && !siblings[j]->state().hidden) {
                    return siblings[j].get();
                }
            }
            // Wrap around
            for (size_t j = 0; j < i; j++) {
                if (!siblings[j]->state().disabled && !siblings[j]->state().hidden) {
                    return siblings[j].get();
                }
            }
            return current;
        }
    }
    return nullptr;
}

AccessibleNode* KeyboardNavigationMap::previousInGroup(AccessibleNode* current) const {
    if (!current || !current->parent()) return nullptr;
    auto* parent = current->parent();
    const auto& siblings = parent->children();

    for (size_t i = 0; i < siblings.size(); i++) {
        if (siblings[i].get() == current) {
            for (int j = static_cast<int>(i) - 1; j >= 0; j--) {
                if (!siblings[j]->state().disabled && !siblings[j]->state().hidden) {
                    return siblings[j].get();
                }
            }
            for (int j = static_cast<int>(siblings.size()) - 1; j > static_cast<int>(i); j--) {
                if (!siblings[j]->state().disabled && !siblings[j]->state().hidden) {
                    return siblings[j].get();
                }
            }
            return current;
        }
    }
    return nullptr;
}

void KeyboardNavigationMap::pushFocusTrap(AccessibleNode* container) {
    focusTraps_.push_back(container);
}

void KeyboardNavigationMap::popFocusTrap() {
    if (!focusTraps_.empty()) focusTraps_.pop_back();
}

bool KeyboardNavigationMap::isInCurrentTrap(AccessibleNode* node) const {
    if (focusTraps_.empty()) return true;
    auto* trap = focusTraps_.back();
    auto* n = node;
    while (n) {
        if (n == trap) return true;
        n = n->parent();
    }
    return false;
}

// ==================================================================
// AccessibilityPreferences
// ==================================================================

AccessibilityPreferences AccessibilityPreferences::query() {
    AccessibilityPreferences prefs;
    // Would query gsettings / XDG / portal for real preferences
    // Default: everything off, font scale 1.0
    return prefs;
}

void AccessibilityPreferences::onChange(ChangeCallback /*cb*/) {
    // Would register D-Bus signal listener for settings changes
}

// ==================================================================
// ScreenReaderOutput
// ==================================================================

void ScreenReaderOutput::speak(const std::string& text, Utterance::Priority priority,
                                bool interrupt) {
    if (interrupt) {
        queue_.clear();
    }
    auto now = std::chrono::high_resolution_clock::now();
    double ts = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    queue_.push_back({text, priority, interrupt, ts});
}

void ScreenReaderOutput::clear() {
    queue_.clear();
}

void ScreenReaderOutput::flush() {
    queue_.clear();
}

ScreenReaderOutput::Utterance ScreenReaderOutput::dequeue() {
    if (queue_.empty()) return {"", Utterance::Priority::Medium, false, 0};
    auto u = queue_.front();
    queue_.erase(queue_.begin());
    return u;
}

void ScreenReaderOutput::announcePosition(AccessibleNode* node) {
    if (!node) return;
    std::string text = describeNode(node);
    speak(text, Utterance::Priority::Medium);
}

void ScreenReaderOutput::announceRole(AccessibleNode* node) {
    if (!node) return;
    speak(ARIAResolver::roleToString(node->role()), Utterance::Priority::Low);
}

void ScreenReaderOutput::announceState(AccessibleNode* node) {
    if (!node) return;
    std::string state;
    if (node->state().disabled) state += "disabled ";
    if (node->state().checked) state += "checked ";
    if (node->state().selected) state += "selected ";
    if (node->state().expanded) state += "expanded ";
    if (node->state().pressed) state += "pressed ";
    if (node->state().readonly_) state += "read only ";
    if (node->state().required) state += "required ";
    if (!state.empty()) speak(state, Utterance::Priority::Low);
}

void ScreenReaderOutput::announceValue(AccessibleNode* node) {
    if (!node || !node->state().hasValue) return;
    if (!node->state().valueText.empty()) {
        speak(node->state().valueText, Utterance::Priority::Medium);
    } else {
        speak(std::to_string(static_cast<int>(node->state().valueNow)),
              Utterance::Priority::Medium);
    }
}

std::string ScreenReaderOutput::describeNode(AccessibleNode* node) const {
    if (!node) return "";
    std::ostringstream ss;

    if (!node->name().empty()) ss << node->name() << ", ";
    ss << ARIAResolver::roleToString(node->role());

    if (node->state().disabled) ss << ", disabled";
    if (node->state().checked) ss << ", checked";
    if (node->state().expanded) ss << ", expanded";
    if (node->state().selected) ss << ", selected";
    if (node->state().hasValue && !node->state().valueText.empty()) {
        ss << ", " << node->state().valueText;
    }

    if (node->level() > 0) ss << ", level " << node->level();
    if (node->positionInSet() > 0 && node->setSize() > 0) {
        ss << ", " << node->positionInSet() << " of " << node->setSize();
    }

    if (!node->description().empty()) ss << ". " << node->description();

    return ss.str();
}

} // namespace NXRender
