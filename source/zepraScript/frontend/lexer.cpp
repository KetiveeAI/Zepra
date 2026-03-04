/**
 * @file lexer.cpp
 * @brief JavaScript tokenizer implementation
 */

#include "frontend/lexer.hpp"
#include <cctype>
#include <cstdlib>
#include <unordered_map>

namespace Zepra::Frontend {

namespace {

// Keyword lookup table
const std::unordered_map<std::string_view, TokenType> keywords = {
    {"var", TokenType::Var},
    {"let", TokenType::Let},
    {"const", TokenType::Const},
    {"function", TokenType::Function},
    {"return", TokenType::Return},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"for", TokenType::For},
    {"while", TokenType::While},
    {"do", TokenType::Do},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"switch", TokenType::Switch},
    {"case", TokenType::Case},
    {"default", TokenType::Default},
    {"try", TokenType::Try},
    {"catch", TokenType::Catch},
    {"finally", TokenType::Finally},
    {"throw", TokenType::Throw},
    {"new", TokenType::New},
    {"delete", TokenType::Delete},
    {"typeof", TokenType::Typeof},
    {"instanceof", TokenType::Instanceof},
    {"in", TokenType::In},
    {"this", TokenType::This},
    {"class", TokenType::Class},
    {"extends", TokenType::Extends},
    {"super", TokenType::Super},
    {"static", TokenType::Static},
    {"import", TokenType::Import},
    {"export", TokenType::Export},
    {"from", TokenType::From},
    {"as", TokenType::As},
    {"async", TokenType::Async},
    {"await", TokenType::Await},
    {"yield", TokenType::Yield},
    {"null", TokenType::Null},
    {"undefined", TokenType::Undefined},
    {"true", TokenType::True},
    {"false", TokenType::False},
    {"void", TokenType::Void},
    {"with", TokenType::With},
    {"debugger", TokenType::Debugger},
    {"of", TokenType::Of},
    {"get", TokenType::Get},
    {"set", TokenType::Set},
};

} // anonymous namespace

Lexer::Lexer(const SourceCode* source)
    : source_(source)
    , offset_(0)
    , location_{1, 1, 0}
    , tokenStart_{1, 1, 0}
{
}

char Lexer::current() const {
    if (offset_ >= source_->length()) return '\0';
    return source_->at(offset_);
}

char Lexer::peek(size_t ahead) const {
    if (offset_ + ahead >= source_->length()) return '\0';
    return source_->at(offset_ + ahead);
}

char Lexer::advance() {
    char c = current();
    offset_++;
    
    if (c == '\n') {
        location_.line++;
        location_.column = 1;
    } else {
        location_.column++;
    }
    location_.offset = offset_;
    
    return c;
}

