// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — interpreter_dispatch.cpp — Threaded bytecode dispatch engine

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <vector>
#include <functional>

namespace Zepra::Interpreter {

enum class Opcode : uint8_t {
    Nop = 0,
    LoadConst,        // R(A) = K(Bx)
    LoadNull,         // R(A) = null
    LoadUndefined,    // R(A) = undefined
    LoadTrue,         // R(A) = true
    LoadFalse,        // R(A) = false
    LoadInt,          // R(A) = sBx (signed immediate)
    Move,             // R(A) = R(B)

    // Arithmetic.
    Add,              // R(A) = R(B) + R(C)
    Sub,              // R(A) = R(B) - R(C)
    Mul,              // R(A) = R(B) * R(C)
    Div,              // R(A) = R(B) / R(C)
    Mod,              // R(A) = R(B) % R(C)
    Exp,              // R(A) = R(B) ** R(C)
    Neg,              // R(A) = -R(B)
    Inc,              // R(A) = R(A) + 1
    Dec,              // R(A) = R(A) - 1

    // Bitwise.
    BitAnd,           // R(A) = R(B) & R(C)
    BitOr,            // R(A) = R(B) | R(C)
    BitXor,           // R(A) = R(B) ^ R(C)
    BitNot,           // R(A) = ~R(B)
    Shl,              // R(A) = R(B) << R(C)
    Shr,              // R(A) = R(B) >> R(C)
    Ushr,             // R(A) = R(B) >>> R(C)

    // Comparison.
    Equal,            // R(A) = R(B) == R(C)
    StrictEqual,      // R(A) = R(B) === R(C)
    LessThan,         // R(A) = R(B) < R(C)
    LessEqual,        // R(A) = R(B) <= R(C)
    GreaterThan,      // R(A) = R(B) > R(C)
    GreaterEqual,     // R(A) = R(B) >= R(C)

    // Logical.
    Not,              // R(A) = !R(B)
    TypeOf,           // R(A) = typeof R(B)
    InstanceOf,       // R(A) = R(B) instanceof R(C)
    In,               // R(A) = R(B) in R(C)

    // Control flow.
    Jump,             // PC += sBx
    JumpIfTrue,       // if R(A) then PC += sBx
    JumpIfFalse,      // if !R(A) then PC += sBx
    JumpIfNull,       // if R(A) == null then PC += sBx
    JumpIfUndefined,  // if R(A) === undefined then PC += sBx

    // Function.
    Call,             // R(A) = R(B)(R(B+1), ..., R(B+C-1))
    TailCall,         // return R(B)(R(B+1), ..., R(B+C-1))
    Return,           // return R(A)
    ReturnUndefined,  // return undefined

    // Object/Property.
    GetProperty,      // R(A) = R(B)[K(C)]
    SetProperty,      // R(A)[K(B)] = R(C)
    GetElement,       // R(A) = R(B)[R(C)]
    SetElement,       // R(A)[R(B)] = R(C)
    DeleteProperty,   // R(A) = delete R(B)[K(C)]
    HasProperty,      // R(A) = K(C) in R(B)

    // Object creation.
    NewObject,        // R(A) = {}
    NewArray,         // R(A) = [] (B elements)
    NewFunction,      // R(A) = function(K(Bx))
    NewClass,         // R(A) = class(K(Bx))

    // Variable access.
    GetLocal,         // R(A) = locals[B]
    SetLocal,         // locals[A] = R(B)
    GetUpvalue,       // R(A) = upvalues[B]
    SetUpvalue,       // upvalues[A] = R(B)
    GetGlobal,        // R(A) = globals[K(Bx)]
    SetGlobal,        // globals[K(Bx)] = R(A)

    // Scope.
    PushScope,
    PopScope,
    PushWith,         // with(R(A))
    PopWith,

    // Iterator.
    GetIterator,      // R(A) = R(B)[Symbol.iterator]()
    IteratorNext,     // R(A) = R(B).next()
    IteratorDone,     // R(A) = R(B).done

    // Async.
    Await,            // R(A) = await R(B)
    Yield,            // yield R(A)
    YieldStar,        // yield* R(A)
    AsyncReturn,      // async return R(A)

