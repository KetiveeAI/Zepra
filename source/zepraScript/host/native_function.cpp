/**
 * @file native_function.cpp
 * @brief C++ ↔ JS native function binding helpers
 *
 * Convenience wrappers around Runtime::createNativeFunction for
 * registering globals and prototype methods.
 */

#include "config.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"

namespace Zepra::Host {

using Runtime::Value;
using Runtime::Function;
using Runtime::Object;
using Runtime::NativeFn;
using Runtime::createNativeFunction;

/**
 * Register a native function as a global variable on the given object.
 */
void registerGlobalFunction(Object* global, const std::string& name,
                            NativeFn callback, size_t arity) {
    Function* fn = createNativeFunction(name, std::move(callback), arity);
    global->set(name, Value::object(fn));
}

/**
 * Register a method on a prototype object.
 */
void registerMethod(Object* prototype, const std::string& name,
                    NativeFn callback, size_t arity) {
    Function* fn = createNativeFunction(name, std::move(callback), arity);
    prototype->set(name, Value::object(fn));
}

} // namespace Zepra::Host
