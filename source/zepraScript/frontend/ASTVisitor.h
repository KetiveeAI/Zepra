/**
 * @file ASTVisitor.h
 * @brief Visitor pattern for AST traversal
 * 
 * Implements:
 * - Type-safe node visiting
 * - Pre/post visit hooks
 * - Context management
 * - Tree transformation
 * 
 * Based on classic visitor pattern for AST
 */

#pragma once

#include "ast.hpp"
#include <functional>
#include <stack>

namespace Zepra::Frontend {

// Forward declare all AST node types
class ASTNode;
class Program;
class FunctionDeclaration;
class VariableDeclaration;
class ClassDeclaration;
class ExpressionStatement;
class BlockStatement;
class IfStatement;
class WhileStatement;
class ForStatement;
class ReturnStatement;
class ThrowStatement;
class TryStatement;
class SwitchStatement;
class BreakStatement;
class ContinueStatement;
class LabeledStatement;
class WithStatement;
class DebuggerStatement;
class ImportDeclaration;
class ExportDeclaration;

class BinaryExpression;
class UnaryExpression;
class UpdateExpression;
class AssignmentExpression;
class MemberExpression;
class CallExpression;
class NewExpression;
class ConditionalExpression;
class SequenceExpression;
class ArrowFunctionExpression;
class YieldExpression;
class AwaitExpression;
class ArrayExpression;
class ObjectExpression;
class TemplateLiteral;
class TaggedTemplateExpression;
class SpreadElement;

class Identifier;
class Literal;
class ThisExpression;
class SuperExpression;

// =============================================================================
// Visitor Interface
// =============================================================================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // =========================================================================
    // Statements
    // =========================================================================
    
    virtual void visit(Program* node) = 0;
    virtual void visit(FunctionDeclaration* node) = 0;
    virtual void visit(VariableDeclaration* node) = 0;
    virtual void visit(ClassDeclaration* node) = 0;
    virtual void visit(ExpressionStatement* node) = 0;
    virtual void visit(BlockStatement* node) = 0;
    virtual void visit(IfStatement* node) = 0;
    virtual void visit(WhileStatement* node) = 0;
    virtual void visit(ForStatement* node) = 0;
    virtual void visit(ReturnStatement* node) = 0;
    virtual void visit(ThrowStatement* node) = 0;
    virtual void visit(TryStatement* node) = 0;
    virtual void visit(SwitchStatement* node) = 0;
    virtual void visit(BreakStatement* node) = 0;
    virtual void visit(ContinueStatement* node) = 0;
    virtual void visit(LabeledStatement* node) = 0;
    virtual void visit(WithStatement* node) = 0;
    virtual void visit(DebuggerStatement* node) = 0;
    virtual void visit(ImportDeclaration* node) = 0;
    virtual void visit(ExportDeclaration* node) = 0;
    
    // =========================================================================
    // Expressions
    // =========================================================================
    
    virtual void visit(BinaryExpression* node) = 0;
    virtual void visit(UnaryExpression* node) = 0;
    virtual void visit(UpdateExpression* node) = 0;
    virtual void visit(AssignmentExpression* node) = 0;
    virtual void visit(MemberExpression* node) = 0;
    virtual void visit(CallExpression* node) = 0;
    virtual void visit(NewExpression* node) = 0;
    virtual void visit(ConditionalExpression* node) = 0;
    virtual void visit(SequenceExpression* node) = 0;
    virtual void visit(ArrowFunctionExpression* node) = 0;
    virtual void visit(YieldExpression* node) = 0;
    virtual void visit(AwaitExpression* node) = 0;
    virtual void visit(ArrayExpression* node) = 0;
    virtual void visit(ObjectExpression* node) = 0;
    virtual void visit(TemplateLiteral* node) = 0;
    virtual void visit(TaggedTemplateExpression* node) = 0;
    virtual void visit(SpreadElement* node) = 0;
    
    // =========================================================================
    // Literals/Identifiers
    // =========================================================================
    
    virtual void visit(Identifier* node) = 0;
    virtual void visit(Literal* node) = 0;
    virtual void visit(ThisExpression* node) = 0;
    virtual void visit(SuperExpression* node) = 0;
};

