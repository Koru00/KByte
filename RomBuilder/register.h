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

enum Registers : uint8_t {
    // --- Integer / General Purpose Registers (0x00 - 0x0F) ---
    R0 = 0x00, // Result / Accumulator
    R1 = 0x01, // Argument 1
    R2 = 0x02, // Argument 2
    R3 = 0x03, // Argument 3
    R4 = 0x04, // Local / Temp
    R5 = 0x05, // Local / Temp
    R6 = 0x06, // Local / Temp
    R7 = 0x07, // Syscall ID
    R8 = 0x08, // Counter / Index
    R9 = 0x09, // Counter / Index
    RA = 0x0A, // Pointer / Base
    RB = 0x0B, // Pointer / Base
    RC = 0x0C, // Scratch
    RD = 0x0D, // Scratch
    RE = 0x0E, // Scratch
    RF = 0x0F, // Internal / Reserved

    // --- Floating Point Registers (0x10 - 0x1F) ---
    F0 = 0x10, // Float Result
    F1 = 0x11, // Float Arg 1
    F2 = 0x12, // Float Arg 2
    F3 = 0x13, // Float Arg 3
    F4 = 0x14, // Float Temp
    F5 = 0x15, // Float Temp
    F6 = 0x16, // Float Temp
    F7 = 0x17, // Float Temp
    F8 = 0x18, // Float Constant
    F9 = 0x19, // Float Constant
    FA = 0x1A, // Float Scratch
    FB = 0x1B, // Float Scratch
    FC = 0x1C, // Float Scratch
    FD = 0x1D, // Float Scratch
    FE = 0x1E, // Float Scratch
    FF = 0x1F, // Internal / FPU Status

    REG_COUNT = 0x20 // Total 32 registers
};

union RegisterValue {
    uint32_t u32;
    int32_t  i32;
    float    f32;
    void* ptr; // Useful if your VM interacts with host pointers
};

typedef struct Register {
    RegisterValue value;

    // Helper methods for "Professional" access
    void setU32(uint32_t val) { value.u32 = val; }
    uint32_t getU32() const { return value.u32; }

    void setI32(int32_t val) { value.i32 = val; }
    int32_t getI32() const { return value.i32; }
    
    void setFloat(float val) { value.f32 = val; }
    float getFloat() const { return value.f32; }
} Register;

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
        {"RA", Registers::RA},
        {"RB", Registers::RB},

        {"RC", Registers::RC},
        {"RD", Registers::RD},
        {"RE", Registers::RE},
        {"RF", Registers::RF},

        {"F0", Registers::F0},
        {"F1", Registers::F1},
        {"F2", Registers::F2},
        {"F3", Registers::F3},
        {"F4", Registers::F4},
        {"F5", Registers::F5},
        {"F6", Registers::F6},
        {"F7", Registers::F7},
        {"F8", Registers::F8},
        {"F9", Registers::F9},
        {"FA", Registers::FA},
        {"FB", Registers::FB},
        {"FC", Registers::FC},
        {"FD", Registers::FD},
        {"FE", Registers::FE},
        {"FF", Registers::FF}
    };

    auto it = table.find(str);
    if (it != table.end())
        return it->second;

    return std::nullopt;
}
