/**
 * @file script.cpp
 * @brief Script compilation stub - API surface for script handling
 */

#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Zepra {

/**
 * @brief Compiled script (stub implementation)
 * 
 * Full implementation requires integration with Parser and BytecodeGenerator.
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
        // Stub - actual implementation would use Parser and BytecodeGenerator
        compiled_ = true;
        return true;
    }
    
    const std::string& source() const { return source_; }
    const std::string& filename() const { return filename_; }
    const std::string& error() const { return error_; }
    bool isCompiled() const { return compiled_; }
    
private:
    std::string source_;
    std::string filename_;
    std::string error_;
    bool compiled_;
    std::vector<uint8_t> bytecode_;
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
    
private:
    std::unique_ptr<ScriptImpl> script_;
};

} // namespace Zepra
