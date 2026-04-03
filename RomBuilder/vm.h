#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <cstddef>
#include <vector>

#define EX_MAKE(cat, code)   (((uint32_t)(cat) << 24) | ((code) & 0xFFFFFF))
#define EX_CATEGORY(x)       ((uint8_t)((x) >> 24))
#define EX_CODE(x)           ((x) & 0xFFFFFF)
#define EX_CLEAR_CODE(x) ((x) & 0xFF000000)
#define EX_REMOVE_SPECIFIC(cat, code) ((cat) & ~(uint32_t)(code))

// increased register count to cover special registers RT/EX/AC (<= 0x12)
// and left room for future expansion.
constexpr std::size_t VM_REGISTER_COUNT = 16;

enum Flags : uint8_t
{
    FLAG_ZERO = 1 << 0,  // Result == 0
    FLAG_CARRY = 1 << 1,  // Carry / borrow in arithmetic
    FLAG_SIGN = 1 << 2,  // MSB set (negative in signed ops)
    FLAG_OVERFLOW = 1 << 3,  // Signed overflow
    FLAG_PARITY = 1 << 4,  // Even number of bits set
    FLAG_EQUAL = 1 << 5,  // Comparison equal
    FLAG_GREATER = 1 << 6,  // Comparison greater
    FLAG_LESS = 1 << 7   // Comparison less
};

enum class VMState : uint8_t
{
    Running = 0,   // Normal execution
    Halted,        // HALT instruction executed
    Stopped,
    Paused,        // Suspended (future scheduler / debugger)
    Waiting,       // Waiting for event / interrupt / IO
    Error,         // Fatal VM error
    Crashed        // Invalid state / corruption detected
};

enum class ExceptionCategory : uint8_t {
    EX_OK          = 0x00, // Nessun errore
    EX_VM          = 0x01, // Errori della virtual machine
    EX_MEM         = 0x02, // Errori di memoria
    EX_ARITH       = 0x03, // Errori aritmetici
    EX_TYPE        = 0x04, // Errori di tipo
    EX_BYTECODE    = 0x05, // Errori nel bytecode
    EX_IO          = 0x06, // Errori di input/output
    EX_SYS         = 0x07, // Errori di sistema
    EX_USER        = 0x08  // Errori definiti dall'utente
};

enum {
    EX_VM_ILLEGAL_STATE = 0x010001,
    EX_VM_ILLEGAL_INSTRUCTION = 0x010002,
    EX_VM_STACK_OVERFLOW = 0x010003,
    EX_VM_STACK_UNDERFLOW = 0x010004,
    EX_VM_DIV_ZERO = 0x010005,
    EX_VM_HALT = 0x010006,
    EX_VM_UNIMPLEMENTED = 0x010007,
};

enum {
    EX_MEM_OUT_OF_MEMORY = 0x020001,
    EX_MEM_INVALID_ADDRESS = 0x020002,
    EX_MEM_READ_FAULT = 0x020003,
    EX_MEM_WRITE_FAULT = 0x020004,
    EX_MEM_ALIGNMENT_ERROR = 0x020005,
};

enum {
    EX_ARITH_OVERFLOW = 0x030001,
    EX_ARITH_UNDERFLOW = 0x030002,
    EX_ARITH_NAN = 0x030003,
    EX_ARITH_INF = 0x030004,
};

enum {
    EX_TYPE_MISMATCH = 0x040001,
    EX_TYPE_BAD_CAST = 0x040002,
    EX_TYPE_UNSUPPORTED_OP = 0x040003,
};

enum {
    EX_BC_OPCODE = 0x050001,
    EX_BC_FORMAT = 0x050002,
    EX_BC_CONSTANT = 0x050003,
    EX_BC_JUMP_TARGET = 0x050004,
};

enum {
    EX_IO_FILE_NOT_FOUND = 0x060001,
    EX_IO_PERMISSION_DENIED = 0x060002,
    EX_IO_End_Of_File = 0x060003,
    EX_IO_DEVICE_ERROR = 0x060004,
};

enum {
    EX_SYS_TIMEOUT = 0x070001,
    EX_SYS_INTERRUPTED = 0x070002,
    EX_SYS_NOT_SUPPORTED = 0x070003,
};

enum {
    EX_USER_GENERIC = 0x080001,
};


class VM
{
public:

    // Constructor
    VM(size_t memorySize, size_t stackSize, size_t codeSize)
        : memorySize(memorySize),
        stackSize(stackSize),
        codeSize(codeSize),
        ip(0),
        sp(0),
        bp(0),
        flags(0),
        ex(0),
        state(VMState::Halted)
    {
        // Allocate heap memory
        memory = new uint8_t[memorySize];
        stack = new int32_t[stackSize];

        // Zero initialize
        std::memset(memory, 0, memorySize * sizeof(uint8_t));
        std::memset(stack, 0, stackSize * sizeof(int32_t));
    }

