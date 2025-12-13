/**
 * @file html_parser.cpp
 * @brief HTML parsing implementation
 */

#include "webcore/html_parser.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace Zepra::WebCore {

// =============================================================================
// HTMLTokenizer Implementation
// =============================================================================

HTMLTokenizer::HTMLTokenizer(const std::string& html) : html_(html), pos_(0) {}

bool HTMLTokenizer::hasMoreTokens() const {
    return pos_ < html_.size();
}

char HTMLTokenizer::current() const {
    if (pos_ >= html_.size()) return '\0';
    return html_[pos_];
}

char HTMLTokenizer::peek(size_t offset) const {
    if (pos_ + offset >= html_.size()) return '\0';
    return html_[pos_ + offset];
}

void HTMLTokenizer::advance(size_t count) {
    pos_ += count;
}

bool HTMLTokenizer::match(const std::string& str) const {
    if (pos_ + str.size() > html_.size()) return false;
    for (size_t i = 0; i < str.size(); ++i) {
        if (std::tolower(html_[pos_ + i]) != std::tolower(str[i])) return false;
    }
    return true;
}

void HTMLTokenizer::consumeWhitespace() {
    while (pos_ < html_.size() && std::isspace(html_[pos_])) {
        pos_++;
    }
}

std::string HTMLTokenizer::consumeTagName() {
    std::string name;
    while (pos_ < html_.size()) {
        char c = html_[pos_];
        if (std::isalnum(c) || c == '-' || c == '_' || c == ':') {
            name += std::tolower(c);
            pos_++;
        } else {
            break;
        }
    }
    return name;
}

std::string HTMLTokenizer::consumeAttributeName() {
    std::string name;
    while (pos_ < html_.size()) {
        char c = html_[pos_];
        if (std::isalnum(c) || c == '-' || c == '_' || c == ':' || c == '.') {
            name += std::tolower(c);
            pos_++;
        } else {
            break;
        }
    }
    return name;
}

std::string HTMLTokenizer::consumeAttributeValue() {
    consumeWhitespace();
    if (current() == '=') {
        advance();
        consumeWhitespace();
    } else {
        return "";
    }
    
    std::string value;
    char quote = current();
    if (quote == '"' || quote == '\'') {
        advance();
        while (pos_ < html_.size() && html_[pos_] != quote) {
            value += html_[pos_++];
        }
        if (current() == quote) advance();
    } else {
        // Unquoted value
        while (pos_ < html_.size() && !std::isspace(html_[pos_]) && 
               html_[pos_] != '>' && html_[pos_] != '/') {
            value += html_[pos_++];
        }
    }
    return value;
}

std::string HTMLTokenizer::consumeText() {
    std::string text;
    while (pos_ < html_.size() && html_[pos_] != '<') {
        text += html_[pos_++];
    }
    return text;
}

std::string HTMLTokenizer::consumeComment() {
    std::string comment;
    // Skip <!--
    advance(4);
    while (pos_ + 2 < html_.size()) {
        if (html_[pos_] == '-' && html_[pos_+1] == '-' && html_[pos_+2] == '>') {
            advance(3);
            break;
        }
        comment += html_[pos_++];
    }
    return comment;
}

std::string HTMLTokenizer::consumeDoctype() {
    std::string doctype;
    // Skip <!DOCTYPE
    advance(9);
    consumeWhitespace();
    while (pos_ < html_.size() && html_[pos_] != '>') {
        doctype += html_[pos_++];
    }
    if (current() == '>') advance();
    return doctype;
}

std::string HTMLTokenizer::consumeRawText(const std::string& endTag) {
    std::string content;
    std::string endPattern = "</" + endTag;
    
    while (pos_ < html_.size()) {
        // Check for end tag
        if (match(endPattern)) {
            break;
        }
        content += html_[pos_++];
    }
    return content;
}

std::string HTMLTokenizer::decodeEntities(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '&') {
            // Named entities
            if (text.compare(i, 5, "&amp;") == 0) {
                result += '&'; i += 4;
            } else if (text.compare(i, 4, "&lt;") == 0) {
                result += '<'; i += 3;
            } else if (text.compare(i, 4, "&gt;") == 0) {
                result += '>'; i += 3;
            } else if (text.compare(i, 6, "&quot;") == 0) {
                result += '"'; i += 5;
            } else if (text.compare(i, 6, "&apos;") == 0) {
                result += '\''; i += 5;
            } else if (text.compare(i, 6, "&nbsp;") == 0) {
                result += ' '; i += 5;
            } else if (text.compare(i, 6, "&copy;") == 0) {
                result += "©"; i += 5;
            } else if (i + 2 < text.size() && text[i + 1] == '#') {
                // Numeric entity &#NNN; or &#xHHH;
                size_t start = i + 2;
                bool hex = (text[start] == 'x' || text[start] == 'X');
                if (hex) start++;
                
                size_t end = start;
                while (end < text.size() && text[end] != ';') end++;
                
                if (end < text.size() && text[end] == ';') {
                    std::string num = text.substr(start, end - start);
                    try {
                        int code = hex ? std::stoi(num, nullptr, 16) : std::stoi(num);
                        if (code < 128) {
                            result += static_cast<char>(code);
                        } else {
                            // UTF-8 encode
                            if (code < 0x800) {
                                result += static_cast<char>(0xC0 | (code >> 6));
                                result += static_cast<char>(0x80 | (code & 0x3F));
                            } else {
                                result += static_cast<char>(0xE0 | (code >> 12));
                                result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                                result += static_cast<char>(0x80 | (code & 0x3F));
                            }
                        }
                        i = end;
                    } catch (...) {
                        result += text[i];
                    }
                } else {
                    result += text[i];
                }
            } else {
                result += text[i];
            }
        } else {
            result += text[i];
        }
    }
    return result;
}