// =============================================================================
// Default Visitor (walks all children)
// =============================================================================

class DefaultASTVisitor : public ASTVisitor {
public:
    void visit(Program* node) override;
    void visit(FunctionDeclaration* node) override;
    void visit(VariableDeclaration* node) override;
    void visit(ClassDeclaration* node) override;
    void visit(ExpressionStatement* node) override;
    void visit(BlockStatement* node) override;
    void visit(IfStatement* node) override;
    void visit(WhileStatement* node) override;
    void visit(ForStatement* node) override;
    void visit(ReturnStatement* node) override;
    void visit(ThrowStatement* node) override;
    void visit(TryStatement* node) override;
    void visit(SwitchStatement* node) override;
    void visit(BreakStatement* node) override;
    void visit(ContinueStatement* node) override;
    void visit(LabeledStatement* node) override;
    void visit(WithStatement* node) override;
    void visit(DebuggerStatement* node) override;
    void visit(ImportDeclaration* node) override;
    void visit(ExportDeclaration* node) override;
    
    void visit(BinaryExpression* node) override;
    void visit(UnaryExpression* node) override;
    void visit(UpdateExpression* node) override;
    void visit(AssignmentExpression* node) override;
    void visit(MemberExpression* node) override;
    void visit(CallExpression* node) override;
    void visit(NewExpression* node) override;
    void visit(ConditionalExpression* node) override;
    void visit(SequenceExpression* node) override;
    void visit(ArrowFunctionExpression* node) override;
    void visit(YieldExpression* node) override;
    void visit(AwaitExpression* node) override;
    void visit(ArrayExpression* node) override;
    void visit(ObjectExpression* node) override;
    void visit(TemplateLiteral* node) override;
    void visit(TaggedTemplateExpression* node) override;
    void visit(SpreadElement* node) override;
    
    void visit(Identifier* node) override {}
    void visit(Literal* node) override {}
    void visit(ThisExpression* node) override {}
    void visit(SuperExpression* node) override {}
    
protected:
    // Override in subclass for pre/post processing
    virtual void preVisit(ASTNode* node) {}
    virtual void postVisit(ASTNode* node) {}
};

// =============================================================================
// Visitor Context
// =============================================================================

struct VisitorContext {
    ASTNode* parent = nullptr;
    size_t depth = 0;
    std::vector<ASTNode*> path;
    
    void push(ASTNode* node) {
        path.push_back(node);
        parent = node;
        depth++;
    }
    
    void pop() {
        path.pop_back();
        parent = path.empty() ? nullptr : path.back();
        depth--;
    }
};

// =============================================================================
// Function-based Visitor
// =============================================================================

using NodeCallback = std::function<void(ASTNode*, VisitorContext&)>;

class FunctionVisitor : public DefaultASTVisitor {
public:
    FunctionVisitor(NodeCallback callback) : callback_(std::move(callback)) {}
    
protected:
    void preVisit(ASTNode* node) override {
        ctx_.push(node);
        callback_(node, ctx_);
    }
    
    void postVisit(ASTNode* node) override {
        ctx_.pop();
    }
    
private:
    NodeCallback callback_;
    VisitorContext ctx_;
};

// =============================================================================
// Tree Transformer
// =============================================================================

class ASTTransformer {
public:
    virtual ~ASTTransformer() = default;
    
    /**
     * @brief Transform AST in place
     */
    virtual void transform(Program* program);
    
protected:
    /**
     * @brief Override to transform specific node types
     * Return nullptr to remove node, or new node to replace
     */
    virtual ASTNode* transformNode(ASTNode* node) { return node; }
};

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * @brief Walk AST and call callback for each node
 */
void walkAST(ASTNode* root, NodeCallback callback);

/**
 * @brief Find all nodes matching predicate
 */
std::vector<ASTNode*> findNodes(ASTNode* root, 
                                 std::function<bool(ASTNode*)> predicate);

/**
 * @brief Find first node matching predicate
 */
ASTNode* findNode(ASTNode* root, std::function<bool(ASTNode*)> predicate);

/**
 * @brief Count nodes of specific type
 */
size_t countNodes(ASTNode* root, const std::string& nodeType);

} // namespace Zepra::Frontend
