#pragma once

/**
 * @file bytecode_generator.hpp
 * @brief AST to bytecode compiler
 */

#include "../config.hpp"
#include "../frontend/ast.hpp"
#include "opcode.hpp"
#include "runtime/objects/value.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace Zepra::Bytecode {

/**
 * @brief A chunk of bytecode with constants pool
 */
class BytecodeChunk {
public:
    BytecodeChunk() = default;
    
    // Bytecode manipulation
    void write(uint8_t byte, uint32_t line = 0);
    void write(Opcode op, uint32_t line = 0);
    void writeShort(uint16_t value, uint32_t line = 0);
    
    size_t addConstant(Runtime::Value value);
    
    // Patching for jumps
    size_t currentOffset() const { return code_.size(); }
    void patchJump(size_t offset);
    
    // Access
    const std::vector<uint8_t>& code() const { return code_; }
    const std::vector<Runtime::Value>& constants() const { return constants_; }
    const std::vector<uint32_t>& lines() const { return lines_; }
    
    uint8_t at(size_t offset) const { return code_[offset]; }
    Runtime::Value constant(size_t index) const { return constants_[index]; }
    uint32_t lineAt(size_t offset) const;
    
    // Debug
    void disassemble(const std::string& name) const;
    size_t disassembleInstruction(size_t offset) const;
    
private:
    std::vector<uint8_t> code_;
    std::vector<Runtime::Value> constants_;
    std::vector<uint32_t> lines_;
};

/**
 * @brief Local variable info
 */
struct Local {
    std::string name;
    int depth = 0;       // Scope depth
    bool isCaptured = false;
    bool isConst = false;
};

/**
 * @brief Upvalue info (captured variable)
 */
struct Upvalue {
    uint8_t index;
    bool isLocal;
};

/**
 * @brief Compiler state for a function
 */
struct CompilerState {
    CompilerState* enclosing = nullptr;
    BytecodeChunk chunk;
    std::vector<Local> locals;
    std::vector<Upvalue> upvalues;
    int scopeDepth = 0;
    std::string functionName;
    bool isMethod = false;
};

/**
 * @brief Compiles AST to bytecode
 */
class BytecodeGenerator {
public:
    BytecodeGenerator();
    
    /**
     * @brief Compile a program to bytecode
     */
    std::unique_ptr<BytecodeChunk> compile(const Frontend::Program* program);
    
    /**
     * @brief Get compilation errors
     */
    const std::vector<std::string>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }
    
private:
    // Statement compilation
    void compileStatement(const Frontend::Statement* stmt);
    void compileBlockStatement(const Frontend::BlockStmt* stmt);
    void compileExpressionStatement(const Frontend::ExprStmt* stmt);
    void compileVariableDeclaration(const Frontend::VariableDecl* decl);
    void compileFunctionDeclaration(const Frontend::FunctionDecl* decl);
    void compileIfStatement(const Frontend::IfStmt* stmt);
    void compileWhileStatement(const Frontend::WhileStmt* stmt);
    void compileDoWhileStatement(const Frontend::DoWhileStmt* stmt);
    void compileForStatement(const Frontend::ForStmt* stmt);
    void compileReturnStatement(const Frontend::ReturnStmt* stmt);
    void compileBreakStatement(const Frontend::BreakStmt* stmt);
    void compileContinueStatement(const Frontend::ContinueStmt* stmt);
    void compileThrowStatement(const Frontend::ThrowStmt* stmt);
    void compileTryStatement(const Frontend::TryStmt* stmt);
    void compileImportDeclaration(const Frontend::ImportDecl* decl);
    void compileExportDeclaration(const Frontend::ExportDecl* decl);
    void compileClassDeclaration(const Frontend::ClassDecl* decl);
    void compileForOfStatement(const Frontend::ForOfStmt* stmt);
    void compileForInStatement(const Frontend::ForInStmt* stmt);
    void compileTemplateLiteral(const Frontend::TemplateLiteralExpr* expr);
    void compileSwitchStatement(const Frontend::SwitchStmt* stmt);
    
    // Expression compilation
    void compileExpression(const Frontend::Expression* expr);
    void compileLiteral(const Frontend::LiteralExpr* expr);
    void compileIdentifier(const Frontend::IdentifierExpr* expr);
    void compileArrayExpression(const Frontend::ArrayExpr* expr);
    void compileObjectExpression(const Frontend::ObjectExpr* expr);
    void compileBinaryExpression(const Frontend::BinaryExpr* expr);
    void compileUnaryExpression(const Frontend::UnaryExpr* expr);
    void compileUpdateExpression(const Frontend::UpdateExpr* expr);
    void compileLogicalExpression(const Frontend::LogicalExpr* expr);
    void compileConditionalExpression(const Frontend::ConditionalExpr* expr);
    void compileAssignmentExpression(const Frontend::AssignmentExpr* expr);
    void compileMemberExpression(const Frontend::MemberExpr* expr);
    void compileCallExpression(const Frontend::CallExpr* expr);
    void compileNewExpression(const Frontend::NewExpr* expr);
    void compileFunctionExpression(const Frontend::FunctionExpr* expr);
    void compileArrowFunction(const Frontend::ArrowFunctionExpr* expr);
    void compileThisExpression(const Frontend::ThisExpr* expr);
    void compileAwaitExpression(const Frontend::AwaitExpr* expr);
    void compileYieldExpression(const Frontend::YieldExpr* expr);
    void compileSpreadElement(const Frontend::SpreadExpr* expr);
    void compileRestElement(const Frontend::RestElem* expr);
    
    // Helpers
    void emit(uint8_t byte);
    void emit(Opcode op);
    void emitShort(uint16_t value);
    size_t emitJump(Opcode op);
    void patchJump(size_t offset);
    void emitLoop(size_t loopStart);
    
    size_t makeConstant(Runtime::Value value);
    void emitConstant(Runtime::Value value);
    
    // Scope management
    void beginScope();
    void endScope();
    int resolveLocal(const std::string& name);
    int resolveUpvalue(const std::string& name);
    void addLocal(const std::string& name, bool isConst = false);
    void declareVariable(const std::string& name, bool isConst = false);
    void defineVariable(const std::string& name);
    
    // Error handling
    void error(const std::string& message);
    void error(const Frontend::ASTNode* node, const std::string& message);
    
    // Current state
    BytecodeChunk* currentChunk();
    
    CompilerState* current_ = nullptr;
    std::vector<std::string> errors_;
    uint32_t currentLine_ = 0;
    
    // Jump targets for break/continue
    std::vector<std::vector<size_t>> breakJumps_;
    std::vector<size_t> continueTargets_;
};

} // namespace Zepra::Bytecode
