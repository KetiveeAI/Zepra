/**
 * @file ZWasmTrap.h
 * @brief ZepraScript WASM trap types for runtime error handling
 * 
 * Defines all runtime trap types for WASM execution.
 * Part of ZepraScript's independent WASM implementation.
 */

#pragma once

#include <cstdint>
#include <cstring>

namespace Zepra::Wasm {

// =============================================================================
// Exception Type Enumeration
// =============================================================================

#define FOR_EACH_ZWASM_TRAP(V) \
    V(OutOfBoundsMemoryAccess,      "Out of bounds memory access") \
    V(UnalignedMemoryAccess,        "Unaligned memory access") \
    V(OutOfBoundsTableAccess,       "Out of bounds table access") \
    V(OutOfBoundsCallIndirect,      "Out of bounds call_indirect") \
    V(NullTableEntry,               "call_indirect to a null table entry") \
    V(NullReference,                "call_ref to a null reference") \
    V(NullExnrefReference,          "throw_ref on a null reference") \
    V(NullI31Get,                   "i31.get to a null reference") \
    V(BadSignature,                 "call_indirect signature mismatch") \
    V(OutOfBoundsTrunc,             "Out of bounds Trunc operation") \
    V(Unreachable,                  "Unreachable code should not be executed") \
    V(DivisionByZero,               "Division by zero") \
    V(IntegerOverflow,              "Integer overflow") \
    V(StackOverflow,                "Stack overflow") \
    V(InvalidGCTypeUse,             "Unsupported use of struct or array type") \
    V(OutOfBoundsArrayGet,          "Out of bounds array.get") \
    V(OutOfBoundsArraySet,          "Out of bounds array.set") \
    V(OutOfBoundsArrayFill,         "Out of bounds array.fill") \
    V(OutOfBoundsArrayCopy,         "Out of bounds array.copy") \
    V(OutOfBoundsArrayInitElem,     "Out of bounds array.init_elem") \
    V(OutOfBoundsArrayInitData,     "Out of bounds array.init_data") \
    V(BadStructNew,                 "Failed to allocate new struct") \
    V(BadArrayNew,                  "Failed to allocate new array") \
    V(BadArrayNewInitElem,          "Out of bounds in array.new_elem") \
    V(BadArrayNewInitData,          "Out of bounds in array.new_data") \
    V(NullAccess,                   "Access to a null reference") \
    V(NullArrayFill,                "array.fill to a null reference") \
    V(NullArrayCopy,                "array.copy to a null reference") \
    V(NullArrayInitElem,            "array.init_elem to a null reference") \
    V(NullArrayInitData,            "array.init_data to a null reference") \
    V(NullRefAsNonNull,             "ref.as_non_null to a null reference") \
    V(CastFailure,                  "ref.cast failed") \
    V(OutOfBoundsDataSegmentAccess, "Out of bounds data segment access") \
    V(OutOfBoundsElementSegmentAccess, "Out of bounds element segment access") \
    V(OutOfMemory,                  "Out of memory") \
    V(IllegalArgument,              "Illegal argument") \
    V(Termination,                  "Termination")

enum class ZTrap : uint32_t {
#define MAKE_ENUM(enumName, message) enumName,
    FOR_EACH_ZWASM_TRAP(MAKE_ENUM)
#undef MAKE_ENUM
    Count
};

// =============================================================================
// Exception Message Lookup
// =============================================================================

inline const char* trapMessage(ZTrap type) {
    switch (type) {
#define SWITCH_CASE(enumName, message) \
    case ZTrap::enumName: return message;
    FOR_EACH_ZWASM_TRAP(SWITCH_CASE)
#undef SWITCH_CASE
    default: return "Unknown trap";
    }
}

// =============================================================================
// Exception Classification
// =============================================================================

inline bool isTypeError(ZTrap type) {
    switch (type) {
    case ZTrap::InvalidGCTypeUse:
        return true;
    default:
        return false;
    }
}

inline bool isOutOfBoundsError(ZTrap type) {
    switch (type) {
    case ZTrap::OutOfBoundsMemoryAccess:
    case ZTrap::OutOfBoundsTableAccess:
    case ZTrap::OutOfBoundsCallIndirect:
    case ZTrap::OutOfBoundsTrunc:
    case ZTrap::OutOfBoundsArrayGet:
    case ZTrap::OutOfBoundsArraySet:
    case ZTrap::OutOfBoundsArrayFill:
    case ZTrap::OutOfBoundsArrayCopy:
    case ZTrap::OutOfBoundsArrayInitElem:
    case ZTrap::OutOfBoundsArrayInitData:
    case ZTrap::OutOfBoundsDataSegmentAccess:
    case ZTrap::OutOfBoundsElementSegmentAccess:
        return true;
    default:
        return false;
    }
}

inline bool isNullError(ZTrap type) {
    switch (type) {
    case ZTrap::NullTableEntry:
    case ZTrap::NullReference:
    case ZTrap::NullExnrefReference:
    case ZTrap::NullI31Get:
    case ZTrap::NullAccess:
    case ZTrap::NullArrayFill:
    case ZTrap::NullArrayCopy:
    case ZTrap::NullArrayInitElem:
    case ZTrap::NullArrayInitData:
    case ZTrap::NullRefAsNonNull:
        return true;
    default:
        return false;
    }
}

// =============================================================================
// Exception Info Structure
// =============================================================================

struct ZTrapInfo {
    ZTrap type;
    uint32_t funcIndex;
    uint32_t bytecodeOffset;
    void* faultingAddress;
    
    const char* message() const {
        return trapMessage(type);
    }
};

} // namespace Zepra::Wasm
