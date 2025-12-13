/**
 * @file elements_panel.cpp
 * @brief DOM Elements panel implementation
 */

#include "devtools/elements_panel.hpp"
#include <sstream>

namespace Zepra::DevTools {

ElementsPanel::ElementsPanel() : DevToolsPanel("Elements") {}

ElementsPanel::~ElementsPanel() = default;

void ElementsPanel::render() {
    // Render DOM tree view
    if (rootNode_) {
        renderTreeNode(rootNode_.get(), 0);
    }
}

void ElementsPanel::refresh() {
    // Refresh DOM tree from document
    if (onRefresh_) {
        onRefresh_();
    }
}

void ElementsPanel::setDOMRoot(std::shared_ptr<DOMNode> root) {
    rootNode_ = std::move(root);
    if (onDOMChange_) {
        onDOMChange_();
    }
}

void ElementsPanel::selectNode(const DOMNode* node) {
    selectedNode_ = const_cast<DOMNode*>(node);
    if (onNodeSelected_ && node) {
        onNodeSelected_(node);
    }
}

void ElementsPanel::highlightNode(const DOMNode* node, bool highlight) {
    if (onHighlight_ && node) {
        onHighlight_(node, highlight);
    }
}

void ElementsPanel::expandNode(DOMNode* node) {
    if (node) {
        node->expanded = true;
    }
}

void ElementsPanel::collapseNode(DOMNode* node) {
    if (node) {
        node->expanded = false;
    }
}

DOMNode* ElementsPanel::findNodeById(const std::string& nodeId) {
    return findNodeRecursive(rootNode_.get(), nodeId);
}

DOMNode* ElementsPanel::findNodeRecursive(DOMNode* node, const std::string& nodeId) {
    if (!node) return nullptr;
    if (node->id == nodeId) return node;
    
    for (auto& child : node->children) {
        if (auto* found = findNodeRecursive(child.get(), nodeId)) {
            return found;
        }
    }
    return nullptr;
}

void ElementsPanel::renderTreeNode(DOMNode* node, int depth) {
    if (!node) return;
    
    // Indentation
    std::string indent(depth * 2, ' ');
    
    // Build display string
    std::ostringstream oss;
    oss << indent;
    
    if (node->type == DOMNodeType::Element) {
        oss << "<" << node->tagName;
        for (const auto& [key, val] : node->attributes) {
            oss << " " << key << "=\"" << val << "\"";
        }
        oss << ">";
    } else if (node->type == DOMNodeType::Text) {
        oss << "\"" << node->textContent.substr(0, 50);
        if (node->textContent.length() > 50) oss << "...";
        oss << "\"";
    } else if (node->type == DOMNodeType::Comment) {
        oss << "<!-- " << node->textContent.substr(0, 30) << " -->";
    }
    
    // Store display string
    node->displayString = oss.str();
    
    // Render children if expanded
    if (node->expanded && node->type == DOMNodeType::Element) {
        for (auto& child : node->children) {
            renderTreeNode(child.get(), depth + 1);
        }
    }
}

std::string ElementsPanel::getComputedStyles(const DOMNode* node) {
    if (!node || node->computedStyles.empty()) {
        return "{}";
    }
    
    std::ostringstream oss;
    oss << "{\n";
    for (const auto& [prop, val] : node->computedStyles) {
        oss << "  " << prop << ": " << val << ";\n";
    }
    oss << "}";
    return oss.str();
}

void ElementsPanel::setNodeAttribute(DOMNode* node, const std::string& attr, 
                                      const std::string& value) {
    if (node) {
        node->attributes[attr] = value;
        if (onDOMChange_) onDOMChange_();
    }
}

void ElementsPanel::removeNodeAttribute(DOMNode* node, const std::string& attr) {
    if (node) {
        node->attributes.erase(attr);
        if (onDOMChange_) onDOMChange_();
    }
}

} // namespace Zepra::DevTools
