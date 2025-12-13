/**
 * @file zepra-repl.cpp
 * @brief ZepraScript interactive REPL (Read-Eval-Print-Loop)
 */

#include <iostream>
#include <string>
#include <sstream>
#include <cmath>

#include "zeprascript/config.hpp"
#include "zeprascript/frontend/source_code.hpp"
#include "zeprascript/frontend/lexer.hpp"
#include "zeprascript/frontend/parser.hpp"
#include "zeprascript/frontend/token.hpp"
#include "zeprascript/bytecode/bytecode_generator.hpp"
#include "zeprascript/runtime/value.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include "zeprascript/runtime/vm.hpp"
#include "zeprascript/builtins/console.hpp"
#include "zeprascript/builtins/math.hpp"
#include "zeprascript/builtins/json.hpp"

using namespace Zepra;

void printBanner() {
    std::cout << "\n";
    std::cout << "+=================================================+\n";
    std::cout << "|          ZepraScript JavaScript Engine          |\n";
    std::cout << "|               Version " << ZEPRA_VERSION << "                    |\n";
    std::cout << "+=================================================+\n";
    std::cout << "\n";
    std::cout << "Type JavaScript code to evaluate.\n";
    std::cout << "Type '.help' for commands, '.exit' to quit.\n";
    std::cout << "\n";
}

void printHelp() {
    std::cout << "\nREPL Commands:\n";
    std::cout << "  .help     Show this help message\n";
    std::cout << "  .exit     Exit the REPL\n";
    std::cout << "  .clear    Clear the screen\n";
    std::cout << "  .tokens   Tokenize input (debug)\n";
    std::cout << "  .ast      Parse and show AST (debug)\n";
    std::cout << "  .bytecode Show bytecode (debug)\n";
    std::cout << "\n";
}

/**
 * @brief Set up global environment with builtins
 */
void setupGlobals(Runtime::VM& vm) {
    // Register console object
    Runtime::Object* consoleObj = Builtins::Console::createConsoleObject(nullptr);
    vm.setGlobal("console", Runtime::Value::object(consoleObj));
    
    // Register Math object
    Runtime::Object* mathObj = Builtins::MathBuiltin::createMathObject();
    vm.setGlobal("Math", Runtime::Value::object(mathObj));
    
    // Register JSON object
    Runtime::Object* jsonObj = Builtins::JSONBuiltin::createJSONObject(nullptr);
    vm.setGlobal("JSON", Runtime::Value::object(jsonObj));
    
    // Register undefined, NaN, Infinity
    vm.setGlobal("undefined", Runtime::Value::undefined());
    vm.setGlobal("NaN", Runtime::Value::number(std::nan("")));
    vm.setGlobal("Infinity", Runtime::Value::number(INFINITY));
}

/**
 * @brief Tokenize and display tokens
 */
void tokenize(const std::string& code) {
    auto source = Frontend::SourceCode::fromString(code, "<repl>");
    Frontend::Lexer lexer(source.get());
    
    std::cout << "Tokens:\n";
    while (!lexer.isEof()) {
        Frontend::Token token = lexer.nextToken();
        std::cout << "  [" << Frontend::Token::typeName(token.type) << "]";
        if (!token.value.empty()) {
            std::cout << " '" << token.value << "'";
        }
        if (token.type == Frontend::TokenType::Number) {
            std::cout << " = " << token.numericValue;
        }
        std::cout << " @ " << token.start.line << ":" << token.start.column;
        std::cout << "\n";
        
        if (token.type == Frontend::TokenType::EndOfFile ||
            token.type == Frontend::TokenType::Error) {
            break;
        }
    }
    
    if (lexer.hasErrors()) {
        std::cout << "\nErrors:\n";
        for (const auto& err : lexer.errors()) {
            std::cout << "  " << err << "\n";
        }
    }
}

/**
 * @brief Evaluate JavaScript code using full pipeline
 */
void evaluate(const std::string& code, Runtime::VM& vm) {
    try {
        // 1. Parse source code to AST
        auto source = Frontend::SourceCode::fromString(code, "<repl>");
        Frontend::Parser parser(source.get());
        
        auto program = parser.parseProgram();
        
        if (parser.hasErrors()) {
            for (const auto& err : parser.errors()) {
                std::cerr << "Parse error: " << err << "\n";
            }
            return;
        }
        
        // 2. Compile AST to bytecode
        Bytecode::BytecodeGenerator generator;
        auto chunk = generator.compile(program.get());
        
        if (generator.hasErrors()) {
            for (const auto& err : generator.errors()) {
                std::cerr << "Compile error: " << err << "\n";
            }
            return;
        }
        
        // 3. Execute bytecode
        Runtime::ExecutionResult result = vm.execute(chunk.get());
        
        if (result.status == Runtime::ExecutionResult::Status::Error) {
            std::cerr << "Runtime error: " << result.error << "\n";
            return;
        }
        
        // 4. Print result
        if (!result.value.isUndefined()) {
            std::cout << result.value.toString() << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    // Create VM
    Runtime::VM vm(nullptr);
    setupGlobals(vm);
    
    // Check for file argument
    if (argc > 1) {
        std::string filename = argv[1];
        auto source = Frontend::SourceCode::fromFile(filename);
        if (!source) {
            std::cerr << "Error: Could not read file '" << filename << "'\n";
            return 1;
        }
        
        evaluate(source->content(), vm);
        return 0;
    }
    
    printBanner();
    
    std::string line;
    std::string multilineBuffer;
    bool inMultiline = false;
    
    while (true) {
        // Print prompt
        if (inMultiline) {
            std::cout << "... ";
        } else {
            std::cout << "zepra> ";
        }
        std::cout.flush();
        
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }
        
        // Handle commands
        if (line == ".exit" || line == ".quit") {
            std::cout << "Goodbye!\n";
            break;
        }
        
        if (line == ".help") {
            printHelp();
            continue;
        }
        
        if (line == ".clear") {
            std::cout << "\033[2J\033[H";
            continue;
        }
        
        if (line.rfind(".tokens ", 0) == 0) {
            tokenize(line.substr(8));
            continue;
        }
        
        if (line.empty()) {
            continue;
        }
        
        // Handle multiline input (check for unclosed braces)
        int braceCount = 0;
        for (char c : line) {
            if (c == '{' || c == '(' || c == '[') braceCount++;
            if (c == '}' || c == ')' || c == ']') braceCount--;
        }
        
        if (inMultiline) {
            multilineBuffer += "\n" + line;
            for (char c : line) {
                if (c == '{' || c == '(' || c == '[') braceCount++;
                if (c == '}' || c == ')' || c == ']') braceCount--;
            }
            if (braceCount <= 0) {
                inMultiline = false;
                evaluate(multilineBuffer, vm);
                multilineBuffer.clear();
            }
        } else if (braceCount > 0) {
            inMultiline = true;
            multilineBuffer = line;
        } else {
            evaluate(line, vm);
        }
    }
    
    return 0;
}
