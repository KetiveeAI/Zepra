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
// HTMLTokenizer 
// =============================================================================

HTMLTokenizer::HTMLTokenizer(const String& input) 
    : input(input), position(0), length(input.length()) {}

HTMLToken HTMLTokenizer::nextToken() {
    if (position >= length) {
        return HTMLToken(TokenType::EOF_TOKEN);
    }
    
    // Skip whitespace-only at start
    while (position < length && std::isspace(input[position]) && input[position] != '<') {
        position++;
    }
    
    if (position >= length) {
        return HTMLToken(TokenType::EOF_TOKEN);
    }
    
    // Check for tag start
    if (input[position] == '<') {
        position++; // consume '<'
        
        if (position >= length) {
            return HTMLToken(TokenType::EOF_TOKEN);
        }
        
        // Comment: <!-- ... -->
        if (position + 2 < length && input[position] == '!' && 
            input[position+1] == '-' && input[position+2] == '-') {
            position += 3;
            size_t end = input.find("-->", position);
            if (end == String::npos) end = length;
            String comment = input.substr(position, end - position);
            position = end + 3;
            HTMLToken token(TokenType::COMMENT);
            token.data = comment;
            return token;
        }
        
        // DOCTYPE: <!DOCTYPE ...>
        if (position + 7 < length && input.substr(position, 8) == "!DOCTYPE") {
            size_t end = input.find('>', position);
            if (end == String::npos) end = length;
            position = end + 1;
            HTMLToken token(TokenType::DOCTYPE);
            return token;
        }
        
        // CDATA: <![CDATA[ ... ]]>
        if (position + 8 < length && input.substr(position, 8) == "![CDATA[") {
            position += 8;
            size_t end = input.find("]]>", position);
            if (end == String::npos) end = length;
            String text = input.substr(position, end - position);
            position = end + 3;
            HTMLToken token(TokenType::CHARACTER);
            token.data = text;
            return token;
        }
        
        // End tag: </tagname>
        if (input[position] == '/') {
            position++; // consume '/'
            String tagName;
            while (position < length && input[position] != '>' && !std::isspace(input[position])) {
                tagName += std::tolower(input[position]);
                position++;
            }
            // Skip to '>'
            while (position < length && input[position] != '>') position++;
            if (position < length) position++; // consume '>'
            
            HTMLToken token(TokenType::END_TAG);
            token.tagName = tagName;
            return token;
        }
        
        // Start tag: <tagname ...>
        String tagName;
        while (position < length && input[position] != '>' && 
               input[position] != '/' && !std::isspace(input[position])) {
            tagName += std::tolower(input[position]);
            position++;
        }
        
        HTMLToken token(TokenType::START_TAG);
        token.tagName = tagName;
        
        // Parse attributes
        while (position < length && input[position] != '>' && input[position] != '/') {
            // Skip whitespace
            while (position < length && std::isspace(input[position])) position++;
            if (position >= length || input[position] == '>' || input[position] == '/') break;
            
            // Attribute name
            String attrName;
            while (position < length && input[position] != '=' && 
                   input[position] != '>' && input[position] != '/' && !std::isspace(input[position])) {
                attrName += std::tolower(input[position]);
                position++;
            }
            
            // Skip whitespace
            while (position < length && std::isspace(input[position])) position++;
            
            String attrValue;
            if (position < length && input[position] == '=') {
                position++; // consume '='
                // Skip whitespace
                while (position < length && std::isspace(input[position])) position++;
                
                if (position < length) {
                    char quote = input[position];
                    if (quote == '"' || quote == '\'') {
                        position++; // consume opening quote
                        while (position < length && input[position] != quote) {
                            attrValue += input[position];
                            position++;
                        }
                        if (position < length) position++; // consume closing quote
                    } else {
                        // Unquoted value
                        while (position < length && !std::isspace(input[position]) && 
                               input[position] != '>' && input[position] != '/') {
                            attrValue += input[position];
                            position++;
                        }
                    }
                }
            }
            
            if (!attrName.empty()) {
                token.attributes[attrName] = attrValue;
            }
        }
        
        // Check for self-closing: />
        if (position < length && input[position] == '/') {
            token.selfClosing = true;
            position++;
        }
        
        // Consume '>'
        if (position < length && input[position] == '>') {
            position++;
        }
        
        return token;
    }
    
    // Text content
    String text;
    while (position < length && input[position] != '<') {
        text += input[position];
        position++;
    }
    
    // Trim and collapse whitespace
    if (!text.empty()) {
        HTMLToken token(TokenType::CHARACTER);
        token.data = text;
        return token;
    }
    
    return HTMLToken(TokenType::EOF_TOKEN);
}