    // Exception.
    Throw,            // throw R(A)
    PushTry,          // try { ... } catch at sBx
    PopTry,
    PushFinally,      // finally at sBx
    GetException,     // R(A) = caught exception

    // Spread/Rest.
    Spread,           // ...R(A)
    CreateRestArray,  // R(A) = [R(B), ..., R(B+C-1)]

    // Destructuring.
    DestructureArray,
    DestructureObject,
    SetDefaultValue,  // R(A) = R(A) === undefined ? R(B) : R(A)

    // Class.
    DefineMethod,
    DefineGetter,
    DefineSetter,
    SuperCall,        // R(A) = super(R(B+1), ..., R(B+C-1))
    SuperGet,         // R(A) = super[K(B)]

    // Module.
    ImportModule,
    ExportValue,
    ImportDynamic,    // R(A) = import(R(B))

    // Debug.
    Debugger,         // debugger statement
    SourceMap,        // source location for stack traces

    // Template literals.
    TemplateLiteral,  // R(A) = template`...`
    TaggedTemplate,   // R(A) = R(B)`...`

    // Nullish.
    NullishCoalesce,  // R(A) = R(B) ?? R(C)
    OptionalChain,    // R(A) = R(B)?.R(C)

    OpcodeCount,
};

struct Instruction {
    Opcode opcode;
    uint8_t a;         // Register A
    uint8_t b;         // Register B
    uint8_t c;         // Register C / flags
    int32_t sbx;       // Signed extended operand
    uint32_t bx;       // Unsigned extended operand

    Instruction() : opcode(Opcode::Nop), a(0), b(0), c(0), sbx(0), bx(0) {}
};

// Value slot — 8-byte NaN-boxed JS value.
union ValueSlot {
    double asDouble;
    uint64_t asBits;
    void* asPtr;
};

static constexpr size_t kMaxRegisters = 256;
static constexpr size_t kMaxCallDepth = 1024;

struct CallFrame {
    const Instruction* code;
    size_t codeLength;
    size_t ip;
    uint8_t baseReg;
    void* functionObj;
    void* scope;
    const CallFrame* caller;
    bool isTailCall;
    bool isAsync;

    CallFrame() : code(nullptr), codeLength(0), ip(0), baseReg(0)
        , functionObj(nullptr), scope(nullptr), caller(nullptr)
        , isTailCall(false), isAsync(false) {}
};

struct DispatchStats {
    uint64_t instructionsExecuted;
    uint64_t calls;
    uint64_t returns;
    uint64_t branches;
    uint64_t exceptions;
    uint64_t propertyAccesses;
    uint64_t allocations;

    DispatchStats() : instructionsExecuted(0), calls(0), returns(0), branches(0)
        , exceptions(0), propertyAccesses(0), allocations(0) {}
};

class BytecodeDispatch {
public:
    using ExternalCall = std::function<ValueSlot(void* fn, ValueSlot* args, uint8_t argc)>;
    using PropertyGet = std::function<ValueSlot(ValueSlot obj, uint32_t nameId)>;
    using PropertySet = std::function<void(ValueSlot obj, uint32_t nameId, ValueSlot val)>;
    using AllocObject = std::function<ValueSlot()>;
    using AllocArray = std::function<ValueSlot(uint32_t length)>;
    using ThrowHandler = std::function<bool(ValueSlot exception, CallFrame& frame)>;

    struct Callbacks {
        ExternalCall externalCall;
        PropertyGet propertyGet;
        PropertySet propertySet;
        AllocObject allocObject;
        AllocArray allocArray;
        ThrowHandler throwHandler;
    };

    BytecodeDispatch() : callDepth_(0) {
        memset(registers_, 0, sizeof(registers_));
    }

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    // Main dispatch loop — computed goto for GCC/Clang, switch fallback.
    ValueSlot execute(const Instruction* code, size_t length) {
        CallFrame frame;
        frame.code = code;
        frame.codeLength = length;
        frame.ip = 0;

        return dispatchLoop(frame);
    }

