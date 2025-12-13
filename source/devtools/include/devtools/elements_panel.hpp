/**
 * @file elements_panel.hpp
 * @brief DOM Inspector Panel - View and edit HTML/CSS
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Zepra::DevTools {

/**
 * @brief DOM node representation
 */
struct DOMNode {
    int nodeId;
    std::string tagName;
    std::string nodeName;
    int nodeType;
    std::string textContent;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::vector<DOMNode> children;
    bool expanded = false;
    bool selected = false;
};

/**
 * @brief CSS style property
 */
struct CSSProperty {
    std::string name;
    std::string value;
    std::string origin;      // "inline", "stylesheet", "computed"
    std::string selector;
    std::string sourceFile;
    int sourceLine;
    bool inherited;
    bool overridden;
    bool editable;
};

/**
 * @brief Box model dimensions
 */
struct BoxModel {
    struct Rect {
        int top, right, bottom, left;
    };
    
    int width, height;
    Rect margin;
    Rect border;
    Rect padding;
    Rect content;
};

/**
 * @brief Node selection callback
 */
using NodeSelectCallback = std::function<void(int nodeId)>;
using StyleChangeCallback = std::function<void(int nodeId, const std::string& property, const std::string& value)>;

/**
 * @brief Elements Panel - DOM/CSS Inspector
 */
class ElementsPanel {
public:
    ElementsPanel();
    ~ElementsPanel();
    
    // --- DOM Tree ---
    
    /**
     * @brief Set the root DOM node
     */
    void setDocument(const DOMNode& root);
    
    /**
     * @brief Get current DOM tree
     */
    const DOMNode& document() const { return document_; }
    
    /**
     * @brief Select a node by ID
     */
    void selectNode(int nodeId);
    
    /**
     * @brief Get selected node
     */
    int selectedNode() const { return selectedNodeId_; }
    
    /**
     * @brief Expand/collapse node
     */
    void toggleNode(int nodeId);
    
    /**
     * @brief Highlight node in browser
     */
    void highlightNode(int nodeId);
    
    /**
     * @brief Stop highlighting
     */
    void clearHighlight();
    
    /**
     * @brief Search DOM tree
     */
    std::vector<int> search(const std::string& query);
    
    // --- CSS Styles ---
    
    /**
     * @brief Get styles for selected node
     */
    std::vector<CSSProperty> getStyles(int nodeId);
    
    /**
     * @brief Get computed styles
     */
    std::vector<CSSProperty> getComputedStyles(int nodeId);
    
    /**
     * @brief Set a style property
     */
    void setStyle(int nodeId, const std::string& property, const std::string& value);
    
    // --- Box Model ---
    
    /**
     * @brief Get box model for node
     */
    BoxModel getBoxModel(int nodeId);
    
    // --- Editing ---
    
    /**
     * @brief Edit node as HTML
     */
    void editAsHTML(int nodeId, const std::string& html);
    
    /**
     * @brief Edit attribute
     */
    void editAttribute(int nodeId, const std::string& name, const std::string& value);
    
    /**
     * @brief Delete node
     */
    void deleteNode(int nodeId);
    
    // --- Callbacks ---
    void onNodeSelect(NodeSelectCallback callback);
    void onStyleChange(StyleChangeCallback callback);
    
    // --- UI ---
    void update();
    void render();
    
private:
    DOMNode* findNode(DOMNode& root, int nodeId);
    void renderNode(const DOMNode& node, int depth);
    void renderStyles();
    void renderBoxModel();
    
    DOMNode document_;
    int selectedNodeId_ = -1;
    int highlightedNodeId_ = -1;
    std::string searchQuery_;
    
    std::vector<NodeSelectCallback> selectCallbacks_;
    std::vector<StyleChangeCallback> styleCallbacks_;
};

} // namespace Zepra::DevTools
