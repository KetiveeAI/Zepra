/**
 * @file vm.cpp
 * @brief JavaScript Virtual Machine implementation
 */

#include "zeprascript/runtime/vm.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include "zeprascript/bytecode/bytecode_generator.hpp"
#include <cmath>
#include <iostream>

namespace Zepra::Runtime {

using Zepra::Bytecode::Opcode;
using Zepra::Bytecode::BytecodeChunk;

VM::VM(Context* context) : context_(context) {
    stack_.reserve(ZEPRA_MAX_CALL_STACK_DEPTH * 256);
    heapStack_.reserve(HEAP_STACK_PREALLOC);  // Pre-allocate for deep recursion
}

VM::~VM() = default;

ExecutionResult VM::execute(const Bytecode::BytecodeChunk* chunk) {
    ExecutionResult result;
    
    if (!chunk) {
        result.status = ExecutionResult::Status::Error;
        result.error = "No bytecode to execute";
        return result;
    }
    
    chunk_ = chunk;
    ip_ = 0;
    
    try {
        run();
        result.status = ExecutionResult::Status::Success;
        result.value = stack_.empty() ? Value::undefined() : pop();
    } catch (const std::exception& e) {
        result.status = ExecutionResult::Status::Error;
        result.error = e.what();
    }
    
    return result;
}

void VM::run() {
    while (ip_ < chunk_->code().size()) {
        Opcode op = static_cast<Opcode>(readByte());
        dispatch(op);
    }
}

void VM::dispatch(Opcode op) {
    
    switch (op) {
        case Opcode::OP_NOP:
            // Do nothing
            break;
            
        case Opcode::OP_CONSTANT: {
            uint8_t constant = readByte();
            push(chunk_->constant(constant));
            break;
        }
        
        case Opcode::OP_CONSTANT_LONG: {
            uint16_t constant = readShort();
            push(chunk_->constant(constant));
            break;
        }
        
        case Opcode::OP_NIL:
            push(Value::null());
            break;
            
        case Opcode::OP_TRUE:
            push(Value::boolean(true));
            break;
            
        case Opcode::OP_FALSE:
            push(Value::boolean(false));
            break;
            
        case Opcode::OP_POP:
            pop();
            break;
            
        case Opcode::OP_DUP:
            push(peek());
            break;
            
        case Opcode::OP_GET_LOCAL: {
            uint8_t slot = readByte();
            push(getLocal(slot));
            break;
        }
        
        case Opcode::OP_SET_LOCAL: {
            uint8_t slot = readByte();
            setLocal(slot, peek());
            break;
        }
        
        case Opcode::OP_GET_GLOBAL: {
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            if (nameValue.isString()) {
                std::string name = static_cast<String*>(nameValue.asObject())->value();
                if (globals_.count(name)) {
                    push(globals_[name]);
                } else {
                    push(Value::undefined());
                }
            }
            break;
        }
        
        case Opcode::OP_SET_GLOBAL: {
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            if (nameValue.isString()) {
                std::string name = static_cast<String*>(nameValue.asObject())->value();
                globals_[name] = peek();
            }
            break;
        }
        
        case Opcode::OP_DEFINE_GLOBAL: {
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            if (nameValue.isString()) {
                std::string name = static_cast<String*>(nameValue.asObject())->value();
                globals_[name] = pop();
            }
            break;
        }
        
        case Opcode::OP_GET_UPVALUE: {
            uint8_t slot = readByte();
            // Get current function from call stack (HYBRID)
            if (callDepth() > 0 && currentFrame().function) {
                RuntimeUpvalue* upvalue = currentFrame().function->upvalue(slot);
                if (upvalue) {
                    push(upvalue->get());
                } else {
                    push(Value::undefined());
                }
            } else {
                push(Value::undefined());
            }
            break;
        }
        
        case Opcode::OP_SET_UPVALUE: {
            uint8_t slot = readByte();
            Value value = peek();
            if (callDepth() > 0 && currentFrame().function) {
                RuntimeUpvalue* upvalue = currentFrame().function->upvalue(slot);
                if (upvalue) {
                    upvalue->set(value);
                }
            }
            break;
        }
            
        case Opcode::OP_GET_PROPERTY: {
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            Value objValue = pop();
            
            if (objValue.isObject()) {
                Object* obj = objValue.asObject();
                if (nameValue.isString()) {
                    std::string name = static_cast<String*>(nameValue.asObject())->value();
                    
                    // Special case: 'length' property for arrays and strings
                    if (name == "length") {
                        push(Value::number(static_cast<double>(obj->length())));
                    } else {
                        push(obj->get(name));
                    }
                } else {
                    push(Value::undefined());
                }
            } else if (objValue.isString()) {
                // String property access
                if (nameValue.isString()) {
                    std::string name = static_cast<String*>(nameValue.asObject())->value();
                    if (name == "length") {
                        String* str = static_cast<String*>(objValue.asObject());
                        push(Value::number(static_cast<double>(str->value().length())));
                    } else {
                        push(Value::undefined());
                    }
                } else {
                    push(Value::undefined());
                }
            } else {
                push(Value::undefined());
            }
            break;
        }
        
        case Opcode::OP_SET_PROPERTY: {
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            Value value = pop();
            Value objValue = pop();
            
            if (objValue.isObject()) {
                Object* obj = objValue.asObject();
                if (nameValue.isString()) {
                    std::string name = static_cast<String*>(nameValue.asObject())->value();
                    obj->set(name, value);
                }
            }
            push(value);
            break;
        }
        
        case Opcode::OP_GET_ELEMENT: {
            Value index = pop();
            Value array = pop();
            
            if (array.isObject() && index.isNumber()) {
                size_t idx = static_cast<size_t>(index.asNumber());
                push(array.asObject()->get(idx));
            } else {
                push(Value::undefined());
            }
            break;
        }
        
        case Opcode::OP_SET_ELEMENT: {
            Value value = pop();
            Value index = pop();
            Value array = pop();
            
            if (array.isObject() && index.isNumber()) {
                size_t idx = static_cast<size_t>(index.asNumber());
                array.asObject()->set(idx, value);
            }
            push(value);
            break;
        }
        
        // Arithmetic
        case Opcode::OP_ADD: {
            Value b = pop();
            Value a = pop();
            push(Value::add(a, b));
            break;
        }
        
        case Opcode::OP_SUBTRACT: {
            Value b = pop();
            Value a = pop();
            push(Value::subtract(a, b));
            break;
        }
        
        case Opcode::OP_MULTIPLY: {
            Value b = pop();
            Value a = pop();
            push(Value::multiply(a, b));
            break;
        }
        
        case Opcode::OP_DIVIDE: {
            Value b = pop();
            Value a = pop();
            push(Value::divide(a, b));
            break;
        }
        
        case Opcode::OP_MODULO: {
            Value b = pop();
            Value a = pop();
            push(Value::modulo(a, b));
            break;
        }
        
        case Opcode::OP_POWER: {
            Value b = pop();
            Value a = pop();
            if (a.isNumber() && b.isNumber()) {
                push(Value::number(std::pow(a.asNumber(), b.asNumber())));
            } else {
                push(Value::number(std::nan("")));
            }
            break;
        }
        
        case Opcode::OP_NEGATE: {
            Value a = pop();
            push(Value::negate(a));
            break;
        }
        
        // Comparison
        case Opcode::OP_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(Value::boolean(a.equals(b)));
            break;
        }
        
        case Opcode::OP_STRICT_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(Value::boolean(a.strictEquals(b)));
            break;
        }
        
