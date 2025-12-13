// Global object stub
#include "zeprascript/runtime/global_object.hpp"
#include "zeprascript/builtins/console.hpp"
#include "zeprascript/builtins/math.hpp"
#include "zeprascript/builtins/json.hpp"
#include "zeprascript/runtime/function.hpp"

namespace Zepra::Runtime {

GlobalObject::GlobalObject(Context* context)
    : Object(ObjectType::Global), context_(context) {}

void GlobalObject::initialize() {
    // Basic globals
    set("undefined", Value::undefined());
    set("NaN", Value::number(std::nan("")));
    set("Infinity", Value::number(std::numeric_limits<double>::infinity()));
    
    // console object
    consoleObject_ = Builtins::Console::createConsoleObject(context_);
    set("console", Value::object(consoleObject_));
    
    // Math object
    mathObject_ = Builtins::MathBuiltin::createMathObject();
    set("Math", Value::object(mathObject_));
    
    // JSON object  
    jsonObject_ = Builtins::JSONBuiltin::createJSONObject(context_);
    set("JSON", Value::object(jsonObject_));
    
    // Global functions
    initializeGlobalFunctions();
}

void GlobalObject::initializeGlobalFunctions() {
    // isNaN
    set("isNaN", Value::object(
        createNativeFunction("isNaN",
            [](Context*, const std::vector<Value>& args) {
                if (args.empty()) return Value::boolean(true);
                return Value::boolean(std::isnan(args[0].toNumber()));
            }, 1)));
    
    // isFinite
    set("isFinite", Value::object(
        createNativeFunction("isFinite",
            [](Context*, const std::vector<Value>& args) {
                if (args.empty()) return Value::boolean(false);
                return Value::boolean(std::isfinite(args[0].toNumber()));
            }, 1)));
    
    // parseInt
    set("parseInt", Value::object(
        createNativeFunction("parseInt",
            [](Context*, const std::vector<Value>& args) {
                if (args.empty()) return Value::number(std::nan(""));
                std::string str = args[0].toString();
                int radix = args.size() > 1 ? static_cast<int>(args[1].toNumber()) : 10;
                try {
                    return Value::number(static_cast<double>(std::stoll(str, nullptr, radix)));
                } catch (...) {
                    return Value::number(std::nan(""));
                }
            }, 2)));
    
    // parseFloat
    set("parseFloat", Value::object(
        createNativeFunction("parseFloat",
            [](Context*, const std::vector<Value>& args) {
                if (args.empty()) return Value::number(std::nan(""));
                try {
                    return Value::number(std::stod(args[0].toString()));
                } catch (...) {
                    return Value::number(std::nan(""));
                }
            }, 1)));
}

} // namespace Zepra::Runtime

