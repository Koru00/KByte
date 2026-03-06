// Bytecode.hpp
#pragma once

#include <cstdint>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>
#include <unordered_map>
#include <string_view>
#include <optional>

#include "vm.h"
#include "register.h"
#include "runtime.h"
#include "system_call.h"

#ifdef _MSC_VER
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

/**
 * @brief Enumeration of all VM bytecode instructions.
 */
enum class Bytecode : uint8_t
{
    NOP = 0x00,
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

// This function returns a Bytecode if found, otherwise std::nullopt
inline std::optional<Bytecode> bytecodeFromString(std::string_view str)
{
    // Static so it's created only once
    static const std::unordered_map<std::string_view, Bytecode> table =
    {
        {"NOP", Bytecode::NOP},
        {"HALT", Bytecode::HALT},

        {"MOV", Bytecode::MOV},
        {"LDI", Bytecode::LDI},
        {"LDI32", Bytecode::LDI32},
        {"LOD", Bytecode::LOD},
        {"STO", Bytecode::STO},

        {"POP", Bytecode::POP},
        {"PUSH", Bytecode::PUSH},

        {"ADD", Bytecode::ADD},
        {"SUB", Bytecode::SUB},
        {"MUL", Bytecode::MUL},
        {"DIV", Bytecode::DIV},
        {"MOD", Bytecode::MOD},
        {"NEG", Bytecode::NEG},
        {"POW", Bytecode::POW},
        {"POWR", Bytecode::POWR},
        {"ABS", Bytecode::ABS},

        {"AND", Bytecode::AND},
        {"OR", Bytecode::OR},
        {"XOR", Bytecode::XOR},
        {"NOT", Bytecode::NOT},

        {"SHL", Bytecode::SHL},
        {"SHR", Bytecode::SHR},

        {"CMP", Bytecode::CMP},

        {"JMP", Bytecode::JMP},
        {"JE", Bytecode::JE},
        {"JNE", Bytecode::JNE},

        {"CALL", Bytecode::CALL},
        { "RET", Bytecode::RET},
        {"SYC", Bytecode::SYC}
    };

    auto it = table.find(str);
    if (it != table.end())
        return it->second;

    return std::nullopt;
}

// MOV instruction: dst = R[b]
FORCE_INLINE  void MOV(VM& vm, uint8_t a, uint8_t b, uint8_t c = 0)
{
    vm.registers[a] = vm.registers[b];
}

FORCE_INLINE  void LDI(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = static_cast<uint32_t>(pack_u8(b, c));
}

FORCE_INLINE  void LDI32(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = vm.code[++vm.ip];
}

FORCE_INLINE  void LOD(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    const uint32_t addr = static_cast<uint32_t>(pack_u8(b, c));
    if (addr >= vm.memorySize)
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_MEM, EX_MEM_INVALID_ADDRESS);
        return;
    }
    vm.registers[dst] = static_cast<uint32_t>(vm.memory[addr]);
}

FORCE_INLINE  void STO(VM& vm, uint8_t src, uint8_t b, uint8_t c)
{
    const uint32_t addr = static_cast<uint32_t>(pack_u8(b, c));
    if (addr >= vm.memorySize)
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_MEM, EX_MEM_INVALID_ADDRESS);
        return;
    }
    vm.memory[addr] = static_cast<int32_t>(vm.registers[src]);
}

FORCE_INLINE  void LODR(VM& vm, uint8_t dst, uint8_t addrReg)
{
    const uint32_t addr = vm.registers[addrReg];
    if (addr >= vm.memorySize)
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_MEM, EX_MEM_INVALID_ADDRESS);
        return;
    }
    vm.registers[dst] = static_cast<uint32_t>(vm.memory[addr]);
}

FORCE_INLINE  void STOR(VM& vm, uint8_t src, uint8_t addrReg)
{
    const uint32_t addr = vm.registers[addrReg];
    if (addr >= vm.memorySize)
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_MEM, EX_MEM_INVALID_ADDRESS);
        return;
    }
    vm.memory[addr] = static_cast<int32_t>(vm.registers[src]);
}

FORCE_INLINE  void PUSH(VM& vm, uint8_t a, uint8_t b = 0, uint8_t c = 0)
{
    if (vm.sp >= vm.stackSize)
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_VM, EX_VM_STACK_OVERFLOW);
        return;
    }
    vm.stack[vm.sp++] = static_cast<int32_t>(vm.registers[a]);
}

FORCE_INLINE  void POP(VM& vm, uint8_t a, uint8_t b = 0, uint8_t c = 0)
{
    if (vm.sp == 0)
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_VM, EX_VM_STACK_UNDERFLOW);
        return;
    }
    vm.registers[a] = static_cast<uint32_t>(vm.stack[--vm.sp]);
}