    // Destructor
    ~VM()
    {
        delete[] memory;
        delete[] stack;
    }

    // Disable copy (VERY important)
    VM(const VM&) = delete;
    VM& operator=(const VM&) = delete;

    // Allow move
    VM(VM&& other) noexcept
    {
        moveFrom(other);
    }

    VM& operator=(VM&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            moveFrom(other);
        }
        return *this;
    }

    Register& getRegister(uint8_t index)
    {
        if (index >= REG_COUNT)
        {
            ex = EX_MAKE(ExceptionCategory::EX_VM, EX_VM_ILLEGAL_INSTRUCTION);
            return registers[0]; 
        }
        return registers[index];
    }

public:

    // 16 general purpose registers
    Register registers[REG_COUNT];

    // Dynamic memory (heap of the VM)
    uint8_t* memory;
    int32_t* stack;
    std::vector<uint32_t> code;

    size_t memorySize;
    size_t stackSize;
    size_t codeSize;

    // Special registers
    Register ip;
    Register sp;
    Register bp;

    uint8_t flags;
    uint32_t ex;

    VMState state;

private:

    void cleanup()
    {
        delete[] memory;
        delete[] stack;
    }

    void moveFrom(VM& other)
    {
        memory = other.memory;
        stack = other.stack;

        memorySize = other.memorySize;
        stackSize = other.stackSize;

        ip = other.ip;
        sp = other.sp;
        bp = other.bp;

        flags = other.flags;
        ex = other.ex;
        state = other.state;

        // Invalidate source
        other.memory = nullptr;
        other.stack = nullptr;
    }
};

// Set a flag bit
inline void setFlag(VM& vm, Flags flag)
{
    vm.flags |= static_cast<uint8_t>(flag);
}

// Clear a flag bit
inline void clearFlag(VM& vm, Flags flag)
{
    vm.flags &= ~static_cast<uint8_t>(flag);
}

// Test a flag bit
inline bool getFlag(const VM& vm, Flags flag)
{
    return (vm.flags & static_cast<uint8_t>(flag)) != 0;
}

// Reset all flags
inline void clearAllFlags(VM& vm)
{
    vm.flags = 0;
}

// Update ZERO and SIGN flags based on result
inline void updateZeroSign(VM& vm, uint32_t result)
{
    // ZERO flag
    if (result == 0)
        setFlag(vm, FLAG_ZERO);
    else
        clearFlag(vm, FLAG_ZERO);

    // SIGN flag (MSB)
    if (result & 0x80000000)
        setFlag(vm, FLAG_SIGN);
    else
        clearFlag(vm, FLAG_SIGN);
}

// Update CARRY for addition
inline void updateCarryAdd(VM& vm, uint32_t a, uint32_t b)
{
    uint64_t result = static_cast<uint64_t>(a) + b;

    if (result > 0xFFFFFFFF)
        setFlag(vm, FLAG_CARRY);
    else
        clearFlag(vm, FLAG_CARRY);
}

// Update CARRY for multiplication
inline void updateCarryMul(VM& vm, uint32_t a, uint32_t b)
{
    uint64_t result = static_cast<uint64_t>(a) * b;

    if (result > 0xFFFFFFFF)
        setFlag(vm, FLAG_CARRY);
    else
        clearFlag(vm, FLAG_CARRY);
}

// Update CARRY for subtraction (borrow detection)
inline void updateCarrySub(VM& vm, uint32_t a, uint32_t b)
{
    if (a < b)
        setFlag(vm, FLAG_CARRY);
    else
        clearFlag(vm, FLAG_CARRY);
}

// Update OVERFLOW for addition
inline void updateOverflowAdd(VM& vm, uint32_t a, uint32_t b, uint32_t result)
{
    bool overflow = (~(a ^ b) & (a ^ result)) & 0x80000000;

    if (overflow)
        setFlag(vm, FLAG_OVERFLOW);
    else
        clearFlag(vm, FLAG_OVERFLOW);
}

// Update OVERFLOW for multiplication
inline void updateOverflowMul(VM& vm, uint32_t a, uint32_t b)
{
    uint64_t full = (uint64_t)a * (uint64_t)b;

    bool overflow = (full >> 32) != 0;

    if (overflow)
        setFlag(vm, FLAG_OVERFLOW);
    else
        clearFlag(vm, FLAG_OVERFLOW);
}
