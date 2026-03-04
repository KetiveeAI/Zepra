/**
 * @file parser_tests.cpp
 * @brief Comprehensive parser tests for ES6 features
 */
#include <gtest/gtest.h>
#include "frontend/parser.hpp"
#include "frontend/ast.hpp"

using namespace Zepra::Frontend;

// =============================================================================
// Empty Program Tests
// =============================================================================

TEST(ParserTests, EmptyProgram) {
    auto program = parse("", "test.js");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->body().size(), 0);
}

// =============================================================================
// ES6 Class Declaration Tests
// =============================================================================

TEST(ParserTests, BasicClassDeclaration) {
    auto program = parse(R"(
        class Animal {
            constructor(name) {
                this.name = name;
            }
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->body().size(), 1);
    
    auto* classDecl = dynamic_cast<ClassDecl*>(program->body()[0].get());
    ASSERT_NE(classDecl, nullptr);
    EXPECT_EQ(classDecl->name(), "Animal");
    EXPECT_EQ(classDecl->superClass(), nullptr);
    ASSERT_NE(classDecl->constructor(), nullptr);
}

TEST(ParserTests, ClassWithExtends) {
    auto program = parse(R"(
        class Dog extends Animal {
            constructor(name) {
                this.name = name;
            }
            bark() {
                return "woof";
            }
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->body().size(), 1);
    
    auto* classDecl = dynamic_cast<ClassDecl*>(program->body()[0].get());
    ASSERT_NE(classDecl, nullptr);
    EXPECT_EQ(classDecl->name(), "Dog");
    ASSERT_NE(classDecl->superClass(), nullptr);
    EXPECT_EQ(classDecl->methods().size(), 1);
}

TEST(ParserTests, ClassWithStaticMethod) {
    auto program = parse(R"(
        class Factory {
            static create(type) {
                return new Factory();
            }
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    auto* classDecl = dynamic_cast<ClassDecl*>(program->body()[0].get());
    ASSERT_NE(classDecl, nullptr);
    ASSERT_EQ(classDecl->methods().size(), 1);
    EXPECT_TRUE(classDecl->methods()[0].isStatic);
}

TEST(ParserTests, ClassWithGetterSetter) {
    auto program = parse(R"(
        class Person {
            get name() {
                return this._name;
            }
            set name(value) {
                this._name = value;
            }
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    auto* classDecl = dynamic_cast<ClassDecl*>(program->body()[0].get());
    ASSERT_NE(classDecl, nullptr);
    ASSERT_EQ(classDecl->methods().size(), 2);
    EXPECT_TRUE(classDecl->methods()[0].isGetter);
    EXPECT_TRUE(classDecl->methods()[1].isSetter);
}

// =============================================================================
// for...of Loop Tests
// =============================================================================

TEST(ParserTests, ForOfWithLet) {
    auto program = parse(R"(
        for (let x of arr) {
            console.log(x);
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->body().size(), 1);
    
    auto* forOf = dynamic_cast<ForOfStmt*>(program->body()[0].get());
    ASSERT_NE(forOf, nullptr);
    ASSERT_NE(forOf->left(), nullptr);
    ASSERT_NE(forOf->right(), nullptr);
    ASSERT_NE(forOf->body(), nullptr);
}

TEST(ParserTests, ForOfWithConst) {
    auto program = parse(R"(
        for (const item of items) {
            process(item);
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    auto* forOf = dynamic_cast<ForOfStmt*>(program->body()[0].get());
    ASSERT_NE(forOf, nullptr);
}

TEST(ParserTests, ForOfWithExistingVariable) {
    auto program = parse(R"(
        let x;
        for (x of arr) {
            console.log(x);
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->body().size(), 2);
    
    auto* forOf = dynamic_cast<ForOfStmt*>(program->body()[1].get());
    ASSERT_NE(forOf, nullptr);
}

// =============================================================================
// Regular For Loop Tests (ensure not broken)
// =============================================================================

TEST(ParserTests, RegularForLoop) {
    auto program = parse(R"(
        for (let i = 0; i < 10; i++) {
            console.log(i);
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    auto* forStmt = dynamic_cast<ForStmt*>(program->body()[0].get());
    ASSERT_NE(forStmt, nullptr);
    ASSERT_NE(forStmt->init(), nullptr);
    ASSERT_NE(forStmt->test(), nullptr);
    ASSERT_NE(forStmt->update(), nullptr);
}

TEST(ParserTests, ForLoopNoInit) {
    auto program = parse(R"(
        let i = 0;
        for (; i < 10; i++) {
            console.log(i);
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    auto* forStmt = dynamic_cast<ForStmt*>(program->body()[1].get());
    ASSERT_NE(forStmt, nullptr);
    EXPECT_EQ(forStmt->init(), nullptr);
}

// =============================================================================
// Arrow Function Tests
// =============================================================================

TEST(ParserTests, ArrowFunctionSingleParam) {
    auto program = parse("const fn = x => x * 2;", "test.js");
    ASSERT_NE(program, nullptr);
}

TEST(ParserTests, ArrowFunctionMultipleParams) {
    auto program = parse("const add = (a, b) => a + b;", "test.js");
    ASSERT_NE(program, nullptr);
}

TEST(ParserTests, ArrowFunctionWithBlock) {
    auto program = parse(R"(
        const fn = (x) => {
            console.log(x);
            return x * 2;
        };
    )", "test.js");
    ASSERT_NE(program, nullptr);
}

// =============================================================================
// Template Literal Tests
// =============================================================================

TEST(ParserTests, VariableDeclarations) {
    auto program = parse(R"(
        var a = 1;
        let b = 2;
        const c = 3;
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->body().size(), 3);
}

// =============================================================================
// Destructuring Tests
// =============================================================================

TEST(ParserTests, FunctionDefaultParams) {
    auto program = parse(R"(
        function greet(name = "World") {
            return "Hello, " + name;
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    auto* funcDecl = dynamic_cast<FunctionDecl*>(program->body()[0].get());
    ASSERT_NE(funcDecl, nullptr);
    ASSERT_EQ(funcDecl->params().size(), 1);
}

TEST(ParserTests, RestParameter) {
    auto program = parse(R"(
        function sum(...numbers) {
            return numbers.reduce((a, b) => a + b, 0);
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
    auto* funcDecl = dynamic_cast<FunctionDecl*>(program->body()[0].get());
    ASSERT_NE(funcDecl, nullptr);
    ASSERT_EQ(funcDecl->params().size(), 1);
    EXPECT_TRUE(funcDecl->params()[0].rest);
}

// =============================================================================
// Async/Await Tests
// =============================================================================

TEST(ParserTests, AwaitExpression) {
    auto program = parse(R"(
        async function fetchData() {
            const result = await fetch("/api");
            return result;
        }
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
}

// =============================================================================
// Module Import/Export Tests
// =============================================================================

TEST(ParserTests, ImportDeclaration) {
    auto program = parse(R"(
        import { foo, bar } from './module.js';
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
}

TEST(ParserTests, ExportDeclaration) {
    auto program = parse(R"(
        export function greet() {}
    )", "test.js");
    
    ASSERT_NE(program, nullptr);
}
