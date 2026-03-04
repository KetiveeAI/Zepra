/**
 * @file global_object.cpp
 * @brief Global object initialization
 *
 * Wires up all built-in constructors, prototypes, and global functions
 * by delegating to their respective Builtin classes.
 */

#include "runtime/execution/global_object.hpp"
#include "builtins/console.hpp"
#include "builtins/math.hpp"
#include "builtins/json.hpp"
#include "builtins/array.hpp"
#include "builtins/string.hpp"
#include "builtins/object_builtins.hpp"
#include "builtins/number.hpp"
#include "builtins/boolean.hpp"
#include "builtins/map.hpp"
#include "builtins/set.hpp"
#include "builtins/date.hpp"
#include "builtins/weakmap.hpp"
#include "builtins/regexp.hpp"
#include "builtins/typed_array.hpp"
#include "builtins/reflect.hpp"
#include "runtime/objects/proxy.hpp"
#include "builtins/generator.hpp"
#include "runtime/objects/well_known_symbols.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/async/promise.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Zepra::Runtime {

// ---------------------------------------------------------------------------
// Global function implementations (non-inline, separate from lambdas)
// These use the NativeFn signature: Value(Context*, const std::vector<Value>&)
// ---------------------------------------------------------------------------

static Value globalIsNaN(Context*, const std::vector<Value>& args) {
    if (args.empty()) return Value::boolean(true);
    return Value::boolean(std::isnan(args[0].toNumber()));
}

static Value globalIsFinite(Context*, const std::vector<Value>& args) {
    if (args.empty()) return Value::boolean(false);
    return Value::boolean(std::isfinite(args[0].toNumber()));
}

static Value globalParseInt(Context*, const std::vector<Value>& args) {
    if (args.empty()) return Value::number(std::nan(""));
    std::string str = args[0].toString();
    int radix = args.size() > 1 ? static_cast<int>(args[1].toNumber()) : 10;
    if (radix == 0) radix = 10;
    try {
        return Value::number(static_cast<double>(std::stoll(str, nullptr, radix)));
    } catch (...) {
        return Value::number(std::nan(""));
    }
}

static Value globalParseFloat(Context*, const std::vector<Value>& args) {
    if (args.empty()) return Value::number(std::nan(""));
    try {
        return Value::number(std::stod(args[0].toString()));
    } catch (...) {
        return Value::number(std::nan(""));
    }
}

static Value globalEncodeURIComponent(Context*, const std::vector<Value>& args) {
    if (args.empty()) return Value::object(new String("undefined"));
    std::string input = args[0].toString();
    std::string result;
    result.reserve(input.size() * 3);
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += static_cast<char>(c);
        } else {
            char hex[4];
            std::snprintf(hex, sizeof(hex), "%%%02X", c);
            result += hex;
        }
    }
    return Value::object(new String(result));
}

static Value globalDecodeURIComponent(Context*, const std::vector<Value>& args) {
    if (args.empty()) return Value::object(new String("undefined"));
    std::string input = args[0].toString();
    std::string result;
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '%' && i + 2 < input.size()) {
            unsigned int value = 0;
            if (std::sscanf(input.c_str() + i + 1, "%2x", &value) == 1) {
                result += static_cast<char>(value);
                i += 2;
                continue;
            }
        }
        result += input[i];
    }
    return Value::object(new String(result));
}

static Value globalQueueMicrotask(Context*, const std::vector<Value>& args) {
    if (!args.empty()) {
        Function* fn = args[0].asFunction();
        if (fn) {
            MicrotaskQueue::instance().enqueue([fn]() {
                // Enqueue for later execution via event loop
            });
        }
    }
    return Value::undefined();
}

// Error constructor factory: creates a native function that constructs Error objects
static Function* createErrorConstructor(const std::string& errorName) {
    return createNativeFunction(errorName,
        [errorName](Context*, const std::vector<Value>& args) -> Value {
            std::string message = args.empty() ? "" : args[0].toString();

            // ES2022: options.cause
            if (args.size() > 1 && args[1].isObject()) {
                Object* opts = args[1].asObject();
                Value cause = opts->get("cause");
                if (!cause.isUndefined()) {
                    return Value::object(Error::withCause(errorName, message, cause));
                }
            }

            return Value::object(new Error(message, errorName));
        }, 1);
}

// ---------------------------------------------------------------------------
// GlobalObject
// ---------------------------------------------------------------------------

GlobalObject::GlobalObject(Context* context)
    : Object(ObjectType::Global), context_(context) {}

