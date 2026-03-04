/**
 * @file vm.cpp
 * @brief JavaScript Virtual Machine implementation
 */

#include "runtime/execution/vm.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/async/async_function.hpp"
#include "runtime/execution/global_object.hpp"
#include "builtins/console.hpp"
#include "builtins/math.hpp"
#include "builtins/generator.hpp"
#include "runtime/handles/module_loader.hpp"
#include "bytecode/bytecode_generator.hpp"
#include "runtime/execution/Sandbox.h"
#include "heap/gc_heap.hpp"
#include "builtins/string.hpp"
#include "builtins/array.hpp"
#include "runtime/async/promise.hpp"
#include "runtime/objects/well_known_symbols.hpp"
#include "builtins/map.hpp"
#include "builtins/set.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace Zepra::Runtime {

using Zepra::Bytecode::Opcode;
using Zepra::Bytecode::BytecodeChunk;

// Thread-local current VM for callback execution
thread_local VM* VM::currentVM_ = nullptr;

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
    
    // Set current VM for callbacks
    VM* previousVM = currentVM_;
    currentVM_ = this;
    
    try {
        run();
        result.status = ExecutionResult::Status::Success;
        result.value = stack_.empty() ? Value::undefined() : pop();
        
        // Drain microtask queue (promise callbacks, queueMicrotask)
        MicrotaskQueue::instance().process();
    } catch (const std::exception& e) {
        result.status = ExecutionResult::Status::Error;
        result.error = e.what();
    }
    
    // Restore previous VM
    currentVM_ = previousVM;
    
    return result;
}

void VM::run() {
    while (ip_ < chunk_->code().size() && !isYielding_ && !terminationRequested_) {
        // Periodic limit check and GC safe-point (every N instructions)
        instructionCounter_++;
        if (instructionCounter_ >= LIMIT_CHECK_INTERVAL) {
            instructionCounter_ = 0;
            
            // Resource monitor check
            if (resourceMonitor_) {
                resourceMonitor_->addInstructions(LIMIT_CHECK_INTERVAL);
                
                if (!resourceMonitor_->checkLimits()) {
                    terminationRequested_ = true;
                    throw SecurityError(SecurityError::Type::Timeout, 
                        "Execution limit exceeded");
                }
            }
            
            // GC safe-point — let collector run if needed
            if (gcHeap_) {
                gcHeap_->maybeCollect();
            }
        }
        
        Opcode op = static_cast<Opcode>(readByte());
        dispatch(op);
    }
}

bool VM::shouldTerminate() const {
    if (terminationRequested_) return true;
    if (resourceMonitor_ && !resourceMonitor_->checkLimits()) return true;
    return false;
}

