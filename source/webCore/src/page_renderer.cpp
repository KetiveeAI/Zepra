/**
 * @file page_renderer.cpp
 * @brief Page renderer implementation
 */

#include "webcore/page_renderer.hpp"
#include <algorithm>
#include <cctype>

namespace Zepra::WebCore {

// =============================================================================
// PageRenderer
// =============================================================================

PageRenderer::PageRenderer() {
    cssEngine_.addStyleSheet("", StyleOrigin::UserAgent);
    lastFrame_ = std::chrono::steady_clock::now();
}

PageRenderer::~PageRenderer() = default;

void PageRenderer::setDocument(DOMDocument* doc) {
    document_ = doc;
    if (document_) {
        cssEngine_.initialize(document_);
    }
}

void PageRenderer::setViewport(int width, int height) {
    if (viewportWidth_ != width || viewportHeight_ != height) {
        viewportWidth_ = width;
        viewportHeight_ = height;
        invalidateAll();
    }
}

void PageRenderer::setBackend(RenderBackend* backend) {
    backend_ = backend;
    compositor_.setBackend(backend);
    if (backend) {
        backend->resize(viewportWidth_, viewportHeight_);
    }
}

void PageRenderer::addStyleSheet(const std::string& cssText, StyleOrigin origin) {
    cssEngine_.addStyleSheet(cssText, origin);
    invalidateAll();
}

void PageRenderer::buildRenderTree() {
    if (!document_) return;
    
    renderRoot_ = RenderTreeBuilder::build(document_);
    
    if (renderRoot_ && document_->documentElement()) {
        renderRoot_->setDomNode(document_->documentElement());
    }
}

void PageRenderer::layout() {
    if (!renderRoot_) return;
    
    double start = now();
    
    layoutEngine_.layout(renderRoot_.get(), 
                         static_cast<float>(viewportWidth_),
                         static_cast<float>(viewportHeight_));
    
    metrics_.layoutTime = now() - start;
}

void PageRenderer::paint() {
    if (!renderRoot_) return;
    
    double start = now();
    
    if (config_.enableCompositing) {
        compositor_.buildLayerTree(renderRoot_.get());
        compositor_.paintDirtyLayers();
        metrics_.layerCount = compositor_.layerCount();
    } else {
        // Direct paint without layering
        DisplayList displayList;
        PaintContext ctx(displayList);
        
        std::function<void(RenderNode*)> paintNode = [&](RenderNode* node) {
            node->paint(ctx);
            metrics_.paintedNodes++;
            for (auto& child : node->children()) {
                paintNode(child.get());
            }
        };
        
        metrics_.paintedNodes = 0;
        paintNode(renderRoot_.get());
        metrics_.displayCommands = displayList.size();
        
        if (backend_) {
            backend_->executeDisplayList(displayList);
        }
    }
    
    metrics_.paintTime = now() - start;
}

void PageRenderer::composite() {
    if (!config_.enableCompositing) return;
    
    double start = now();
    compositor_.composite();
    metrics_.compositeTime = now() - start;
}

void PageRenderer::render() {
    if (!document_) return;
    
    double frameStart = now();
    
    // Step 1: Build render tree if needed
    if (!renderRoot_) {
        buildRenderTree();
    }
    
    // Step 2: Apply CSS styles
    double styleStart = now();
    applyStyles();
    metrics_.styleTime = now() - styleStart;
    
    // Step 3: Layout
    layout();
    
    // Step 4: Paint
    paint();
    
    // Step 5: Composite
    composite();
    
    // Final metrics
    metrics_.totalTime = now() - frameStart;
    
    // Calculate FPS
    auto currentTime = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(currentTime - lastFrame_).count();
    if (elapsed > 0) {
        metrics_.fps = 1000.0 / elapsed;
    }
    lastFrame_ = currentTime;
    
    if (frameCallback_) {
        frameCallback_(metrics_);
    }
}

void PageRenderer::renderIncremental() {
    if (!renderRoot_) {
        render();
        return;
    }
    
    double frameStart = now();
    
    // Only repaint dirty layers
    if (config_.enableCompositing && config_.enableLayerCaching) {
        compositor_.paintDirtyLayers();
        compositor_.composite();
    } else {
        paint();
    }
    
    metrics_.totalTime = now() - frameStart;
}

void PageRenderer::invalidate(DOMElement* element) {
    if (!element) return;
    
    std::function<RenderNode*(RenderNode*)> findNode = [&](RenderNode* node) -> RenderNode* {
        if (node->domNode() == element) {
            return node;
        }
        for (auto& child : node->children()) {
            if (auto* found = findNode(child.get())) {
                return found;
            }
        }
        return nullptr;
    };
    
    if (renderRoot_) {
        if (auto* node = findNode(renderRoot_.get())) {
            node->setNeedsLayout();
            node->setNeedsPaint();
        }
    }
    
    cssEngine_.invalidate(element);
}

void PageRenderer::invalidateAll() {
    if (renderRoot_) {
        renderRoot_->setNeedsLayout();
        renderRoot_->setNeedsPaint();
    }
    cssEngine_.invalidate(nullptr);
}

void PageRenderer::applyStyles() {
    if (!renderRoot_) return;
    
    // For now, apply default styles from DOM attributes
    // The CSS engine integration will be handled via StyleResolverAdapter
    // when rendering full pages with stylesheets
    
    std::function<void(RenderNode*)> apply = [&](RenderNode* node) {
        DOMNode* dom = node->domNode();
        if (dom && dom->nodeType() == NodeType::Element) {
            auto* element = static_cast<DOMElement*>(dom);
            auto& style = node->style();
            
            // Apply inline styles from element
            std::string inlineStyle = element->getAttribute("style");
            if (!inlineStyle.empty()) {
                // Parse simple inline styles
                // TODO: Use CSSParser for full parsing
                auto parseValue = [](const std::string& s) -> float {
                    try { return std::stof(s); } catch (...) { return 0; }
                };
                
                size_t pos = 0;
                while (pos < inlineStyle.size()) {
                    size_t colon = inlineStyle.find(':', pos);
                    if (colon == std::string::npos) break;
                    size_t semi = inlineStyle.find(';', colon);
                    if (semi == std::string::npos) semi = inlineStyle.size();
                    
                    std::string prop = inlineStyle.substr(pos, colon - pos);
                    std::string val = inlineStyle.substr(colon + 1, semi - colon - 1);
                    
                    // Trim whitespace
                    while (!prop.empty() && std::isspace(static_cast<unsigned char>(prop.front()))) prop.erase(0, 1);
                    while (!prop.empty() && std::isspace(static_cast<unsigned char>(prop.back()))) prop.pop_back();
                    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.front()))) val.erase(0, 1);
                    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back()))) val.pop_back();
                    
                    if (prop == "width") {
                        style.width = parseValue(val);
                        style.autoWidth = false;
                    } else if (prop == "height") {
                        style.height = parseValue(val);
                        style.autoHeight = false;
                    } else if (prop == "margin") {
                        float m = parseValue(val);
                        style.margin = {m, m, m, m};
                    } else if (prop == "padding") {
                        float p = parseValue(val);
                        style.padding = {p, p, p, p};
                    } else if (prop == "font-size") {
                        style.fontSize = parseValue(val);
                    } else if (prop == "display") {
                        if (val == "none") style.display = Display::None;
                        else if (val == "block") style.display = Display::Block;
                        else if (val == "inline") style.display = Display::Inline;
                        else if (val == "flex") style.display = Display::Flex;
                        else if (val == "grid") style.display = Display::Grid;
                    } else if (prop == "position") {
                        if (val == "relative") 
                            style.position = WebCore::ComputedStyle::Position::Relative;
                        else if (val == "absolute")
                            style.position = WebCore::ComputedStyle::Position::Absolute;
                        else if (val == "fixed")
                            style.position = WebCore::ComputedStyle::Position::Fixed;
                    }
                    
                    pos = semi + 1;
                }
            }
            
            // Apply tag defaults
            std::string tag = element->tagName();
            if (tag == "DIV" || tag == "P" || tag == "H1" || tag == "H2" || 
                tag == "H3" || tag == "SECTION" || tag == "ARTICLE") {
                style.display = Display::Block;
            } else if (tag == "SPAN" || tag == "A" || tag == "B" || tag == "I") {
                style.display = Display::Inline;
            }
            
            // Heading sizes
            if (tag == "H1") { style.fontSize = 32; style.fontBold = true; }
            else if (tag == "H2") { style.fontSize = 24; style.fontBold = true; }
            else if (tag == "H3") { style.fontSize = 20; style.fontBold = true; }
            else if (tag == "B" || tag == "STRONG") { style.fontBold = true; }
            else if (tag == "I" || tag == "EM") { style.fontItalic = true; }
        }
        
        for (auto& child : node->children()) {
            apply(child.get());
        }
    };
    
    apply(renderRoot_.get());
}

