/**
 * @file context.cpp
 * @brief JavaScript execution context — global object provider
 * 
 * Manages the global environment for JS execution.
 * VM instances use this context's global object for name resolution.
 */

#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <limits>
#include <memory>

namespace Zepra {

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
    
    void setGlobal(const std::string& name, Runtime::Value value) {
        globalObject_->set(name, value);
    }
    
    Runtime::Value getGlobal(const std::string& name) const {
        return globalObject_->get(name);
    }
    
private:
    void initializeGlobals() {
        globalObject_->set("undefined", Runtime::Value::undefined());
        globalObject_->set("NaN", Runtime::Value::number(std::numeric_limits<double>::quiet_NaN()));
        globalObject_->set("Infinity", Runtime::Value::number(std::numeric_limits<double>::infinity()));
    }
    
    void* isolate_;
    Runtime::Object* globalObject_;
};

void* createContext(void* isolate) {
    return new ContextImpl(isolate);
}

void destroyContext(void* ctx) {
    delete static_cast<ContextImpl*>(ctx);
}

} // namespace Zepra