bool HTMLTokenizer::hasMoreTokens() const {
    return position < length;
}

void HTMLTokenizer::reset() {
    position = 0;
}

// =============================================================================
// DOMBuilder - REAL IMPLEMENTATION
// =============================================================================

// Void elements that don't have closing tags
static bool isVoidElement(const String& tag) {
    static const char* voidTags[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr", nullptr
    };
    for (int i = 0; voidTags[i]; i++) {
        if (tag == voidTags[i]) return true;
    }
    return false;
}

DOMBuilder::DOMBuilder() : document(nullptr) {}

DocumentNode* DOMBuilder::build(const String& html) {
    document = new DocumentNode();
    document->documentElement = createElement("html");
    document->head = createElement("head");
    document->body = createElement("body");
    
    // Link document structure
    document->documentElement->appendChild(std::shared_ptr<DOMNode>(document->head));
    document->documentElement->appendChild(std::shared_ptr<DOMNode>(document->body));
    
    // Start with body as current element
    openElements.clear();
    openElements.push_back(document->body);
    
    // Now tokenize and build the real DOM tree
    HTMLTokenizer tokenizer(html);
    bool inHead = false;
    bool inBody = true;
    
    while (tokenizer.hasMoreTokens()) {
        HTMLToken token = tokenizer.nextToken();
        
        if (token.type == TokenType::EOF_TOKEN) break;
        
        switch (token.type) {
            case TokenType::DOCTYPE:
                // Skip DOCTYPE for now
                break;
                
            case TokenType::COMMENT:
                // Skip comments for now
                break;
                
            case TokenType::START_TAG: {
                // Special handling for html, head, body
                if (token.tagName == "html" || token.tagName == "!doctype") {
                    continue; // Skip, already created
                }
                if (token.tagName == "head") {
                    inHead = true;
                    inBody = false;
                    openElements.clear();
                    openElements.push_back(document->head);
                    continue;
                }
                if (token.tagName == "body") {
                    inHead = false;
                    inBody = true;
                    openElements.clear();
                    openElements.push_back(document->body);
                    continue;
                }
                
                // Create the element
                ElementNode* element = createElement(token.tagName);
                
                // Apply attributes
                for (const auto& attr : token.attributes) {
                    element->setAttribute(attr.first, attr.second);
                }
                
                // Append to current element
                if (!openElements.empty()) {
                    DOMNode* parent = openElements.back();
                    parent->appendChild(std::shared_ptr<DOMNode>(element));
                }
                
                // Push non-void elements onto stack
                if (!isVoidElement(token.tagName) && !token.selfClosing) {
                    openElements.push_back(element);
                }
                break;
            }
            
            case TokenType::END_TAG: {
                // Pop matching element from stack
                if (token.tagName == "head") {
                    inHead = false;
                    inBody = true;
                    openElements.clear();
                    openElements.push_back(document->body);
                    continue;
                }
                if (token.tagName == "body" || token.tagName == "html") {
                    continue; // Don't pop these
                }
                
                // Find and pop matching element
                for (auto it = openElements.rbegin(); it != openElements.rend(); ++it) {
                    if ((*it)->nodeName == token.tagName) {
                        openElements.erase(std::next(it).base());
                        break;
                    }
                }
                break;
            }
            
            case TokenType::CHARACTER: {
                if (!token.data.empty()) {
                    // Trim excessive whitespace
                    String text = token.data;
                    // Create text node
                    auto textNode = std::make_shared<TextNode>(text);
                    if (!openElements.empty()) {
                        openElements.back()->appendChild(textNode);
                    }
                }
                break;
            }
            
            default:
                break;
        }
    }
    
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
    // Full mapping for HTML5 elements
    // Document structure
    if (tagName == "html") return ElementType::HTML;
    if (tagName == "head") return ElementType::HEAD;
    if (tagName == "body") return ElementType::BODY;
    if (tagName == "title") return ElementType::HEAD; // special
    if (tagName == "meta") return ElementType::HEAD;
    if (tagName == "link") return ElementType::HEAD;
    if (tagName == "style") return ElementType::HEAD;
    if (tagName == "script") return ElementType::SPAN; // treat as inline
    
    // Block elements
    if (tagName == "div") return ElementType::DIV;
    if (tagName == "p") return ElementType::P;
    if (tagName == "section") return ElementType::DIV;
    if (tagName == "article") return ElementType::DIV;
    if (tagName == "nav") return ElementType::DIV;
    if (tagName == "aside") return ElementType::DIV;
    if (tagName == "header") return ElementType::DIV;
    if (tagName == "footer") return ElementType::DIV;
    if (tagName == "main") return ElementType::DIV;
    if (tagName == "figure") return ElementType::DIV;
    if (tagName == "figcaption") return ElementType::DIV;
    
    // Headings
    if (tagName == "h1") return ElementType::P;
    if (tagName == "h2") return ElementType::P;
    if (tagName == "h3") return ElementType::P;
    if (tagName == "h4") return ElementType::P;
    if (tagName == "h5") return ElementType::P;
    if (tagName == "h6") return ElementType::P;
    
    // Lists
    if (tagName == "ul") return ElementType::DIV;
    if (tagName == "ol") return ElementType::DIV;
    if (tagName == "li") return ElementType::DIV;
    if (tagName == "dl") return ElementType::DIV;
    if (tagName == "dt") return ElementType::DIV;
    if (tagName == "dd") return ElementType::DIV;
    
    // Tables
    if (tagName == "table") return ElementType::DIV;
    if (tagName == "thead") return ElementType::DIV;
    if (tagName == "tbody") return ElementType::DIV;
    if (tagName == "tfoot") return ElementType::DIV;
    if (tagName == "tr") return ElementType::DIV;
    if (tagName == "td") return ElementType::DIV;
    if (tagName == "th") return ElementType::DIV;
    if (tagName == "caption") return ElementType::DIV;
    
    // Forms
    if (tagName == "form") return ElementType::DIV;
    if (tagName == "input") return ElementType::SPAN;
    if (tagName == "button") return ElementType::SPAN;
    if (tagName == "textarea") return ElementType::DIV;
    if (tagName == "select") return ElementType::SPAN;
    if (tagName == "option") return ElementType::SPAN;
    if (tagName == "label") return ElementType::SPAN;
    if (tagName == "fieldset") return ElementType::DIV;
    if (tagName == "legend") return ElementType::DIV;
    
    // Inline elements
    if (tagName == "span") return ElementType::SPAN;
    if (tagName == "a") return ElementType::A;
    if (tagName == "strong") return ElementType::SPAN;
    if (tagName == "em") return ElementType::SPAN;
    if (tagName == "b") return ElementType::SPAN;
    if (tagName == "i") return ElementType::SPAN;
    if (tagName == "u") return ElementType::SPAN;
    if (tagName == "small") return ElementType::SPAN;
    if (tagName == "mark") return ElementType::SPAN;
    if (tagName == "del") return ElementType::SPAN;
    if (tagName == "ins") return ElementType::SPAN;
    if (tagName == "sub") return ElementType::SPAN;
    if (tagName == "sup") return ElementType::SPAN;
    if (tagName == "code") return ElementType::SPAN;
    if (tagName == "pre") return ElementType::DIV;
    if (tagName == "blockquote") return ElementType::DIV;
    if (tagName == "q") return ElementType::SPAN;
    if (tagName == "cite") return ElementType::SPAN;
    if (tagName == "abbr") return ElementType::SPAN;
    if (tagName == "time") return ElementType::SPAN;
    if (tagName == "kbd") return ElementType::SPAN;
    if (tagName == "samp") return ElementType::SPAN;
    if (tagName == "var") return ElementType::SPAN;
    
    // Media
    if (tagName == "img") return ElementType::IMG;
    if (tagName == "video") return ElementType::DIV;
    if (tagName == "audio") return ElementType::DIV;
    if (tagName == "source") return ElementType::SPAN;
    if (tagName == "track") return ElementType::SPAN;
    if (tagName == "canvas") return ElementType::DIV;
    if (tagName == "svg") return ElementType::DIV;
    if (tagName == "iframe") return ElementType::DIV;
    if (tagName == "embed") return ElementType::SPAN;
    if (tagName == "object") return ElementType::DIV;
    
    // Special
    if (tagName == "br") return ElementType::SPAN;
    if (tagName == "hr") return ElementType::DIV;
    if (tagName == "wbr") return ElementType::SPAN;
    
    // Default
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
