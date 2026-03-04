/**
 * @file ErrorRecovery.h
 * @brief Parser error recovery and synchronization
 * 
 * Implements:
 * - Panic mode recovery
 * - Synchronization tokens
 * - Error productions
 * - Multiple error collection
 * 
 * For robust parsing with good error messages
 */

#pragma once

#include "SourceLocation.h"
#include "token.hpp"
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>

namespace Zepra::Frontend {

// =============================================================================
// Diagnostic Severity
// =============================================================================

enum class DiagnosticSeverity : uint8_t {
    Error,
    Warning,
    Info,
    Hint
};

// =============================================================================
// Diagnostic
// =============================================================================

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string code;           // e.g. "E001", "W002"
    std::string message;
    SourceLocation location;
    
    // Related locations (e.g. "first declared here")
    std::vector<std::pair<std::string, SourceLocation>> relatedInfo;
    
    // Suggested fix
    struct Fix {
        std::string description;
        std::string replacement;
        SourceRange range;
    };
    std::optional<Fix> suggestedFix;
    
    bool isError() const { return severity == DiagnosticSeverity::Error; }
    bool isWarning() const { return severity == DiagnosticSeverity::Warning; }
    
    std::string format() const;
};

// =============================================================================
// Diagnostic Collector
// =============================================================================

class DiagnosticCollector {
public:
    DiagnosticCollector() = default;
    
    void addError(const std::string& message, const SourceLocation& loc);
    void addWarning(const std::string& message, const SourceLocation& loc);
    void addInfo(const std::string& message, const SourceLocation& loc);
    
    void add(Diagnostic diagnostic);
    
    bool hasErrors() const;
    bool hasWarnings() const;
    size_t errorCount() const;
    size_t warningCount() const;
    
    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
    std::vector<Diagnostic> errors() const;
    std::vector<Diagnostic> warnings() const;
    
    void clear() { diagnostics_.clear(); }
    
    /**
     * @brief Format all diagnostics for display
     */
    std::string formatAll() const;
    
    /**
     * @brief Set max errors before aborting
     */
    void setMaxErrors(size_t max) { maxErrors_ = max; }
    bool shouldAbort() const { return errorCount() >= maxErrors_; }
    
private:
    std::vector<Diagnostic> diagnostics_;
    size_t maxErrors_ = 100;
};

// =============================================================================
// Recovery Strategy
// =============================================================================

enum class RecoveryStrategy : uint8_t {
    None,           // Give up
    SyncToSemi,     // Skip to next semicolon
    SyncToBrace,    // Skip to next closing brace
    SyncToDecl,     // Skip to next declaration
    InsertToken,    // Insert missing token
    DeleteToken     // Delete current token
};

// =============================================================================
// Synchronization Tokens
// =============================================================================

class SyncTokens {
public:
    SyncTokens() = default;
    
    static SyncTokens forStatement() {
        return SyncTokens({TokenType::Semicolon, TokenType::RBrace, 
                           TokenType::Function, TokenType::Class,
                           TokenType::If, TokenType::While, TokenType::For});
    }
    
    static SyncTokens forExpression() {
        return SyncTokens({TokenType::Semicolon, TokenType::Comma,
                           TokenType::RParen, TokenType::RBracket});
    }
    
    static SyncTokens forBlock() {
        return SyncTokens({TokenType::RBrace});
    }
    
    bool contains(TokenType type) const {
        return tokens_.find(type) != tokens_.end();
    }
    
    void add(TokenType type) { tokens_.insert(type); }
    void remove(TokenType type) { tokens_.erase(type); }
    
private:
    explicit SyncTokens(std::initializer_list<TokenType> tokens) 
        : tokens_(tokens) {}
    
    std::unordered_set<TokenType> tokens_;
};

// =============================================================================
// Error Recovery Handler
// =============================================================================

class ErrorRecovery {
public:
    using TokenStream = std::function<Token()>;
    using PeekFunction = std::function<Token()>;
    
    ErrorRecovery(DiagnosticCollector& collector) : collector_(collector) {}
    
    /**
     * @brief Synchronize to recovery point
     * @return true if synchronized, false if EOF reached
     */
    bool synchronize(TokenStream advance, PeekFunction peek, const SyncTokens& sync);
    
    /**
     * @brief Report error and attempt recovery
     */
    RecoveryStrategy reportAndRecover(
        const std::string& message,
        const SourceLocation& loc,
        TokenStream advance,
        PeekFunction peek);
    
    /**
     * @brief Suggest token insertion
     */
    void suggestInsertion(TokenType expected, const SourceLocation& loc);
    
    /**
     * @brief Report unexpected token
     */
    void reportUnexpected(const Token& token, const std::string& expected = "");
    
    /**
     * @brief Check if in panic mode
     */
    bool isPanicking() const { return panicking_; }
    void resetPanic() { panicking_ = false; }
    
private:
    DiagnosticCollector& collector_;
    bool panicking_ = false;
    size_t syncDepth_ = 0;
    
    static constexpr size_t MAX_SYNC_TOKENS = 100;
};

// =============================================================================
// Error Production
// =============================================================================

/**
 * @brief Represents an error in the AST for partial parsing
 */
class ErrorNode {
public:
    ErrorNode(const std::string& message, const SourceLocation& loc)
        : message_(message), location_(loc) {}
    
    const std::string& message() const { return message_; }
    const SourceLocation& location() const { return location_; }
    
    // Skipped tokens
    void addSkippedToken(const Token& token) { skippedTokens_.push_back(token); }
    const std::vector<Token>& skippedTokens() const { return skippedTokens_; }
    
private:
    std::string message_;
    SourceLocation location_;
    std::vector<Token> skippedTokens_;
};

// =============================================================================
// Common Error Messages
// =============================================================================

namespace ErrorMessages {

std::string unexpectedToken(const Token& token);
std::string expectedToken(TokenType expected, const Token& got);
std::string expectedExpression(const Token& got);
std::string expectedIdentifier(const Token& got);
std::string undeclaredVariable(const std::string& name);
std::string duplicateDeclaration(const std::string& name);
std::string invalidAssignment();
std::string illegalBreak();
std::string illegalContinue();
std::string illegalReturn();

} // namespace ErrorMessages

} // namespace Zepra::Frontend