        case Opcode::OP_NOT_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(Value::boolean(!a.equals(b)));
            break;
        }
        
        case Opcode::OP_STRICT_NOT_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(Value::boolean(!a.strictEquals(b)));
            break;
        }
        
        case Opcode::OP_LESS: {
            Value b = pop();
            Value a = pop();
            push(Value::lessThan(a, b));
            break;
        }
        
        case Opcode::OP_LESS_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(Value::lessEqual(a, b));
            break;
        }
        
        case Opcode::OP_GREATER: {
            Value b = pop();
            Value a = pop();
            push(Value::greaterThan(a, b));
            break;
        }
        
        case Opcode::OP_GREATER_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(Value::greaterEqual(a, b));
            break;
        }
        
        // Logical
        case Opcode::OP_NOT: {
            Value a = pop();
            push(Value::boolean(a.isFalsy()));
            break;
        }
        
        // Bitwise
        case Opcode::OP_BITWISE_AND: {
            Value b = pop();
            Value a = pop();
            push(Value::bitwiseAnd(a, b));
            break;
        }
        
        case Opcode::OP_BITWISE_OR: {
            Value b = pop();
            Value a = pop();
            push(Value::bitwiseOr(a, b));
            break;
        }
        
        case Opcode::OP_BITWISE_XOR: {
            Value b = pop();
            Value a = pop();
            push(Value::bitwiseXor(a, b));
            break;
        }
        
        case Opcode::OP_BITWISE_NOT: {
            Value a = pop();
            push(Value::bitwiseNot(a));
            break;
        }
        
        case Opcode::OP_LEFT_SHIFT: {
            Value b = pop();
            Value a = pop();
            push(Value::leftShift(a, b));
            break;
        }
        
        case Opcode::OP_RIGHT_SHIFT: {
            Value b = pop();
            Value a = pop();
            push(Value::rightShift(a, b));
            break;
        }
        
        case Opcode::OP_UNSIGNED_RIGHT_SHIFT: {
            Value b = pop();
            Value a = pop();
            push(Value::unsignedRightShift(a, b));
            break;
        }
        
        // Jumps
        case Opcode::OP_JUMP: {
            uint16_t offset = readShort();
            ip_ += offset;
            break;
        }
        
        case Opcode::OP_JUMP_IF_FALSE: {
            uint16_t offset = readShort();
            if (peek().isFalsy()) {
                ip_ += offset;
            }
            break;
        }
        
        case Opcode::OP_JUMP_IF_NIL: {
            uint16_t offset = readShort();
            if (peek().isNull() || peek().isUndefined()) {
                ip_ += offset;
            }
            break;
        }
        
        case Opcode::OP_LOOP: {
            uint16_t offset = readShort();
            ip_ -= offset;
            break;
        }
        
        // Functions
        case Opcode::OP_CALL: {
            uint8_t argCount = readByte();
            
            // Get the callee (function/object) from the stack
            Value callee = peek(argCount);
            
            if (!callee.isObject()) {
                throw std::runtime_error("Attempted to call non-callable value");
            }
            
            Object* calleeObj = callee.asObject();
            
            // Check if it's a function
            if (!calleeObj->isFunction()) {
                throw std::runtime_error("Object is not callable");
            }
            
            Function* function = static_cast<Function*>(calleeObj);
            
            // Collect arguments from stack in correct order
            // Stack: [callee, arg0, arg1, ...] with arg0 at bottom
            std::vector<Value> args;
            args.reserve(argCount);
            for (int i = argCount - 1; i >= 0; i--) {
                args.push_back(peek(i));
            }
            
            // Pop arguments and callee from stack
            popN(argCount + 1);
            
            // Execute the function
            if (function->isBuiltin()) {
                // Call builtin function with FunctionCallInfo
                FunctionCallInfo info(context_, Value::undefined(), args);
                Value result = function->builtinFunction()(info);
                push(result);
            } else if (function->isNative()) {
                // Call native function
                Value result = function->nativeFunction()(context_, args);
                push(result);
            } else if (function->isCompiled()) {
                // Compiled JavaScript function - push call frame and execute bytecode
                // Uses HYBRID TWO-STACK: native for fast path, heap for deep recursion
                VMCallFrame frame;
                frame.function = function;
                frame.returnAddress = ip_;
                frame.slotBase = stack_.size();
                frame.thisValue = Value::undefined();
                // Store current chunk so we can restore it on return
                frame.savedChunk = chunk_;
                pushCallFrame(frame);  // Hybrid: auto-switches native/heap
                
                // Push arguments as local variables
                for (const auto& arg : args) {
                    push(arg);
                }
                
                // Switch to the function's bytecode chunk
                // NO RECURSIVE run() CALL - just continue the main loop!
                chunk_ = function->bytecodeChunk();
                ip_ = 0;
                // The main loop in run() will now execute the function's bytecode
            } else {
                // AST-based function (not yet compiled) - push undefined for now
                // TODO: Interpret AST directly or compile first
                push(Value::undefined());
            }
            break;
        }
        
        case Opcode::OP_RETURN: {
            Value result = pop();
            
            // If we have call frames, pop one and restore state (HYBRID STACK)
            if (callDepth() > 0) {
                VMCallFrame frame = popCallFrame();  // Hybrid: auto-switches heap/native
                
                // Pop locals for this frame
                while (stack_.size() > frame.slotBase) {
                    pop();
                }
                
                // Push the return value
                push(result);
                
                // Restore caller's chunk and IP
                chunk_ = frame.savedChunk;
                ip_ = frame.returnAddress;
            } else {
                // Top-level return - end execution
                push(result);
                ip_ = chunk_->code().size();
            }
            break;
        }
        
        case Opcode::OP_CALL_METHOD: {
            // Stack layout: [receiver, arg0, arg1, ...]
            // Operands: 1 byte method name constant index, 1 byte arg count
            uint8_t nameIdx = readByte();
            uint8_t argCount = readByte();
            
            // Get method name from constant pool
            Value nameValue = chunk_->constant(nameIdx);
            if (!nameValue.isString()) {
                throw std::runtime_error("Invalid method name");
            }
            std::string methodName = static_cast<String*>(nameValue.asObject())->value();
            
            // Get receiver (below all arguments)
            Value receiver = peek(argCount);
            
            // Handle string primitives - auto-box to String object
            Object* obj = nullptr;
            if (receiver.isString()) {
                // String primitive - use the String object directly (it has prototype)
                obj = receiver.asObject();
            } else if (receiver.isObject()) {
                obj = receiver.asObject();
            } else {
                throw std::runtime_error("Cannot call method on " + receiver.toString());
            }
            
            // Look up the method on the receiver
            Value method = obj->get(methodName);
            
            if (!method.isObject() || !method.asObject()->isFunction()) {
                throw std::runtime_error("Method '" + methodName + "' is not a function");
            }
            
            Function* function = static_cast<Function*>(method.asObject());
            
            // Collect arguments from stack
            std::vector<Value> args;
            args.reserve(argCount);
            for (int i = argCount - 1; i >= 0; i--) {
                args.push_back(peek(i));
            }
            
            // Pop arguments and receiver
            popN(argCount + 1);
            
            // Execute the method with receiver as 'this'
            if (function->isBuiltin()) {
                FunctionCallInfo info(context_, receiver, args);
                Value result = function->builtinFunction()(info);
                push(result);
            } else if (function->isNative()) {
                Value result = function->nativeFunction()(context_, args);
                push(result);
            } else if (function->isCompiled()) {
                // Compiled function - execute with this binding
                VMCallFrame frame;
                frame.function = function;
                frame.returnAddress = ip_;
                frame.slotBase = stack_.size();
                frame.thisValue = receiver;
                frame.savedChunk = chunk_;  // Save current chunk
                pushCallFrame(frame);  // HYBRID
                
                for (const auto& arg : args) {
                    push(arg);
                }
                
                // Switch to function's bytecode - NO RECURSIVE run() CALL!
                chunk_ = function->bytecodeChunk();
                ip_ = 0;
                // Main loop continues with function's bytecode
            } else {
                push(Value::undefined());
            }
            break;
        }
            
        case Opcode::OP_CLOSURE: {
            // Read the function constant index
            uint8_t funcIdx = readByte();
            Value funcValue = chunk_->constant(funcIdx);
            
            // For now, just push the function value as-is
            // TODO: Capture upvalues
            push(funcValue);
            break;
        }
            
        case Opcode::OP_CLOSE_UPVALUE: {
            // Close upvalue at top of stack
            if (!stack_.empty()) {
                closeUpvalues(&stack_.back());
            }
            pop();
            break;
        }
            
        // Object creation
        case Opcode::OP_CREATE_ARRAY: {
            uint8_t count = readByte();
            std::vector<Value> elements;
            for (int i = count - 1; i >= 0; i--) {
                elements.insert(elements.begin(), stack_[stack_.size() - count + i]);
            }
            popN(count);
            push(Value::object(new Array(std::move(elements))));
            break;
        }
        
        case Opcode::OP_CREATE_OBJECT:
            push(Value::object(new Object()));
            break;
            
        case Opcode::OP_INIT_PROPERTY: {
            Value value = pop();
            Value key = pop();
            Value obj = peek();
            
            if (obj.isObject() && key.isString()) {
                std::string propName = static_cast<String*>(key.asObject())->value();
                obj.asObject()->set(propName, value);
            }
            break;
        }
        
        case Opcode::OP_NEW: {
            uint8_t argCount = readByte();
            // TODO: Implement constructor calls
            (void)argCount;
            push(Value::object(new Object()));
            break;
        }
        
        // Type operations
        case Opcode::OP_TYPEOF: {
            Value a = pop();
            std::string type;
            if (a.isUndefined()) type = "undefined";
            else if (a.isNull()) type = "object";
            else if (a.isBoolean()) type = "boolean";
            else if (a.isNumber()) type = "number";
            else if (a.isString()) type = "string";
            else if (a.isObject() && dynamic_cast<Function*>(a.asObject())) type = "function";
            else type = "object";
            push(Value::string(new String(type)));
            break;
        }
        
        case Opcode::OP_INSTANCEOF:
            // TODO: Implement instanceof
            pop();
            pop();
            push(Value::boolean(false));
            break;
            
        // Exceptions
        case Opcode::OP_THROW: {
            // Pop the exception value
            exceptionValue_ = pop();
            hasException_ = true;
            
            // Find nearest exception handler
            if (!exceptionHandlers_.empty()) {
                ExceptionHandler handler = exceptionHandlers_.back();
                exceptionHandlers_.pop_back();
                
                // Unwind stack to handler level
                while (stack_.size() > handler.stackLevel) {
                    pop();
                }
                
                // Jump to catch block
                ip_ = handler.catchAddress;
            } else {
                // No handler - propagate as runtime error
                throw std::runtime_error("Uncaught exception: " + exceptionValue_.toString());
            }
            break;
        }
            
        case Opcode::OP_TRY_BEGIN: {
            // Read catch block offset
            uint16_t catchOffset = readShort();
            
            // Push exception handler
            ExceptionHandler handler;
            handler.catchAddress = ip_ + catchOffset;
            handler.stackLevel = stack_.size();
            handler.callStackLevel = callDepth();  // HYBRID
            exceptionHandlers_.push_back(handler);
            break;
        }
        
        case Opcode::OP_TRY_END:
            // End of try block without exception - pop handler
            if (!exceptionHandlers_.empty()) {
                exceptionHandlers_.pop_back();
            }
            break;
            
        case Opcode::OP_CATCH:
            // Push exception value onto stack for catch block
            push(exceptionValue_);
            hasException_ = false;
            exceptionValue_ = Value::undefined();
            break;
            
        case Opcode::OP_FINALLY:
            // Finally block - just continues execution
            // Exception state preserved if re-throwing
            break;
        
        case Opcode::OP_IMPORT: {
            // Read module path constant index
            uint8_t pathIdx = readByte();
            Value pathValue = chunk_->constant(pathIdx);
            if (!pathValue.isString()) {
                throw std::runtime_error("Invalid module path");
            }
            std::string modulePath = static_cast<String*>(pathValue.asObject())->value();
            
            // For now, create an empty exports object
            // Full implementation requires module loader and file system access
            Object* exports = new Object();
            push(Value::object(exports));
            
            // TODO: Implement actual module loading:
            // 1. Resolve path relative to current module
            // 2. Check module cache
            // 3. Load and compile module file
            // 4. Execute module in new context
            // 5. Return exports object
            break;
        }
        
        case Opcode::OP_EXPORT: {
            // Read export name constant index
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            Value exportValue = pop();
            
            // Store in module exports
            // For now, just store in globals as a simplification
            if (nameValue.isString()) {
                std::string name = static_cast<String*>(nameValue.asObject())->value();
                setGlobal(name, exportValue);
            }
            break;
        }
        
        case Opcode::OP_IMPORT_BINDING: {
            // Read binding name constant index  
            uint8_t nameIdx = readByte();
            // This is handled by OP_GET_PROPERTY in the generated code
            break;
        }
            
        default:
            // Unknown opcode - skip
            break;
    }
}

