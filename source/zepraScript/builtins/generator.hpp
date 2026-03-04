#pragma once

/**
 * @file generator.hpp
 * @brief ES6 Generator Object implementation
 */

#include "../config.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/execution/vm.hpp"
#include "runtime/objects/function.hpp"

namespace Zepra::Builtins {

/**
 * @brief JavaScript Generator Object
 * 
 * Represents the iterator object returned by calling a generator function.
 * Wraps the VM's GeneratorFrame which tracks execution state.
 */
class GeneratorObject : public Runtime::Object {
public:
    GeneratorObject(Runtime::Function* function, Runtime::Value thisBinding, const std::vector<Runtime::Value>& args);
    ~GeneratorObject() override = default;

    // The internal VM state for this generator
    Runtime::GeneratorFrame frame;
    // Initial arguments passed to the generator function
    std::vector<Runtime::Value> initialArgs;
};

/**
 * @brief Generator builtin methods (Generator.prototype)
 */
class GeneratorBuiltin {
public:
    // Generator.prototype.next(value)
    static Runtime::Value next(const Runtime::FunctionCallInfo& info);
    
    // Generator.prototype.return(value)
    static Runtime::Value return_(const Runtime::FunctionCallInfo& info);
    
    // Generator.prototype.throw(exception)
    static Runtime::Value throw_(const Runtime::FunctionCallInfo& info);
    
    // Create the Generator.prototype object
    static Runtime::Object* createGeneratorPrototype();
};

} // namespace Zepra::Builtins