    const DispatchStats& stats() const { return stats_; }
    size_t callDepth() const { return callDepth_; }

private:
    ValueSlot dispatchLoop(CallFrame& frame) {
        ValueSlot* R = registers_;

#if defined(__GNUC__) || defined(__clang__)
        // Threaded dispatch via computed goto.
        static const void* dispatchTable[] = {
            &&L_Nop, &&L_LoadConst, &&L_LoadNull, &&L_LoadUndefined,
            &&L_LoadTrue, &&L_LoadFalse, &&L_LoadInt, &&L_Move,
            &&L_Add, &&L_Sub, &&L_Mul, &&L_Div, &&L_Mod, &&L_Exp,
            &&L_Neg, &&L_Inc, &&L_Dec,
            &&L_BitAnd, &&L_BitOr, &&L_BitXor, &&L_BitNot,
            &&L_Shl, &&L_Shr, &&L_Ushr,
            &&L_Equal, &&L_StrictEqual, &&L_LessThan, &&L_LessEqual,
            &&L_GreaterThan, &&L_GreaterEqual,
            &&L_Not, &&L_TypeOf, &&L_InstanceOf, &&L_In,
            &&L_Jump, &&L_JumpIfTrue, &&L_JumpIfFalse,
            &&L_JumpIfNull, &&L_JumpIfUndefined,
            &&L_Call, &&L_TailCall, &&L_Return, &&L_ReturnUndefined,
            &&L_GetProperty, &&L_SetProperty, &&L_GetElement, &&L_SetElement,
            &&L_DeleteProperty, &&L_HasProperty,
            &&L_NewObject, &&L_NewArray, &&L_NewFunction, &&L_NewClass,
            &&L_GetLocal, &&L_SetLocal, &&L_GetUpvalue, &&L_SetUpvalue,
            &&L_GetGlobal, &&L_SetGlobal,
            &&L_PushScope, &&L_PopScope, &&L_PushWith, &&L_PopWith,
            &&L_GetIterator, &&L_IteratorNext, &&L_IteratorDone,
            &&L_Await, &&L_Yield, &&L_YieldStar, &&L_AsyncReturn,
            &&L_Throw, &&L_PushTry, &&L_PopTry, &&L_PushFinally,
            &&L_GetException,
            &&L_Spread, &&L_CreateRestArray,
            &&L_DestructureArray, &&L_DestructureObject, &&L_SetDefaultValue,
            &&L_DefineMethod, &&L_DefineGetter, &&L_DefineSetter,
            &&L_SuperCall, &&L_SuperGet,
            &&L_ImportModule, &&L_ExportValue, &&L_ImportDynamic,
            &&L_Debugger, &&L_SourceMap,
            &&L_TemplateLiteral, &&L_TaggedTemplate,
            &&L_NullishCoalesce, &&L_OptionalChain,
        };

#define DISPATCH() do { \
    if (__builtin_expect(frame.ip >= frame.codeLength, 0)) goto L_ReturnUndefined; \
    goto *dispatchTable[static_cast<uint8_t>(frame.code[frame.ip].opcode)]; \
} while(0)

#define NEXT() frame.ip++; DISPATCH()

        DISPATCH();

L_Nop: NEXT();

L_LoadConst: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asBits = instr.bx;
        NEXT();
}

L_LoadNull:
        R[frame.code[frame.ip].a].asBits = 0;
        NEXT();

L_LoadUndefined:
        R[frame.code[frame.ip].a].asBits = 0x7FF8000000000001ULL;
        NEXT();

L_LoadTrue:
        R[frame.code[frame.ip].a].asBits = 0x7FF8000000000002ULL;
        NEXT();

L_LoadFalse:
        R[frame.code[frame.ip].a].asBits = 0x7FF8000000000003ULL;
        NEXT();

L_LoadInt: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = static_cast<double>(instr.sbx);
        NEXT();
}

L_Move: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a] = R[instr.b];
        NEXT();
}

L_Add: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = R[instr.b].asDouble + R[instr.c].asDouble;
        NEXT();
}

L_Sub: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = R[instr.b].asDouble - R[instr.c].asDouble;
        NEXT();
}

L_Mul: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = R[instr.b].asDouble * R[instr.c].asDouble;
        NEXT();
}

L_Div: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = R[instr.b].asDouble / R[instr.c].asDouble;
        NEXT();
}

L_Mod: {
        const auto& instr = frame.code[frame.ip];
        double b = R[instr.b].asDouble;
        double c = R[instr.c].asDouble;
        R[instr.a].asDouble = b - static_cast<int64_t>(b / c) * c;
        NEXT();
}

