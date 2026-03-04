/**
 * @file context.cpp
 * @brief Context implementation stub - JavaScript execution context
 */

#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <limits>
#include <memory>

namespace Zepra {

// Forward declarations from zepra_api.hpp
class Isolate;
class Context;

/**
 * @brief Minimal Context implementation
 * 
 * This is a stub that provides the basic structure.
 * Full implementation requires VM integration.
 */
class ContextImpl {
public:
    explicit ContextImpl(void* isolate)
        : isolate_(isolate)
        , globalObject_(nullptr)
    {
        globalObject_ = new Runtime::Object(Runtime::ObjectType::Global);
        initializeGlobals();
    }
    
    ~ContextImpl() {
        delete globalObject_;
    }
    
    Runtime::Object* globalObject() {
        return globalObject_;
    }
    
    void* isolate() {
        return isolate_;
    }
    
private:
    void initializeGlobals() {
        // Add global constants
        globalObject_->set("undefined", Runtime::Value::undefined());
        globalObject_->set("NaN", Runtime::Value::number(std::numeric_limits<double>::quiet_NaN()));
        globalObject_->set("Infinity", Runtime::Value::number(std::numeric_limits<double>::infinity()));
        
        // structuredClone is available via StructuredClone::clone()
    }
    
    void* isolate_;
    Runtime::Object* globalObject_;
};

// Context factory - called from isolate
void* createContext(void* isolate) {
    return new ContextImpl(isolate);
}

void destroyContext(void* ctx) {
    delete static_cast<ContextImpl*>(ctx);
}

} // namespace Zepra