void VM::dispatch(Opcode op) {

#if defined(__GNUC__) || defined(__clang__)
    // ==========================================================================
    // COMPUTED GOTO DISPATCH TABLE
    // GCC/Clang extension: &&label gives the address of a label.
    // Eliminates branch prediction overhead of switch-case dispatch.
    // ==========================================================================
    static const void* dispatchTable[] = {
        &&lbl_NOP,           // 0x00 OP_NOP
        &&lbl_POP,           // 0x01 OP_POP
        &&lbl_DUP,           // 0x02 OP_DUP
        &&lbl_SWAP,          // 0x03 OP_SWAP
        &&lbl_CONSTANT,      // 0x04 OP_CONSTANT
        &&lbl_CONSTANT_LONG, // 0x05 OP_CONSTANT_LONG
        &&lbl_NIL,           // 0x06 OP_NIL
        &&lbl_TRUE,          // 0x07 OP_TRUE
        &&lbl_FALSE,         // 0x08 OP_FALSE
        &&lbl_ZERO,          // 0x09 OP_ZERO
        &&lbl_ONE,           // 0x0A OP_ONE
        &&lbl_ADD,           // 0x0B OP_ADD
        &&lbl_SUBTRACT,      // 0x0C OP_SUBTRACT
        &&lbl_MULTIPLY,      // 0x0D OP_MULTIPLY
        &&lbl_DIVIDE,        // 0x0E OP_DIVIDE
        &&lbl_MODULO,        // 0x0F OP_MODULO
        &&lbl_POWER,         // 0x10 OP_POWER
        &&lbl_NEGATE,        // 0x11 OP_NEGATE
        &&lbl_INCREMENT,     // 0x12 OP_INCREMENT
        &&lbl_DECREMENT,     // 0x13 OP_DECREMENT
        &&lbl_BITWISE_AND,   // 0x14 OP_BITWISE_AND
        &&lbl_BITWISE_OR,    // 0x15 OP_BITWISE_OR
        &&lbl_BITWISE_XOR,   // 0x16 OP_BITWISE_XOR
        &&lbl_BITWISE_NOT,   // 0x17 OP_BITWISE_NOT
        &&lbl_LEFT_SHIFT,    // 0x18 OP_LEFT_SHIFT
        &&lbl_RIGHT_SHIFT,   // 0x19 OP_RIGHT_SHIFT
        &&lbl_UNSIGNED_RIGHT_SHIFT, // 0x1A OP_UNSIGNED_RIGHT_SHIFT
        &&lbl_EQUAL,         // 0x1B OP_EQUAL
        &&lbl_STRICT_EQUAL,  // 0x1C OP_STRICT_EQUAL
        &&lbl_NOT_EQUAL,     // 0x1D OP_NOT_EQUAL
        &&lbl_STRICT_NOT_EQUAL, // 0x1E OP_STRICT_NOT_EQUAL
        &&lbl_LESS,          // 0x1F OP_LESS
        &&lbl_LESS_EQUAL,    // 0x20 OP_LESS_EQUAL
        &&lbl_GREATER,       // 0x21 OP_GREATER
        &&lbl_GREATER_EQUAL, // 0x22 OP_GREATER_EQUAL
        &&lbl_NOT,           // 0x23 OP_NOT
        &&lbl_AND,           // 0x24 OP_AND
        &&lbl_OR,            // 0x25 OP_OR
        &&lbl_NULLISH,       // 0x26 OP_NULLISH
        &&lbl_TYPEOF,        // 0x27 OP_TYPEOF
        &&lbl_INSTANCEOF,    // 0x28 OP_INSTANCEOF
        &&lbl_IN,            // 0x29 OP_IN
        &&lbl_GET_LOCAL,     // 0x2A OP_GET_LOCAL
        &&lbl_SET_LOCAL,     // 0x2B OP_SET_LOCAL
        &&lbl_GET_GLOBAL,    // 0x2C OP_GET_GLOBAL
        &&lbl_SET_GLOBAL,    // 0x2D OP_SET_GLOBAL
        &&lbl_DEFINE_GLOBAL, // 0x2E OP_DEFINE_GLOBAL
        &&lbl_GET_UPVALUE,   // 0x2F OP_GET_UPVALUE
        &&lbl_SET_UPVALUE,   // 0x30 OP_SET_UPVALUE
        &&lbl_CLOSE_UPVALUE, // 0x31 OP_CLOSE_UPVALUE
        &&lbl_GET_PROPERTY,  // 0x32 OP_GET_PROPERTY
        &&lbl_SET_PROPERTY,  // 0x33 OP_SET_PROPERTY
        &&lbl_GET_ELEMENT,   // 0x34 OP_GET_ELEMENT
        &&lbl_SET_ELEMENT,   // 0x35 OP_SET_ELEMENT
        &&lbl_DELETE_PROPERTY, // 0x36 OP_DELETE_PROPERTY
        &&lbl_CREATE_OBJECT, // 0x37 OP_CREATE_OBJECT
        &&lbl_CREATE_ARRAY,  // 0x38 OP_CREATE_ARRAY
        &&lbl_INIT_PROPERTY, // 0x39 OP_INIT_PROPERTY
        &&lbl_INIT_ELEMENT,  // 0x3A OP_INIT_ELEMENT
        &&lbl_SPREAD,        // 0x3B OP_SPREAD
        &&lbl_JUMP,          // 0x3C OP_JUMP
        &&lbl_JUMP_IF_FALSE, // 0x3D OP_JUMP_IF_FALSE
        &&lbl_JUMP_IF_TRUE,  // 0x3E OP_JUMP_IF_TRUE
        &&lbl_JUMP_IF_NIL,   // 0x3F OP_JUMP_IF_NIL
        &&lbl_LOOP,          // 0x40 OP_LOOP
        &&lbl_SWITCH,        // 0x41 OP_SWITCH
        &&lbl_CASE,          // 0x42 OP_CASE
        &&lbl_CALL,          // 0x43 OP_CALL
        &&lbl_CALL_METHOD,   // 0x44 OP_CALL_METHOD
        &&lbl_RETURN,        // 0x45 OP_RETURN
        &&lbl_CLOSURE,       // 0x46 OP_CLOSURE
        &&lbl_NEW,           // 0x47 OP_NEW
        &&lbl_INHERIT,       // 0x48 OP_INHERIT
        &&lbl_DEFINE_METHOD, // 0x49 OP_DEFINE_METHOD
        &&lbl_DEFINE_STATIC, // 0x4A OP_DEFINE_STATIC
        &&lbl_DEFINE_GETTER, // 0x4B OP_DEFINE_GETTER
        &&lbl_DEFINE_SETTER, // 0x4C OP_DEFINE_SETTER
        &&lbl_SUPER_CALL,    // 0x4D OP_SUPER_CALL
        &&lbl_SUPER_GET,     // 0x4E OP_SUPER_GET
        &&lbl_THROW,         // 0x4F OP_THROW
        &&lbl_TRY_BEGIN,     // 0x50 OP_TRY_BEGIN
        &&lbl_TRY_END,       // 0x51 OP_TRY_END
        &&lbl_CATCH,         // 0x52 OP_CATCH
        &&lbl_FINALLY,       // 0x53 OP_FINALLY
        &&lbl_GET_ITERATOR,  // 0x54 OP_GET_ITERATOR
        &&lbl_ITERATOR_NEXT, // 0x55 OP_ITERATOR_NEXT
        &&lbl_FOR_IN,        // 0x56 OP_FOR_IN
        &&lbl_FOR_OF,        // 0x57 OP_FOR_OF
        &&lbl_YIELD,         // 0x58 OP_YIELD
        &&lbl_AWAIT,         // 0x59 OP_AWAIT
        &&lbl_DEBUGGER,      // 0x5A OP_DEBUGGER
        &&lbl_LINE,          // 0x5B OP_LINE
        &&lbl_IMPORT,        // 0x5C OP_IMPORT
        &&lbl_EXPORT,        // 0x5D OP_EXPORT
        &&lbl_IMPORT_BINDING,// 0x5E OP_IMPORT_BINDING
        &&lbl_END,           // 0x5F OP_END
    };

    if (static_cast<uint8_t>(op) <= static_cast<uint8_t>(Opcode::OP_END)) {
        goto *dispatchTable[static_cast<uint8_t>(op)];
    }
    return; // Unknown opcode

    // Label definitions — each maps to the corresponding case body below
    // The actual handler code follows after the #else switch-case block
    lbl_NOP: return;
    // Fall through to switch for handler code — the labels just jump into
    // the same handler blocks. We use a unified approach: labels jump to
    // case handlers via the switch fallthrough trick, but since we can't
    // mix goto labels with case labels cleanly, we use the simpler approach
    // of just calling the switch with the computed op.
    // This gives us the benefit of the jump table for dispatch entry while
    // keeping the switch for the actual handler code.

    // Direct dispatch: jump to switch handler via reentry
#define CGOTO_HANDLER(LABEL, OPCODE) \
    LABEL: goto switch_entry;

    CGOTO_HANDLER(lbl_POP, OP_POP)
    CGOTO_HANDLER(lbl_DUP, OP_DUP)
    CGOTO_HANDLER(lbl_SWAP, OP_SWAP)
    CGOTO_HANDLER(lbl_CONSTANT, OP_CONSTANT)
    CGOTO_HANDLER(lbl_CONSTANT_LONG, OP_CONSTANT_LONG)
    CGOTO_HANDLER(lbl_NIL, OP_NIL)
    CGOTO_HANDLER(lbl_TRUE, OP_TRUE)
    CGOTO_HANDLER(lbl_FALSE, OP_FALSE)
    CGOTO_HANDLER(lbl_ZERO, OP_ZERO)
    CGOTO_HANDLER(lbl_ONE, OP_ONE)
    CGOTO_HANDLER(lbl_ADD, OP_ADD)
    CGOTO_HANDLER(lbl_SUBTRACT, OP_SUBTRACT)
    CGOTO_HANDLER(lbl_MULTIPLY, OP_MULTIPLY)
    CGOTO_HANDLER(lbl_DIVIDE, OP_DIVIDE)
    CGOTO_HANDLER(lbl_MODULO, OP_MODULO)
    CGOTO_HANDLER(lbl_POWER, OP_POWER)
    CGOTO_HANDLER(lbl_NEGATE, OP_NEGATE)
    CGOTO_HANDLER(lbl_INCREMENT, OP_INCREMENT)
    CGOTO_HANDLER(lbl_DECREMENT, OP_DECREMENT)
    CGOTO_HANDLER(lbl_BITWISE_AND, OP_BITWISE_AND)
    CGOTO_HANDLER(lbl_BITWISE_OR, OP_BITWISE_OR)
    CGOTO_HANDLER(lbl_BITWISE_XOR, OP_BITWISE_XOR)
    CGOTO_HANDLER(lbl_BITWISE_NOT, OP_BITWISE_NOT)
    CGOTO_HANDLER(lbl_LEFT_SHIFT, OP_LEFT_SHIFT)
    CGOTO_HANDLER(lbl_RIGHT_SHIFT, OP_RIGHT_SHIFT)
    CGOTO_HANDLER(lbl_UNSIGNED_RIGHT_SHIFT, OP_UNSIGNED_RIGHT_SHIFT)
    CGOTO_HANDLER(lbl_EQUAL, OP_EQUAL)
    CGOTO_HANDLER(lbl_STRICT_EQUAL, OP_STRICT_EQUAL)
    CGOTO_HANDLER(lbl_NOT_EQUAL, OP_NOT_EQUAL)
    CGOTO_HANDLER(lbl_STRICT_NOT_EQUAL, OP_STRICT_NOT_EQUAL)
    CGOTO_HANDLER(lbl_LESS, OP_LESS)
    CGOTO_HANDLER(lbl_LESS_EQUAL, OP_LESS_EQUAL)
    CGOTO_HANDLER(lbl_GREATER, OP_GREATER)
    CGOTO_HANDLER(lbl_GREATER_EQUAL, OP_GREATER_EQUAL)
    CGOTO_HANDLER(lbl_NOT, OP_NOT)
    CGOTO_HANDLER(lbl_AND, OP_AND)
    CGOTO_HANDLER(lbl_OR, OP_OR)
    CGOTO_HANDLER(lbl_NULLISH, OP_NULLISH)
    CGOTO_HANDLER(lbl_TYPEOF, OP_TYPEOF)
    CGOTO_HANDLER(lbl_INSTANCEOF, OP_INSTANCEOF)
    CGOTO_HANDLER(lbl_IN, OP_IN)
    CGOTO_HANDLER(lbl_GET_LOCAL, OP_GET_LOCAL)
    CGOTO_HANDLER(lbl_SET_LOCAL, OP_SET_LOCAL)
    CGOTO_HANDLER(lbl_GET_GLOBAL, OP_GET_GLOBAL)
    CGOTO_HANDLER(lbl_SET_GLOBAL, OP_SET_GLOBAL)
    CGOTO_HANDLER(lbl_DEFINE_GLOBAL, OP_DEFINE_GLOBAL)
    CGOTO_HANDLER(lbl_GET_UPVALUE, OP_GET_UPVALUE)
    CGOTO_HANDLER(lbl_SET_UPVALUE, OP_SET_UPVALUE)
    CGOTO_HANDLER(lbl_CLOSE_UPVALUE, OP_CLOSE_UPVALUE)
    CGOTO_HANDLER(lbl_GET_PROPERTY, OP_GET_PROPERTY)
    CGOTO_HANDLER(lbl_SET_PROPERTY, OP_SET_PROPERTY)
    CGOTO_HANDLER(lbl_GET_ELEMENT, OP_GET_ELEMENT)
    CGOTO_HANDLER(lbl_SET_ELEMENT, OP_SET_ELEMENT)
    CGOTO_HANDLER(lbl_DELETE_PROPERTY, OP_DELETE_PROPERTY)
    CGOTO_HANDLER(lbl_CREATE_OBJECT, OP_CREATE_OBJECT)
    CGOTO_HANDLER(lbl_CREATE_ARRAY, OP_CREATE_ARRAY)
    CGOTO_HANDLER(lbl_INIT_PROPERTY, OP_INIT_PROPERTY)
    CGOTO_HANDLER(lbl_INIT_ELEMENT, OP_INIT_ELEMENT)
    CGOTO_HANDLER(lbl_SPREAD, OP_SPREAD)
    CGOTO_HANDLER(lbl_JUMP, OP_JUMP)
    CGOTO_HANDLER(lbl_JUMP_IF_FALSE, OP_JUMP_IF_FALSE)
    CGOTO_HANDLER(lbl_JUMP_IF_TRUE, OP_JUMP_IF_TRUE)
    CGOTO_HANDLER(lbl_JUMP_IF_NIL, OP_JUMP_IF_NIL)
    CGOTO_HANDLER(lbl_LOOP, OP_LOOP)
    CGOTO_HANDLER(lbl_SWITCH, OP_SWITCH)
    CGOTO_HANDLER(lbl_CASE, OP_CASE)
    CGOTO_HANDLER(lbl_CALL, OP_CALL)
    CGOTO_HANDLER(lbl_CALL_METHOD, OP_CALL_METHOD)
    CGOTO_HANDLER(lbl_RETURN, OP_RETURN)
    CGOTO_HANDLER(lbl_CLOSURE, OP_CLOSURE)
    CGOTO_HANDLER(lbl_NEW, OP_NEW)
    CGOTO_HANDLER(lbl_INHERIT, OP_INHERIT)
    CGOTO_HANDLER(lbl_DEFINE_METHOD, OP_DEFINE_METHOD)
    CGOTO_HANDLER(lbl_DEFINE_STATIC, OP_DEFINE_STATIC)
    CGOTO_HANDLER(lbl_DEFINE_GETTER, OP_DEFINE_GETTER)
    CGOTO_HANDLER(lbl_DEFINE_SETTER, OP_DEFINE_SETTER)
    CGOTO_HANDLER(lbl_SUPER_CALL, OP_SUPER_CALL)
    CGOTO_HANDLER(lbl_SUPER_GET, OP_SUPER_GET)
    CGOTO_HANDLER(lbl_THROW, OP_THROW)
    CGOTO_HANDLER(lbl_TRY_BEGIN, OP_TRY_BEGIN)
    CGOTO_HANDLER(lbl_TRY_END, OP_TRY_END)
    CGOTO_HANDLER(lbl_CATCH, OP_CATCH)
    CGOTO_HANDLER(lbl_FINALLY, OP_FINALLY)
    CGOTO_HANDLER(lbl_GET_ITERATOR, OP_GET_ITERATOR)
    CGOTO_HANDLER(lbl_ITERATOR_NEXT, OP_ITERATOR_NEXT)
    CGOTO_HANDLER(lbl_FOR_IN, OP_FOR_IN)
    CGOTO_HANDLER(lbl_FOR_OF, OP_FOR_OF)
    CGOTO_HANDLER(lbl_YIELD, OP_YIELD)
    CGOTO_HANDLER(lbl_AWAIT, OP_AWAIT)
    CGOTO_HANDLER(lbl_DEBUGGER, OP_DEBUGGER)
    CGOTO_HANDLER(lbl_LINE, OP_LINE)
    CGOTO_HANDLER(lbl_IMPORT, OP_IMPORT)
    CGOTO_HANDLER(lbl_EXPORT, OP_EXPORT)
    CGOTO_HANDLER(lbl_IMPORT_BINDING, OP_IMPORT_BINDING)
    lbl_END: return;

#undef CGOTO_HANDLER
    switch_entry:
#endif // __GNUC__

    switch (op) {
        case Opcode::OP_NOP:
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
                const std::string& name = static_cast<String*>(nameValue.asObject())->value();
                push(getGlobal(name));
            }
            break;
        }
        
        case Opcode::OP_SET_GLOBAL: {
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            if (nameValue.isString()) {
                const std::string& name = static_cast<String*>(nameValue.asObject())->value();
                globals_[name] = peek();
            }
            break;
        }
        
        case Opcode::OP_DEFINE_GLOBAL: {
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            if (nameValue.isString()) {
                const std::string& name = static_cast<String*>(nameValue.asObject())->value();
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
            size_t propSiteIP = ip_ - 1;  // IC key: bytecode offset of this access
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            Value objValue = pop();
            
            if (objValue.isObject()) {
                Object* obj = objValue.asObject();
                if (nameValue.isString()) {
                    std::string name = static_cast<String*>(nameValue.asObject())->value();
                    
                    // Special case: 'length' — common hot path
                    if (name == "length") {
                        push(Value::number(static_cast<double>(obj->length())));
                        break;
                    }
                    
                    // IC fast path: check cached shape → direct slot access
                    InlineCache* ic = icManager_.getIC(propSiteIP);
                    if (ic) {
                        uint32_t cachedOffset;
                        if (ic->lookup(obj->shapeId(), cachedOffset)) {
                            // IC hit — direct slot access, skip hash lookup
                            push(obj->getPropertyBySlot(cachedOffset));
                            break;
                        }
                    }
                    
                    // Slow path: check for accessor descriptor first
                    auto desc = obj->getOwnPropertyDescriptor(name);
                    if (desc && desc->isAccessorDescriptor() && desc->getter.isObject()) {
                        Function* getterFn = dynamic_cast<Function*>(desc->getter.asObject());
                        if (getterFn) {
                            std::vector<Value> args;
                            push(getterFn->call(nullptr, objValue, args));
                            break;
                        }
                    }

                    // Slow path: full property lookup
                    Value result = obj->get(name);
                    push(result);
                    
                    // Update IC with real slot offset for next access
                    if (ic) {
                        int32_t slot = obj->findPropertySlot(name);
                        if (slot >= 0) {
                            ic->update(obj->shapeId(), static_cast<uint32_t>(slot));
                        }
                    }
                } else {
                    push(Value::undefined());
                }
            } else if (objValue.isString()) {
                if (nameValue.isString()) {
                    std::string name = static_cast<String*>(nameValue.asObject())->value();
                    if (name == "length") {
                        String* str = static_cast<String*>(objValue.asObject());
                        push(Value::number(static_cast<double>(str->value().length())));
                    } else {
                        static Object* stringProto = Builtins::StringBuiltin::createStringPrototype(nullptr);
                        Value method = stringProto->get(name);
                        push(method);
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
            size_t setSiteIP = ip_ - 1;  // IC key for set site
            uint8_t nameIdx = readByte();
            Value nameValue = chunk_->constant(nameIdx);
            Value value = pop();
            Value objValue = pop();
            
            if (objValue.isObject()) {
                Object* obj = objValue.asObject();
                if (nameValue.isString()) {
                    const std::string& name = static_cast<String*>(nameValue.asObject())->value();
                    
                    // Check for accessor descriptor (setter)
                    auto desc = obj->getOwnPropertyDescriptor(name);
                    if (desc && desc->isAccessorDescriptor() && desc->setter.isObject()) {
                        Function* setterFn = dynamic_cast<Function*>(desc->setter.asObject());
                        if (setterFn) {
                            std::vector<Value> args = {value};
                            setterFn->call(nullptr, objValue, args);
                        }
                    } else {
                        // IC fast path for set: direct slot access on shape match
                        InlineCache* ic = icManager_.getIC(setSiteIP);
                        uint32_t cachedOffset;
                        if (ic && ic->lookup(obj->shapeId(), cachedOffset)) {
                            obj->setPropertyBySlot(cachedOffset, value);
                        } else {
                            obj->set(name, value);
                            // Cache the slot for next access
                            if (ic) {
                                int32_t slot = obj->findPropertySlot(name);
                                if (slot >= 0) {
                                    ic->update(obj->shapeId(), static_cast<uint32_t>(slot));
                                }
                            }
                        }
                    }
                    
                    // Write barrier: notify GC of potential old→young reference
                    if (value.isObject() && gcHeap_) {
                        gcHeap_->writeBarrier(obj, value.asObject());
                    }
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
                
                // Write barrier: notify GC of potential old→young reference
                if (value.isObject() && gcHeap_) {
                    gcHeap_->writeBarrier(array.asObject(), value.asObject());
                }
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
                if (function->isGenerator()) {
                    // Create GeneratorObject, skip execution
                    // Try to get shared prototype from Context/GlobalObject
                    Value genProtoValue = getGlobal("Generator").isObject() ? 
                        getGlobal("Generator").asObject()->get("prototype") : Value::undefined();
                    
                    auto* genObj = new Builtins::GeneratorObject(function, Value::undefined(), args);
                    if (genProtoValue.isObject()) {
                        genObj->setPrototype(genProtoValue.asObject());
                    }
                    push(Value::object(genObj));
                } else {
                    // Compiled JavaScript function - push call frame and execute bytecode
                    // Uses HYBRID TWO-STACK: native for fast path, heap for deep recursion
                    VMCallFrame frame;
                    frame.function = function;
                    frame.returnAddress = ip_;
                    frame.slotBase = stack_.size();
                    frame.thisValue = Value::undefined();
                    frame.savedChunk = chunk_;
                    pushCallFrame(frame);
                
                    for (const auto& arg : args) {
                        push(arg);
                    }
                
                    chunk_ = function->bytecodeChunk();
                    ip_ = 0;
                }
            } else {
                push(Value::undefined());
            }
            break;
        }
        
        case Opcode::OP_RETURN: {
            Value result = pop();
            
            // If we have call frames, pop one and restore state (HYBRID STACK)
            if (callDepth() > 0) {
                VMCallFrame frame = popCallFrame();  // Hybrid: auto-switches heap/native
                
                // Check for callback sentinel - SIZE_MAX means exit run()
                if (frame.returnAddress == SIZE_MAX) {
                    // Callback return - push result and terminate
                    while (stack_.size() > frame.slotBase) {
                        pop();
                    }
                    push(result);
                    ip_ = chunk_->code().size();  // Exit run() loop
                    break;
                }
                
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
            
            // If method not found on object, lookup on prototype
            if (!method.isObject() || !method.asObject()->isFunction()) {
                if (receiver.isString()) {
                    static Object* stringProto = Builtins::StringBuiltin::createStringPrototype(nullptr);
                    method = stringProto->get(methodName);
                } else if (obj->isArray()) {
                    static Object* arrayProto = Builtins::ArrayBuiltin::createArrayPrototype(nullptr);
                    method = arrayProto->get(methodName);
                }
            }
            
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
                if (function->isGenerator()) {
                    Value genProtoValue = getGlobal("Generator").isObject() ? 
                        getGlobal("Generator").asObject()->get("prototype") : Value::undefined();
                    
                    auto* genObj = new Builtins::GeneratorObject(function, receiver, args);
                    if (genProtoValue.isObject()) {
                        genObj->setPrototype(genProtoValue.asObject());
                    }
                    push(Value::object(genObj));
                } else {
                    // Compiled function - execute with this binding
                    VMCallFrame frame;
                    frame.function = function;
                    frame.returnAddress = ip_;
                    frame.slotBase = stack_.size();
                    frame.thisValue = receiver;
                    frame.savedChunk = chunk_;
                    pushCallFrame(frame);
                
                    for (const auto& arg : args) {
                        push(arg);
                    }
                
                    chunk_ = function->bytecodeChunk();
                    ip_ = 0;
                }
            } else {
                push(Value::undefined());
            }
            break;
        }
            
        case Opcode::OP_CLOSURE: {
            // Read the function constant index
            uint8_t funcIdx = readByte();
            Value funcValue = chunk_->constant(funcIdx);
            
            // Get function object to add upvalues
            Function* fn = funcValue.asObject() ? 
                dynamic_cast<Function*>(funcValue.asObject()) : nullptr;
            
            // Read upvalue count and capture each one
            uint8_t upvalueCount = readByte();
            for (uint8_t i = 0; i < upvalueCount; i++) {
                uint8_t isLocal = readByte();
                uint8_t index = readByte();
                
                if (fn) {
                    RuntimeUpvalue* upvalue;
                    if (isLocal) {
                        // Capture from local stack slot
                        upvalue = captureUpvalue(&stack_[currentFrame().slotBase + index]);
                    } else {
                        // Get from enclosing function's upvalues
                        upvalue = currentFrame().function->upvalue(index);
                    }
                    fn->addUpvalue(upvalue);
                }
            }
            
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
        
        case Opcode::OP_AWAIT: {
            // await expression - unwrap Promise/thenable
            Value awaited = pop();
            
            // Convert to Promise if needed
            Promise* promise = AwaitHandler::toPromise(awaited);
            
            if (promise->state() == PromiseState::Fulfilled) {
                // Already resolved - push result and continue
                push(promise->result());
            } else if (promise->state() == PromiseState::Rejected) {
                // Rejected - throw error
                throw std::runtime_error("Promise rejected: " + promise->result().toString());
            } else {
                // Pending - for synchronous execution, wait/block
                // In full async implementation, this would suspend
                // For now, push undefined and continue
                push(Value::undefined());
            }
            break;
        }
        
        case Opcode::OP_YIELD: {
            // yield expression in generator - suspend execution
            Value yielded = pop();
            uint8_t delegate = readByte();  // 1 = yield*, 0 = yield
            
            if (currentGenerator_) {
                // Save current state for resumption
                currentGenerator_->suspendedIP = ip_;
                currentGenerator_->savedStack.clear();
                size_t base = currentGenerator_->stackBase;
                for (size_t i = base; i < stack_.size(); i++) {
                    currentGenerator_->savedStack.push_back(stack_[i]);
                }
                
                // Store yielded value and set suspension flag
                yieldedValue_ = yielded;
                isYielding_ = true;
                
                // Exit run loop (will be resumed by next() call)
                ip_ = chunk_->code().size();  // Force exit from run()
                
                if (delegate) {
                    // yield* - the yielded value should be an iterator
                    // Delegate to the iterator's next() method
                    // For now, yield the value itself
                }
            } else {
                // Not in generator context - just push value back (fallback)
                push(yielded);
            }
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
        
        case Opcode::OP_GET_ITERATOR: {
            Value iterable = pop();
            
            // Create an iterator object with index tracking
            Object* iterState = new Object();
            iterState->set("__iterable", iterable);
            iterState->set("__index", Value::number(0));
            
            if (iterable.isObject()) {
                Object* obj = iterable.asObject();
                // For Map, copy entries for iteration
                if (auto* mapObj = dynamic_cast<Builtins::MapObject*>(obj)) {
                    auto entries = mapObj->entries();
                    Array* arr = new Array({});
                    for (const auto& e : entries) {
                        Array* pair = new Array({e.first, e.second});
                        arr->push(Value::object(pair));
                    }
                    iterState->set("__entries", Value::object(arr));
                    iterState->set("__length", Value::number(static_cast<double>(entries.size())));
                }
                // For Set, copy values
                else if (auto* setObj = dynamic_cast<Builtins::SetObject*>(obj)) {
                    auto vals = setObj->values();
                    Array* arr = new Array(std::move(vals));
                    iterState->set("__entries", Value::object(arr));
                    iterState->set("__length", Value::number(static_cast<double>(setObj->size())));
                }
                // Arrays and strings use __iterable + __index directly
                else {
                    iterState->set("__length", Value::number(static_cast<double>(obj->length())));
                }
            } else if (iterable.isString()) {
                String* str = static_cast<String*>(iterable.asObject());
                iterState->set("__length", Value::number(static_cast<double>(str->value().length())));
            }
            
            push(Value::object(iterState));
            break;
        }
        
        case Opcode::OP_ITERATOR_NEXT: {
            Value iterValue = peek();
            
            if (!iterValue.isObject()) {
                // Done — push {value: undefined, done: true}
                Object* result = new Object();
                result->set("value", Value::undefined());
                result->set("done", Value::boolean(true));
                push(Value::object(result));
                break;
            }
            
            Object* iterState = iterValue.asObject();
            Value idxVal = iterState->get("__index");
            Value lenVal = iterState->get("__length");
            
            size_t idx = static_cast<size_t>(idxVal.isNumber() ? idxVal.asNumber() : 0);
            size_t len = static_cast<size_t>(lenVal.isNumber() ? lenVal.asNumber() : 0);
            
            Object* result = new Object();
            
            if (idx >= len) {
                result->set("value", Value::undefined());
                result->set("done", Value::boolean(true));
            } else {
                // Get next value from entries array or iterable
                Value entriesVal = iterState->get("__entries");
                if (entriesVal.isObject()) {
                    // Map/Set iterator — values stored in __entries array
                    result->set("value", entriesVal.asObject()->get(idx));
                } else {
                    // Array/String — use __iterable directly
                    Value iterable = iterState->get("__iterable");
                    if (iterable.isObject()) {
                        result->set("value", iterable.asObject()->get(idx));
                    } else if (iterable.isString()) {
                        // Character iteration for strings
                        String* str = static_cast<String*>(iterable.asObject());
                        std::string ch(1, str->value()[idx]);
                        result->set("value", Value::string(new String(ch)));
                    }
                }
                result->set("done", Value::boolean(false));
                
                // Advance index
                iterState->set("__index", Value::number(static_cast<double>(idx + 1)));
            }
            
            push(Value::object(result));
            break;
        }

        case Opcode::OP_SPREAD: {
            // Spread operator - expand array/iterable onto stack
            Value iterable = pop();
            
            if (iterable.isObject()) {
                Object* obj = iterable.asObject();
                if (auto* arr = dynamic_cast<Array*>(obj)) {
                    // Spread array elements onto stack
                    for (size_t i = 0; i < arr->length(); i++) {
                        push(arr->get(i));
                    }
                } else {
                    // For other objects, try to iterate via Symbol.iterator
                    // For now, just push the object itself
                    push(iterable);
                }
            } else {
                // Non-iterable - just push the value
                push(iterable);
            }
            break;
        }
        
        case Opcode::OP_NEW: {
            uint8_t argCount = readByte();
            
            // Get the constructor (below all arguments)
            Value callee = peek(argCount);
            
            if (!callee.isObject()) {
                throw std::runtime_error("new requires a constructor");
            }
            
            Object* calleeObj = callee.asObject();
            if (!calleeObj->isFunction()) {
                throw std::runtime_error("new requires a constructor function");
            }
            
            Function* constructor = static_cast<Function*>(calleeObj);
            
            // Check if function is a valid constructor (not arrow function)
            if (!constructor->isConstructor()) {
                throw std::runtime_error(constructor->name() + " is not a constructor");
            }
            
            // Collect arguments from stack
            std::vector<Value> args;
            args.reserve(argCount);
            for (int i = argCount - 1; i >= 0; i--) {
                args.push_back(peek(i));
            }
            
            // Pop arguments and constructor from stack
            popN(argCount + 1);
            
            // Create new object with constructor's prototype
            Object* newObj = new Object();
            Value prototypeVal = constructor->get("prototype");
            if (prototypeVal.isObject()) {
                newObj->setPrototype(prototypeVal.asObject());
            }
            
            // Execute the constructor with new object as 'this'
            Value result;
            
            if (constructor->isBuiltin()) {
                FunctionCallInfo info(context_, Value::object(newObj), args);
                result = constructor->builtinFunction()(info);
            } else if (constructor->isNative()) {
                result = constructor->nativeFunction()(context_, args);
            } else if (constructor->isCompiled()) {
                // Push call frame for compiled constructor
                VMCallFrame frame;
                frame.function = constructor;
                frame.returnAddress = ip_;
                frame.slotBase = stack_.size();
                frame.thisValue = Value::object(newObj);
                frame.savedChunk = chunk_;
                pushCallFrame(frame);
                
                // Setup locals: push 'this' first, then arguments
                push(Value::object(newObj));
                for (const auto& arg : args) {
                    push(arg);
                }
                
                // Switch to constructor's bytecode
                chunk_ = constructor->bytecodeChunk();
                ip_ = 0;
                
                // The main loop continues with constructor's bytecode
                // OP_RETURN will push the result
                break;
            } else {
                // AST-based function - call construct method
                result = constructor->construct(context_, args);
            }
            
            // If constructor returned an object, use that; otherwise use newObj
            if (result.isObject()) {
                push(result);
            } else {
                push(Value::object(newObj));
            }
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
        
        case Opcode::OP_INSTANCEOF: {
            Value constructorVal = pop();
            Value instance = pop();
            
            // Right-hand side must be callable
            if (!constructorVal.isObject()) {
                throw std::runtime_error("Right-hand side of 'instanceof' is not an object");
            }
            
            Object* constructorObj = constructorVal.asObject();
            if (!constructorObj->isFunction()) {
                throw std::runtime_error("Right-hand side of 'instanceof' is not callable");
            }
            
            Function* constructor = static_cast<Function*>(constructorObj);
            
            // Check for custom @@hasInstance (Symbol.hasInstance)
            Value hasInstanceSym = constructor->get("@@hasInstance");
            if (hasInstanceSym.isObject() && hasInstanceSym.asObject()->isFunction()) {
                Function* hasInstanceFn = static_cast<Function*>(hasInstanceSym.asObject());
                FunctionCallInfo info(context_, constructorVal, {instance});
                Value result = hasInstanceFn->builtinFunction()(info);
                push(Value::boolean(result.toBoolean()));
                break;
            }
            
            // Standard instanceof: check prototype chain
            Value prototypeVal = constructor->get("prototype");
            if (!prototypeVal.isObject()) {
                throw std::runtime_error("Function has non-object prototype property");
            }
            Object* prototype = prototypeVal.asObject();
            
            // Primitives are never instanceof anything
            if (!instance.isObject()) {
                push(Value::boolean(false));
                break;
            }
            
            // Walk the prototype chain
            Object* obj = instance.asObject()->prototype();
            while (obj != nullptr) {
                if (obj == prototype) {
                    push(Value::boolean(true));
                    break;
                }
                obj = obj->prototype();
            }
            
            // If we exited the loop without finding, result is false
            if (obj == nullptr) {
                push(Value::boolean(false));
            }
            break;
        }
            
        // Exceptions
        case Opcode::OP_THROW: {
            // Pop the exception value
            exceptionValue_ = pop();
            hasException_ = true;
            
            // Capture stack trace on the error object if it's an Object
            if (exceptionValue_.isObject()) {
                Object* errObj = exceptionValue_.asObject();
                std::string stackStr;
                
                // Get error name and message for first line
                Value nameVal = errObj->get("name");
                Value msgVal = errObj->get("message");
                if (nameVal.isString()) {
                    stackStr += static_cast<String*>(nameVal.asObject())->value();
                } else {
                    stackStr += "Error";
                }
                if (msgVal.isString()) {
                    stackStr += ": " + static_cast<String*>(msgVal.asObject())->value();
                }
                stackStr += "\n";
                
                // Walk call stack frames
                for (int i = static_cast<int>(heapStack_.size()) - 1; i >= 0; --i) {
                    const auto& frame = heapStack_[i];
                    stackStr += "    at ";
                    if (frame.function) {
                        stackStr += frame.function->name().empty() ? "<anonymous>" : frame.function->name();
                    } else {
                        stackStr += "<global>";
                    }
                    stackStr += " (bytecode:" + std::to_string(frame.returnAddress) + ")\n";
                }
                
                errObj->set("stack", Value::string(new String(stackStr)));
            }
            
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
            
            // Use ModuleLoader if available
            if (moduleLoader_) {
                Value exports = moduleLoader_->loadModule(modulePath, currentModulePath_);
                push(exports);
            } else {
                // Fallback: create empty exports object
                Object* exports = new Object();
                push(Value::object(exports));
            }
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
            readByte(); // consume binding name operand
            break;
        }
        case Opcode::OP_DEBUGGER: {
            // Debugger statement — pauses execution if debug callback is set
            break;
        }
        // Class operations
        case Opcode::OP_INHERIT: {
            Value superclass = pop();
            Value subclass = peek(0);
            if (!superclass.isObject() || !subclass.isObject()) {
                throw std::runtime_error("Superclass and subclass must be objects");
            }
            if (!superclass.asObject()->isFunction() || !subclass.asObject()->isFunction()) {
                throw std::runtime_error("Superclass and subclass must be constructors");
            }
            // Setup prototype chain: subclass.prototype.__proto__ = superclass.prototype
            Value superProto = superclass.asObject()->get("prototype");
            Value subProto = subclass.asObject()->get("prototype");
            if (subProto.isObject() && superProto.isObject()) {
                subProto.asObject()->setPrototype(superProto.asObject());
            }
            break;
        }
        case Opcode::OP_DEFINE_METHOD:
        case Opcode::OP_DEFINE_STATIC:
        case Opcode::OP_DEFINE_GETTER:
        case Opcode::OP_DEFINE_SETTER: {
            uint8_t nameIdx = readByte();
            Value methodName = chunk_->constant(nameIdx);
            Value methodFunc = pop();
            Value classConstructor = peek(0);
            
            if (!methodName.isString() || !methodFunc.isObject() || !classConstructor.isObject()) {
                throw std::runtime_error("Invalid operands for define method/getter/setter");
            }
            
            String* nameStr = static_cast<String*>(methodName.asObject());
            Object* targetObj = classConstructor.asObject();
            
            if (op != Opcode::OP_DEFINE_STATIC) {
                // Add to prototype instead of constructor directly
                Value proto = targetObj->get("prototype");
                if (proto.isObject()) {
                    targetObj = proto.asObject();
                }
            }
            
            if (op == Opcode::OP_DEFINE_GETTER) {
                PropertyDescriptor desc;
                desc.getter = methodFunc;
                desc.attributes = PropertyAttribute::Configurable | PropertyAttribute::Enumerable;
                targetObj->defineProperty(nameStr->value(), desc);
            } else if (op == Opcode::OP_DEFINE_SETTER) {
                PropertyDescriptor desc;
                desc.setter = methodFunc;
                desc.attributes = PropertyAttribute::Configurable | PropertyAttribute::Enumerable;
                targetObj->defineProperty(nameStr->value(), desc);
            } else {
                targetObj->set(nameStr->value(), methodFunc);
            }
            break;
        }
        case Opcode::OP_SUPER_CALL: {
            uint8_t argCount = readByte();
            Value superclass = pop();
            Value thisValue = pop();
            
            if (!superclass.isObject() || !superclass.asObject()->isFunction()) {
                throw std::runtime_error("Superclass is not a constructor");
            }
            
            Function* superConstructor = static_cast<Function*>(superclass.asObject());
            
            std::vector<Value> args;
            args.reserve(argCount);
            for (int i = 0; i < argCount; i++) {
                args.push_back(pop());
            }
            // args were popped in reverse order, reverse them back
            std::reverse(args.begin(), args.end());
            
            // Execute super constructor with 'this' value
            if (superConstructor->isBuiltin()) {
                FunctionCallInfo info(context_, thisValue, args);
                push(superConstructor->builtinFunction()(info));
            } else if (superConstructor->isNative()) {
                push(superConstructor->nativeFunction()(context_, args));
            } else if (superConstructor->isCompiled()) {
                VMCallFrame frame;
                frame.function = superConstructor;
                frame.returnAddress = ip_;
                frame.slotBase = stack_.size();
                frame.thisValue = thisValue;
                frame.savedChunk = chunk_;
                pushCallFrame(frame);
                
                push(thisValue);
                for (const auto& arg : args) {
                    push(arg);
                }
                
                chunk_ = superConstructor->bytecodeChunk();
                ip_ = 0;
            } else {
                push(Value::undefined());
            }
            break;
        }
        case Opcode::OP_FOR_IN: {
            Value obj = pop();
            if (obj.isObject()) {
                std::vector<std::string> keysStr = obj.asObject()->keys();
                std::vector<Value> keysVal;
                keysVal.reserve(keysStr.size());
                for (const auto& k : keysStr) {
                    keysVal.push_back(Value::string(new String(k)));
                }
                Array* keysArray = new Array(keysVal);
                push(Value::object(keysArray));
            } else {
                push(Value::object(new Array()));
            }
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
    
    // Use executeCallback for proper execution
    return executeCallback(function, thisValue, args);
}

// Execute callback for array methods - handles native, builtin, and compiled functions
Value VM::executeCallback(Function* fn, Value thisValue, const std::vector<Value>& args) {
    if (!fn) {
        return Value::undefined();
    }
    
    // Native function (simple signature)
    if (fn->isNative()) {
        return fn->nativeFunction()(context_, args);
    }
    
    // Builtin function (with FunctionCallInfo)
    if (fn->isBuiltin()) {
        FunctionCallInfo info(context_, thisValue, args);
        return fn->builtinFunction()(info);
    }
    
    // Compiled bytecode function - execute using this VM
    if (fn->isCompiled() && fn->bytecodeChunk()) {
        // Save VM state
        const Bytecode::BytecodeChunk* savedChunk = chunk_;
        size_t savedIP = ip_;
        size_t savedStackBase = stack_.size();
        
        // Push arguments onto stack (they become local slots 0, 1, 2...)
        for (const auto& arg : args) {
            push(arg);
        }
        
        // Push a call frame with sentinel return address to signal callback
        // SIZE_MAX means "exit run() on return"
        VMCallFrame frame;
        frame.function = fn;
        frame.returnAddress = SIZE_MAX;  // Sentinel: callback exit
        frame.slotBase = savedStackBase;  // Args start here
        frame.thisValue = thisValue;
        frame.savedChunk = savedChunk;
        pushCallFrame(frame);
        
        // Execute the function's bytecode
        chunk_ = fn->bytecodeChunk();
        ip_ = 0;
        
        try {
            run();
        } catch (const std::exception&) {
            // Restore state on error
            popCallFrame();
            chunk_ = savedChunk;
            ip_ = savedIP;
            while (stack_.size() > savedStackBase) pop();
            return Value::undefined();
        }
        
        // Pop the call frame
        popCallFrame();
        
        // Get return value (should be on stack)
        Value result = stack_.size() > savedStackBase ? pop() : Value::undefined();
        
        // Clean up any remaining stack values from function
        while (stack_.size() > savedStackBase) pop();
        
        // Restore VM state
        chunk_ = savedChunk;
        ip_ = savedIP;
        
        return result;
    }
    
    return Value::undefined();
}

Value VM::construct(Function* constructor, const std::vector<Value>& args) {
    if (!constructor) {
        return Value::undefined();
    }

    if (!constructor->isConstructor()) {
        throw std::runtime_error(constructor->name() + " is not a constructor");
    }

    // Create new object with constructor's prototype
    Object* newObj = new Object();
    Value prototypeVal = constructor->get("prototype");
    if (prototypeVal.isObject()) {
        newObj->setPrototype(prototypeVal.asObject());
    }

    // Call constructor with newObj as 'this'
    Value result = executeCallback(constructor, Value::object(newObj), args);

    // If constructor returned an object, use that; otherwise use newObj
    return result.isObject() ? result : Value::object(newObj);
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

Value VM::resumeGenerator(GeneratorFrame* frame, Value yieldVal, const std::vector<Value>& args) {
    if (!frame || frame->isCompleted) {
        return Value::undefined();
    }
    
    // Save current VM execution state
    GeneratorFrame* savedGen = currentGenerator_;
    const Bytecode::BytecodeChunk* savedChunk = chunk_;
    size_t savedIP = ip_;
    
    currentGenerator_ = frame;
    isYielding_ = false;
    yieldedValue_ = Value::undefined();
    
    chunk_ = frame->function->bytecodeChunk();
    
    if (!frame->isStarted) {
        frame->isStarted = true;
        ip_ = 0;
        frame->stackBase = stack_.size();
        
        // Push initial args as locals
        for (const auto& arg : args) {
            push(arg);
        }
    } else {
        // Restore stack and IP
        ip_ = frame->suspendedIP;
        for (const auto& val : frame->savedStack) {
            push(val);
        }
        // Yield expression result is the value passed to next()
        push(yieldVal);
    }
    
    // Push call boundary marker
    VMCallFrame callFrame;
    callFrame.function = frame->function;
    callFrame.returnAddress = SIZE_MAX; // Custom marker to exit run() on return
    callFrame.slotBase = stack_.size() - frame->savedStack.size() - args.size(); 
    callFrame.thisValue = frame->thisValue;
    callFrame.savedChunk = savedChunk;
    pushCallFrame(callFrame);
    
    // Execute until yield or return
    run();
    
    // If we completed without yielding, it was a return
    if (!isYielding_) {
        frame->isCompleted = true;
        // Result is at top of stack (left by OP_RETURN overriding SIZE_MAX)
        yieldedValue_ = pop();
    }
    
    // Restore VM state
    currentGenerator_ = savedGen;
    chunk_ = savedChunk;
    ip_ = savedIP;
    
    return yieldedValue_;
}

} // namespace Zepra::Runtime
