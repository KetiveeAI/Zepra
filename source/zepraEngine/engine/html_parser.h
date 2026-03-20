// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "../common/types.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace zepra {

// Forward declarations
class HTMLToken;
class HTMLTokenizer;
class DOMBuilder;

// HTML Token Types
enum class TokenType {
    DOCTYPE,
    START_TAG,
    END_TAG,
    COMMENT,
    CHARACTER,
    EOF_TOKEN
};

// HTML Token
class HTMLToken {
public:
    TokenType type;
    String data;
    String tagName;
    AttributeMap attributes;
    bool selfClosing;
    
    HTMLToken() : type(TokenType::EOF_TOKEN), selfClosing(false) {}
    HTMLToken(TokenType t) : type(t), selfClosing(false) {}
};

// DOM Node Base Class
class DOMNode : public std::enable_shared_from_this<DOMNode> {
public:
    NodeType nodeType;
    String nodeName;
    String nodeValue;
    DOMNodePtr parentNode;
    DOMNodeList childNodes;
    
    DOMNode(NodeType type, const String& name = "") 
        : nodeType(type), nodeName(name) {}
    
    virtual ~DOMNode() = default;
    
    // DOM Methods
    virtual void appendChild(DOMNodePtr child);
    virtual void removeChild(DOMNodePtr child);
    virtual DOMNodePtr getElementById(const String& id);
    virtual DOMNodeList getElementsByTagName(const String& tagName);
    virtual DOMNodeList getElementsByClassName(const String& className);
    
    // Utility methods
    virtual String getTextContent() const;
    virtual void setTextContent(const String& text);
    virtual bool hasChildNodes() const { return !childNodes.empty(); }
    virtual Size getChildCount() const { return childNodes.size(); }
};

// Element Node
class ElementNode : public DOMNode {
public:
    ElementType elementType;
    AttributeMap attributes;
    StyleMap computedStyles;
    
    ElementNode(ElementType type, const String& tagName) 
        : DOMNode(NodeType::ELEMENT, tagName), elementType(type) {}
    
    // Element-specific methods
    void setAttribute(const String& name, const String& value);
    String getAttribute(const String& name) const;
    void removeAttribute(const String& name);
    bool hasAttribute(const String& name) const;
    
    // Style methods
    void setStyle(const String& property, const String& value);
    String getStyle(const String& property) const;
    
    // Override DOM methods
    DOMNodePtr getElementById(const String& id) override;
    DOMNodeList getElementsByTagName(const String& tagName) override;
    DOMNodeList getElementsByClassName(const String& className) override;
};

// Text Node
class TextNode : public DOMNode {
public:
    TextNode(const String& text) 
        : DOMNode(NodeType::TEXT, "#text") {
        nodeValue = text;
    }
    
    String getTextContent() const override { return nodeValue; }
    void setTextContent(const String& text) override { nodeValue = text; }
};

// Document Node
class DocumentNode : public DOMNode {
public:
    ElementNode* documentElement;
    ElementNode* head;
    ElementNode* body;
    
    DocumentNode() : DOMNode(NodeType::DOCUMENT, "#document"), 
                     documentElement(nullptr), head(nullptr), body(nullptr) {}
    
    // Document-specific methods
    ElementNode* getDocumentElement() const { return documentElement; }
    ElementNode* getHead() const { return head; }
    ElementNode* getBody() const { return body; }
    
    // Override DOM methods
    DOMNodePtr getElementById(const String& id) override;
    DOMNodeList getElementsByTagName(const String& tagName) override;
    DOMNodeList getElementsByClassName(const String& className) override;
};

// HTML Tokenizer
class HTMLTokenizer {
public:
    HTMLTokenizer(const String& input);
    ~HTMLTokenizer() = default;
    
    HTMLToken nextToken();
    bool hasMoreTokens() const;
    void reset();
    
private:
    String input;
    Size position;
    Size length;
    
    // Tokenization helpers
    void skipWhitespace();
    void skipComments();
    HTMLToken parseDoctype();
    HTMLToken parseStartTag();
    HTMLToken parseEndTag();
    HTMLToken parseComment();
    HTMLToken parseCharacter();
    AttributeMap parseAttributes();
    String parseTagName();
    String parseAttributeName();
    String parseAttributeValue();
    
    // Character helpers
    bool isWhitespace(char c) const;
    bool isLetter(char c) const;
    bool isDigit(char c) const;
    bool isNameChar(char c) const;
    char peek() const;
    char peekNext() const;
    char consume();
    bool consumeIf(char expected);
    String consumeWhile(std::function<bool(char)> predicate);
};

// DOM Builder
class DOMBuilder {
public:
    DOMBuilder();
    ~DOMBuilder() = default;
    
    DocumentNode* build(const String& html);
    void reset();
    
private:
    std::unique_ptr<HTMLTokenizer> tokenizer;
    DocumentNode* document;
    std::vector<DOMNode*> openElements;
    
    // Building helpers
    void processToken(const HTMLToken& token);
    void processStartTag(const HTMLToken& token);
    void processEndTag(const HTMLToken& token);
    void processCharacter(const HTMLToken& token);
    void processDoctype(const HTMLToken& token);
    void processComment(const HTMLToken& token);
    
    // Stack management
    void pushElement(ElementNode* element);
    void popElement();
    ElementNode* currentElement() const;
    bool hasElementInScope(const String& tagName) const;
    
    // Element creation
    ElementNode* createElement(const String& tagName);
    TextNode* createTextNode(const String& text);
    ElementType parseElementType(const String& tagName) const;
};

// HTML Parser Main Class
class HTMLParser {
public:
    HTMLParser();
    ~HTMLParser() = default;
    
    // Main parsing interface
    DocumentNode* parse(const String& html);
    DocumentNode* parseFile(const String& filename);
    
    // Error handling
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<String>& getErrors() const { return errors; }
    void clearErrors() { errors.clear(); }
    
    // Performance
    void setMaxNodes(Size max) { maxNodes = max; }
    Size getMaxNodes() const { return maxNodes; }
    
private:
    std::unique_ptr<DOMBuilder> builder;
    std::vector<String> errors;
    Size maxNodes;
    Size nodeCount;
    
    // Error reporting
    void reportError(const String& message);
    bool checkNodeLimit();
};

// Utility functions
namespace html_utils {
    String escapeHtml(const String& text);
    String unescapeHtml(const String& text);
    bool isValidTagName(const String& tagName);
    bool isVoidElement(const String& tagName);
    String normalizeWhitespace(const String& text);
    String extractTextContent(const DOMNode* node);
    String serializeNode(const DOMNode* node);
}

} // namespace zepra 