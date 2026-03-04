/**
 * @file syntax_checker.cpp
 * @brief Post-parse static syntax validation
 *
 * Catches early errors that are hard to detect during recursive descent:
 * - const without initializer
 * - break/continue outside loops
 * - return outside functions
 * - duplicate let/const in same scope
 */

#include "frontend/syntax_checker.hpp"
#include <unordered_set>

namespace Zepra::Frontend {

void SyntaxChecker::error(const ASTNode* node, const std::string& message) {
    std::string err;
    if (node) {
        err = std::to_string(node->location().line) + ":" +
              std::to_string(node->location().column) + ": ";
    }
    err += message;
    errors_.push_back(err);
}

bool SyntaxChecker::check(const Program* program) {
    errors_.clear();
    inFunction_ = false;
    inLoop_ = false;
    inSwitch_ = false;

    if (!program) return true;

    for (const auto& stmt : program->body()) {
        visitStatement(stmt.get());
    }

    return errors_.empty();
}

void SyntaxChecker::visitStatement(const Statement* stmt) {
    if (!stmt) return;

    switch (stmt->type()) {
        case NodeType::VariableDeclaration:
            checkVariableDeclaration(stmt->as<VariableDecl>());
            break;

        case NodeType::FunctionDeclaration:
            checkFunctionDeclaration(stmt->as<FunctionDecl>());
            break;

        case NodeType::ReturnStatement:
            checkReturnStatement(stmt->as<ReturnStmt>());
            break;

        case NodeType::BreakStatement:
            checkBreakStatement(stmt->as<BreakStmt>());
            break;

        case NodeType::ContinueStatement:
            checkContinueStatement(stmt->as<ContinueStmt>());
            break;

        case NodeType::BlockStatement: {
            auto* block = stmt->as<BlockStmt>();
            std::unordered_set<std::string> blockBindings;
            for (const auto& s : block->body()) {
                if (s->type() == NodeType::VariableDeclaration) {
                    auto* decl = s->as<VariableDecl>();
                    if (decl->kind() != VariableDecl::Kind::Var) {
                        for (const auto& declarator : decl->declarators()) {
                            if (declarator.id && declarator.id->type() == NodeType::Identifier) {
                                const auto& name = static_cast<const IdentifierExpr*>(declarator.id.get())->name();
                                if (blockBindings.count(name)) {
                                    error(s.get(), "Identifier '" + name +
                                          "' has already been declared");
                                }
                                blockBindings.insert(name);
                            }
                        }
                    }
                }
                visitStatement(s.get());
            }
            break;
        }

        case NodeType::IfStatement: {
            auto* ifStmt = stmt->as<IfStmt>();
            visitStatement(ifStmt->consequent());
            if (ifStmt->alternate()) {
                visitStatement(ifStmt->alternate());
            }
            break;
        }

        case NodeType::WhileStatement: {
            bool prevLoop = inLoop_;
            inLoop_ = true;
            visitStatement(stmt->as<WhileStmt>()->body());
            inLoop_ = prevLoop;
            break;
        }

        case NodeType::DoWhileStatement: {
            bool prevLoop = inLoop_;
            inLoop_ = true;
            visitStatement(stmt->as<DoWhileStmt>()->body());
            inLoop_ = prevLoop;
            break;
        }

        case NodeType::ForStatement: {
            bool prevLoop = inLoop_;
            inLoop_ = true;
            visitStatement(stmt->as<ForStmt>()->body());
            inLoop_ = prevLoop;
            break;
        }

        case NodeType::ForOfStatement: {
            bool prevLoop = inLoop_;
            inLoop_ = true;
            visitStatement(stmt->as<ForOfStmt>()->body());
            inLoop_ = prevLoop;
            break;
        }

        case NodeType::SwitchStatement: {
            auto* switchStmt = stmt->as<SwitchStmt>();
            bool prevSwitch = inSwitch_;
            inSwitch_ = true;
            for (const auto& c : switchStmt->cases()) {
                for (const auto& s : c.consequent) {
                    visitStatement(s.get());
                }
            }
            inSwitch_ = prevSwitch;
            break;
        }

        case NodeType::TryStatement: {
            auto* tryStmt = stmt->as<TryStmt>();
            visitStatement(tryStmt->block());
            if (tryStmt->handler()) {
                visitStatement(tryStmt->handler()->body.get());
            }
            if (tryStmt->finalizer()) {
                visitStatement(tryStmt->finalizer());
            }
            break;
        }

        default:
            break;
    }
}

void SyntaxChecker::checkVariableDeclaration(const VariableDecl* decl) {
    if (!decl) return;
    if (decl->kind() == VariableDecl::Kind::Const) {
        for (const auto& declarator : decl->declarators()) {
            if (!declarator.init) {
                error(decl, "Missing initializer in const declaration");
            }
        }
    }
}

void SyntaxChecker::checkFunctionDeclaration(const FunctionDecl* decl) {
    if (!decl) return;

    bool prevFunction = inFunction_;
    bool prevLoop = inLoop_;
    bool prevSwitch = inSwitch_;

    inFunction_ = true;
    inLoop_ = false;
    inSwitch_ = false;

    if (decl->body()) {
        visitStatement(decl->body());
    }

    inFunction_ = prevFunction;
    inLoop_ = prevLoop;
    inSwitch_ = prevSwitch;
}

void SyntaxChecker::checkReturnStatement(const ReturnStmt*) {
    if (!inFunction_) {
        error(nullptr, "Illegal return statement");
    }
}

void SyntaxChecker::checkBreakStatement(const BreakStmt* stmt) {
    if (!inLoop_ && !inSwitch_) {
        error(stmt, "Illegal break statement");
    }
}

void SyntaxChecker::checkContinueStatement(const ContinueStmt* stmt) {
    if (!inLoop_) {
        error(stmt, "Illegal continue statement");
    }
}

void SyntaxChecker::visitExpression(const Expression*) {}
void SyntaxChecker::visitDeclaration(const Declaration*) {}

} // namespace Zepra::Frontend
