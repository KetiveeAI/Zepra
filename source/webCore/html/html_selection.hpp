/**
 * @file html_selection.hpp
 * @brief Selection and Range APIs
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief DOM Range
 */
class Range {
public:
    Range();
    ~Range() = default;
    
    // Boundary points
    HTMLElement* startContainer() const { return startContainer_; }
    int startOffset() const { return startOffset_; }
    HTMLElement* endContainer() const { return endContainer_; }
    int endOffset() const { return endOffset_; }
    
    bool collapsed() const { 
        return startContainer_ == endContainer_ && startOffset_ == endOffset_; 
    }
    
    HTMLElement* commonAncestorContainer() const;
    
    // Mutating boundary points
    void setStart(HTMLElement* node, int offset);
    void setEnd(HTMLElement* node, int offset);
    void setStartBefore(HTMLElement* node);
    void setStartAfter(HTMLElement* node);
    void setEndBefore(HTMLElement* node);
    void setEndAfter(HTMLElement* node);
    void collapse(bool toStart = false);
    void selectNode(HTMLElement* node);
    void selectNodeContents(HTMLElement* node);
    
    // Comparing
    enum CompareHowType { 
        START_TO_START = 0, 
        START_TO_END = 1, 
        END_TO_END = 2, 
        END_TO_START = 3 
    };
    int compareBoundaryPoints(CompareHowType how, const Range& sourceRange) const;
    bool isPointInRange(HTMLElement* node, int offset) const;
    int comparePoint(HTMLElement* node, int offset) const;
    bool intersectsNode(HTMLElement* node) const;
    
    // Extracting content
    std::unique_ptr<HTMLElement> cloneContents() const;
    void deleteContents();
    std::unique_ptr<HTMLElement> extractContents();
    void insertNode(HTMLElement* node);
    void surroundContents(HTMLElement* newParent);
    
    // Misc
    std::unique_ptr<Range> cloneRange() const;
    void detach() { detached_ = true; }
    std::string toString() const;
    
    // Geometry
    struct Rect {
        float x, y, width, height;
    };
    Rect getBoundingClientRect() const;
    std::vector<Rect> getClientRects() const;
    
private:
    HTMLElement* startContainer_ = nullptr;
    int startOffset_ = 0;
    HTMLElement* endContainer_ = nullptr;
    int endOffset_ = 0;
    bool detached_ = false;
};

/**
 * @brief Text selection
 */
class Selection {
public:
    Selection() = default;
    ~Selection() = default;
    
    // Properties
    HTMLElement* anchorNode() const { return anchorNode_; }
    int anchorOffset() const { return anchorOffset_; }
    HTMLElement* focusNode() const { return focusNode_; }
    int focusOffset() const { return focusOffset_; }
    bool isCollapsed() const;
    int rangeCount() const { return ranges_.empty() ? 0 : 1; }
    std::string type() const;
    
    // Range access
    Range* getRangeAt(int index);
    void addRange(std::unique_ptr<Range> range);
    void removeRange(Range* range);
    void removeAllRanges();
    void empty() { removeAllRanges(); }
    
    // Collapsing
    void collapse(HTMLElement* node, int offset = 0);
    void collapseToStart();
    void collapseToEnd();
    
    // Extending
    void extend(HTMLElement* node, int offset = 0);
    void setBaseAndExtent(HTMLElement* anchorNode, int anchorOffset,
                          HTMLElement* focusNode, int focusOffset);
    
    // Selection by word/line
    void selectAllChildren(HTMLElement* node);
    
    // Modification
    void modify(const std::string& alter, const std::string& direction,
                const std::string& granularity);
    
    // Interrogating
    bool containsNode(HTMLElement* node, bool allowPartial = false) const;
    
    std::string toString() const;
    
private:
    HTMLElement* anchorNode_ = nullptr;
    int anchorOffset_ = 0;
    HTMLElement* focusNode_ = nullptr;
    int focusOffset_ = 0;
    std::vector<std::unique_ptr<Range>> ranges_;
};

} // namespace Zepra::WebCore
