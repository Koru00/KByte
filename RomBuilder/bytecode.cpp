#include "bytecode.h"

using Instruction = void(*)(VM&, uint8_t a, uint8_t b, uint8_t c);

static const std::array<Instruction, 0x31> dispatchTable = {
    nullptr,               // NOP
    
    HALT,

    MOV,
    LDI,
    LDI32,
    LOD,
    STO,

    POP,
    PUSH,

    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NEG,
    POW,
    POWR,
    ABS,

    AND,
    OR,
    XOR,
    NOT,

    SHL,
    SHR,

    CMP,

    JMP,
    JE,
    JNE,

    CALL,
    RET,
    SYC
};

void execute(VM& vm, Bytecode opcode, uint8_t a, uint8_t b, uint8_t c)
{
    auto fn = dispatchTable[static_cast<uint8_t>(opcode)];
    bool jumped = false;

    if (fn) {
        fn(vm, a, b, c);
    }

    // Only increment if the instruction didn't perform a jump or stop
    if (vm.state != VMState::Stopped) {
        vm.ip++;
    }
}