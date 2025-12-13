// Opcode stub
#include "zeprascript/bytecode/opcode.hpp"

namespace Zepra::Bytecode {

const char* opcodeName(Opcode op) {
    switch (op) {
        case Opcode::OP_NOP: return "NOP";
        case Opcode::OP_POP: return "POP";
        case Opcode::OP_CONSTANT: return "CONSTANT";
        case Opcode::OP_NIL: return "NIL";
        case Opcode::OP_TRUE: return "TRUE";
        case Opcode::OP_FALSE: return "FALSE";
        case Opcode::OP_ADD: return "ADD";
        case Opcode::OP_SUBTRACT: return "SUBTRACT";
        case Opcode::OP_MULTIPLY: return "MULTIPLY";
        case Opcode::OP_DIVIDE: return "DIVIDE";
        case Opcode::OP_RETURN: return "RETURN";
        default: return "UNKNOWN";
    }
}

int opcodeOperandCount(Opcode op) {
    switch (op) {
        case Opcode::OP_CONSTANT: return 1;
        case Opcode::OP_CONSTANT_LONG: return 2;
        case Opcode::OP_GET_LOCAL:
        case Opcode::OP_SET_LOCAL: return 1;
        case Opcode::OP_JUMP:
        case Opcode::OP_JUMP_IF_FALSE:
        case Opcode::OP_LOOP: return 2;
        default: return 0;
    }
}

int opcodeStackEffect(Opcode op) {
    switch (op) {
        case Opcode::OP_CONSTANT:
        case Opcode::OP_NIL:
        case Opcode::OP_TRUE:
        case Opcode::OP_FALSE: return 1;
        case Opcode::OP_POP: return -1;
        case Opcode::OP_ADD:
        case Opcode::OP_SUBTRACT: return -1;
        default: return 0;
    }
}

} // namespace Zepra::Bytecode
