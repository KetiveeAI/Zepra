/**
 * @file bytecode_generator.cpp
 * @brief AST to bytecode compiler implementation
 */

#include "zeprascript/bytecode/bytecode_generator.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include <iostream>
#include <iomanip>

namespace Zepra::Bytecode {

// BytecodeChunk implementation
void BytecodeChunk::write(uint8_t byte, uint32_t line) {
    code_.push_back(byte);
    lines_.push_back(line);
}

void BytecodeChunk::write(Opcode op, uint32_t line) {
    write(static_cast<uint8_t>(op), line);
}

void BytecodeChunk::writeShort(uint16_t value, uint32_t line) {
    write(static_cast<uint8_t>((value >> 8) & 0xFF), line);
    write(static_cast<uint8_t>(value & 0xFF), line);
}

size_t BytecodeChunk::addConstant(Runtime::Value value) {
    constants_.push_back(value);
    return constants_.size() - 1;
}

void BytecodeChunk::patchJump(size_t offset) {
    size_t jump = currentOffset() - offset - 2;
    
    if (jump > 0xFFFF) {
        // Jump too long - would need a different strategy
        return;
    }
    
    code_[offset] = (jump >> 8) & 0xFF;
    code_[offset + 1] = jump & 0xFF;
}

uint32_t BytecodeChunk::lineAt(size_t offset) const {
    if (offset < lines_.size()) return lines_[offset];
    return 0;
}

void BytecodeChunk::disassemble(const std::string& name) const {
    std::cout << "== " << name << " ==\n";
    
    for (size_t offset = 0; offset < code_.size();) {
        offset = disassembleInstruction(offset);
    }
}

size_t BytecodeChunk::disassembleInstruction(size_t offset) const {
    std::cout << std::setw(4) << std::setfill('0') << offset << " ";
    
    if (offset > 0 && lineAt(offset) == lineAt(offset - 1)) {
        std::cout << "   | ";
    } else {
        std::cout << std::setw(4) << lineAt(offset) << " ";
    }
    
    Opcode instruction = static_cast<Opcode>(code_[offset]);
    
    switch (instruction) {
        case Opcode::OP_CONSTANT: {
            uint8_t constant = code_[offset + 1];
            std::cout << "OP_CONSTANT     " << (int)constant << " '";
            // Print constant value
            std::cout << "'\n";
            return offset + 2;
        }
        case Opcode::OP_NIL:
            std::cout << "OP_NIL\n";
            return offset + 1;
        case Opcode::OP_TRUE:
            std::cout << "OP_TRUE\n";
            return offset + 1;
        case Opcode::OP_FALSE:
            std::cout << "OP_FALSE\n";
            return offset + 1;
        case Opcode::OP_ADD:
            std::cout << "OP_ADD\n";
            return offset + 1;
        case Opcode::OP_SUBTRACT:
            std::cout << "OP_SUBTRACT\n";
            return offset + 1;
        case Opcode::OP_MULTIPLY:
            std::cout << "OP_MULTIPLY\n";
            return offset + 1;
        case Opcode::OP_DIVIDE:
            std::cout << "OP_DIVIDE\n";
            return offset + 1;
        case Opcode::OP_RETURN:
            std::cout << "OP_RETURN\n";
            return offset + 1;
        default:
            std::cout << "Unknown opcode " << (int)instruction << "\n";
            return offset + 1;
    }
}

// BytecodeGenerator implementation
BytecodeGenerator::BytecodeGenerator() {
}

std::unique_ptr<BytecodeChunk> BytecodeGenerator::compile(const Frontend::Program* program) {
    // Create main script state
    CompilerState mainState;
    mainState.functionName = "<script>";
    current_ = &mainState;
    
    bool lastWasExprStatement = false;
    
    // Compile all statements
    for (size_t i = 0; i < program->body().size(); i++) {
        const auto& stmt = program->body()[i];
        bool isLast = (i == program->body().size() - 1);
        
        // For the last expression statement, don't pop the result
        if (isLast && stmt->type() == Frontend::NodeType::ExpressionStatement) {
            compileExpression(static_cast<const Frontend::ExprStmt*>(stmt.get())->expression());
            lastWasExprStatement = true;
        } else {
            compileStatement(stmt.get());
        }
    }
    
    // End with a return - if last was expression, it's on stack; otherwise push nil
    if (!lastWasExprStatement) {
        emit(Opcode::OP_NIL);
    }
    emit(Opcode::OP_RETURN);
    
    current_ = nullptr;
    
    if (hasErrors()) {
        return nullptr;
    }
    
    return std::make_unique<BytecodeChunk>(std::move(mainState.chunk));
}

BytecodeChunk* BytecodeGenerator::currentChunk() {
    return current_ ? &current_->chunk : nullptr;
}

// Emit helpers
void BytecodeGenerator::emit(uint8_t byte) {
    currentChunk()->write(byte, currentLine_);
}

void BytecodeGenerator::emit(Opcode op) {
    currentChunk()->write(op, currentLine_);
}

void BytecodeGenerator::emitShort(uint16_t value) {
    currentChunk()->writeShort(value, currentLine_);
}

size_t BytecodeGenerator::emitJump(Opcode op) {
    emit(op);
    emit(static_cast<uint8_t>(0xFF));
    emit(static_cast<uint8_t>(0xFF));
    return currentChunk()->currentOffset() - 2;
}

void BytecodeGenerator::patchJump(size_t offset) {
    currentChunk()->patchJump(offset);
}

void BytecodeGenerator::emitLoop(size_t loopStart) {
    emit(Opcode::OP_LOOP);
    
    size_t offset = currentChunk()->currentOffset() - loopStart + 2;
    if (offset > 0xFFFF) {
        error("Loop body too large");
    }
    
    emitShort(static_cast<uint16_t>(offset));
}

size_t BytecodeGenerator::makeConstant(Runtime::Value value) {
    size_t constant = currentChunk()->addConstant(value);
    if (constant > 255) {
        error("Too many constants in one chunk");
        return 0;
    }
    return constant;
}

void BytecodeGenerator::emitConstant(Runtime::Value value) {
    emit(Opcode::OP_CONSTANT);
    emit(static_cast<uint8_t>(makeConstant(value)));
}

// Scope management
void BytecodeGenerator::beginScope() {
    current_->scopeDepth++;
}

void BytecodeGenerator::endScope() {
    current_->scopeDepth--;
    
    // Pop locals that are going out of scope
    while (!current_->locals.empty() &&
           current_->locals.back().depth > current_->scopeDepth) {
        if (current_->locals.back().isCaptured) {
            emit(Opcode::OP_CLOSE_UPVALUE);
        } else {
            emit(Opcode::OP_POP);
        }
        current_->locals.pop_back();
    }
}

void BytecodeGenerator::addLocal(const std::string& name, bool isConst) {
    Local local;
    local.name = name;
    local.depth = current_->scopeDepth;
    local.isCaptured = false;
    local.isConst = isConst;
    current_->locals.push_back(local);
}

int BytecodeGenerator::resolveLocal(const std::string& name) {
    for (int i = static_cast<int>(current_->locals.size()) - 1; i >= 0; i--) {
        const Local& local = current_->locals[i];
        if (local.name == name) {
            return i;
        }
    }
    return -1;
}

int BytecodeGenerator::resolveUpvalue(const std::string& name) {
    if (!current_->enclosing) return -1;
    
    // Check enclosing function's locals
    int local = -1;
    CompilerState* enclosing = current_->enclosing;
    for (int i = static_cast<int>(enclosing->locals.size()) - 1; i >= 0; i--) {
        if (enclosing->locals[i].name == name) {
            enclosing->locals[i].isCaptured = true;
            local = i;
            break;
        }
    }
    
    if (local != -1) {
        Upvalue upvalue;
        upvalue.index = static_cast<uint8_t>(local);
        upvalue.isLocal = true;
        current_->upvalues.push_back(upvalue);
        return static_cast<int>(current_->upvalues.size() - 1);
    }
    
    return -1;
}

void BytecodeGenerator::declareVariable(const std::string& name, bool isConst) {
    if (current_->scopeDepth == 0) {
        // Global variable
        return;
    }
    
    // Check for duplicate declaration
    for (int i = static_cast<int>(current_->locals.size()) - 1; i >= 0; i--) {
        const Local& local = current_->locals[i];
        if (local.depth < current_->scopeDepth) break;
        if (local.name == name) {
            error("Variable '" + name + "' already declared in this scope");
        }
    }
    
    addLocal(name, isConst);
}

void BytecodeGenerator::defineVariable(const std::string& name) {
    if (current_->scopeDepth > 0) {
        // Local variable - already on stack
        return;
    }
    
    // Global variable
    size_t constant = makeConstant(Runtime::Value::string(new Runtime::String(name)));
    emit(Opcode::OP_DEFINE_GLOBAL);
    emit(static_cast<uint8_t>(constant));
}

// Error handling
void BytecodeGenerator::error(const std::string& message) {
    errors_.push_back(message);
}

void BytecodeGenerator::error(const Frontend::ASTNode* node, const std::string& message) {
    std::string err = message;
    if (node) {
        err = "Line " + std::to_string(node->location().line) + ": " + message;
    }
    errors_.push_back(err);
}

// Statement compilation
void BytecodeGenerator::compileStatement(const Frontend::Statement* stmt) {
    if (!stmt) return;
    
    currentLine_ = stmt->location().line;
    
    switch (stmt->type()) {
        case Frontend::NodeType::ExpressionStatement:
            compileExpressionStatement(static_cast<const Frontend::ExprStmt*>(stmt));
            break;
        case Frontend::NodeType::BlockStatement:
            compileBlockStatement(static_cast<const Frontend::BlockStmt*>(stmt));
            break;
        case Frontend::NodeType::VariableDeclaration:
            compileVariableDeclaration(static_cast<const Frontend::VariableDecl*>(stmt));
            break;
        case Frontend::NodeType::FunctionDeclaration:
            compileFunctionDeclaration(static_cast<const Frontend::FunctionDecl*>(stmt));
            break;
        case Frontend::NodeType::IfStatement:
            compileIfStatement(static_cast<const Frontend::IfStmt*>(stmt));
            break;
        case Frontend::NodeType::WhileStatement:
            compileWhileStatement(static_cast<const Frontend::WhileStmt*>(stmt));
            break;
        case Frontend::NodeType::DoWhileStatement:
            compileDoWhileStatement(static_cast<const Frontend::DoWhileStmt*>(stmt));
            break;
        case Frontend::NodeType::ForStatement:
            compileForStatement(static_cast<const Frontend::ForStmt*>(stmt));
            break;
        case Frontend::NodeType::ReturnStatement:
            compileReturnStatement(static_cast<const Frontend::ReturnStmt*>(stmt));
            break;
        case Frontend::NodeType::BreakStatement:
            compileBreakStatement(static_cast<const Frontend::BreakStmt*>(stmt));
            break;
        case Frontend::NodeType::ContinueStatement:
            compileContinueStatement(static_cast<const Frontend::ContinueStmt*>(stmt));
            break;
        case Frontend::NodeType::ThrowStatement:
            compileThrowStatement(static_cast<const Frontend::ThrowStmt*>(stmt));
            break;
        case Frontend::NodeType::TryStatement:
            compileTryStatement(static_cast<const Frontend::TryStmt*>(stmt));
            break;
        case Frontend::NodeType::ImportDeclaration:
            compileImportDeclaration(static_cast<const Frontend::ImportDecl*>(stmt));
            break;
        case Frontend::NodeType::ExportDeclaration:
            compileExportDeclaration(static_cast<const Frontend::ExportDecl*>(stmt));
            break;
        default:
            error("Unknown statement type");
            break;
    }
}

void BytecodeGenerator::compileBlockStatement(const Frontend::BlockStmt* stmt) {
    beginScope();
    for (const auto& s : stmt->body()) {
        compileStatement(s.get());
    }
    endScope();
}

void BytecodeGenerator::compileExpressionStatement(const Frontend::ExprStmt* stmt) {
    compileExpression(stmt->expression());
    emit(Opcode::OP_POP);
}

void BytecodeGenerator::compileVariableDeclaration(const Frontend::VariableDecl* decl) {
    bool isConstVar = decl->kind() == Frontend::VariableDecl::Kind::Const;
    
    for (const auto& declarator : decl->declarators()) {
        const auto* id = static_cast<const Frontend::IdentifierExpr*>(declarator.id.get());
        std::string varName = id->name();
        
        declareVariable(varName, isConstVar);
        
        if (declarator.init) {
            compileExpression(declarator.init.get());
        } else {
            emit(Opcode::OP_NIL);
        }
        
        defineVariable(varName);
    }
}

void BytecodeGenerator::compileFunctionDeclaration(const Frontend::FunctionDecl* decl) {
    declareVariable(decl->name(), false);
    
    // Compile function body into new chunk
    CompilerState fnState;
    fnState.enclosing = current_;
    fnState.functionName = decl->name();
    current_ = &fnState;
    
    // Add parameters as locals at depth 0 (function's base scope)
    // They are already on the stack when the function is called
    for (const auto& param : decl->params()) {
        const auto* id = static_cast<const Frontend::IdentifierExpr*>(param.pattern.get());
        // Add local at depth 0 so they don't get popped by endScope
        Local local;
        local.name = id->name();
        local.depth = 0;  // At function's base level
        local.isCaptured = false;
        local.isConst = false;
        current_->locals.push_back(local);
    }
    
    // Compile body (each block statement inside will handle its own scope)
    for (const auto& stmt : decl->body()->body()) {
        compileStatement(stmt.get());
    }
    
    // Default return
    emit(Opcode::OP_NIL);
    emit(Opcode::OP_RETURN);
    
    // Create the function's bytecode chunk
    auto functionChunk = std::make_unique<BytecodeChunk>(std::move(fnState.chunk));
    
    current_ = fnState.enclosing;
    
    // Create a Function object with the compiled bytecode
    auto* function = new Runtime::Function(decl, nullptr);
    function->setBytecodeChunk(functionChunk.get());
    
    // Store the chunk (transferred ownership to function)
    // Note: In a real implementation, this would be part of the Function object
    static std::vector<std::unique_ptr<BytecodeChunk>> compiledChunks;
    compiledChunks.push_back(std::move(functionChunk));
    
    // Add function as constant and emit OP_CLOSURE
    size_t funcIndex = makeConstant(Runtime::Value::object(function));
    emit(Opcode::OP_CLOSURE);
    emit(static_cast<uint8_t>(funcIndex));
    
    defineVariable(decl->name());
}

void BytecodeGenerator::compileIfStatement(const Frontend::IfStmt* stmt) {
    compileExpression(stmt->test());
    
    size_t thenJump = emitJump(Opcode::OP_JUMP_IF_FALSE);
    emit(Opcode::OP_POP); // Pop condition
    
    compileStatement(stmt->consequent());
    
    size_t elseJump = emitJump(Opcode::OP_JUMP);
    
    patchJump(thenJump);
    emit(Opcode::OP_POP); // Pop condition
    
    if (stmt->alternate()) {
        compileStatement(stmt->alternate());
    }
    
    patchJump(elseJump);
}

void BytecodeGenerator::compileWhileStatement(const Frontend::WhileStmt* stmt) {
    size_t loopStart = currentChunk()->currentOffset();
    
    compileExpression(stmt->test());
    
    size_t exitJump = emitJump(Opcode::OP_JUMP_IF_FALSE);
    emit(Opcode::OP_POP);
    
    // Track loop for break/continue
    breakJumps_.push_back({});
    continueTargets_.push_back(loopStart);
    
    compileStatement(stmt->body());
    
    emitLoop(loopStart);
    
    patchJump(exitJump);
    emit(Opcode::OP_POP);
    
    // Patch breaks
    for (size_t breakJump : breakJumps_.back()) {
        patchJump(breakJump);
    }
    breakJumps_.pop_back();
    continueTargets_.pop_back();
}

void BytecodeGenerator::compileDoWhileStatement(const Frontend::DoWhileStmt* stmt) {
    size_t loopStart = currentChunk()->currentOffset();
    
    breakJumps_.push_back({});
    continueTargets_.push_back(loopStart);
    
    compileStatement(stmt->body());
    
    compileExpression(stmt->test());
    size_t exitJump = emitJump(Opcode::OP_JUMP_IF_FALSE);
    emit(Opcode::OP_POP);
    
    emitLoop(loopStart);
    
    patchJump(exitJump);
    emit(Opcode::OP_POP);
    
    for (size_t breakJump : breakJumps_.back()) {
        patchJump(breakJump);
    }
    breakJumps_.pop_back();
    continueTargets_.pop_back();
}

void BytecodeGenerator::compileForStatement(const Frontend::ForStmt* stmt) {
    beginScope();
    
    // Initializer
    if (stmt->init()) {
        if (stmt->init()->type() == Frontend::NodeType::VariableDeclaration) {
            compileStatement(static_cast<const Frontend::Statement*>(stmt->init()));
        } else {
            compileExpression(static_cast<const Frontend::Expression*>(stmt->init()));
            emit(Opcode::OP_POP);
        }
    }
    
    size_t loopStart = currentChunk()->currentOffset();
    
    // Condition
    size_t exitJump = 0;
    if (stmt->test()) {
        compileExpression(stmt->test());
        exitJump = emitJump(Opcode::OP_JUMP_IF_FALSE);
        emit(Opcode::OP_POP);
    }
    
    breakJumps_.push_back({});
    size_t updateStart = loopStart;
    
    // If there's an update, jump over it first time
    if (stmt->update()) {
        size_t bodyJump = emitJump(Opcode::OP_JUMP);
        updateStart = currentChunk()->currentOffset();
        compileExpression(stmt->update());
        emit(Opcode::OP_POP);
        emitLoop(loopStart);
        patchJump(bodyJump);
    }
    
    continueTargets_.push_back(updateStart);
    
    // Body
    compileStatement(stmt->body());
    
    // Loop back
    emitLoop(updateStart);
    
    if (stmt->test()) {
        patchJump(exitJump);
        emit(Opcode::OP_POP);
    }
    
    for (size_t breakJump : breakJumps_.back()) {
        patchJump(breakJump);
    }
    breakJumps_.pop_back();
    continueTargets_.pop_back();
    
    endScope();
}

void BytecodeGenerator::compileReturnStatement(const Frontend::ReturnStmt* stmt) {
    if (stmt->argument()) {
        compileExpression(stmt->argument());
    } else {
        emit(Opcode::OP_NIL);
    }
    emit(Opcode::OP_RETURN);
}

void BytecodeGenerator::compileBreakStatement(const Frontend::BreakStmt*) {
    if (breakJumps_.empty()) {
        error("'break' outside of loop");
        return;
    }
    breakJumps_.back().push_back(emitJump(Opcode::OP_JUMP));
}

void BytecodeGenerator::compileContinueStatement(const Frontend::ContinueStmt*) {
    if (continueTargets_.empty()) {
        error("'continue' outside of loop");
        return;
    }
    emitLoop(continueTargets_.back());
}

void BytecodeGenerator::compileThrowStatement(const Frontend::ThrowStmt* stmt) {
    compileExpression(stmt->argument());
    emit(Opcode::OP_THROW);
}

void BytecodeGenerator::compileTryStatement(const Frontend::TryStmt* stmt) {
    // Emit OP_TRY_BEGIN + placeholder offset (uses same pattern as emitJump)
    emit(Opcode::OP_TRY_BEGIN);
    emit(static_cast<uint8_t>(0xFF));
    emit(static_cast<uint8_t>(0xFF));
    size_t catchOffsetSlot = currentChunk()->currentOffset() - 2;
    
    // Compile try block
    compileBlockStatement(stmt->block());
    
    // End of try block (no exception) - pop handler and skip catch
    emit(Opcode::OP_TRY_END);
    size_t skipCatchJump = emitJump(Opcode::OP_JUMP);
    
    // Patch the catch offset to point here
    patchJump(catchOffsetSlot);
    
    // Catch block
    if (stmt->handler()) {
        emit(Opcode::OP_CATCH);
        if (!stmt->handler()->param.empty()) {
            beginScope();
            declareVariable(stmt->handler()->param, false);
            // The exception value is on the stack from OP_CATCH
            defineVariable(stmt->handler()->param);
        }
        compileBlockStatement(stmt->handler()->body.get());
        if (!stmt->handler()->param.empty()) {
            endScope();
        }
    }
    
    // Patch jump to skip catch block
    patchJump(skipCatchJump);
    
    // Finally block (if present)
    if (stmt->finalizer()) {
        emit(Opcode::OP_FINALLY);
        compileBlockStatement(stmt->finalizer());
    }
}

// Expression compilation
void BytecodeGenerator::compileExpression(const Frontend::Expression* expr) {
    if (!expr) {
        emit(Opcode::OP_NIL);
        return;
    }
    
    currentLine_ = expr->location().line;
    
    switch (expr->type()) {
        case Frontend::NodeType::Literal:
            compileLiteral(static_cast<const Frontend::LiteralExpr*>(expr));
            break;
        case Frontend::NodeType::Identifier:
            compileIdentifier(static_cast<const Frontend::IdentifierExpr*>(expr));
            break;
        case Frontend::NodeType::BinaryExpression:
            compileBinaryExpression(static_cast<const Frontend::BinaryExpr*>(expr));
            break;
        case Frontend::NodeType::UnaryExpression:
            compileUnaryExpression(static_cast<const Frontend::UnaryExpr*>(expr));
            break;
        case Frontend::NodeType::LogicalExpression:
            compileLogicalExpression(static_cast<const Frontend::LogicalExpr*>(expr));
            break;
        case Frontend::NodeType::AssignmentExpression:
            compileAssignmentExpression(static_cast<const Frontend::AssignmentExpr*>(expr));
            break;
        case Frontend::NodeType::CallExpression:
            compileCallExpression(static_cast<const Frontend::CallExpr*>(expr));
            break;
        case Frontend::NodeType::MemberExpression:
            compileMemberExpression(static_cast<const Frontend::MemberExpr*>(expr));
            break;
        case Frontend::NodeType::ArrayExpression:
            compileArrayExpression(static_cast<const Frontend::ArrayExpr*>(expr));
            break;
        case Frontend::NodeType::ObjectExpression:
            compileObjectExpression(static_cast<const Frontend::ObjectExpr*>(expr));
            break;
        case Frontend::NodeType::ConditionalExpression:
            compileConditionalExpression(static_cast<const Frontend::ConditionalExpr*>(expr));
            break;
        case Frontend::NodeType::ThisExpression:
            compileThisExpression(static_cast<const Frontend::ThisExpr*>(expr));
            break;
        default:
            error("Unknown expression type");
            break;
    }
}

void BytecodeGenerator::compileLiteral(const Frontend::LiteralExpr* expr) {
    const auto& value = expr->value();
    
    if (std::holds_alternative<double>(value)) {
        emitConstant(Runtime::Value::number(std::get<double>(value)));
    } else if (std::holds_alternative<std::string>(value)) {
        emitConstant(Runtime::Value::string(new Runtime::String(std::get<std::string>(value))));
    } else if (std::holds_alternative<bool>(value)) {
        emit(std::get<bool>(value) ? Opcode::OP_TRUE : Opcode::OP_FALSE);
    } else if (std::holds_alternative<std::nullptr_t>(value)) {
        emit(Opcode::OP_NIL);
    } else {
        emit(Opcode::OP_NIL);
    }
}

void BytecodeGenerator::compileIdentifier(const Frontend::IdentifierExpr* expr) {
    const std::string& name = expr->name();
    
    // Check for local
    int local = resolveLocal(name);
    if (local != -1) {
        emit(Opcode::OP_GET_LOCAL);
        emit(static_cast<uint8_t>(local));
        return;
    }
    
    // Check for upvalue
    int upvalue = resolveUpvalue(name);
    if (upvalue != -1) {
        emit(Opcode::OP_GET_UPVALUE);
        emit(static_cast<uint8_t>(upvalue));
        return;
    }
    
    // Global
    size_t constant = makeConstant(Runtime::Value::string(new Runtime::String(name)));
    emit(Opcode::OP_GET_GLOBAL);
    emit(static_cast<uint8_t>(constant));
}

void BytecodeGenerator::compileBinaryExpression(const Frontend::BinaryExpr* expr) {
    compileExpression(expr->left());
    compileExpression(expr->right());
    
    switch (expr->op()) {
        case Frontend::TokenType::Plus: emit(Opcode::OP_ADD); break;
        case Frontend::TokenType::Minus: emit(Opcode::OP_SUBTRACT); break;
        case Frontend::TokenType::Star: emit(Opcode::OP_MULTIPLY); break;
        case Frontend::TokenType::Slash: emit(Opcode::OP_DIVIDE); break;
        case Frontend::TokenType::Percent: emit(Opcode::OP_MODULO); break;
        case Frontend::TokenType::StarStar: emit(Opcode::OP_POWER); break;
        case Frontend::TokenType::Equal: emit(Opcode::OP_EQUAL); break;
        case Frontend::TokenType::StrictEqual: emit(Opcode::OP_STRICT_EQUAL); break;
        case Frontend::TokenType::NotEqual: emit(Opcode::OP_NOT_EQUAL); break;
        case Frontend::TokenType::StrictNotEqual: emit(Opcode::OP_STRICT_NOT_EQUAL); break;
        case Frontend::TokenType::Less: emit(Opcode::OP_LESS); break;
        case Frontend::TokenType::LessEqual: emit(Opcode::OP_LESS_EQUAL); break;
        case Frontend::TokenType::Greater: emit(Opcode::OP_GREATER); break;
        case Frontend::TokenType::GreaterEqual: emit(Opcode::OP_GREATER_EQUAL); break;
        case Frontend::TokenType::Ampersand: emit(Opcode::OP_BITWISE_AND); break;
        case Frontend::TokenType::Pipe: emit(Opcode::OP_BITWISE_OR); break;
        case Frontend::TokenType::Caret: emit(Opcode::OP_BITWISE_XOR); break;
        case Frontend::TokenType::LeftShift: emit(Opcode::OP_LEFT_SHIFT); break;
        case Frontend::TokenType::RightShift: emit(Opcode::OP_RIGHT_SHIFT); break;
        case Frontend::TokenType::UnsignedRightShift: emit(Opcode::OP_UNSIGNED_RIGHT_SHIFT); break;
        default:
            error("Unknown binary operator");
            break;
    }
}

void BytecodeGenerator::compileUnaryExpression(const Frontend::UnaryExpr* expr) {
    compileExpression(expr->argument());
    
    switch (expr->op()) {
        case Frontend::TokenType::Minus: emit(Opcode::OP_NEGATE); break;
        case Frontend::TokenType::Not: emit(Opcode::OP_NOT); break;
        case Frontend::TokenType::Tilde: emit(Opcode::OP_BITWISE_NOT); break;
        case Frontend::TokenType::Typeof: emit(Opcode::OP_TYPEOF); break;
        default:
            error("Unknown unary operator");
            break;
    }
}

void BytecodeGenerator::compileLogicalExpression(const Frontend::LogicalExpr* expr) {
    compileExpression(expr->left());
    
    if (expr->op() == Frontend::TokenType::And) {
        size_t endJump = emitJump(Opcode::OP_JUMP_IF_FALSE);
        emit(Opcode::OP_POP);
        compileExpression(expr->right());
        patchJump(endJump);
    } else if (expr->op() == Frontend::TokenType::Or) {
        size_t elseJump = emitJump(Opcode::OP_JUMP_IF_FALSE);
        size_t endJump = emitJump(Opcode::OP_JUMP);
        patchJump(elseJump);
        emit(Opcode::OP_POP);
        compileExpression(expr->right());
        patchJump(endJump);
    } else if (expr->op() == Frontend::TokenType::QuestionQuestion) {
        emit(Opcode::OP_DUP);
        size_t elseJump = emitJump(Opcode::OP_JUMP_IF_NIL);
        size_t endJump = emitJump(Opcode::OP_JUMP);
        patchJump(elseJump);
        emit(Opcode::OP_POP);
        compileExpression(expr->right());
        patchJump(endJump);
    }
}

void BytecodeGenerator::compileConditionalExpression(const Frontend::ConditionalExpr* expr) {
    compileExpression(expr->test());
    
    size_t thenJump = emitJump(Opcode::OP_JUMP_IF_FALSE);
    emit(Opcode::OP_POP);
    compileExpression(expr->consequent());
    
    size_t elseJump = emitJump(Opcode::OP_JUMP);
    patchJump(thenJump);
    emit(Opcode::OP_POP);
    compileExpression(expr->alternate());
    patchJump(elseJump);
}

void BytecodeGenerator::compileAssignmentExpression(const Frontend::AssignmentExpr* expr) {
    // For compound assignment, get the current value first
    if (expr->op() != Frontend::TokenType::Assign) {
        compileExpression(expr->left());
    }
    
    compileExpression(expr->right());
    
    // Apply operator for compound assignment
    switch (expr->op()) {
        case Frontend::TokenType::PlusAssign: emit(Opcode::OP_ADD); break;
        case Frontend::TokenType::MinusAssign: emit(Opcode::OP_SUBTRACT); break;
        case Frontend::TokenType::StarAssign: emit(Opcode::OP_MULTIPLY); break;
        case Frontend::TokenType::SlashAssign: emit(Opcode::OP_DIVIDE); break;
        case Frontend::TokenType::PercentAssign: emit(Opcode::OP_MODULO); break;
        case Frontend::TokenType::Assign: break; // No operation needed
        default: break;
    }
    
    // Store the result
    if (expr->left()->type() == Frontend::NodeType::Identifier) {
        const auto* id = static_cast<const Frontend::IdentifierExpr*>(expr->left());
        const std::string& identName = id->name();
        
        int local = resolveLocal(identName);
        if (local != -1) {
            emit(Opcode::OP_SET_LOCAL);
            emit(static_cast<uint8_t>(local));
        } else {
            size_t constant = makeConstant(Runtime::Value::string(new Runtime::String(identName)));
            emit(Opcode::OP_SET_GLOBAL);
            emit(static_cast<uint8_t>(constant));
        }
    } else if (expr->left()->type() == Frontend::NodeType::MemberExpression) {
        // Member assignment
        const auto* member = static_cast<const Frontend::MemberExpr*>(expr->left());
        compileExpression(member->object());
        if (member->isComputed()) {
            compileExpression(member->property());
            emit(Opcode::OP_SET_ELEMENT);
        } else {
            const auto* prop = static_cast<const Frontend::IdentifierExpr*>(member->property());
            size_t constant = makeConstant(Runtime::Value::string(new Runtime::String(prop->name())));
            emit(Opcode::OP_SET_PROPERTY);
            emit(static_cast<uint8_t>(constant));
        }
    }
}

void BytecodeGenerator::compileCallExpression(const Frontend::CallExpr* expr) {
    const Frontend::Expression* callee = expr->callee();
    
    // Check if this is a method call (callee is a member expression)
    if (callee->type() == Frontend::NodeType::MemberExpression) {
        const auto* memberExpr = static_cast<const Frontend::MemberExpr*>(callee);
        
        // Compile the receiver object
        compileExpression(memberExpr->object());
        
        // Compile arguments
        for (const auto& arg : expr->arguments()) {
            compileExpression(arg.get());
        }
        
        // Emit OP_CALL_METHOD with method name and arg count
        if (!memberExpr->isComputed()) {
            const auto* prop = static_cast<const Frontend::IdentifierExpr*>(memberExpr->property());
            size_t nameConstant = makeConstant(Runtime::Value::string(new Runtime::String(prop->name())));
            emit(Opcode::OP_CALL_METHOD);
            emit(static_cast<uint8_t>(nameConstant));
            emit(static_cast<uint8_t>(expr->arguments().size()));
        } else {
            // Computed property - fall back to regular call
            compileExpression(memberExpr->property());
            emit(Opcode::OP_GET_ELEMENT);
            emit(Opcode::OP_CALL);
            emit(static_cast<uint8_t>(expr->arguments().size()));
        }
    } else {
        // Regular function call
        compileExpression(callee);
        
        for (const auto& arg : expr->arguments()) {
            compileExpression(arg.get());
        }
        
        emit(Opcode::OP_CALL);
        emit(static_cast<uint8_t>(expr->arguments().size()));
    }
}

void BytecodeGenerator::compileMemberExpression(const Frontend::MemberExpr* expr) {
    compileExpression(expr->object());
    
    if (expr->isComputed()) {
        compileExpression(expr->property());
        emit(Opcode::OP_GET_ELEMENT);
    } else {
        const auto* prop = static_cast<const Frontend::IdentifierExpr*>(expr->property());
        size_t constant = makeConstant(Runtime::Value::string(new Runtime::String(prop->name())));
        emit(Opcode::OP_GET_PROPERTY);
        emit(static_cast<uint8_t>(constant));
    }
}

void BytecodeGenerator::compileNewExpression(const Frontend::NewExpr* expr) {
    compileExpression(expr->callee());
    
    for (const auto& arg : expr->arguments()) {
        compileExpression(arg.get());
    }
    
    emit(Opcode::OP_NEW);
    emit(static_cast<uint8_t>(expr->arguments().size()));
}

void BytecodeGenerator::compileArrayExpression(const Frontend::ArrayExpr* expr) {
    for (const auto& element : expr->elements()) {
        if (element) {
            compileExpression(element.get());
        } else {
            emit(Opcode::OP_NIL);
        }
    }
    
    emit(Opcode::OP_CREATE_ARRAY);
    emit(static_cast<uint8_t>(expr->elements().size()));
}

void BytecodeGenerator::compileObjectExpression(const Frontend::ObjectExpr* expr) {
    emit(Opcode::OP_CREATE_OBJECT);
    
    for (const auto& prop : expr->properties()) {
        if (prop.computed) {
            compileExpression(prop.key.get());
        } else if (prop.key->type() == Frontend::NodeType::Identifier) {
            const auto* id = static_cast<const Frontend::IdentifierExpr*>(prop.key.get());
            emitConstant(Runtime::Value::string(new Runtime::String(id->name())));
        }
        compileExpression(prop.value.get());
        emit(Opcode::OP_INIT_PROPERTY);
    }
}

void BytecodeGenerator::compileThisExpression(const Frontend::ThisExpr*) {
    // TODO: Handle 'this' properly
    emit(Opcode::OP_NIL);
}

void BytecodeGenerator::compileFunctionExpression(const Frontend::FunctionExpr*) {
    // TODO: Implement function expression compilation
    emit(Opcode::OP_NIL);
}

void BytecodeGenerator::compileArrowFunction(const Frontend::ArrowFunctionExpr*) {
    // TODO: Implement arrow function compilation
    emit(Opcode::OP_NIL);
}

void BytecodeGenerator::compileUpdateExpression(const Frontend::UpdateExpr*) {
    // TODO: Implement update expression (++, --)
    emit(Opcode::OP_NIL);
}

void BytecodeGenerator::compileImportDeclaration(const Frontend::ImportDecl* decl) {
    // Emit OP_IMPORT with module path constant
    size_t pathConstant = makeConstant(
        Runtime::Value::string(new Runtime::String(decl->source())));
    emit(Opcode::OP_IMPORT);
    emit(static_cast<uint8_t>(pathConstant));
    
    // The module exports object is now on the stack
    // For each specifier, bind the imported name to a local variable
    for (const auto& spec : decl->specifiers()) {
        // Duplicate the exports object
        emit(Opcode::OP_DUP);
        
        // Get the imported property
        size_t nameConstant = makeConstant(
            Runtime::Value::string(new Runtime::String(spec.imported)));
        emit(Opcode::OP_GET_PROPERTY);
        emit(static_cast<uint8_t>(nameConstant));
        
        // Define as local variable
        declareVariable(spec.local, false);
        defineVariable(spec.local);
    }
    
    // Pop the exports object when done
    emit(Opcode::OP_POP);
}

void BytecodeGenerator::compileExportDeclaration(const Frontend::ExportDecl* decl) {
    if (decl->hasDeclaration()) {
        // Compile the declaration (function, variable, etc.)
        compileStatement(decl->declaration());
        
        // If it's a named export, register the exported name
        // For now, exports are tracked but not fully implemented
        // This requires a module context to store exports
    }
    
    // Named exports: export { foo, bar }
    for (const auto& spec : decl->specifiers()) {
        // Get the local variable value
        int slot = resolveLocal(spec.local);
        if (slot != -1) {
            emit(Opcode::OP_GET_LOCAL);
            emit(static_cast<uint8_t>(slot));
        } else {
            size_t constant = makeConstant(
                Runtime::Value::string(new Runtime::String(spec.local)));
            emit(Opcode::OP_GET_GLOBAL);
            emit(static_cast<uint8_t>(constant));
        }
        
        // Register as export (requires module context)
        size_t exportNameConstant = makeConstant(
            Runtime::Value::string(new Runtime::String(spec.exported)));
        emit(Opcode::OP_EXPORT);
        emit(static_cast<uint8_t>(exportNameConstant));
    }
}

} // namespace Zepra::Bytecode
