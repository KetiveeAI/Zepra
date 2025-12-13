/**
 * @file page.cpp
 * @brief Page implementation - combines DOM, CSS, render tree
 */

#include "webcore/page.hpp"
#include <algorithm>
#include <cctype>

namespace Zepra::WebCore {

// =============================================================================
// Page Implementation
// =============================================================================

Page::Page() {
    scriptContext_ = std::make_unique<ScriptContext>();
    resourceLoader_ = std::make_unique<ResourceLoader>();
    scriptLoader_ = std::make_unique<ScriptLoader>(scriptContext_.get());
    scriptLoader_->setResourceLoader(resourceLoader_.get());
    // Add default/user-agent stylesheet
    styleResolver_.addUserAgentStylesheet();
}

Page::~Page() = default;

void Page::loadHTML(const std::string& html, const std::string& baseURL) {
    baseURL_ = baseURL;
    updateProgress(PageState::Loading, 0.1f, "Loading...");
    
    // Parse HTML
    updateProgress(PageState::Parsing, 0.2f, "Parsing HTML...");
    HTMLParser parser;
    document_ = parser.parse(html);
    scriptContext_->initialize(document_.get());
    
    // Extract title
    std::function<std::string(DOMNode*)> findTitle = [&](DOMNode* node) -> std::string {
        if (node->nodeType() == NodeType::Element) {
            auto* el = static_cast<DOMElement*>(node);
            if (el->tagName() == "title") {
                // Get text content
                for (auto& child : el->childNodes()) {
                    if (child->nodeType() == NodeType::Text) {
                        return static_cast<DOMText*>(child.get())->data();
                    }
                }
            }
        }
        for (auto& child : node->childNodes()) {
            std::string t = findTitle(child.get());
            if (!t.empty()) return t;
        }
        return "";
    };
    title_ = findTitle(document_.get());
    if (title_.empty()) title_ = "Untitled";
    
    // Extract and parse inline styles
    std::function<void(DOMNode*)> extractStyles = [&](DOMNode* node) {
        if (node->nodeType() == NodeType::Element) {
            auto* el = static_cast<DOMElement*>(node);
            if (el->tagName() == "style") {
                // Get style content
                for (auto& child : el->childNodes()) {
                    if (child->nodeType() == NodeType::Text) {
                        addStylesheet(static_cast<DOMText*>(child.get())->data());
                    }
                }
            }
            // Check for link[rel=stylesheet]
            if (el->tagName() == "link" && el->getAttribute("rel") == "stylesheet") {
                // Would load external stylesheet here
            }
        }
        for (auto& child : node->childNodes()) {
            extractStyles(child.get());
        }
    };
    extractStyles(document_.get());
    
    // Execute scripts using ScriptLoader for proper async/defer handling
    std::function<void(DOMNode*)> processScripts = [&](DOMNode* node) {
        if (node->nodeType() == NodeType::Element) {
            auto* el = static_cast<DOMElement*>(node);
            if (el->tagName() == "script") {
                bool parserBlocked = false;
                scriptLoader_->processScript(el, parserBlocked);
                
                // If parser is blocked by external script, wait for it
                if (parserBlocked) {
                    // In a real async implementation, we'd yield here
                    // For now, execute blocking scripts synchronously
                    scriptLoader_->executeBlockingScripts();
                }
            }
        }
        for (auto& child : node->childNodes()) {
            processScripts(child.get());
        }
    };
    if (scriptContext_) {
        processScripts(document_.get());
        
        // Execute any remaining blocking scripts
        scriptLoader_->executeBlockingScripts();
        
        // Fire DOMContentLoaded and execute deferred scripts
        scriptContext_->fireDOMContentLoaded();
        scriptLoader_->onDOMContentLoaded();
    }
    
    // Build render tree
    updateProgress(PageState::Styling, 0.5f, "Applying styles...");
    buildRenderTree();
    
    // Layout
    updateProgress(PageState::Layout, 0.7f, "Layout...");
    layout();
    
    // Done
    updateProgress(PageState::Complete, 1.0f, "Complete");
    state_ = PageState::Complete;
    
    // Fire load event after all resources loaded
    if (scriptContext_) {
        scriptContext_->fireLoadEvent();
        scriptLoader_->onLoad();
        // Process any ready async scripts
        scriptLoader_->processAsyncQueue();
    }
    
    if (onLoad_) onLoad_();
}

void Page::loadFromURL(const std::string& url) {
    url_ = url;
    baseURL_ = url;
    
    updateProgress(PageState::Loading, 0.1f, "Connecting to " + url + "...");
    
    if (resourceLoader_) {
        // Sync load
        Response response = resourceLoader_->loadURL(url);
        
        if (response.ok()) {
            loadHTML(response.text(), url);
        } else {
            // Error page
            std::string errorHtml = "<html><head><title>Error</title></head><body>"
                                    "<h1>Load Error</h1>"
                                    "<p>Failed to load: " + url + "</p>"
                                    "<p>Status: " + std::to_string(response.status()) + "</p>"
                                    "</body></html>";
            loadHTML(errorHtml, "about:error");
            state_ = PageState::Error;
            updateProgress(PageState::Error, 1.0f, "Failed to load");
        }
    } else {
        state_ = PageState::Error;
        updateProgress(PageState::Error, 1.0f, "Internal Error: No ResourceLoader");
    }
}

void Page::addStylesheet(const std::string& css) {
    auto stylesheet = cssParser_.parse(css);
    auto sharedStylesheet = std::shared_ptr<Stylesheet>(std::move(stylesheet));
    stylesheets_.push_back(sharedStylesheet);
    
    // Add to resolver
    styleResolver_.addStylesheet(sharedStylesheet);
}

void Page::addInlineStyle(DOMElement* element, const std::string& style) {
    (void)element;
    (void)style;
    // Would parse inline style and apply to element
}

void Page::setViewport(float width, float height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
}

void Page::layout() {
    if (!renderTree_) return;
    layoutEngine_.layout(renderTree_.get(), viewportWidth_, viewportHeight_);
}

void Page::paint(DisplayList& displayList) {
    if (!renderTree_) return;
    
    // Apply scroll offset
    PaintContext ctx(displayList);
    
    // Offset by scroll
    // For now, we'll modify the render tree position temporarily
    if (scrollX_ != 0 || scrollY_ != 0) {
        auto& box = const_cast<BoxModel&>(renderTree_->boxModel());
        box.contentBox.x -= scrollX_;
        box.contentBox.y -= scrollY_;
    }
    
    renderTree_->paint(ctx);
    
    // Restore
    if (scrollX_ != 0 || scrollY_ != 0) {
        auto& box = const_cast<BoxModel&>(renderTree_->boxModel());
        box.contentBox.x += scrollX_;
        box.contentBox.y += scrollY_;
    }
}

void Page::scrollTo(float x, float y) {
    scrollX_ = std::max(0.0f, std::min(x, contentWidth() - viewportWidth_));
    scrollY_ = std::max(0.0f, std::min(y, contentHeight() - viewportHeight_));
}

void Page::scrollBy(float dx, float dy) {
    scrollTo(scrollX_ + dx, scrollY_ + dy);
}

float Page::contentWidth() const {
    if (!renderTree_) return viewportWidth_;
    return renderTree_->boxModel().marginBox().width;
}

float Page::contentHeight() const {
    if (!renderTree_) return viewportHeight_;
    return renderTree_->boxModel().marginBox().height;
}

DOMElement* Page::elementFromPoint(float x, float y) {
    (void)x; (void)y;
    // Would need element tracking
    return nullptr;
}

RenderNode* Page::hitTest(float x, float y) {
    if (!renderTree_) return nullptr;
    
    // Adjust for scroll
    x += scrollX_;
    y += scrollY_;
    
    std::function<RenderNode*(RenderNode*)> test = [&](RenderNode* node) -> RenderNode* {
        Rect box = node->boxModel().borderBox();
        if (x >= box.x && x < box.x + box.width &&
            y >= box.y && y < box.y + box.height) {
            // Check children first (front to back)
            for (auto& child : node->children()) {
                if (auto* hit = test(child.get())) {
                    return hit;
                }
            }
            return node;
        }
        return nullptr;
    };
    
    return test(renderTree_.get());
}

void Page::buildRenderTree() {
    if (!document_) return;
    
    // Find body element
    std::function<DOMElement*(DOMNode*)> findBody = [&](DOMNode* node) -> DOMElement* {
        if (node->nodeType() == NodeType::Element) {
            auto* el = static_cast<DOMElement*>(node);
            if (el->tagName() == "body") return el;
        }
        for (auto& child : node->childNodes()) {
            if (auto* found = findBody(child.get())) return found;
        }
        return nullptr;
    };
    
    DOMElement* body = findBody(document_.get());
    if (!body) return;
    
    // Create root render node
    auto root = std::make_unique<RenderBlock>();
    root->setDomNode(body); // LINKED!
    root->style().display = Display::Block;
    root->style().backgroundColor = Color::white();
    
    // Build children
    for (auto& child : body->childNodes()) {
        if (child->nodeType() == NodeType::Element) {
            auto renderNode = buildRenderNode(static_cast<DOMElement*>(child.get()), &root->style());
            if (renderNode) {
                root->appendChild(std::move(renderNode));
            }
        } else if (child->nodeType() == NodeType::Text) {
            auto* textNode = static_cast<DOMText*>(child.get());
            if (!textNode->data().empty()) {
                auto text = std::make_unique<RenderText>(textNode->data());
                text->style().color = Color::black();
                text->setDomNode(textNode); // LINKED!
                root->appendChild(std::move(text));
            }
        }
    }
    
    renderTree_ = std::move(root);
}

std::unique_ptr<RenderNode> Page::buildRenderNode(DOMElement* element, const ComputedStyle* parentStyle) {
    const std::string& tag = element->tagName();
    
    // Skip non-visual elements
    if (tag == "script" || tag == "style" || tag == "meta" || 
        tag == "link" || tag == "head" || tag == "title") {
        return nullptr;
    }
    
    // Compute style including inheritance
    ComputedStyle style = styleResolver_.computeStyle(element, parentStyle);
    
    // Display none check
    if (style.display == Display::None) return nullptr;
    
    std::unique_ptr<RenderNode> node;
    
    // Create appropriate render node type
    if (tag == "br") {
        auto text = std::make_unique<RenderText>("\n");
        text->setDomNode(element); // LINKED!
        text->style() = style;
        return text;
    }
    
    if (tag == "img") {
        auto img = std::make_unique<RenderImage>(element->getAttribute("src"));
        img->setDomNode(element);
        img->style() = style;
        // Get dimensions from attributes
        std::string widthAttr = element->getAttribute("width");
        std::string heightAttr = element->getAttribute("height");
        if (!widthAttr.empty()) {
            img->style().width = std::stof(widthAttr);
            img->style().autoWidth = false;
        }
        if (!heightAttr.empty()) {
            img->style().height = std::stof(heightAttr);
            img->style().autoHeight = false;
        }
        node = std::move(img);
    } else if (tag == "video") {
        auto video = std::make_unique<RenderVideo>();
        video->setDomNode(element);
        video->style() = style;
        video->setSrc(element->getAttribute("src"));
        video->setPoster(element->getAttribute("poster"));
        video->setControls(element->hasAttribute("controls"));
        video->setAutoplay(element->hasAttribute("autoplay"));
        video->setLoop(element->hasAttribute("loop"));
        video->setMuted(element->hasAttribute("muted"));
        // Get dimensions
        std::string widthAttr = element->getAttribute("width");
        std::string heightAttr = element->getAttribute("height");
        if (!widthAttr.empty()) {
            video->style().width = std::stof(widthAttr);
            video->style().autoWidth = false;
        }
        if (!heightAttr.empty()) {
            video->style().height = std::stof(heightAttr);
            video->style().autoHeight = false;
        }
        // Default size if not specified
        if (video->style().autoWidth) video->style().width = 640;
        if (video->style().autoHeight) video->style().height = 360;
        video->style().autoWidth = false;
        video->style().autoHeight = false;
        node = std::move(video);
    } else if (tag == "audio") {
        auto audio = std::make_unique<RenderAudio>();
        audio->setDomNode(element);
        audio->style() = style;
        audio->setSrc(element->getAttribute("src"));
        // Audio has controls by default when attribute present
        // If no controls, audio is invisible
        node = std::move(audio);
    } else {
        // Block vs Inline based on computed display
        if (style.display == Display::Block || style.display == Display::Flex || style.display == Display::Grid || style.display == Display::InlineBlock) {
             node = std::make_unique<RenderBlock>();
        } else {
             node = std::make_unique<RenderInline>(); // Fallback
        }
        node->setDomNode(element); // LINKED!
        node->style() = style;
    }
    
    // Build children
    bool isBlock = (style.display == Display::Block);
    
    for (auto& child : element->childNodes()) {
        if (child->nodeType() == NodeType::Element) {
            auto childNode = buildRenderNode(static_cast<DOMElement*>(child.get()), &node->style());
            if (childNode) {
                node->appendChild(std::move(childNode));
            }
        } else if (child->nodeType() == NodeType::Text) {
            auto* textNode = static_cast<DOMText*>(child.get());
            std::string data = textNode->data();
            
            // Trim whitespace for block elements
            if (isBlock) {
                // Normalize whitespace
                std::string normalized;
                bool lastSpace = true;
                for (char c : data) {
                    if (std::isspace(c)) {
                        if (!lastSpace) { normalized += ' '; lastSpace = true; }
                    } else {
                        normalized += c;
                        lastSpace = false;
                    }
                }
                data = normalized;
            }
            
            if (!data.empty()) {
                auto text = std::make_unique<RenderText>(data);
                text->setDomNode(child.get()); // LINKED!
                // Inherit style from parent node
                text->style() = node->style();
                node->appendChild(std::move(text));
            }
        }
    }
    
    return node;
}

void Page::updateProgress(PageState state, float progress, const std::string& status) {
    state_ = state;
    progress_.state = state;
    progress_.progress = progress;
    progress_.statusText = status;
    
    if (onProgress_) onProgress_(progress_);
}

// =============================================================================
// Frame Implementation
// =============================================================================

Frame::Frame(Page* page) : page_(page) {}

void Frame::navigate(const std::string& url) {
    url_ = url;
    page_->loadFromURL(url);
}

void Frame::reload() {
    navigate(url_);
}

void Frame::stop() {
    // Would stop loading
}

void Frame::layout() {
    page_->setViewport(bounds_.width, bounds_.height);
    page_->layout();
}

void Frame::paint(DisplayList& displayList) {
    page_->paint(displayList);
}

// =============================================================================
// Viewport Implementation
// =============================================================================

Viewport::Viewport(float width, float height) : width_(width), height_(height) {}

void Viewport::resize(float width, float height) {
    width_ = width;
    height_ = height;
}

void Viewport::scrollTo(float x, float y) {
    scrollX_ = std::max(0.0f, std::min(x, maxScrollX_));
    scrollY_ = std::max(0.0f, std::min(y, maxScrollY_));
}

void Viewport::scrollBy(float dx, float dy) {
    scrollTo(scrollX_ + dx, scrollY_ + dy);
}

void Viewport::setScrollBounds(float maxX, float maxY) {
    maxScrollX_ = std::max(0.0f, maxX);
    maxScrollY_ = std::max(0.0f, maxY);
}

void Viewport::setZoom(float zoom) {
    zoom_ = std::max(0.25f, std::min(zoom, 4.0f));
}

Rect Viewport::visibleRect() const {
    return {scrollX_, scrollY_, width_ / zoom_, height_ / zoom_};
}

void Viewport::viewportToContent(float& x, float& y) const {
    x = x / zoom_ + scrollX_;
    y = y / zoom_ + scrollY_;
}

void Viewport::contentToViewport(float& x, float& y) const {
    x = (x - scrollX_) * zoom_;
    y = (y - scrollY_) * zoom_;
}

} // namespace Zepra::WebCore