FORCE_INLINE  void ADD(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluAdd(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void SUB(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluSub(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void MUL(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluMul(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void DIV(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluDiv(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void MOD(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluMod(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void NEG(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluSub(vm, 0u, vm.registers[b]);
}

// R[a] = b^c (immediate exponent)
FORCE_INLINE  void POW(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluPow(vm, b, c);
}

// R[a] = R[b]^R[c]
FORCE_INLINE  void POWR(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluPow(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void ABS(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    const uint32_t val = vm.registers[b];

    if (static_cast<int32_t>(val) < 0)
    {
        // Use existing aluSub to maintain consistent flags.
        vm.registers[dst] = aluSub(vm, 0u, val);

        // abs(INT32_MIN) is overflow for signed 32-bit
        if (val == 0x80000000u)
            setFlag(vm, FLAG_OVERFLOW);
        else
            clearFlag(vm, FLAG_OVERFLOW);
    }
    else
    {
        vm.registers[dst] = val;
        updateZeroSign(vm, val);
        clearFlag(vm, FLAG_CARRY);
        clearFlag(vm, FLAG_OVERFLOW);
    }
}

FORCE_INLINE  void AND(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluAnd(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void OR(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluOr(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void XOR(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = aluXor(vm, vm.registers[b], vm.registers[c]);
}

FORCE_INLINE  void NOT(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    vm.registers[dst] = ~vm.registers[b];
}

FORCE_INLINE  void SHL(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    const uint32_t value = vm.registers[b];
    const uint32_t shift = vm.registers[c] & 0x1Fu; // 0..31

    const uint32_t result = (shift == 0) ? value : (value << shift);
    updateZeroSign(vm, result);

    if (shift == 0)
        clearFlag(vm, FLAG_CARRY);
    else
    {
        const uint32_t shifted_out = (value >> (32 - shift));
        if (shifted_out != 0u) setFlag(vm, FLAG_CARRY); else clearFlag(vm, FLAG_CARRY);
    }

    clearFlag(vm, FLAG_OVERFLOW);
    vm.registers[dst] = result;
}

FORCE_INLINE  void SHR(VM& vm, uint8_t dst, uint8_t b, uint8_t c)
{
    const uint32_t value = vm.registers[b];
    const uint32_t shift = vm.registers[c] & 0x1Fu; // 0..31

    const uint32_t result = (shift == 0) ? value : (value >> shift);
    updateZeroSign(vm, result);

    if (shift == 0)
        clearFlag(vm, FLAG_CARRY);
    else
    {
        const uint32_t mask = (shift >= 32) ? 0xFFFFFFFFu : ((1u << shift) - 1u);
        const uint32_t shifted_out = value & mask;
        if (shifted_out != 0u) setFlag(vm, FLAG_CARRY); else clearFlag(vm, FLAG_CARRY);
    }

    clearFlag(vm, FLAG_OVERFLOW);
    vm.registers[dst] = result;
}

FORCE_INLINE  void CMP(VM& vm, uint8_t a, uint8_t b, uint8_t c)
{
    // Keep comparisons consistent with ALU compare (unsigned registers)
    aluCmp(vm, vm.registers[a], vm.registers[b]);
}

FORCE_INLINE  void JMP(VM& vm, uint8_t a, uint8_t b, uint8_t c)
{
    vm.ip = pack_u8(b, a)-1;
}

FORCE_INLINE  void JE(VM& vm, uint8_t a, uint8_t b, uint8_t c)
{
    if (getFlag(vm, FLAG_EQUAL))
        vm.ip = pack_u8(b, a)-1;
}

FORCE_INLINE  void JNE(VM& vm, uint8_t a, uint8_t b, uint8_t c)
{
    if (!getFlag(vm, FLAG_EQUAL))
        vm.ip = pack_u8(b, a)-1;
}

FORCE_INLINE  void SYC(VM& vm, uint8_t a, uint8_t b, uint8_t c)
{
    const uint32_t callIndex = vm.registers[R7];
    const uint32_t arg = vm.registers[R0];

    const size_t handlerCount = sizeof(sys_handlers) / sizeof(sys_handlers[0]);
    if (callIndex < handlerCount)
    {
        sys_handlers[callIndex](vm, arg);
    }
    else
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_VM, EX_VM_ILLEGAL_INSTRUCTION);
    }
}

FORCE_INLINE  void CALL(VM& vm, uint8_t a, uint8_t b, uint8_t c)
{
    if (vm.sp >= vm.stackSize) {
        vm.ex = EX_MAKE(ExceptionCategory::EX_VM, EX_VM_STACK_OVERFLOW);
        return;
    }

    // Save the NEXT instruction (the one after the CALL)
    vm.stack[vm.sp++] = static_cast<int32_t>(vm.ip + 1);

    // Set IP to (target - 1) because execute() will do vm.ip++ immediately after this
    vm.ip = pack_u8(b, a) - 1;
}

FORCE_INLINE  void RET(VM& vm, uint8_t a, uint8_t b, uint8_t c)
{
    if (vm.sp == 0) {
        vm.ex = EX_MAKE(ExceptionCategory::EX_VM, EX_VM_STACK_UNDERFLOW);
        return;
    }

    // Pull the saved address and subtract 1 
    // to counter the vm.ip++ in the main loop
    vm.ip = static_cast<uint32_t>(vm.stack[--vm.sp]) - 1;
}

FORCE_INLINE  void HALT(VM& vm, uint8_t a, uint8_t b, uint8_t c)
{
    vm.state = VMState::Halted;
}

void execute(VM& vm, Bytecode opcode, uint8_t a, uint8_t b, uint8_t c);