#pragma once

/**
 * @file layout_engine.hpp
 * @brief CSS Box Model layout engine
 */

#include "render_tree.hpp"
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Layout constraints for sizing
 */
struct LayoutConstraints {
    float minWidth = 0;
    float maxWidth = 1e9f;
    float minHeight = 0;
    float maxHeight = 1e9f;
    
    static LayoutConstraints tight(float width, float height);
    static LayoutConstraints loose(float maxWidth, float maxHeight);
};

/**
 * @brief Layout engine for computing render tree positions
 */
class LayoutEngine {
public:
    /**
     * @brief Perform layout on render tree
     */
    void layout(RenderNode* root, float viewportWidth, float viewportHeight);
    
    /**
     * @brief Layout a single node
     */
    void layoutNode(RenderNode* node, const LayoutConstraints& constraints);
    
private:
    // Block layout
    void layoutBlock(RenderNode* node, const LayoutConstraints& constraints);
    
    // Inline layout
    void layoutInline(RenderNode* node, const LayoutConstraints& constraints);
    
    // Flex layout
    void layoutFlex(RenderNode* node, const LayoutConstraints& constraints);
    
    // Grid layout
    void layoutGrid(RenderNode* node, const LayoutConstraints& constraints);
    
    // Positioned elements (absolute/fixed)
    void layoutPositioned(RenderNode* node, const LayoutConstraints& constraints);
    
    // Text layout
    void layoutText(RenderText* text, float maxWidth);
    
    // Calculate intrinsic sizes
    float computeIntrinsicWidth(RenderNode* node);
    float computeIntrinsicHeight(RenderNode* node, float width);
    
    // Resolve CSS values
    float resolveLength(float value, float percentBase, bool isAuto);
    
    float viewportWidth_ = 0;
    float viewportHeight_ = 0;
};

/**
 * @brief Font metrics for text layout
 */
struct FontMetrics {
    float ascent = 0;
    float descent = 0;
    float lineHeight = 0;
    float xHeight = 0;
    float capHeight = 0;
    
    float height() const { return ascent + descent; }
};

/**
 * @brief Text measurement interface
 */
class TextMeasurer {
public:
    virtual ~TextMeasurer() = default;
    
    virtual float measureWidth(const std::string& text, 
                                const std::string& fontFamily,
                                float fontSize, bool bold) = 0;
    
    virtual FontMetrics getFontMetrics(const std::string& fontFamily,
                                        float fontSize, bool bold) = 0;
    
    // Break text into lines fitting maxWidth
    virtual std::vector<std::string> breakIntoLines(const std::string& text,
                                                     const std::string& fontFamily,
                                                     float fontSize, bool bold,
                                                     float maxWidth) = 0;
};

/**
 * @brief Simple text measurer using fixed-width approximation
 */
class SimpleTextMeasurer : public TextMeasurer {
public:
    float measureWidth(const std::string& text, 
                        const std::string& fontFamily,
                        float fontSize, bool bold) override;
    
    FontMetrics getFontMetrics(const std::string& fontFamily,
                                float fontSize, bool bold) override;
    
    std::vector<std::string> breakIntoLines(const std::string& text,
                                             const std::string& fontFamily,
                                             float fontSize, bool bold,
                                             float maxWidth) override;
};

} // namespace Zepra::WebCore