void GlobalObject::initialize() {
    // Intrinsic values
    set("undefined", Value::undefined());
    set("NaN", Value::number(std::nan("")));
    set("Infinity", Value::number(std::numeric_limits<double>::infinity()));
    set("globalThis", Value::object(this));

    // Constructors & prototypes (delegate to builtin classes)
    initializeObjectConstructor();
    initializeArrayConstructor();
    initializeStringConstructor();
    initializeNumberConstructor();
    initializeBooleanConstructor();
    initializeFunctionConstructor();
    initializeErrorConstructors();

    // Built-in objects
    initializeMath();
    initializeJSON();
    initializeConsole();

    // Global functions
    initializeGlobalFunctions();
}

void GlobalObject::initializeObjectConstructor() {
    objectPrototype_ = Builtins::ObjectBuiltin::createObjectPrototype(context_);
    objectConstructor_ = static_cast<Function*>(
        Builtins::ObjectBuiltin::createObjectConstructor(context_));
    objectConstructor_->set("prototype", Value::object(objectPrototype_));
    set("Object", Value::object(objectConstructor_));
}

void GlobalObject::initializeArrayConstructor() {
    arrayPrototype_ = Builtins::ArrayBuiltin::createArrayPrototype(context_);
    arrayConstructor_ = createNativeFunction("Array",
        [](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::object(new Array({}));
            if (args.size() == 1 && args[0].isNumber()) {
                size_t len = static_cast<size_t>(args[0].asNumber());
                return Value::object(new Array(std::vector<Value>(len, Value::undefined())));
            }
            return Value::object(new Array(std::vector<Value>(args.begin(), args.end())));
        }, 1);
    arrayConstructor_->set("isArray", Value::object(
        new Function("isArray", Builtins::ArrayBuiltin::isArray, 1)));
    arrayConstructor_->set("from", Value::object(
        new Function("from", Builtins::ArrayBuiltin::from, 1)));
    arrayConstructor_->set("of", Value::object(
        new Function("of", Builtins::ArrayBuiltin::of, 0)));
    arrayConstructor_->set("prototype", Value::object(arrayPrototype_));
    set("Array", Value::object(arrayConstructor_));
}

void GlobalObject::initializeStringConstructor() {
    stringPrototype_ = Builtins::StringBuiltin::createStringPrototype(context_);
    stringConstructor_ = createNativeFunction("String",
        [](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::object(new String(""));
            return Value::object(new String(args[0].toString()));
        }, 1);
    stringConstructor_->set("prototype", Value::object(stringPrototype_));
    set("String", Value::object(stringConstructor_));
}

void GlobalObject::initializeNumberConstructor() {
    numberPrototype_ = Builtins::NumberBuiltin::createNumberPrototype();
    numberConstructor_ = createNativeFunction("Number",
        [](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::number(0.0);
            return Value::number(args[0].toNumber());
        }, 1);

    // Constants
    numberConstructor_->set("MAX_SAFE_INTEGER", Value::number(9007199254740991.0));
    numberConstructor_->set("MIN_SAFE_INTEGER", Value::number(-9007199254740991.0));
    numberConstructor_->set("MAX_VALUE", Value::number(std::numeric_limits<double>::max()));
    numberConstructor_->set("MIN_VALUE", Value::number(std::numeric_limits<double>::min()));
    numberConstructor_->set("EPSILON", Value::number(std::numeric_limits<double>::epsilon()));
    numberConstructor_->set("POSITIVE_INFINITY",
        Value::number(std::numeric_limits<double>::infinity()));
    numberConstructor_->set("NEGATIVE_INFINITY",
        Value::number(-std::numeric_limits<double>::infinity()));
    numberConstructor_->set("NaN", Value::number(std::nan("")));
    numberConstructor_->set("prototype", Value::object(numberPrototype_));
    set("Number", Value::object(numberConstructor_));
}

void GlobalObject::initializeBooleanConstructor() {
    booleanPrototype_ = Builtins::BooleanBuiltin::createBooleanPrototype();
    booleanConstructor_ = createNativeFunction("Boolean",
        [](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::boolean(false);
            return Value::boolean(args[0].toBoolean());
        }, 1);
    booleanConstructor_->set("prototype", Value::object(booleanPrototype_));
    set("Boolean", Value::object(booleanConstructor_));
}

void GlobalObject::initializeFunctionConstructor() {
    functionPrototype_ = new Object();
    functionConstructor_ = createNativeFunction("Function",
        [](Context*, const std::vector<Value>&) -> Value {
            // Function() constructor requires eval — not supported for security
            return Value::undefined();
        }, 1);
    functionConstructor_->set("prototype", Value::object(functionPrototype_));
    set("Function", Value::object(functionConstructor_));
}

