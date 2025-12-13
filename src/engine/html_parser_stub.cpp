/**
 * @file html_parser_stub.cpp
 * @brief Minimal stub implementations for HTMLParser and related classes
 * @note Real implementation requires full HTML tokenizer and DOM builder.
 *       This stub allows the browser to build without those dependencies.
 */

#include "engine/html_parser.h"
#include <fstream>
#include <sstream>

namespace zepra {

// =============================================================================
// DOMNode Stubs
// =============================================================================

void DOMNode::appendChild(DOMNodePtr child) {
    if (child) {
        // Don't set parentNode to avoid shared_from_this() issues
        childNodes.push_back(child);
    }
}

void DOMNode::removeChild(DOMNodePtr child) {
    if (!child) return;
    auto it = std::find(childNodes.begin(), childNodes.end(), child);
    if (it != childNodes.end()) {
        (*it)->parentNode.reset();
        childNodes.erase(it);
    }
}

DOMNodePtr DOMNode::getElementById(const String& id) {
    (void)id;
    return nullptr;
}

DOMNodeList DOMNode::getElementsByTagName(const String& tagName) {
    (void)tagName;
    return DOMNodeList();
}

DOMNodeList DOMNode::getElementsByClassName(const String& className) {
    (void)className;
    return DOMNodeList();
}

String DOMNode::getTextContent() const {
    return nodeValue;
}

void DOMNode::setTextContent(const String& text) {
    nodeValue = text;
}

// =============================================================================
// ElementNode Stubs
// =============================================================================

void ElementNode::setAttribute(const String& name, const String& value) {
    attributes[name] = value;
}

String ElementNode::getAttribute(const String& name) const {
    auto it = attributes.find(name);
    return (it != attributes.end()) ? it->second : "";
}

void ElementNode::removeAttribute(const String& name) {
    attributes.erase(name);
}

bool ElementNode::hasAttribute(const String& name) const {
    return attributes.find(name) != attributes.end();
}

void ElementNode::setStyle(const String& property, const String& value) {
    // Stub - would need to convert property string to CSSPropertyType
    (void)property;
    (void)value;
}

String ElementNode::getStyle(const String& property) const {
    // Stub - would need to convert property string to CSSPropertyType
    (void)property;
    return "";
}

DOMNodePtr ElementNode::getElementById(const String& id) {
    if (getAttribute("id") == id) {
        // Return nullptr for self - can't use shared_from_this on stack/raw objects
        return nullptr;
    }
    for (const auto& child : childNodes) {
        if (auto result = child->getElementById(id)) {
            return result;
        }
    }
    return nullptr;
}

DOMNodeList ElementNode::getElementsByTagName(const String& tagName) {
    DOMNodeList result;
    // Skip self - can't use shared_from_this on stack/raw objects
    for (const auto& child : childNodes) {
        auto childResults = child->getElementsByTagName(tagName);
        result.insert(result.end(), childResults.begin(), childResults.end());
    }
    return result;
}

DOMNodeList ElementNode::getElementsByClassName(const String& className) {
    DOMNodeList result;
    // Skip self - can't use shared_from_this on stack/raw objects
    (void)className;
    for (const auto& child : childNodes) {
        auto childResults = child->getElementsByClassName(className);
        result.insert(result.end(), childResults.begin(), childResults.end());
    }
    return result;
}

// =============================================================================
// DocumentNode Stubs
// =============================================================================

DOMNodePtr DocumentNode::getElementById(const String& id) {
    if (documentElement) {
        return documentElement->getElementById(id);
    }
    return nullptr;
}

DOMNodeList DocumentNode::getElementsByTagName(const String& tagName) {
    if (documentElement) {
        return documentElement->getElementsByTagName(tagName);
    }
    return DOMNodeList();
}

DOMNodeList DocumentNode::getElementsByClassName(const String& className) {
    if (documentElement) {
        return documentElement->getElementsByClassName(className);
    }
    return DOMNodeList();
}

// =============================================================================
// HTMLTokenizer Stubs
// =============================================================================

HTMLTokenizer::HTMLTokenizer(const String& input) 
    : input(input), position(0), length(input.length()) {}

HTMLToken HTMLTokenizer::nextToken() {
    // Stub - just return EOF
    return HTMLToken(TokenType::EOF_TOKEN);
}

bool HTMLTokenizer::hasMoreTokens() const {
    return position < length;
}

void HTMLTokenizer::reset() {
    position = 0;
}

// =============================================================================
// DOMBuilder Stubs
// =============================================================================

DOMBuilder::DOMBuilder() : document(nullptr) {}

DocumentNode* DOMBuilder::build(const String& html) {
    // Stub - create minimal document structure
    document = new DocumentNode();
    document->documentElement = createElement("html");
    document->head = createElement("head");
    document->body = createElement("body");
    
    // Create a text node with the HTML content
    auto textNode = std::make_shared<TextNode>(html);
    document->body->appendChild(textNode);
    
    return document;
}

void DOMBuilder::reset() {
    openElements.clear();
    document = nullptr;
}

void DOMBuilder::processToken(const HTMLToken& token) { (void)token; }
void DOMBuilder::processStartTag(const HTMLToken& token) { (void)token; }
void DOMBuilder::processEndTag(const HTMLToken& token) { (void)token; }
void DOMBuilder::processCharacter(const HTMLToken& token) { (void)token; }
void DOMBuilder::processDoctype(const HTMLToken& token) { (void)token; }
void DOMBuilder::processComment(const HTMLToken& token) { (void)token; }

void DOMBuilder::pushElement(ElementNode* element) {
    if (element) openElements.push_back(element);
}

void DOMBuilder::popElement() {
    if (!openElements.empty()) openElements.pop_back();
}

ElementNode* DOMBuilder::currentElement() const {
    return openElements.empty() ? nullptr : static_cast<ElementNode*>(openElements.back());
}

bool DOMBuilder::hasElementInScope(const String& tagName) const {
    for (auto it = openElements.rbegin(); it != openElements.rend(); ++it) {
        if ((*it)->nodeName == tagName) return true;
    }
    return false;
}

ElementNode* DOMBuilder::createElement(const String& tagName) {
    return new ElementNode(parseElementType(tagName), tagName);
}

TextNode* DOMBuilder::createTextNode(const String& text) {
    return new TextNode(text);
}

ElementType DOMBuilder::parseElementType(const String& tagName) const {
    // Simple mapping for common elements
    if (tagName == "html") return ElementType::HTML;
    if (tagName == "head") return ElementType::HEAD;
    if (tagName == "body") return ElementType::BODY;
    if (tagName == "div") return ElementType::DIV;
    if (tagName == "span") return ElementType::SPAN;
    if (tagName == "p") return ElementType::P;
    if (tagName == "a") return ElementType::A;
    if (tagName == "img") return ElementType::IMG;
    return ElementType::UNKNOWN;
}

// =============================================================================
// HTMLParser Stubs
// =============================================================================

HTMLParser::HTMLParser() 
    : builder(std::make_unique<DOMBuilder>()), maxNodes(100000), nodeCount(0) {}

DocumentNode* HTMLParser::parse(const String& html) {
    errors.clear();
    nodeCount = 0;
    return builder->build(html);
}

DocumentNode* HTMLParser::parseFile(const String& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        reportError("Could not open file: " + filename);
        return nullptr;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str());
}