// Stack operations
void VM::push(Value value) {
    if (stack_.size() >= ZEPRA_MAX_CALL_STACK_DEPTH * 256) {
        throw std::runtime_error("Stack overflow");
    }
    stack_.push_back(value);
}

Value VM::pop() {
    if (stack_.empty()) {
        return Value::undefined();
    }
    Value v = stack_.back();
    stack_.pop_back();
    return v;
}

Value VM::peek(size_t distance) const {
    if (distance >= stack_.size()) {
        return Value::undefined();
    }
    return stack_[stack_.size() - 1 - distance];
}

void VM::popN(size_t count) {
    while (count-- > 0 && !stack_.empty()) {
        stack_.pop_back();
    }
}

// Helper read methods
uint8_t VM::readByte() {
    return chunk_->at(ip_++);
}

uint16_t VM::readShort() {
    uint8_t high = readByte();
    uint8_t low = readByte();
    return (high << 8) | low;
}

Value VM::readConstant() {
    return chunk_->constant(readByte());
}

// Local variable access
Value VM::getLocal(size_t slot) {
    if (callDepth() > 0) {  // HYBRID
        size_t base = currentFrame().slotBase;
        return stack_[base + slot];
    }
    return slot < stack_.size() ? stack_[slot] : Value::undefined();
}

