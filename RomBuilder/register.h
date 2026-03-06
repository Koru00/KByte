#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <iomanip>

// safer, explicit extraction masks
#define FETCH()  (*ip++)
#define OP(i)    (((i) >> 24) & 0xFF)
#define A(i)     (((i) >> 16) & 0xFF)
#define B(i)     (((i) >>  8) & 0xFF)
#define C(i)     ((i) & 0xFF)
#define UI16(i)    ((i) & 0xFFFF)

// proper signed-16 extraction
#define SI16(i)   (static_cast<int16_t>((i) & 0xFFFF))

inline uint16_t pack_u8(uint8_t high, uint8_t low)
{
    return (static_cast<uint16_t>(low) << 8) | high;
}

inline uint32_t pack_u8_to_u32(uint8_t byte_3, uint8_t byte_2, uint8_t byte_1, uint8_t byte_0)
{
    return (static_cast<uint32_t>(byte_3) << 24) |
        (static_cast<uint32_t>(byte_2) << 16) |
        (static_cast<uint32_t>(byte_1) << 8) |
        static_cast<uint32_t>(byte_0);
}


#define ENCODE_OP(op, a, b, c) \
    ((uint32_t)(op) << 24 | ((uint32_t)(a) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(c))

enum Registers : uint8_t
{
    // General purpose registers for function calls and arithmetic
    R0 = 0x00,  // Return value / primary result of functions
    R1 = 0x01,  // Argument 1 / temp storage
    R2 = 0x02,  // Argument 2 / temp storage
    R3 = 0x03,  // Argument 3 / temp storage

    R4 = 0x04,  // Local variable / temp for complex operations
    R5 = 0x05,  // Local variable / temp for complex operations
    R6 = 0x06,  // Scratch register / intermediate calculations
    R7 = 0x07,  // System call index or special instruction indicator

    // Additional general purpose registers for computation or function usage
    R8 = 0x08,  // Extra temp / loop counter
    R9 = 0x09,  // Extra temp / loop counter
    R10 = 0x0A,  // Can be used as index / array pointer
    R11 = 0x0B,  // Can be used as index / array pointer

    R12 = 0x0C,  // Reserved for future special use (e.g., flags shadow, status)
    R13 = 0x0D,  // Reserved for future special use (e.g., interrupt return)
    R14 = 0x0E,  // Scratch / temporary accumulator for math / ALU
    R15 = 0x0F   // Reserved / scratch for VM internal use, or syscall result buffer
};

inline std::optional<Registers> registersFromString(std::string_view str)
{
    // Static so it's created only once
    static const std::unordered_map<std::string_view, Registers> table =
    {
        {"R0", Registers::R0},
        {"R1", Registers::R1},
        {"R2", Registers::R2},
        {"R3", Registers::R3},

        {"R4", Registers::R4},
        {"R5", Registers::R5},
        {"R6", Registers::R6},
        {"R7", Registers::R7},

        {"R8", Registers::R8},
        {"R9", Registers::R9},
        {"R10", Registers::R10},
        {"R11", Registers::R11},

        {"R12", Registers::R12},
        {"R13", Registers::R13},
        {"R14", Registers::R14},
        {"R15", Registers::R15}
    };

    auto it = table.find(str);
    if (it != table.end())
        return it->second;

    return std::nullopt;
}