void GlobalObject::initializeErrorConstructors() {
    errorConstructor_ = createErrorConstructor("Error");
    typeErrorConstructor_ = createErrorConstructor("TypeError");
    syntaxErrorConstructor_ = createErrorConstructor("SyntaxError");
    referenceErrorConstructor_ = createErrorConstructor("ReferenceError");
    rangeErrorConstructor_ = createErrorConstructor("RangeError");

    set("Error", Value::object(errorConstructor_));
    set("TypeError", Value::object(typeErrorConstructor_));
    set("SyntaxError", Value::object(syntaxErrorConstructor_));
    set("ReferenceError", Value::object(referenceErrorConstructor_));
    set("RangeError", Value::object(rangeErrorConstructor_));
    set("URIError", Value::object(createErrorConstructor("URIError")));
    set("EvalError", Value::object(createErrorConstructor("EvalError")));
}

void GlobalObject::initializeMath() {
    mathObject_ = Builtins::MathBuiltin::createMathObject();
    set("Math", Value::object(mathObject_));
}

void GlobalObject::initializeJSON() {
    jsonObject_ = Builtins::JSONBuiltin::createJSONObject(context_);
    set("JSON", Value::object(jsonObject_));
}

void GlobalObject::initializeConsole() {
    consoleObject_ = Builtins::Console::createConsoleObject(context_);
    set("console", Value::object(consoleObject_));
}