bool Lexer::match(char expected) {
    if (current() != expected) return false;
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = current();
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '\n':
                advance();
                break;
            case '/':
                if (peek(1) == '/') {
                    skipLineComment();
                } else if (peek(1) == '*') {
                    skipBlockComment();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

void Lexer::skipLineComment() {
    // Skip //
    advance();
    advance();
    
    while (current() != '\0' && current() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    // Skip /*
    advance();
    advance();
    
    while (current() != '\0') {
        if (current() == '*' && peek(1) == '/') {
            advance();
            advance();
            return;
        }
        advance();
    }
    
    addError("Unterminated block comment");
}

Token Lexer::nextToken() {
    if (hasPeeked_) {
        hasPeeked_ = false;
        return std::move(peekedToken_);
    }
    
    skipWhitespace();
    tokenStart_ = location_;
    
    if (isEof()) {
        return makeToken(TokenType::EndOfFile);
    }
    
    char c = advance();
    
    // Identifiers and keywords
    if (isIdentifierStart(c)) {
        return scanIdentifierOrKeyword();
    }
    
    // Numbers
    if (isDigit(c) || (c == '.' && isDigit(peek(0)))) {
        return scanNumber();
    }
    
    // Strings
    if (c == '"' || c == '\'') {
        return scanString(c);
    }
    
    // Template literals
    if (c == '`') {
        return scanTemplate();
    }
    
    // Operators and punctuation
    return scanOperator();
}

const Token& Lexer::peek() {
    if (!hasPeeked_) {
        peekedToken_ = nextToken();
        hasPeeked_ = true;
    }
    return peekedToken_;
}

Token Lexer::scanNumber() {
    size_t start = offset_ - 1;
    
    // Handle hex, octal, binary
    if (source_->at(start) == '0' && offset_ < source_->length()) {
        char next = current();
        if (next == 'x' || next == 'X') {
            advance();
            while (isHexDigit(current())) advance();
            std::string value = std::string(source_->substring(start, offset_ - start));
            Token token = makeToken(TokenType::Number, value);
            token.numericValue = std::strtol(value.c_str() + 2, nullptr, 16);
            return token;
        }
        if (next == 'b' || next == 'B') {
            advance();
            while (current() == '0' || current() == '1') advance();
            std::string value = std::string(source_->substring(start, offset_ - start));
            Token token = makeToken(TokenType::Number, value);
            token.numericValue = std::strtol(value.c_str() + 2, nullptr, 2);
            return token;
        }
        if (next == 'o' || next == 'O') {
            advance();
            while (current() >= '0' && current() <= '7') advance();
            std::string value = std::string(source_->substring(start, offset_ - start));
            Token token = makeToken(TokenType::Number, value);
            token.numericValue = std::strtol(value.c_str() + 2, nullptr, 8);
            return token;
        }
    }
    
    // Integer part
    while (isDigit(current())) advance();
    
    // Decimal part
    if (current() == '.' && isDigit(peek(1))) {
        advance();
        while (isDigit(current())) advance();
    }
    
    // Exponent
    if (current() == 'e' || current() == 'E') {
        advance();
        if (current() == '+' || current() == '-') advance();
        while (isDigit(current())) advance();
    }
    
    std::string value = std::string(source_->substring(start, offset_ - start));
    Token token = makeToken(TokenType::Number, value);
    token.numericValue = std::strtod(value.c_str(), nullptr);
    return token;
}

Token Lexer::scanString(char quote) {
    std::string value;
    bool hasEscapes = false;
    
    while (current() != '\0' && current() != quote) {
        if (current() == '\\') {
            hasEscapes = true;
            advance();
            switch (current()) {
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                case 'b': value += '\b'; break;
                case 'f': value += '\f'; break;
                case 'v': value += '\v'; break;
                case '0': value += '\0'; break;
                case '\\': value += '\\'; break;
                case '\'': value += '\''; break;
                case '"': value += '"'; break;
                case 'u': {
                    // Unicode escape
                    advance();
                    // TODO: Parse \uXXXX and \u{XXXXX}
                    value += '?';
                    break;
                }
                default:
                    value += current();
                    break;
            }
            advance();
        } else if (current() == '\n') {
            addError("Unterminated string literal");
            break;
        } else {
            value += advance();
        }
    }
    
    if (current() != quote) {
        return errorToken("Unterminated string literal");
    }
    advance(); // Closing quote
    
    Token token = makeToken(TokenType::String, value);
    token.hasEscapes = hasEscapes;
    return token;
}

Token Lexer::scanIdentifierOrKeyword() {
    size_t start = offset_ - 1;
    
    while (isIdentifierPart(current())) {
        advance();
    }
    
    std::string value = std::string(source_->substring(start, offset_ - start));
    
    // Check if keyword
    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return makeToken(it->second, value);
    }
    
    return makeToken(TokenType::Identifier, value);
}

Token Lexer::scanTemplate() {
    // Scan template literal: `Hello ${name}!`
    // Returns Template token with parts and expressions
    
    std::string value;
    bool hasExpressions = false;
    
    while (current() != '\0' && current() != '`') {
        if (current() == '\\') {
            // Escape sequence
            advance();
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '`': value += '`'; break;
                case '$': value += '$'; break;
                default: value += escaped; break;
            }
        } else if (current() == '$' && peek(1) == '{') {
            // Embedded expression: ${...}
            hasExpressions = true;
            
            // Mark where expression starts
            value += "\x01"; // Use special marker for parser
            advance(); // $
            advance(); // {
            
            // Scan until } (with nesting support)
            int braceDepth = 1;
            while (current() != '\0' && braceDepth > 0) {
                if (current() == '{') braceDepth++;
                else if (current() == '}') braceDepth--;
                
                if (braceDepth > 0) {
                    value += advance();
                }
            }
            
            if (current() == '}') {
                advance(); // Close }
                value += "\x02"; // End expression marker
            } else {
                return errorToken("Unterminated template expression");
            }
        } else {
            value += advance();
        }
    }
    
    if (current() != '`') {
        return errorToken("Unterminated template literal");
    }
    advance(); // Closing `
    
    return makeToken(TokenType::Template, value);
}

Token Lexer::scanOperator() {
    char c = source_->at(offset_ - 1);
    
    switch (c) {
        // Single character
        case '(': return makeToken(TokenType::LeftParen);
        case ')': return makeToken(TokenType::RightParen);
        case '{': return makeToken(TokenType::LeftBrace);
        case '}': return makeToken(TokenType::RightBrace);
        case '[': return makeToken(TokenType::LeftBracket);
        case ']': return makeToken(TokenType::RightBracket);
        case ',': return makeToken(TokenType::Comma);
        case ';': return makeToken(TokenType::Semicolon);
        case ':': return makeToken(TokenType::Colon);
        case '~': return makeToken(TokenType::Tilde);
        
        // Potentially multi-character
        case '.':
            if (match('.') && match('.')) {
                return makeToken(TokenType::DotDotDot);
            }
            return makeToken(TokenType::Dot);
            
        case '+':
            if (match('+')) return makeToken(TokenType::PlusPlus);
            if (match('=')) return makeToken(TokenType::PlusAssign);
            return makeToken(TokenType::Plus);
            
        case '-':
            if (match('-')) return makeToken(TokenType::MinusMinus);
            if (match('=')) return makeToken(TokenType::MinusAssign);
            return makeToken(TokenType::Minus);
            
        case '*':
            if (match('*')) {
                if (match('=')) return makeToken(TokenType::StarStarAssign);
                return makeToken(TokenType::StarStar);
            }
            if (match('=')) return makeToken(TokenType::StarAssign);
            return makeToken(TokenType::Star);
            
        case '/':
            if (match('=')) return makeToken(TokenType::SlashAssign);
            return makeToken(TokenType::Slash);
            
        case '%':
            if (match('=')) return makeToken(TokenType::PercentAssign);
            return makeToken(TokenType::Percent);
            
        case '=':
            if (match('=')) {
                if (match('=')) return makeToken(TokenType::StrictEqual);
                return makeToken(TokenType::Equal);
            }
            if (match('>')) return makeToken(TokenType::Arrow);
            return makeToken(TokenType::Assign);
            
        case '!':
            if (match('=')) {
                if (match('=')) return makeToken(TokenType::StrictNotEqual);
                return makeToken(TokenType::NotEqual);
            }
            return makeToken(TokenType::Not);
            
        case '<':
            if (match('<')) {
                if (match('=')) return makeToken(TokenType::LeftShiftAssign);
                return makeToken(TokenType::LeftShift);
            }
            if (match('=')) return makeToken(TokenType::LessEqual);
            return makeToken(TokenType::Less);
            
        case '>':
            if (match('>')) {
                if (match('>')) {
                    if (match('=')) return makeToken(TokenType::UnsignedRightShiftAssign);
                    return makeToken(TokenType::UnsignedRightShift);
                }
                if (match('=')) return makeToken(TokenType::RightShiftAssign);
                return makeToken(TokenType::RightShift);
            }
            if (match('=')) return makeToken(TokenType::GreaterEqual);
            return makeToken(TokenType::Greater);
            
        case '&':
            if (match('&')) {
                if (match('=')) return makeToken(TokenType::AndAssign);
                return makeToken(TokenType::And);
            }
            if (match('=')) return makeToken(TokenType::AmpersandAssign);
            return makeToken(TokenType::Ampersand);
            
        case '|':
            if (match('|')) {
                if (match('=')) return makeToken(TokenType::OrAssign);
                return makeToken(TokenType::Or);
            }
            if (match('=')) return makeToken(TokenType::PipeAssign);
            return makeToken(TokenType::Pipe);
            
        case '^':
            if (match('=')) return makeToken(TokenType::CaretAssign);
            return makeToken(TokenType::Caret);
            
        case '?':
            if (match('?')) {
                if (match('=')) return makeToken(TokenType::QuestionQuestionAssign);
                return makeToken(TokenType::QuestionQuestion);
            }
            if (match('.')) return makeToken(TokenType::QuestionDot);
            return makeToken(TokenType::Question);
    }
    
    return errorToken("Unexpected character");
}

Token Lexer::makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = tokenStart_;
    token.end = location_;
    return token;
}