L_Exp: {
        const auto& instr = frame.code[frame.ip];
        double base = R[instr.b].asDouble;
        double exp = R[instr.c].asDouble;
        double result = 1.0;
        if (exp == 0.0) { result = 1.0; }
        else {
            int64_t iexp = static_cast<int64_t>(exp);
            if (static_cast<double>(iexp) == exp && iexp >= 0) {
                result = 1.0;
                double b = base;
                while (iexp > 0) {
                    if (iexp & 1) result *= b;
                    b *= b;
                    iexp >>= 1;
                }
            } else {
                result = __builtin_pow(base, exp);
            }
        }
        R[instr.a].asDouble = result;
        NEXT();
}

L_Neg: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = -R[instr.b].asDouble;
        NEXT();
}

L_Inc:
        R[frame.code[frame.ip].a].asDouble += 1.0;
        NEXT();

L_Dec:
        R[frame.code[frame.ip].a].asDouble -= 1.0;
        NEXT();

L_BitAnd: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = static_cast<double>(
            static_cast<int32_t>(R[instr.b].asDouble) &
            static_cast<int32_t>(R[instr.c].asDouble));
        NEXT();
}

L_BitOr: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = static_cast<double>(
            static_cast<int32_t>(R[instr.b].asDouble) |
            static_cast<int32_t>(R[instr.c].asDouble));
        NEXT();
}

L_BitXor: {
        const auto& instr = frame.code[frame.ip];
        R[instr.a].asDouble = static_cast<double>(
            static_cast<int32_t>(R[instr.b].asDouble) ^
            static_cast<int32_t>(R[instr.c].asDouble));
        NEXT();
}

L_BitNot:
        R[frame.code[frame.ip].a].asDouble = static_cast<double>(
            ~static_cast<int32_t>(R[frame.code[frame.ip].b].asDouble));
        NEXT();

L_Shl:
        R[frame.code[frame.ip].a].asDouble = static_cast<double>(
            static_cast<int32_t>(R[frame.code[frame.ip].b].asDouble) <<
            (static_cast<uint32_t>(R[frame.code[frame.ip].c].asDouble) & 0x1F));
        NEXT();

L_Shr:
        R[frame.code[frame.ip].a].asDouble = static_cast<double>(
            static_cast<int32_t>(R[frame.code[frame.ip].b].asDouble) >>
            (static_cast<uint32_t>(R[frame.code[frame.ip].c].asDouble) & 0x1F));
        NEXT();

L_Ushr:
        R[frame.code[frame.ip].a].asDouble = static_cast<double>(
            static_cast<uint32_t>(R[frame.code[frame.ip].b].asDouble) >>
            (static_cast<uint32_t>(R[frame.code[frame.ip].c].asDouble) & 0x1F));
        NEXT();

L_Equal:
        // Loose equality — fast path for same-type.
        R[frame.code[frame.ip].a].asBits =
            (R[frame.code[frame.ip].b].asBits == R[frame.code[frame.ip].c].asBits)
            ? 0x7FF8000000000002ULL : 0x7FF8000000000003ULL;
        NEXT();

L_StrictEqual:
        R[frame.code[frame.ip].a].asBits =
            (R[frame.code[frame.ip].b].asBits == R[frame.code[frame.ip].c].asBits)
            ? 0x7FF8000000000002ULL : 0x7FF8000000000003ULL;
        NEXT();

L_LessThan:
        R[frame.code[frame.ip].a].asBits =
            (R[frame.code[frame.ip].b].asDouble < R[frame.code[frame.ip].c].asDouble)
            ? 0x7FF8000000000002ULL : 0x7FF8000000000003ULL;
        NEXT();

L_LessEqual:
        R[frame.code[frame.ip].a].asBits =
            (R[frame.code[frame.ip].b].asDouble <= R[frame.code[frame.ip].c].asDouble)
            ? 0x7FF8000000000002ULL : 0x7FF8000000000003ULL;
        NEXT();

L_GreaterThan:
        R[frame.code[frame.ip].a].asBits =
            (R[frame.code[frame.ip].b].asDouble > R[frame.code[frame.ip].c].asDouble)
            ? 0x7FF8000000000002ULL : 0x7FF8000000000003ULL;
        NEXT();