HTMLToken HTMLTokenizer::nextToken() {
    HTMLToken token;
    
    while (pos_ < html_.size()) {
        if (current() == '<') {
            // Comment
            if (match("<!--")) {
                token.type = HTMLTokenType::Comment;
                token.data = consumeComment();
                return token;
            }
            
            // DOCTYPE
            if (match("<!DOCTYPE") || match("<!doctype")) {
                token.type = HTMLTokenType::DOCTYPE;
                token.data = consumeDoctype();
                return token;
            }
            
            // End tag
            if (peek() == '/') {
                advance(2); // Skip </
                token.type = HTMLTokenType::EndTag;
                token.name = consumeTagName();
                consumeWhitespace();
                if (current() == '>') advance();
                return token;
            }
            
            // Start tag
            advance(); // Skip <
            token.type = HTMLTokenType::StartTag;
            token.name = consumeTagName();
            
            // Parse attributes
            while (pos_ < html_.size()) {
                consumeWhitespace();
                if (current() == '>' || current() == '/') break;
                
                std::string attrName = consumeAttributeName();
                if (attrName.empty()) break;
                
                std::string attrValue = consumeAttributeValue();
                token.attributes.push_back({attrName, attrValue});
            }
            
            // Self-closing?
            if (current() == '/') {
                token.selfClosing = true;
                advance();
            }
            if (current() == '>') advance();
            
            return token;
        }
        
        // Text content
        std::string text = consumeText();
        if (!text.empty()) {
            // Trim and normalize whitespace
            std::string normalized;
            bool lastWasSpace = true;
            for (char c : text) {
                if (std::isspace(c)) {
                    if (!lastWasSpace) {
                        normalized += ' ';
                        lastWasSpace = true;
                    }
                } else {
                    normalized += c;
                    lastWasSpace = false;
                }
            }
            
            // Trim trailing space
            if (!normalized.empty() && normalized.back() == ' ') {
                normalized.pop_back();
            }
            
            if (!normalized.empty()) {
                token.type = HTMLTokenType::Text;
                token.data = normalized;
                return token;
            }
        }
    }
    
    token.type = HTMLTokenType::EndOfFile;
    return token;
}

// =============================================================================
// HTMLParser Implementation
// =============================================================================

std::unique_ptr<DOMDocument> HTMLParser::parse(const std::string& html) {
    document_ = std::make_unique<DOMDocument>();
    
    // Create basic structure
    auto htmlEl = document_->createElement("html");
    DOMElement* htmlPtr = htmlEl.get();
    document_->appendChild(std::move(htmlEl));
    document_->setDocumentElement(htmlPtr);
    
    auto head = document_->createElement("head");
    auto body = document_->createElement("body");
    DOMElement* bodyPtr = body.get();
    
    htmlPtr->appendChild(std::move(head));
    htmlPtr->appendChild(std::move(body));
    
    openElements_.push(bodyPtr);
    
    HTMLTokenizer tokenizer(html);
    while (tokenizer.hasMoreTokens()) {
        HTMLToken token = tokenizer.nextToken();
        if (token.type == HTMLTokenType::EndOfFile) break;
        processToken(token);
    }
    
    return std::move(document_);
}

std::unique_ptr<DOMElement> HTMLParser::parseFragment(const std::string& html) {
    auto doc = parse(html);
    
    // Extract body content
    auto result = doc->createElement("div");
    
    // Find body
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
    
    if (auto* body = findBody(doc.get())) {
        // Clone children to result
        for (auto& child : body->childNodes()) {
            (void)child; // Would need proper cloning
        }
    }
    
    return result;
}

void HTMLParser::processToken(const HTMLToken& token) {
    switch (token.type) {
        case HTMLTokenType::StartTag:
            processStartTag(token);
            break;
        case HTMLTokenType::EndTag:
            processEndTag(token);
            break;
        case HTMLTokenType::Text:
            processText(token);
            break;
        case HTMLTokenType::Comment:
            processComment(token);
            break;
        default:
            break;
    }
}