void VM::setLocal(size_t slot, Value value) {
    if (callDepth() > 0) {  // HYBRID
        size_t base = currentFrame().slotBase;
        if (base + slot < stack_.size()) {
            stack_[base + slot] = value;
        }
    } else if (slot < stack_.size()) {
        stack_[slot] = value;
    }
}

// Function calls
Value VM::call(Function* function, Value thisValue, const std::vector<Value>& args) {
    if (!function) {
        return Value::undefined();
    }
    
    // Native function call
    if (function->isNative()) {
        return function->call(context_, thisValue, args);
    }
    
    // TODO: Implement JS function calls
    return Value::undefined();
}

Value VM::construct(Function* constructor, const std::vector<Value>& args) {
    if (!constructor) {
        return Value::undefined();
    }
    
    // TODO: Implement constructor calls
    (void)args;
    return Value::object(new Object());
}

// Upvalue management
RuntimeUpvalue* VM::captureUpvalue(Value* local) {
    // Look for existing open upvalue for this location
    RuntimeUpvalue* prevUpvalue = nullptr;
    RuntimeUpvalue* upvalue = openUpvalues_;
    
    while (upvalue != nullptr && upvalue->location() > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }
    
    // Reuse existing upvalue if found
    if (upvalue != nullptr && upvalue->location() == local) {
        return upvalue;
    }
    
    // Create new upvalue
    RuntimeUpvalue* createdUpvalue = new RuntimeUpvalue(local);
    createdUpvalue->next = upvalue;
    
    // Insert into linked list
    if (prevUpvalue == nullptr) {
        openUpvalues_ = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }
    
    return createdUpvalue;
}

