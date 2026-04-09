// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "accessibility/accessible.h"
#include "widgets/widget.h"
#include <algorithm>

namespace NXRender {

uint32_t AccessibleNode::s_nextId = 1;

AccessibleNode::AccessibleNode() : id_(s_nextId++) {}

AccessibleNode::AccessibleNode(Widget* widget) : widget_(widget), id_(s_nextId++) {}

AccessibleNode::~AccessibleNode() {}

std::string AccessibleNode::roleString() const {
    switch (role_) {
        case AccessibleRole::None:          return "none";
        case AccessibleRole::Button:        return "button";
        case AccessibleRole::Checkbox:      return "checkbox";
        case AccessibleRole::RadioButton:   return "radio";
        case AccessibleRole::Slider:        return "slider";
        case AccessibleRole::TextBox:       return "textbox";
        case AccessibleRole::Label:         return "label";
        case AccessibleRole::Link:          return "link";
        case AccessibleRole::List:          return "list";
        case AccessibleRole::ListItem:      return "listitem";
        case AccessibleRole::Menu:          return "menu";
        case AccessibleRole::MenuItem:      return "menuitem";
        case AccessibleRole::MenuBar:       return "menubar";
        case AccessibleRole::Tab:           return "tab";
        case AccessibleRole::TabPanel:      return "tabpanel";
        case AccessibleRole::TabList:       return "tablist";
        case AccessibleRole::Tree:          return "tree";
        case AccessibleRole::TreeItem:      return "treeitem";
        case AccessibleRole::Grid:          return "grid";
        case AccessibleRole::GridCell:      return "gridcell";
        case AccessibleRole::Row:           return "row";
        case AccessibleRole::ColumnHeader:  return "columnheader";
        case AccessibleRole::RowHeader:     return "rowheader";
        case AccessibleRole::Dialog:        return "dialog";
        case AccessibleRole::Alert:         return "alert";
        case AccessibleRole::AlertDialog:   return "alertdialog";
        case AccessibleRole::Tooltip:       return "tooltip";
        case AccessibleRole::ProgressBar:   return "progressbar";
        case AccessibleRole::ScrollBar:     return "scrollbar";
        case AccessibleRole::Separator:     return "separator";
        case AccessibleRole::Toolbar:       return "toolbar";
        case AccessibleRole::StatusBar:     return "status";
        case AccessibleRole::Group:         return "group";
        case AccessibleRole::Region:        return "region";
        case AccessibleRole::Heading:       return "heading";
        case AccessibleRole::Image:         return "img";
        case AccessibleRole::Document:      return "document";
        case AccessibleRole::Application:   return "application";
        case AccessibleRole::Window:        return "window";
    }
    return "none";
}

Rect AccessibleNode::bounds() const {
    if (widget_) return widget_->bounds();
    return Rect();
}

void AccessibleNode::addChild(std::unique_ptr<AccessibleNode> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
}

void AccessibleNode::removeChild(AccessibleNode* child) {
    children_.erase(
        std::remove_if(children_.begin(), children_.end(),
            [child](const std::unique_ptr<AccessibleNode>& c) { return c.get() == child; }),
        children_.end()
    );
}

void AccessibleNode::addAction(const std::string& name, const std::string& desc,
                                std::function<void()> fn) {
    actions_.push_back({name, desc, std::move(fn)});
}

bool AccessibleNode::performAction(const std::string& name) {
    for (auto& action : actions_) {
        if (action.name == name && action.perform) {
            action.perform();
            return true;
        }
    }
    return false;
}

bool AccessibleNode::performDefaultAction() {
    if (!actions_.empty() && actions_[0].perform) {
        actions_[0].perform();
        return true;
    }
    return false;
}

} // namespace NXRender
