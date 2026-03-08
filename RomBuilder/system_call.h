#pragma once

#include <cstdint>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdio.h>

#include "vm.h"

enum class SystemCall : uint8_t {
    // VM control
    EXT, YLD, ERR,

    // Console I/O
    PRT, PRN, IPT, IPN, CLR, CCH,

    // Memory
    MAL, FRE, RDB, WRB, CPY, SET,

    // File system
    FOP, FCL, FRD, FWR, FSE, FSZ,

    // Time
    TMS, SLP, CLK,

    // Graphics
    PIX, GFX, CLS, PAL, DRW,

    // Audio
    SND, STP, VOL,

    // Math
    SIN, COS, SQRT, RND,

    // System / Host
    ENV, LOG, DBG, HST
};

// Reads a signed 32-bit integer from an input stream.
// Returns true if parsing succeeded, false if invalid input was provided.
static inline bool read_int32(std::istream& in, int32_t& out) noexcept
{
    int c = in.get();

    // Skip leading whitespace
    while (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        c = in.get();

    if (c == EOF)
        return false;

    bool negative = false;

    // Handle sign
    if (c == '-' || c == '+')
    {
        negative = (c == '-');
        c = in.get();
    }

    if (c < '0' || c > '9')
        return false;

    int32_t value = 0;

    while (c >= '0' && c <= '9')
    {
        int digit = c - '0';

        // Overflow check
        if (value > (std::numeric_limits<int32_t>::max() - digit) / 10)
            return false;

        value = value * 10 + digit;

        c = in.get();
    }

    out = negative ? -value : value;
    return true;
}

// ----- fast syscall handlers (file scope, no nested functions) -----
using SysHandler = void(*)(VM&, uint32_t) noexcept;

static inline void sys_ext_handler(VM& vm, uint32_t v) noexcept
{
    vm.state = VMState::Stopped;
    if (v != 0)
    {
        std::fflush(nullptr);
        std::exit(static_cast<int>(static_cast<int32_t>(v)));
    }
}

static inline void sys_yld_handler(VM& vm, uint32_t v) noexcept
{

}

static inline void sys_err_handler(VM& vm, uint32_t v) noexcept
{

}

// Print string from memory
static inline void sys_prt_handler(VM& vm, uint32_t v) noexcept
{
    // TODO
    // v -> memory offset to where to look for the start of the string
    // use putchar(vm.memory[i]) for printing until memory[i] is not \0 
}

// Print number
static inline void sys_prn_handler(VM& vm, uint32_t v) noexcept
{
    // Convert signed int quickly and write to stdout without iostream.
    char buf[16]; // enough for "-2147483648"
    const auto signed_val = static_cast<int32_t>(v);
    const auto res = std::to_chars(buf, buf + sizeof(buf), signed_val);
    if (res.ec == std::errc{}) // success
    {
        std::fwrite(buf, 1, static_cast<size_t>(res.ptr - buf), stdout);
        // optional flush control left to caller; do not flush every call for perf
    }
}

// Input string
static inline void sys_ipt_handler(VM& vm, uint32_t v) noexcept
{

}

static inline void sys_ipn_handler(VM& vm, uint32_t v) noexcept
{
    int32_t value;

    if (!read_int32(std::cin, value))
    {
        // Handle invalid input
        // TODO set vm excetion to EX_TYPE_MISMATCH
        return;
    }

    vm.registers[R0] = static_cast<uint32_t>(value);
}

// Clear screen
static inline void sys_clr_handler(VM& vm, uint32_t v) noexcept
{

}

// Print a single character
static inline void sys_cch_handler(VM& vm, uint32_t v) noexcept
{
    if (v & 0xFFFFFF00)
    {
        // If v is more then a uint8 return
        // TODO set vm excetion to EX_TYPE_MISMATCH
        std::cout << v << " is more then 8 bit" << std::endl;
        return;
    }
    std::cout << static_cast<char>(v);
}

static inline SysHandler sys_handlers[] = {
    sys_ext_handler, // EXT == 0
    sys_yld_handler,
    sys_err_handler,
    sys_prt_handler,
    sys_prn_handler,
    sys_ipt_handler,
    sys_ipn_handler,
    sys_clr_handler,
    sys_cch_handler
};
