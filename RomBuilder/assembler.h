#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <array>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <string>

#include "bytecode.h"
#include "system_call.h"
#include "register.h"
#include "vm.h"

#define XBC_MAGIC 0x58424330  // "XBC0"
#define ASM_VERSION 0x0001 // v0.01

/**
 * @brief File header for XBC bytecode modules.
 *
 * This structure defines the layout of the serialized bytecode file used by
 * the virtual machine. All fields are stored in little-endian format.
 * The header provides metadata required to correctly load, validate, and
 * execute the bytecode module.
 */
typedef struct Header
{
    uint32_t magic;           // File identifier; must be XBC_MAGIC ("XBC0")
    uint16_t isa_version;     // Instruction set version supported by this module
    uint16_t flags;           // Module flags (bitfield)

    uint32_t code_size;       // Size of the bytecode section in bytes
    uint32_t code_offset;     // Offset (from file start) to the bytecode section
    uint32_t entry_point;     // Initial instruction index (program entry)

    uint32_t memory_size;     // Required global memory (in words)
    uint32_t stack_size;      // Required VM stack size (in words)

    uint16_t registers;       // Number of registers required by this module
    uint16_t reserved0;       // Reserved for alignment / future use

    uint32_t const_pool_size; // Size of the constant pool (optional)

    uint32_t checksum;        // CRC32 checksum of the bytecode section

    uint8_t reserved[32];     // Reserved for future expansion
} Header;

int assembler(const char* filepath, const char* outName);