void HTMLParser::reportError(const String& message) {
    errors.push_back(message);
}

bool HTMLParser::checkNodeLimit() {
    return nodeCount < maxNodes;
}

// =============================================================================
// html_utils Namespace Stubs
// =============================================================================

namespace html_utils {

String escapeHtml(const String& text) {
    String result;
    for (char c : text) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }
    return result;
}

String unescapeHtml(const String& text) {
    String result = text;
    // Simple replacements
    size_t pos;
    while ((pos = result.find("&lt;")) != String::npos) result.replace(pos, 4, "<");
    while ((pos = result.find("&gt;")) != String::npos) result.replace(pos, 4, ">");
    while ((pos = result.find("&amp;")) != String::npos) result.replace(pos, 5, "&");
    while ((pos = result.find("&quot;")) != String::npos) result.replace(pos, 6, "\"");
    while ((pos = result.find("&#39;")) != String::npos) result.replace(pos, 5, "'");
    return result;
}

bool isValidTagName(const String& tagName) {
    if (tagName.empty()) return false;
    for (char c : tagName) {
        if (!std::isalnum(c) && c != '-' && c != '_') return false;
    }
    return true;
}

bool isVoidElement(const String& tagName) {
    static const std::vector<String> voidElements = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr"
    };
    return std::find(voidElements.begin(), voidElements.end(), tagName) != voidElements.end();
}

String normalizeWhitespace(const String& text) {
    String result;
    bool lastWasSpace = false;
    for (char c : text) {
        if (std::isspace(c)) {
            if (!lastWasSpace) {
                result += ' ';
                lastWasSpace = true;
            }
        } else {
            result += c;
            lastWasSpace = false;
        }
    }
    return result;
}

String extractTextContent(const DOMNode* node) {
    if (!node) return "";
    return node->getTextContent();
}

String serializeNode(const DOMNode* node) {
    if (!node) return "";
    
    if (node->nodeType == NodeType::TEXT) {
        return escapeHtml(node->nodeValue);
    }
    
    String result = "<" + node->nodeName + ">";
    for (const auto& child : node->childNodes) {
        result += serializeNode(child.get());
    }
    result += "</" + node->nodeName + ">";
    return result;
}

} // namespace html_utils

} // namespace zepra