void HTMLParser::processStartTag(const HTMLToken& token) {
    auto element = document_->createElement(token.name);
    
    // Apply attributes
    for (const auto& [name, value] : token.attributes) {
        element->setAttribute(name, value);
    }
    
    if (!openElements_.empty()) {
        DOMElement* parent = openElements_.top();
        DOMElement* elPtr = element.get();
        parent->appendChild(std::move(element));
        
        // Don't push void elements
        if (!token.selfClosing && !isVoidElement(token.name)) {
            openElements_.push(elPtr);
        }
    }
}

void HTMLParser::processEndTag(const HTMLToken& token) {
    // Find matching open element
    std::stack<DOMElement*> temp;
    bool found = false;
    
    while (!openElements_.empty()) {
        DOMElement* el = openElements_.top();
        if (el->tagName() == token.name) {
            openElements_.pop();
            found = true;
            break;
        }
        // Save for restoration if not found
        temp.push(el);
        openElements_.pop();
    }
    
    // Restore if not found (malformed HTML)
    if (!found) {
        while (!temp.empty()) {
            openElements_.push(temp.top());
            temp.pop();
        }
    }
}

void HTMLParser::processText(const HTMLToken& token) {
    if (!openElements_.empty() && !token.data.empty()) {
        auto text = document_->createTextNode(token.data);
        openElements_.top()->appendChild(std::move(text));
    }
}

void HTMLParser::processComment(const HTMLToken& token) {
    (void)token; // Comments are ignored in this simple parser
}

DOMElement* HTMLParser::currentElement() const {
    return openElements_.empty() ? nullptr : openElements_.top();
}

void HTMLParser::pushElement(std::unique_ptr<DOMElement> element) {
    DOMElement* ptr = element.get();
    if (!openElements_.empty()) {
        openElements_.top()->appendChild(std::move(element));
    }
    openElements_.push(ptr);
}

void HTMLParser::popElement() {
    if (!openElements_.empty()) {
        openElements_.pop();
    }
}

bool HTMLParser::isElementInScope(const std::string& tagName) const {
    std::stack<DOMElement*> temp = openElements_;
    while (!temp.empty()) {
        if (temp.top()->tagName() == tagName) return true;
        temp.pop();
    }
    return false;
}

bool HTMLParser::isVoidElement(const std::string& tagName) const {
    static const std::vector<std::string> voidElements = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr"
    };
    return std::find(voidElements.begin(), voidElements.end(), tagName) != voidElements.end();
}

bool HTMLParser::isFormattingElement(const std::string& tagName) const {
    static const std::vector<std::string> formattingElements = {
        "a", "b", "big", "code", "em", "font", "i", "nobr", "s",
        "small", "strike", "strong", "tt", "u"
    };
    return std::find(formattingElements.begin(), formattingElements.end(), tagName) != formattingElements.end();
}

void HTMLParser::adoptionAgencyAlgorithm(const std::string& tagName) {
    (void)tagName; // Advanced algorithm not implemented
}

void HTMLParser::fosterParent(std::unique_ptr<DOMNode> node) {
    (void)node; // Foster parenting not implemented
}

// =============================================================================
// HTMLSerializer Implementation
// =============================================================================

std::string HTMLSerializer::serialize(DOMNode* node) {
    if (!node) return "";
    
    switch (node->nodeType()) {
        case NodeType::Element:
            return serializeElement(static_cast<DOMElement*>(node));
        case NodeType::Text:
            return serializeText(static_cast<DOMText*>(node));
        case NodeType::Document: {
            std::string result = "<!DOCTYPE html>\n";
            for (auto& child : node->childNodes()) {
                result += serialize(child.get());
            }
            return result;
        }
        default:
            return "";
    }
}

std::string HTMLSerializer::serializeElement(DOMElement* element) {
    std::string result = "<" + element->tagName();
    
    // Serialize attributes
    for (const auto& [name, value] : element->attributes()) {
        result += " " + name + "=\"" + escapeAttribute(value) + "\"";
    }
    
    // Void elements
    static const std::vector<std::string> voidElements = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr"
    };
    
    bool isVoid = std::find(voidElements.begin(), voidElements.end(), 
                            element->tagName()) != voidElements.end();
    
    if (isVoid) {
        result += " />";
    } else {
        result += ">";
        for (auto& child : element->childNodes()) {
            result += serialize(child.get());
        }
        result += "</" + element->tagName() + ">";
    }
    
    return result;
}

std::string HTMLSerializer::serializeText(DOMText* text) {
    return escapeHTML(text->data());
}

std::string HTMLSerializer::escapeHTML(const std::string& text) {
    std::string result;
    for (char c : text) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            default: result += c;
        }
    }
    return result;
}

std::string HTMLSerializer::escapeAttribute(const std::string& value) {
    std::string result;
    for (char c : value) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            default: result += c;
        }
    }
    return result;
}

} // namespace Zepra::WebCore
