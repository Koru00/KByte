#pragma once

#include "vm.h"

/* ALU */

// Perform 32-bit addition and update flags
inline uint32_t aluAdd(VM& vm, uint32_t a, uint32_t b)
{
    uint32_t result = a + b;

    updateZeroSign(vm, result);
    updateCarryAdd(vm, a, b);
    updateOverflowAdd(vm, a, b, result);

    return result;
}

// Perform 32-bit subtraction and update flags
inline uint32_t aluSub(VM& vm, uint32_t a, uint32_t b)
{
    uint32_t result = a - b;

    updateZeroSign(vm, result);
    updateCarrySub(vm, a, b);

    return result;
}

// Perform 32-bit multiplication and update flags
inline uint32_t aluMul(VM& vm, uint32_t a, uint32_t b)
{
    uint32_t result = a * b;

    updateZeroSign(vm, result);
    updateCarryMul(vm, a, b);
    updateOverflowMul(vm, a, b);

    return result;
}

// Perform 32-bit division and update flags
inline uint32_t aluDiv(VM& vm, uint32_t a, uint32_t b)
{
    if (b == 0)
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_VM, EX_VM_DIV_ZERO);
        return NULL;
    }

    uint32_t result = a / b;

    updateZeroSign(vm, result);

    EX_REMOVE_SPECIFIC(EX_CATEGORY(vm.ex), EX_VM_DIV_ZERO);

    return result;
}

// Perform 32-bit modulus and update flags
inline uint32_t aluMod(VM& vm, uint32_t a, uint32_t b)
{
    if (b == 0)
    {
        vm.ex = EX_MAKE(ExceptionCategory::EX_VM, EX_VM_DIV_ZERO);
        return NULL;
    }

    uint32_t result = a % b;

    updateZeroSign(vm, result);

    EX_REMOVE_SPECIFIC(EX_CATEGORY(vm.ex), EX_VM_DIV_ZERO);

    return result;
}

// Perform 32-bit exponentiation (base^exp) using exponentiation by squaring and update flags
inline uint32_t aluPow(VM& vm, uint32_t base, uint32_t exp)
{
    // Fast exponentiation by squaring.
    // Use 64-bit intermediate to detect overflow/carry on each multiply.
    uint32_t result = 1;
    uint32_t b = base;

    bool anyCarry = false;
    bool anyOverflow = false;

    // Special-case: exp == 0 -> 1 (even 0^0 returns 1 by convention here)
    while (exp)
    {
        if (exp & 1u)
        {
            uint64_t prod = static_cast<uint64_t>(result) * static_cast<uint64_t>(b);
            if ((prod >> 32) != 0)
                anyCarry = anyOverflow = true;
            result = static_cast<uint32_t>(prod);
        }

        exp >>= 1u;
        if (exp)
        {
            uint64_t prod = static_cast<uint64_t>(b) * static_cast<uint64_t>(b);
            if ((prod >> 32) != 0)
                anyCarry = anyOverflow = true;
            b = static_cast<uint32_t>(prod);
        }
    }

    updateZeroSign(vm, result);

    if (anyCarry)
        setFlag(vm, FLAG_CARRY);
    else
        clearFlag(vm, FLAG_CARRY);

    if (anyOverflow)
        setFlag(vm, FLAG_OVERFLOW);
    else
        clearFlag(vm, FLAG_OVERFLOW);

    return result;
}

// Bitwise AND
inline uint32_t aluAnd(VM& vm, uint32_t a, uint32_t b)
{
    uint32_t result = a & b;

    updateZeroSign(vm, result);
    clearFlag(vm, FLAG_CARRY);
    clearFlag(vm, FLAG_OVERFLOW);

    return result;
}

inline uint32_t aluOr(VM& vm, uint32_t a, uint32_t b)
{
    uint32_t result = a | b;

    updateZeroSign(vm, result);
    clearFlag(vm, FLAG_CARRY);
    clearFlag(vm, FLAG_OVERFLOW);

    return result;
}

inline uint32_t aluXor(VM& vm, uint32_t a, uint32_t b)
{
    uint32_t result = a ^ b;

    updateZeroSign(vm, result);
    clearFlag(vm, FLAG_CARRY);
    clearFlag(vm, FLAG_OVERFLOW);

    return result;
}

// Compare two values (like SUB but discard result)
inline void aluCmp(VM& vm, uint32_t a, uint32_t b)
{
    uint32_t result = a - b;

    updateZeroSign(vm, result);
    updateCarrySub(vm, a, b);

    if (a == b)
        setFlag(vm, FLAG_EQUAL);
    else
        clearFlag(vm, FLAG_EQUAL);

    if (a > b)
        setFlag(vm, FLAG_GREATER);
    else
        clearFlag(vm, FLAG_GREATER);

    if (a < b)
        setFlag(vm, FLAG_LESS);
    else
        clearFlag(vm, FLAG_LESS);
}

/* 32 bit OP */

inline void push(VM& vm, uint32_t value)
{
    vm.stack[vm.sp++] = value;
}

inline uint32_t pop(VM& vm)
{
    return vm.stack[--vm.sp];
}