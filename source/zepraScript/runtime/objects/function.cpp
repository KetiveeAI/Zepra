/**
 * @file function.cpp
 * @brief JavaScript function object implementation
 * 
 * Implements function calling, construction, and binding semantics
 * as specified in ECMAScript. Supports native, builtin, bytecode-compiled,
 * and bound function types.
 */

#include "runtime/objects/function.hpp"
#include "runtime/execution/environment.hpp"
#include "runtime/execution/vm.hpp"

namespace Zepra::Runtime {

// =============================================================================
// Constructors
// =============================================================================

Function::Function(const Frontend::FunctionDecl* decl, Environment* closure)
    : Object(ObjectType::Function)
    , functionDecl_(decl)
    , closure_(closure) {
    if (decl) {
        name_ = decl->name();
        // Set arity from parameter count
        arity_ = decl->params().size();
        isGenerator_ = decl->isGenerator();
    }
}

Function::Function(const Frontend::FunctionExpr* expr, Environment* closure)
    : Object(ObjectType::Function)
    , functionExpr_(expr)
    , closure_(closure) {
    if (expr) {
        name_ = expr->name();
        arity_ = expr->params().size();
        isGenerator_ = expr->isGenerator();
    }
}

Function::Function(const Frontend::ArrowFunctionExpr* arrow, Environment* closure)
    : Object(ObjectType::Function)
    , arrowExpr_(arrow)
    , closure_(closure)
    , isArrow_(true) {
    if (arrow) {
        arity_ = arrow->params().size();
    }
}

Function::Function(std::string name, NativeFn native, size_t arity)
    : Object(ObjectType::Function)
    , name_(std::move(name))
    , arity_(arity)
    , native_(std::move(native)) {}

Function::Function(std::string name, BuiltinFn builtin, size_t arity)
    : Object(ObjectType::Function)
    , name_(std::move(name))
    , arity_(arity)
    , builtin_(std::move(builtin)) {}

Function::Function(Function* target, Value boundThis, std::vector<Value> boundArgs)
    : Object(ObjectType::BoundFunction)
    , boundTarget_(target)
    , boundThis_(boundThis)
    , boundArgs_(std::move(boundArgs)) {
    if (target) {
        name_ = "bound " + target->name();
        // Adjust arity: max(0, target.arity - boundArgs.length)
        size_t targetArity = target->arity();
        size_t boundCount = boundArgs_.size();
        arity_ = targetArity > boundCount ? targetArity - boundCount : 0;
    }
}

// =============================================================================
// Function::call - Execute the function
// =============================================================================

Value Function::call(Context* ctx, Value thisValue, const std::vector<Value>& args) {
    // Handle bound functions - unwrap and prepend bound arguments
    if (isBound() && boundTarget_) {
        // Combine bound args with call args
        std::vector<Value> combinedArgs;
        combinedArgs.reserve(boundArgs_.size() + args.size());
        combinedArgs.insert(combinedArgs.end(), boundArgs_.begin(), boundArgs_.end());
        combinedArgs.insert(combinedArgs.end(), args.begin(), args.end());
        
        // Use bound 'this' value (ignore passed thisValue)
        return boundTarget_->call(ctx, boundThis_, combinedArgs);
    }
    
    // Handle builtin functions (with FunctionCallInfo)
    if (builtin_) {
        FunctionCallInfo info(ctx, thisValue, args);
        return builtin_(info);
    }
    
    // Handle native functions (simple signature)
    if (native_) {
        return native_(ctx, args);
    }
    
    // Handle bytecode-compiled functions via mini-VM execution
    if (isCompiled() && bytecodeChunk_) {
        // Create a temporary VM to execute the function
        VM miniVM(ctx);
        
        // Set up arguments as locals/globals
        for (size_t i = 0; i < args.size(); i++) {
            // Simple argument passing - first args become locals
            std::string argName = i < arity_ ? std::to_string(i) : "";
            if (!argName.empty()) {
                miniVM.setGlobal(argName, args[i]);
            }
        }
        
        // Execute the bytecode
        auto result = miniVM.execute(bytecodeChunk_);
        
        if (result.status == ExecutionResult::Status::Success) {
            return result.value;
        }
        
        // On error, return undefined
        return Value::undefined();
    }
    
    // Handle AST-based functions (not yet compiled)
    // These require interpretation which needs the AST interpreter
    // For now, return undefined - the full interpreter handles these
    if (functionDecl_ || functionExpr_ || arrowExpr_) {
        // AST-based execution would go here
        // The bytecode compiler should compile these first
        return Value::undefined();
    }
    
    return Value::undefined();
}

// =============================================================================
// Function::construct - Call function as constructor (new)
// =============================================================================

Value Function::construct(Context* ctx, const std::vector<Value>& args) {
    // Arrow functions cannot be constructors
    if (isArrow_) {
        // In a full implementation, this would throw TypeError:
        // "ArrowFunctionName is not a constructor"
        return Value::undefined();
    }
    
    // Bound functions delegate to the target
    if (isBound() && boundTarget_) {
        // Combine bound args with construct args
        std::vector<Value> combinedArgs;
        combinedArgs.reserve(boundArgs_.size() + args.size());
        combinedArgs.insert(combinedArgs.end(), boundArgs_.begin(), boundArgs_.end());
        combinedArgs.insert(combinedArgs.end(), args.begin(), args.end());
        
        return boundTarget_->construct(ctx, combinedArgs);
    }
    
    // Create new instance object
    Object* instance = new Object();
    
    // Set prototype chain: instance.[[Prototype]] = F.prototype
    Value protoValue = get("prototype");
    if (protoValue.isObject()) {
        instance->setPrototype(protoValue.asObject());
    } else {
        // Use Object.prototype as fallback
        // In full implementation, this comes from the realm
    }
    
    // Call the function with 'this' set to the new instance
    Value result = call(ctx, Value::object(instance), args);
    
    // If the constructor returns an object, use that instead
    // Otherwise, return the newly created instance
    if (result.isObject()) {
        return result;
    }
    
    return Value::object(instance);
}

// =============================================================================
// Function.prototype methods
// =============================================================================

Function* Function::bind(Value thisArg, const std::vector<Value>& args) {
    // Create a new bound function wrapping this function
    return new Function(this, thisArg, args);
}

Value Function::apply(Context* ctx, Value thisArg, const std::vector<Value>& args) {
    // apply() is just call() with an array of arguments
    return call(ctx, thisArg, args);
}

} // namespace Zepra::Runtime