void VM::closeUpvalues(Value* last) {
    while (openUpvalues_ != nullptr && openUpvalues_->location() >= last) {
        RuntimeUpvalue* upvalue = openUpvalues_;
        upvalue->close();
        openUpvalues_ = upvalue->next;
    }
}

// =============================================================================
// HYBRID TWO-STACK IMPLEMENTATION
// =============================================================================

void VM::pushCallFrame(const VMCallFrame& frame) {
    if (!shouldUseHeapStack()) {
        // FAST PATH: Use native C stack (zero allocation)
        if (nativeDepth_ < NATIVE_STACK_SIZE) {
            nativeStack_[nativeDepth_++] = frame;
            return;
        }
    }
    
    // SLOW PATH: Switch to heap stack for deep recursion
    if (heapDepth_ >= HEAP_STACK_MAX) {
        throw std::runtime_error("Maximum call stack size exceeded (65536 frames)");
    }
    
    if (heapDepth_ >= heapStack_.size()) {
        heapStack_.push_back(frame);
    } else {
        heapStack_[heapDepth_] = frame;
    }
    heapDepth_++;
}

VMCallFrame VM::popCallFrame() {
    // Pop from heap stack first (LIFO)
    if (heapDepth_ > 0) {
        VMCallFrame frame = heapStack_[--heapDepth_];
        shrinkHeapIfNeeded();
        return frame;
    }
    
    // Pop from native stack
    if (nativeDepth_ > 0) {
        return nativeStack_[--nativeDepth_];
    }
    
    // Empty - return default frame
    return VMCallFrame{};
}

void VM::shrinkHeapIfNeeded() {
    // Auto-cleanup excess heap memory when returning to shallow depth
    // Only shrink if heap is significantly oversized and we're back to native
    if (heapDepth_ == 0 && heapStack_.size() > HEAP_STACK_PREALLOC * 2) {
        heapStack_.resize(HEAP_STACK_PREALLOC);
        heapStack_.shrink_to_fit();
    }
}

} // namespace Zepra::Runtime
