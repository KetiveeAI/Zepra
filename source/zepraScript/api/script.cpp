/**
 * @file script.cpp
 * @brief Script compilation API — full pipeline integration
 */

#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include "frontend/source_code.hpp"
#include "frontend/parser.hpp"
#include "frontend/syntax_checker.hpp"
#include "bytecode/bytecode_generator.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Zepra {

/**
 * @brief Compiled script — runs parse→check→compile pipeline
 */
class ScriptImpl {
public:
    ScriptImpl(std::string source, std::string filename)
        : source_(std::move(source))
        , filename_(std::move(filename))
        , compiled_(false)
    {
    }
    
    bool compile() {
        try {
            // Create SourceCode object
            auto sourceCode = Frontend::SourceCode::fromString(source_, filename_);
            
            // Parse (Parser creates its own Lexer internally)
            Frontend::Parser parser(sourceCode.get());
            auto ast = parser.parseProgram();
            if (parser.hasErrors()) {
                error_ = parser.errors().front();
                return false;
            }
            
            // Syntax check
            Frontend::SyntaxChecker checker;
            checker.check(ast.get());
            if (checker.hasErrors()) {
                error_ = checker.errors().front();
                return false;
            }
            
            // Compile to bytecode
            Bytecode::BytecodeGenerator generator;
            chunk_ = generator.compile(ast.get());
            
            compiled_ = true;
            return true;
        } catch (const std::exception& e) {
            error_ = e.what();
            return false;
        }
    }
    
    const std::string& source() const { return source_; }
    const std::string& filename() const { return filename_; }
    const std::string& error() const { return error_; }
    bool isCompiled() const { return compiled_; }
    Bytecode::BytecodeChunk* chunk() const { return chunk_.get(); }
    
private:
    std::string source_;
    std::string filename_;
    std::string error_;
    bool compiled_;
    std::unique_ptr<Bytecode::BytecodeChunk> chunk_;
};

/**
 * @brief Script factory
 */
class ScriptCompiler {
public:
    static std::unique_ptr<ScriptImpl> compile(const std::string& source,
                                                const std::string& filename = "<script>") {
        auto script = std::make_unique<ScriptImpl>(source, filename);
        script->compile();
        return script;
    }
};

/**
 * @brief UnboundScript - compiled but not yet bound to a context
 */
class UnboundScript {
public:
    explicit UnboundScript(std::unique_ptr<ScriptImpl> script)
        : script_(std::move(script)) {}
    
    bool isCompiled() const { return script_ && script_->isCompiled(); }
    const std::string& source() const { return script_->source(); }
    Bytecode::BytecodeChunk* chunk() const { return script_->chunk(); }
    
private:
    std::unique_ptr<ScriptImpl> script_;
};

} // namespace Zepra