L_GreaterEqual:
        R[frame.code[frame.ip].a].asBits =
            (R[frame.code[frame.ip].b].asDouble >= R[frame.code[frame.ip].c].asDouble)
            ? 0x7FF8000000000002ULL : 0x7FF8000000000003ULL;
        NEXT();

L_Not:
        // !value — check for falsy (false, 0, null, undefined, NaN, "")
        R[frame.code[frame.ip].a].asBits =
            (R[frame.code[frame.ip].b].asBits == 0 ||
             R[frame.code[frame.ip].b].asBits == 0x7FF8000000000001ULL ||
             R[frame.code[frame.ip].b].asBits == 0x7FF8000000000003ULL)
            ? 0x7FF8000000000002ULL : 0x7FF8000000000003ULL;
        NEXT();

L_TypeOf:
L_InstanceOf:
L_In:
        // These require runtime type dispatch — delegate to callbacks.
        R[frame.code[frame.ip].a].asBits = 0;
        NEXT();

L_Jump:
        frame.ip += frame.code[frame.ip].sbx;
        stats_.branches++;
        DISPATCH();

L_JumpIfTrue:
        if (R[frame.code[frame.ip].a].asBits == 0x7FF8000000000002ULL) {
            frame.ip += frame.code[frame.ip].sbx;
            stats_.branches++;
            DISPATCH();
        }
        NEXT();

L_JumpIfFalse:
        if (R[frame.code[frame.ip].a].asBits != 0x7FF8000000000002ULL) {
            frame.ip += frame.code[frame.ip].sbx;
            stats_.branches++;
            DISPATCH();
        }
        NEXT();

L_JumpIfNull:
        if (R[frame.code[frame.ip].a].asBits == 0) {
            frame.ip += frame.code[frame.ip].sbx;
            stats_.branches++;
            DISPATCH();
        }
        NEXT();

L_JumpIfUndefined:
        if (R[frame.code[frame.ip].a].asBits == 0x7FF8000000000001ULL) {
            frame.ip += frame.code[frame.ip].sbx;
            stats_.branches++;
            DISPATCH();
        }
        NEXT();

L_Call: {
        stats_.calls++;
        callDepth_++;
        if (callDepth_ >= kMaxCallDepth) {
            // Stack overflow.
            callDepth_--;
            goto L_Throw;
        }
        if (cb_.externalCall) {
            uint8_t fn = frame.code[frame.ip].b;
            uint8_t argc = frame.code[frame.ip].c;
            R[frame.code[frame.ip].a] = cb_.externalCall(R[fn].asPtr, &R[fn + 1], argc);
        }
        callDepth_--;
        NEXT();
}

L_TailCall: {
        stats_.calls++;
        if (cb_.externalCall) {
            uint8_t fn = frame.code[frame.ip].b;
            uint8_t argc = frame.code[frame.ip].c;
            return cb_.externalCall(R[fn].asPtr, &R[fn + 1], argc);
        }
        NEXT();
}

L_Return:
        stats_.returns++;
        return R[frame.code[frame.ip].a];

L_ReturnUndefined: {
        stats_.returns++;
        ValueSlot undef;
        undef.asBits = 0x7FF8000000000001ULL;
        return undef;
}

L_GetProperty:
        stats_.propertyAccesses++;
        if (cb_.propertyGet) {
            R[frame.code[frame.ip].a] = cb_.propertyGet(R[frame.code[frame.ip].b],
                                                         frame.code[frame.ip].c);
        }
        NEXT();

L_SetProperty:
        stats_.propertyAccesses++;
        if (cb_.propertySet) {
            cb_.propertySet(R[frame.code[frame.ip].a], frame.code[frame.ip].b,
                           R[frame.code[frame.ip].c]);
        }
        NEXT();

L_GetElement:
L_SetElement:
L_DeleteProperty:
L_HasProperty:
        stats_.propertyAccesses++;
        NEXT();

L_NewObject:
        stats_.allocations++;
        if (cb_.allocObject) R[frame.code[frame.ip].a] = cb_.allocObject();
        NEXT();

L_NewArray:
        stats_.allocations++;
        if (cb_.allocArray) {
            R[frame.code[frame.ip].a] = cb_.allocArray(frame.code[frame.ip].b);
        }
        NEXT();

L_NewFunction:
L_NewClass:
        stats_.allocations++;
        NEXT();

