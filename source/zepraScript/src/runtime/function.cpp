// Runtime function stub
#include "zeprascript/runtime/function.hpp"
#include "zeprascript/runtime/environment.hpp"

namespace Zepra::Runtime {

Function::Function(const Frontend::FunctionDecl* decl, Environment* closure)
    : Object(ObjectType::Function), functionDecl_(decl), closure_(closure) {
    if (decl) name_ = decl->name();
}

Function::Function(const Frontend::FunctionExpr* expr, Environment* closure)
    : Object(ObjectType::Function), functionExpr_(expr), closure_(closure) {
    if (expr) name_ = expr->name();
}

Function::Function(const Frontend::ArrowFunctionExpr* arrow, Environment* closure)
    : Object(ObjectType::Function), arrowExpr_(arrow), closure_(closure), isArrow_(true) {}

Function::Function(std::string name, NativeFn native, size_t arity)
    : Object(ObjectType::Function), name_(std::move(name)), native_(std::move(native)), arity_(arity) {}

Function::Function(std::string name, BuiltinFn builtin, size_t arity)
    : Object(ObjectType::Function), name_(std::move(name)), builtin_(std::move(builtin)), arity_(arity) {}


Function::Function(Function* target, Value boundThis, std::vector<Value> boundArgs)
    : Object(ObjectType::BoundFunction)
    , boundTarget_(target), boundThis_(boundThis), boundArgs_(std::move(boundArgs)) {}

Value Function::call(Context*, Value, const std::vector<Value>&) {
    // TODO: Implement function call
    return Value::undefined();
}

Value Function::construct(Context*, const std::vector<Value>&) {
    // TODO: Implement construction
    return Value::undefined();
}

Function* Function::bind(Value thisArg, const std::vector<Value>& args) {
    return new Function(this, thisArg, args);
}

Value Function::apply(Context* ctx, Value thisArg, const std::vector<Value>& args) {
    return call(ctx, thisArg, args);
}

} // namespace Zepra::Runtime
