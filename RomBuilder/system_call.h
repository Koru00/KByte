#pragma once

#include <cstdint>
#include <charconv>
#include <cstdio>
#include <cstdlib>

#include "vm.h"

enum class SystemCall : uint8_t {
    // VM control
    EXT, YLD, ERR,

    // Console I/O
    PRT, PRN, IPT, IPN, CCH, CLR,

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

// Print string
static inline void sys_prt_handler(VM& /*vm*/, uint32_t v) noexcept
{
    
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

// Read line / string
static inline void sys_ipt_handler(VM& vm, uint32_t v) noexcept
{

}

// Input number
static inline void sys_ipn_handler(VM& vm, uint32_t v) noexcept
{

}

// Clear screen
static inline void sys_clr_handler(VM& vm, uint32_t v) noexcept
{

}

// Print a single character
static inline void sys_cch_handler(VM& vm, uint32_t v) noexcept
{

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