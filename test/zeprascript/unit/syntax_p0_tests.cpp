/**
 * @file syntax_p0_tests.cpp
 * @brief Tests for P0 syntax features: optional chaining, nullish coalescing,
 *        destructuring, for-await-of, tagged templates, compound assignments
 *
 * Validates that the ZepraScript parser correctly parses these syntax patterns
 * and produces the expected AST nodes.
 */

#include <gtest/gtest.h>
#include "frontend/parser.hpp"
#include "frontend/ast.hpp"

using namespace Zepra::Frontend;

namespace {

// Helper: parse code and return the program AST
std::unique_ptr<Program> parseCode(const std::string& code) {
    return parse(code, "<test>");
}

// Helper: parse and get first statement
Statement* firstStmt(Program* prog) {
    if (!prog || prog->body().empty()) return nullptr;
    return prog->body()[0].get();
}

// Helper: get expression from expression statement
Expression* exprFromStmt(Statement* stmt) {
    if (!stmt) return nullptr;
    auto* es = stmt->as<ExprStmt>();
    return es ? es->expression() : nullptr;
}

} // anonymous namespace

// =========================================================================
// Optional Chaining Tests
// =========================================================================

TEST(OptionalChaining, PropertyAccess) {
    auto prog = parseCode("obj?.prop;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    auto* member = expr->as<MemberExpr>();
    ASSERT_NE(member, nullptr);
    EXPECT_TRUE(member->isOptional());
    EXPECT_FALSE(member->isComputed());
}

TEST(OptionalChaining, ComputedAccess) {
    auto prog = parseCode("obj?.[0];");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    auto* member = expr->as<MemberExpr>();
    ASSERT_NE(member, nullptr);
    EXPECT_TRUE(member->isOptional());
    EXPECT_TRUE(member->isComputed());
}

TEST(OptionalChaining, MethodCall) {
    auto prog = parseCode("obj?.method();");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    auto* call = expr->as<CallExpr>();
    ASSERT_NE(call, nullptr);
    // The optional flag is on the member access (obj?.method), not the call
    auto* callee = call->callee()->as<MemberExpr>();
    ASSERT_NE(callee, nullptr);
    EXPECT_TRUE(callee->isOptional());
}

TEST(OptionalChaining, ChainedAccess) {
    auto prog = parseCode("a?.b?.c;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    auto* outer = expr->as<MemberExpr>();
    ASSERT_NE(outer, nullptr);
    EXPECT_TRUE(outer->isOptional());
    auto* inner = outer->object()->as<MemberExpr>();
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(inner->isOptional());
}

TEST(OptionalChaining, MixedWithRegularAccess) {
    auto prog = parseCode("a.b?.c;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    auto* outer = expr->as<MemberExpr>();
    ASSERT_NE(outer, nullptr);
    EXPECT_TRUE(outer->isOptional());
    auto* inner = outer->object()->as<MemberExpr>();
    ASSERT_NE(inner, nullptr);
    EXPECT_FALSE(inner->isOptional());
}

// =========================================================================
// Nullish Coalescing Tests
// =========================================================================

TEST(NullishCoalescing, BasicParsing) {
    auto prog = parseCode("a ?? b;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    auto* logical = expr->as<LogicalExpr>();
    ASSERT_NE(logical, nullptr);
    EXPECT_EQ(logical->op(), TokenType::QuestionQuestion);
}

TEST(NullishCoalescing, WithOptionalChaining) {
    auto prog = parseCode("a?.b ?? c;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    auto* logical = expr->as<LogicalExpr>();
    ASSERT_NE(logical, nullptr);
    EXPECT_EQ(logical->op(), TokenType::QuestionQuestion);
    // Left side should be optional member
    auto* left = logical->left()->as<MemberExpr>();
    ASSERT_NE(left, nullptr);
    EXPECT_TRUE(left->isOptional());
}

TEST(NullishCoalescing, Chained) {
    auto prog = parseCode("a ?? b ?? c;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    // Left-associative: (a ?? b) ?? c
    auto* outer = expr->as<LogicalExpr>();
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->op(), TokenType::QuestionQuestion);
}

// =========================================================================
// Compound Assignment Tests
// =========================================================================

TEST(CompoundAssignment, NullishAssign) {
    auto prog = parseCode("a ??= b;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    auto* assign = expr->as<AssignmentExpr>();
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->op(), TokenType::QuestionQuestionAssign);
}

TEST(CompoundAssignment, LogicalAndAssign) {
    auto prog = parseCode("a &&= b;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    auto* assign = expr->as<AssignmentExpr>();
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->op(), TokenType::AndAssign);
}

TEST(CompoundAssignment, LogicalOrAssign) {
    auto prog = parseCode("a ||= b;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    auto* assign = expr->as<AssignmentExpr>();
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->op(), TokenType::OrAssign);
}

TEST(CompoundAssignment, ExponentiationAssign) {
    auto prog = parseCode("a **= 2;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    auto* assign = expr->as<AssignmentExpr>();
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->op(), TokenType::StarStarAssign);
}

// =========================================================================
// Destructuring Tests
// =========================================================================

TEST(Destructuring, ObjectBasic) {
    auto prog = parseCode("let {a, b} = obj;");
    auto* stmt = firstStmt(prog.get());
    ASSERT_NE(stmt, nullptr);
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    ASSERT_EQ(decl->declarators().size(), 1u);
    auto* pattern = decl->declarators()[0].id.get();
    EXPECT_EQ(pattern->type(), NodeType::ObjectPattern);
}

TEST(Destructuring, ObjectWithRename) {
    auto prog = parseCode("const {name: fullName} = obj;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ObjectPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    ASSERT_EQ(pattern->properties().size(), 1u);
    EXPECT_FALSE(pattern->properties()[0].shorthand);
}

TEST(Destructuring, ObjectWithDefault) {
    auto prog = parseCode("let {x = 10} = obj;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ObjectPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    ASSERT_EQ(pattern->properties().size(), 1u);
    // The value should be an AssignmentPattern
    EXPECT_EQ(pattern->properties()[0].value->type(), NodeType::AssignmentPattern);
}

TEST(Destructuring, ObjectWithRest) {
    auto prog = parseCode("let {a, ...rest} = obj;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ObjectPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    EXPECT_NE(pattern->rest(), nullptr);
}

TEST(Destructuring, ArrayBasic) {
    auto prog = parseCode("let [x, y] = arr;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id.get();
    EXPECT_EQ(pattern->type(), NodeType::ArrayPattern);
}

TEST(Destructuring, ArrayWithElision) {
    auto prog = parseCode("let [, , z] = arr;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ArrayPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    // First two elements are elisions (nullptr)
    EXPECT_EQ(pattern->elements()[0], nullptr);
    EXPECT_EQ(pattern->elements()[1], nullptr);
    EXPECT_NE(pattern->elements()[2], nullptr);
}

TEST(Destructuring, ArrayWithRest) {
    auto prog = parseCode("let [first, ...rest] = arr;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ArrayPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    EXPECT_NE(pattern->rest(), nullptr);
}

TEST(Destructuring, ArrayWithDefault) {
    auto prog = parseCode("let [a = 1, b = 2] = arr;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ArrayPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->elements()[0]->type(), NodeType::AssignmentPattern);
    EXPECT_EQ(pattern->elements()[1]->type(), NodeType::AssignmentPattern);
}

TEST(Destructuring, NestedObject) {
    auto prog = parseCode("let {a: {b, c}} = obj;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ObjectPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    // Inner value should be ObjectPattern
    EXPECT_EQ(pattern->properties()[0].value->type(), NodeType::ObjectPattern);
}

TEST(Destructuring, NestedArray) {
    auto prog = parseCode("let [[a, b], [c, d]] = matrix;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ArrayPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->elements()[0]->type(), NodeType::ArrayPattern);
    EXPECT_EQ(pattern->elements()[1]->type(), NodeType::ArrayPattern);
}

TEST(Destructuring, FunctionParam) {
    auto prog = parseCode("function f({a, b}) {}");
    auto* stmt = firstStmt(prog.get());
    ASSERT_NE(stmt, nullptr);
    auto* fn = stmt->as<FunctionDecl>();
    ASSERT_NE(fn, nullptr);
    ASSERT_EQ(fn->params().size(), 1u);
    EXPECT_EQ(fn->params()[0].pattern->type(), NodeType::ObjectPattern);
}

TEST(Destructuring, FunctionParamArray) {
    auto prog = parseCode("function f([x, y]) {}");
    auto* stmt = firstStmt(prog.get());
    auto* fn = stmt->as<FunctionDecl>();
    ASSERT_NE(fn, nullptr);
    ASSERT_EQ(fn->params().size(), 1u);
    EXPECT_EQ(fn->params()[0].pattern->type(), NodeType::ArrayPattern);
}

TEST(Destructuring, ObjectComputedKey) {
    auto prog = parseCode("let {[key]: val} = obj;");
    auto* stmt = firstStmt(prog.get());
    auto* decl = stmt->as<VariableDecl>();
    ASSERT_NE(decl, nullptr);
    auto* pattern = decl->declarators()[0].id->as<ObjectPatternExpr>();
    ASSERT_NE(pattern, nullptr);
    EXPECT_TRUE(pattern->properties()[0].computed);
}

// =========================================================================
// For-Of Destructuring Tests
// =========================================================================

TEST(ForOfDestructuring, ObjectPattern) {
    auto prog = parseCode("for (const {a, b} of items) {}");
    auto* stmt = firstStmt(prog.get());
    ASSERT_NE(stmt, nullptr);
    auto* forOf = stmt->as<ForOfStmt>();
    ASSERT_NE(forOf, nullptr);
    EXPECT_FALSE(forOf->isAwait());
    // Left should be VariableDecl with ObjectPattern
    auto* varDecl = forOf->left()->as<VariableDecl>();
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->declarators()[0].id->type(), NodeType::ObjectPattern);
}

TEST(ForOfDestructuring, ArrayPattern) {
    auto prog = parseCode("for (const [key, val] of entries) {}");
    auto* stmt = firstStmt(prog.get());
    auto* forOf = stmt->as<ForOfStmt>();
    ASSERT_NE(forOf, nullptr);
    auto* varDecl = forOf->left()->as<VariableDecl>();
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->declarators()[0].id->type(), NodeType::ArrayPattern);
}

// =========================================================================
// For-Await-Of Tests
// =========================================================================

TEST(ForAwaitOf, BasicSyntax) {
    auto prog = parseCode("for await (const x of stream) {}");
    auto* stmt = firstStmt(prog.get());
    ASSERT_NE(stmt, nullptr);
    auto* forOf = stmt->as<ForOfStmt>();
    ASSERT_NE(forOf, nullptr);
    EXPECT_TRUE(forOf->isAwait());
}

TEST(ForAwaitOf, WithDestructuring) {
    auto prog = parseCode("for await (const {data} of responses) {}");
    auto* stmt = firstStmt(prog.get());
    auto* forOf = stmt->as<ForOfStmt>();
    ASSERT_NE(forOf, nullptr);
    EXPECT_TRUE(forOf->isAwait());
    auto* varDecl = forOf->left()->as<VariableDecl>();
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->declarators()[0].id->type(), NodeType::ObjectPattern);
}

// =========================================================================
// Tagged Template Tests
// =========================================================================

TEST(TaggedTemplate, BasicTag) {
    auto prog = parseCode("tag`hello`;");
    auto* expr = exprFromStmt(firstStmt(prog.get()));
    ASSERT_NE(expr, nullptr);
    // Should be a CallExpr with tag as callee
    auto* call = expr->as<CallExpr>();
    ASSERT_NE(call, nullptr);
    auto* callee = call->callee()->as<IdentifierExpr>();
    ASSERT_NE(callee, nullptr);
    EXPECT_EQ(callee->name(), "tag");
}

// =========================================================================
// Regular For Loop (regression)
// =========================================================================

TEST(ForLoop, RegularForLoop) {
    auto prog = parseCode("for (let i = 0; i < 10; i++) {}");
    auto* stmt = firstStmt(prog.get());
    ASSERT_NE(stmt, nullptr);
    auto* forStmt = stmt->as<ForStmt>();
    ASSERT_NE(forStmt, nullptr);
}

TEST(ForLoop, ForOf) {
    auto prog = parseCode("for (const x of arr) {}");
    auto* stmt = firstStmt(prog.get());
    ASSERT_NE(stmt, nullptr);
    auto* forOf = stmt->as<ForOfStmt>();
    ASSERT_NE(forOf, nullptr);
    EXPECT_FALSE(forOf->isAwait());
}

TEST(ForLoop, ForIn) {
    auto prog = parseCode("for (const key in obj) {}");
    auto* stmt = firstStmt(prog.get());
    ASSERT_NE(stmt, nullptr);
    auto* forIn = stmt->as<ForInStmt>();
    ASSERT_NE(forIn, nullptr);
}