RenderNode* PageRenderer::hitTest(float x, float y) {
    if (!renderRoot_) return nullptr;
    return renderRoot_->hitTest(x + scrollX_, y + scrollY_);
}

DOMElement* PageRenderer::elementAtPoint(float x, float y) {
    if (auto* node = hitTest(x, y)) {
        DOMNode* dom = node->domNode();
        if (dom && dom->nodeType() == NodeType::Element) {
            return static_cast<DOMElement*>(dom);
        }
    }
    return nullptr;
}

void PageRenderer::scrollTo(float x, float y) {
    scrollX_ = std::max(0.0f, x);
    scrollY_ = std::max(0.0f, y);
    
    if (compositor_.rootLayer()) {
        compositor_.scrollLayer(compositor_.rootLayer(), 0, 0);
    }
}

void PageRenderer::scrollBy(float dx, float dy) {
    scrollTo(scrollX_ + dx, scrollY_ + dy);
}

double PageRenderer::now() const {
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// =============================================================================
// RenderTreeBuilder
// =============================================================================

std::unique_ptr<RenderNode> RenderTreeBuilder::build(DOMDocument* doc) {
    if (!doc || !doc->documentElement()) {
        return nullptr;
    }
    
    return buildElement(doc->documentElement());
}

std::unique_ptr<RenderNode> RenderTreeBuilder::buildElement(DOMElement* element) {
    if (!element) return nullptr;
    
    auto node = std::make_unique<RenderBlock>();
    node->setDomNode(element);
    
    // Process children
    for (const auto& child : element->childNodes()) {
        if (child->nodeType() == NodeType::Element) {
            auto* childElement = static_cast<DOMElement*>(child.get());
            if (auto childNode = buildElement(childElement)) {
                node->appendChild(std::move(childNode));
            }
        } else if (child->nodeType() == NodeType::Text) {
            auto* textNode = static_cast<DOMText*>(child.get());
            if (auto textRender = buildText(textNode)) {
                node->appendChild(std::move(textRender));
            }
        }
    }
    
    return node;
}

std::unique_ptr<RenderText> RenderTreeBuilder::buildText(DOMText* text) {
    if (!text) return nullptr;
    
    std::string data = text->data();
    
    // Skip whitespace-only text
    bool hasContent = false;
    for (char c : data) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            hasContent = true;
            break;
        }
    }
    
    if (!hasContent) return nullptr;
    
    auto node = std::make_unique<RenderText>(data);
    node->setDomNode(text);
    return node;
}

} // namespace Zepra::WebCore