Token Lexer::makeToken(TokenType type, std::string value) {
    Token token;
    token.type = type;
    token.value = std::move(value);
    token.start = tokenStart_;
    token.end = location_;
    return token;
}

Token Lexer::errorToken(const std::string& message) {
    addError(message);
    return makeToken(TokenType::Error, message);
}

void Lexer::addError(const std::string& message) {
    errors_.push_back(std::to_string(location_.line) + ":" + 
                      std::to_string(location_.column) + ": " + message);
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isHexDigit(char c) const {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

bool Lexer::isIdentifierStart(char c) const {
    return isAlpha(c) || c == '_' || c == '$';
}

bool Lexer::isIdentifierPart(char c) const {
    return isIdentifierStart(c) || isDigit(c);
}

// Token helper methods
std::string SourceLocation::toString() const {
    return std::to_string(line) + ":" + std::to_string(column);
}

bool Token::isKeyword() const {
    return type >= TokenType::Var && type <= TokenType::Set;
}

bool Token::isOperator() const {
    return type >= TokenType::Plus && type <= TokenType::Arrow;
}

bool Token::isPunctuation() const {
    return type >= TokenType::LeftParen && type <= TokenType::Arrow;
}

bool Token::isLiteral() const {
    return type == TokenType::Number || type == TokenType::String ||
           type == TokenType::True || type == TokenType::False ||
           type == TokenType::Null;
}

bool Token::isAssignment() const {
    return type == TokenType::Assign || type == TokenType::PlusAssign ||
           type == TokenType::MinusAssign || type == TokenType::StarAssign ||
           type == TokenType::SlashAssign || type == TokenType::PercentAssign;
}

const char* Token::typeName(TokenType type) {
    switch (type) {
        case TokenType::Number: return "Number";
        case TokenType::String: return "String";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Var: return "var";
        case TokenType::Let: return "let";
        case TokenType::Const: return "const";
        case TokenType::Function: return "function";
        case TokenType::Return: return "return";
        case TokenType::If: return "if";
        case TokenType::Else: return "else";
        case TokenType::For: return "for";
        case TokenType::While: return "while";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Equal: return "==";
        case TokenType::StrictEqual: return "===";
        case TokenType::Assign: return "=";
        case TokenType::LeftParen: return "(";
        case TokenType::RightParen: return ")";
        case TokenType::LeftBrace: return "{";
        case TokenType::RightBrace: return "}";
        case TokenType::Semicolon: return ";";
        case TokenType::EndOfFile: return "EOF";
        default: return "Token";
    }
}

bool isKeyword(std::string_view str) {
    return keywords.find(str) != keywords.end();
}

TokenType keywordType(std::string_view str) {
    auto it = keywords.find(str);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::Identifier;
}

} // namespace Zepra::Frontend
