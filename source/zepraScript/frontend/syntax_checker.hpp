#pragma once

/**
 * @file syntax_checker.hpp
 * @brief Static syntax validation
 */

#include "ast.hpp"
#include <vector>
#include <string>

namespace Zepra::Frontend {

/**
 * @brief Static syntax analyzer
 * 
 * Performs additional syntax checks that are easier to do
 * after parsing (e.g., duplicate declarations, label validation).
 */
class SyntaxChecker {
public:
    /**
     * @brief Check a program for syntax errors
     * @return true if no errors found
     */
    bool check(const Program* program);
    
    /**
     * @brief Get all errors found
     */
    const std::vector<std::string>& errors() const { return errors_; }
    
    /**
     * @brief Check if any errors were found
     */
    bool hasErrors() const { return !errors_.empty(); }
    
private:
    void visitStatement(const Statement* stmt);
    void visitExpression(const Expression* expr);
    void visitDeclaration(const Declaration* decl);
    
    void checkVariableDeclaration(const VariableDecl* decl);
    void checkFunctionDeclaration(const FunctionDecl* decl);
    void checkReturnStatement(const ReturnStmt* stmt);
    void checkBreakStatement(const BreakStmt* stmt);
    void checkContinueStatement(const ContinueStmt* stmt);
    
    void error(const ASTNode* node, const std::string& message);
    
    std::vector<std::string> errors_;
    
    // Context tracking
    bool inFunction_ = false;
    bool inLoop_ = false;
    bool inSwitch_ = false;
};

} // namespace Zepra::Frontend