void GlobalObject::initializeGlobalFunctions() {
    set("isNaN", Value::object(createNativeFunction("isNaN", globalIsNaN, 1)));
    set("isFinite", Value::object(createNativeFunction("isFinite", globalIsFinite, 1)));
    set("parseInt", Value::object(createNativeFunction("parseInt", globalParseInt, 2)));
    set("parseFloat", Value::object(createNativeFunction("parseFloat", globalParseFloat, 1)));
    set("encodeURIComponent", Value::object(
        createNativeFunction("encodeURIComponent", globalEncodeURIComponent, 1)));
    set("decodeURIComponent", Value::object(
        createNativeFunction("decodeURIComponent", globalDecodeURIComponent, 1)));
    set("queueMicrotask", Value::object(
        createNativeFunction("queueMicrotask", globalQueueMicrotask, 1)));

    // Symbol constructor + well-known symbols
    Object* symbolCtor = createNativeFunction("Symbol",
        [](Context*, const std::vector<Value>&) -> Value {
            static uint32_t nextId = 1;
            return Value::symbol(nextId++);
        }, 0);
    symbolCtor->set("iterator",      Value::symbol(WellKnownSymbols::Iterator));
    symbolCtor->set("toPrimitive",   Value::symbol(WellKnownSymbols::ToPrimitive));
    symbolCtor->set("hasInstance",   Value::symbol(WellKnownSymbols::HasInstance));
    symbolCtor->set("toStringTag",   Value::symbol(WellKnownSymbols::ToStringTag));
    symbolCtor->set("species",       Value::symbol(WellKnownSymbols::Species));
    symbolCtor->set("asyncIterator", Value::symbol(WellKnownSymbols::AsyncIterator));
    symbolCtor->set("match",         Value::symbol(WellKnownSymbols::Match));
    symbolCtor->set("replace",       Value::symbol(WellKnownSymbols::Replace));
    symbolCtor->set("search",        Value::symbol(WellKnownSymbols::Search));
    symbolCtor->set("split",         Value::symbol(WellKnownSymbols::Split));
    symbolCtor->set("isConcatSpreadable", Value::symbol(WellKnownSymbols::IsConcatSpreadable));
    set("Symbol", Value::object(symbolCtor));

    // Map constructor + prototype
    static Object* mapProto = Builtins::MapBuiltin::createMapPrototype();
    Object* mapCtor = createNativeFunction("Map",
        [](Context*, const std::vector<Value>&) -> Value {
            Builtins::MapObject* m = new Builtins::MapObject();
            m->setPrototype(mapProto);
            return Value::object(m);
        }, 0);
    set("Map", Value::object(mapCtor));

    // Set constructor + prototype
    static Object* setProto = Builtins::SetBuiltin::createSetPrototype();
    Object* setCtor = createNativeFunction("Set",
        [](Context*, const std::vector<Value>&) -> Value {
            Builtins::SetObject* s = new Builtins::SetObject();
            s->setPrototype(setProto);
            return Value::object(s);
        }, 0);
    set("Set", Value::object(setCtor));

    // Date constructor + prototype
    static Object* dateProto = Builtins::DateBuiltin::createDatePrototype();
    Object* dateCtor = createNativeFunction("Date",
        [](Context* ctx, const std::vector<Value>& args) -> Value {
            Value result = Builtins::DateBuiltin::constructor(ctx, args);
            if (result.isObject()) {
                result.asObject()->setPrototype(dateProto);
            }
            return result;
        }, 7);
    // Date.now() as static method
    dateCtor->set("now", Value::object(
        createNativeFunction("now",
            [](Context* ctx, const std::vector<Value>& args) -> Value {
                return Builtins::DateBuiltin::now(ctx, args);
            }, 0)));
    set("Date", Value::object(dateCtor));

    // WeakMap constructor + prototype
    static Object* weakMapProto = Builtins::WeakMapBuiltin::createWeakMapPrototype();
    Object* weakMapCtor = createNativeFunction("WeakMap",
        [](Context*, const std::vector<Value>&) -> Value {
            Builtins::WeakMapObject* wm = new Builtins::WeakMapObject();
            wm->setPrototype(weakMapProto);
            return Value::object(wm);
        }, 0);
    set("WeakMap", Value::object(weakMapCtor));

    // RegExp constructor + prototype
    static Object* regexpProto = Builtins::RegExpBuiltin::createRegExpPrototype();
    Object* regexpCtor = createNativeFunction("RegExp",
        [](Context* ctx, const std::vector<Value>& args) -> Value {
            Value result = Builtins::RegExpBuiltin::constructor(ctx, args);
            if (result.isObject()) {
                result.asObject()->setPrototype(regexpProto);
            }
            return result;
        }, 2);
    set("RegExp", Value::object(regexpCtor));

    // ArrayBuffer + TypedArray constructors
    static Object* typedArrayProto = Builtins::TypedArrayBuiltin::createTypedArrayPrototype();

    set("ArrayBuffer", Value::object(
        createNativeFunction("ArrayBuffer", Builtins::TypedArrayBuiltin::arrayBufferConstructor, 1)));

    auto registerTypedArray = [&](const char* name, Value(*ctor)(Context*, const std::vector<Value>&)) {
        Object* ctorObj = createNativeFunction(name, ctor, 1);
        ctorObj->set("prototype", Value::object(typedArrayProto));
        set(name, Value::object(ctorObj));
    };

    registerTypedArray("Int8Array",    Builtins::TypedArrayBuiltin::int8ArrayConstructor);
    registerTypedArray("Uint8Array",   Builtins::TypedArrayBuiltin::uint8ArrayConstructor);
    registerTypedArray("Int16Array",   Builtins::TypedArrayBuiltin::int16ArrayConstructor);
    registerTypedArray("Uint16Array",  Builtins::TypedArrayBuiltin::uint16ArrayConstructor);
    registerTypedArray("Uint32Array",  Builtins::TypedArrayBuiltin::uint32ArrayConstructor);
    registerTypedArray("Float32Array", Builtins::TypedArrayBuiltin::float32ArrayConstructor);
    registerTypedArray("Float64Array", Builtins::TypedArrayBuiltin::float64ArrayConstructor);

    // Proxy constructor
    Object* proxyCtor = createNativeFunction("Proxy",
        [](Context*, const std::vector<Value>& args) -> Value {
            if (args.size() < 2 || !args[0].isObject() || !args[1].isObject()) {
                throw std::runtime_error("Proxy requires two Object arguments");
            }
            return Value::object(Proxy::create(args[0].asObject(), args[1].asObject()));
        }, 2);
    
    // Proxy.revocable
    proxyCtor->set("revocable", Value::object(createNativeFunction("revocable",
        [](Context*, const std::vector<Value>& args) -> Value {
            if (args.size() < 2 || !args[0].isObject() || !args[1].isObject()) {
                throw std::runtime_error("Proxy.revocable requires two Object arguments");
            }
            auto pair = Proxy::createRevocable(args[0].asObject(), args[1].asObject());
            
            Object* result = new Object();
            result->set("proxy", Value::object(pair.first));
            
            Object* revokeFn = createNativeFunction("revoke",
                [revoker = pair.second](Context*, const std::vector<Value>&) -> Value {
                    revoker();
                    return Value::undefined();
                }, 0);
            result->set("revoke", Value::object(revokeFn));
            
            return Value::object(result);
        }, 2)));
        
    set("Proxy", Value::object(proxyCtor));

    // Reflect object
    set("Reflect", Value::object(Builtins::ReflectBuiltin::createReflectObject(nullptr)));

    // Generator constructor and prototype
    Object* generatorProto = Builtins::GeneratorBuiltin::createGeneratorPrototype();
    Object* generatorFunc = createNativeFunction("Generator", 
        [](Context*, const std::vector<Value>&) -> Value {
            throw std::runtime_error("Generator constructor is not intended to be called directly");
        }, 0);
    generatorFunc->set("prototype", Value::object(generatorProto));
    set("Generator", Value::object(generatorFunc));
}

} // namespace Zepra::Runtime