L_GetLocal:
L_SetLocal:
L_GetUpvalue:
L_SetUpvalue:
L_GetGlobal:
L_SetGlobal:
        // Local/upvalue/global access delegated to scope chain.
        NEXT();

L_PushScope:
L_PopScope:
L_PushWith:
L_PopWith:
        NEXT();

L_GetIterator:
L_IteratorNext:
L_IteratorDone:
        NEXT();

L_Await:
L_Yield:
L_YieldStar:
L_AsyncReturn:
        // Async/generator — needs coroutine state machine.
        NEXT();

L_Throw:
        stats_.exceptions++;
        if (cb_.throwHandler && cb_.throwHandler(R[frame.code[frame.ip].a], frame)) {
            DISPATCH();
        }
        return R[frame.code[frame.ip].a];

L_PushTry:
L_PopTry:
L_PushFinally:
L_GetException:
        NEXT();

L_Spread:
L_CreateRestArray:
L_DestructureArray:
L_DestructureObject:
L_SetDefaultValue:
        NEXT();

L_DefineMethod:
L_DefineGetter:
L_DefineSetter:
L_SuperCall:
L_SuperGet:
        NEXT();

L_ImportModule:
L_ExportValue:
L_ImportDynamic:
        NEXT();

L_Debugger:
        NEXT();

L_SourceMap:
        NEXT();

L_TemplateLiteral:
L_TaggedTemplate:
        NEXT();

L_NullishCoalesce:
        if (R[frame.code[frame.ip].b].asBits == 0 ||
            R[frame.code[frame.ip].b].asBits == 0x7FF8000000000001ULL) {
            R[frame.code[frame.ip].a] = R[frame.code[frame.ip].c];
        } else {
            R[frame.code[frame.ip].a] = R[frame.code[frame.ip].b];
        }
        NEXT();

L_OptionalChain:
        if (R[frame.code[frame.ip].b].asBits == 0 ||
            R[frame.code[frame.ip].b].asBits == 0x7FF8000000000001ULL) {
            R[frame.code[frame.ip].a].asBits = 0x7FF8000000000001ULL;
        } else {
            if (cb_.propertyGet) {
                R[frame.code[frame.ip].a] = cb_.propertyGet(R[frame.code[frame.ip].b],
                                                             frame.code[frame.ip].c);
            }
        }
        NEXT();

#undef DISPATCH
#undef NEXT

#else
        // Fallback: switch-based dispatch.
        while (frame.ip < frame.codeLength) {
            stats_.instructionsExecuted++;
            const Instruction& instr = frame.code[frame.ip];
            switch (instr.opcode) {
                case Opcode::Nop: frame.ip++; break;
                case Opcode::LoadInt:
                    R[instr.a].asDouble = static_cast<double>(instr.sbx);
                    frame.ip++; break;
                case Opcode::Move: R[instr.a] = R[instr.b]; frame.ip++; break;
                case Opcode::Add:
                    R[instr.a].asDouble = R[instr.b].asDouble + R[instr.c].asDouble;
                    frame.ip++; break;
                case Opcode::Sub:
                    R[instr.a].asDouble = R[instr.b].asDouble - R[instr.c].asDouble;
                    frame.ip++; break;
                case Opcode::Mul:
                    R[instr.a].asDouble = R[instr.b].asDouble * R[instr.c].asDouble;
                    frame.ip++; break;
                case Opcode::Return: return R[instr.a];
                case Opcode::Jump:
                    frame.ip += instr.sbx; stats_.branches++; break;
                case Opcode::JumpIfTrue:
                    if (R[instr.a].asBits == 0x7FF8000000000002ULL)
                        frame.ip += instr.sbx;
                    else frame.ip++;
                    break;
                case Opcode::JumpIfFalse:
                    if (R[instr.a].asBits != 0x7FF8000000000002ULL)
                        frame.ip += instr.sbx;
                    else frame.ip++;
                    break;
                default: frame.ip++; break;
            }
        }
        ValueSlot undef;
        undef.asBits = 0x7FF8000000000001ULL;
        return undef;
#endif
    }

    ValueSlot registers_[kMaxRegisters];
    size_t callDepth_;
    Callbacks cb_;
    DispatchStats stats_;
};

} // namespace Zepra::Interpreter
