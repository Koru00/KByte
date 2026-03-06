# Bytecode Specification

This document describes every bytecode instruction implemented in the VM, their binary encoding, operand meanings, semantics, side effects (flags / state), and common usage examples.

Layout
- Every instruction is encoded in 4 bytes: `[ OPCODE | A | B | C ]`.
  - `OPCODE` — 1 byte (uint8_t) selecting the instruction.
  - `A`, `B`, `C` — instruction-specific operand bytes.
  - Where the instruction treats `B` and `C` as a 16-bit immediate, the immediate value is computed as `IMM16 = B | (C << 8)` (little-endian).
- Registers are `R0`..`R15` (VM stores 16 general-purpose registers).
- The VM exposes flags and exceptions. See `vm.h` for flag bit definitions and exception codes referenced below.

Opcode assignments
- `NOP`  = 0x00
- `HALT` = 0x01
- `MOV`  = 0x02
- `LDI`  = 0x03
- `LOD`  = 0x04
- `STO`  = 0x05
- `POP`  = 0x06
- `PUSH` = 0x07
- `ADD`  = 0x08
- `SUB`  = 0x09
- `MUL`  = 0x0A
- `DIV`  = 0x0B
- `MOD`  = 0x0C
- `NEG`  = 0x0D
- `POW`  = 0x0E
- `POWR` = 0x0F
- `ABS`  = 0x10
- `SHL`  = 0x11
- `SHR`  = 0x12
- `CMP`  = 0x13
- `CNE`  = 0x14
- `JMP`  = 0x15
- `JE`   = 0x16
- `JNE`  = 0x17
- `CALL` = 0x18
- `SYC`  = 0x19

Instruction reference

NOP (0x00)
- Encoding: `[0x00 | 0x00 | 0x00 | 0x00]` (operands ignored)
- Semantics: No operation — does nothing.
- Flags: unchanged.
- Exceptions: none.
- Example: `NOP`

HALT (0x01)
- Encoding: `[0x01 | A | B | C]` (operands ignored)
- Semantics: Sets VM state to `VMState::Halted`. Execution loop should stop.
- Flags: unchanged.
- Exceptions: none.
- Example: `HALT`

MOV (0x02)
- Encoding: `[0x02 | dst | src | 0x00]`
- Operands: `A = dst` (register index), `B = src` (register index)
- Semantics: `R[A] = R[B]` (simple register copy).
- Flags: unchanged.
- Exceptions: none.
- Example: `MOV R1, R0` → bytes: `[0x02, 0x01, 0x00, 0x00]`

LDI (0x03)
- Encoding: `[0x03 | dst | imm_lo | imm_hi]`
- Operands: `A = dst`, `IMM16 = B | (C << 8)`
- Semantics: Load 16-bit immediate (zero-extended to 32-bit) into `R[A]`.
  - Implementation: `vm.registers[dst] = pack_u8(b, c)`.
- Flags: may be updated by callers; instruction itself does not set any flags.
- Exceptions: none.
- Example: `LDI R0, 0x1234` → bytes: `[0x03, 0x00, 0x34, 0x12]`

LOD (0x04)
- Encoding: `[0x04 | dst | addr_lo | addr_hi]`
- Operands: `A = dst`, `IMM16 = B | (C << 8)`
- Semantics: Load memory at `IMM16` into `R[A]`.
  - Performs bounds check: if `addr >= vm.memorySize` then `vm.ex = EX_MAKE(EX_MEM, EX_MEM_INVALID_ADDRESS)` and returns.
  - Read performed as `vm.memory[addr]` (32-bit signed memory cell) and stored into `R[A]` as `uint32_t`.
- Flags: none directly; reading a value does not set flags.
- Exceptions: `EX_MEM_INVALID_ADDRESS` on out-of-range address.
- Example: `LOD R2, 0x0010` → bytes: `[0x04, 0x02, 0x10, 0x00]`

STO (0x05)
- Encoding: `[0x05 | src | addr_lo | addr_hi]`
- Operands: `A = src` (register to store), `IMM16 = B | (C << 8)`
- Semantics: Store `R[A]` to memory at `IMM16`.
  - Bounds-checked; sets `EX_MEM_INVALID_ADDRESS` if out-of-range.
  - Writes `vm.memory[addr] = static_cast<int32_t>(vm.registers[src])`.
- Flags: none.
- Exceptions: `EX_MEM_INVALID_ADDRESS`.
- Example: `STO R3, 0x0020` → bytes: `[0x05, 0x03, 0x20, 0x00]`

POP (0x06)
- Encoding: `[0x06 | dst | 0x00 | 0x00]`
- Operands: `A = dst` register to receive popped value.
- Semantics: Pop top of VM stack into `R[A]`.
  - If `vm.sp == 0` then `vm.ex = EX_MAKE(EX_VM, EX_VM_STACK_UNDERFLOW)` and returns.
  - Otherwise `vm.registers[A] = vm.stack[--vm.sp]`.
- Flags: none.
- Exceptions: `EX_VM_STACK_UNDERFLOW`.
- Example: `POP R0` → bytes: `[0x06, 0x00, 0x00, 0x00]`

PUSH (0x07)
- Encoding: `[0x07 | src | 0x00 | 0x00]`
- Operands: `A = src` register to push.
- Semantics: Push `R[A]` onto VM stack.
  - If `vm.sp >= vm.stackSize` then `vm.ex = EX_MAKE(EX_VM, EX_VM_STACK_OVERFLOW)` and returns.
  - Otherwise `vm.stack[vm.sp++] = static_cast<int32_t>(vm.registers[A])`.
- Flags: none.
- Exceptions: `EX_VM_STACK_OVERFLOW`.
- Example: `PUSH R1` → bytes: `[0x07, 0x01, 0x00, 0x00]`

ADD (0x08), SUB (0x09), MUL (0x0A), DIV (0x0B), MOD (0x0C)
- Encoding (generic): `[OP | dst | lhs | rhs]`
- Operands: `A = dst`, `B = lhs`, `C = rhs`
- Semantics:
  - `ADD`: `R[A] = aluAdd(vm, R[B], R[C])`
  - `SUB`: `R[A] = aluSub(vm, R[B], R[C])`
  - `MUL`: `R[A] = aluMul(vm, R[B], R[C])`
  - `DIV`: `R[A] = aluDiv(vm, R[B], R[C])`
  - `MOD`: `R[A] = aluMod(vm, R[B], R[C])`
- Behavior:
  - Arithmetic is delegated to ALU helpers (`aluAdd`, `aluDiv`, etc.). That code is responsible for:
    - Proper integer / float semantics (project-specific).
    - Setting flags like `FLAG_ZERO`, `FLAG_CARRY`, `FLAG_OVERFLOW`, etc.
    - Raising exceptions (for example division by zero should set `EX_VM_DIV_ZERO` via ALU).
- Exceptions: ALU helpers may set `vm.ex` to relevant codes (e.g., division by zero).
- Example: `ADD R0, R1, R2` → `[0x08, 0x00, 0x01, 0x02]`

NEG (0x0D)
- Encoding: `[0x0D | dst | src | 0x00]`
- Operands: `A = dst`, `B = src`
- Semantics: `R[A] = 0 - R[B]` via `aluSub(vm, 0, R[B])`.
- Flags: set/cleared by underlying ALU (carry/overflow/sign/zero).
- Exceptions: as produced by ALU.
- Example: `NEG R0, R1` → `[0x0D, 0x00, 0x01, 0x00]`

POW (0x0E)
- Encoding: `[0x0E | dst | base_imm | exp_imm]`
- Operands: `A = dst`, `B` and `C` are used as immediate bytes (treated as 8-bit values in the inline `POW` implementation).
- Semantics: `R[A] = aluPow(vm, b, c)` where `b` and `c` are the raw 8-bit operand bytes (immediate small base/exponent).
- Notes: Implementation uses the operand bytes themselves rather than registers. For larger bases/exponents use `POWR`.
- Example: `POW R0, 2, 8` would encode base=2, exp=8: `[0x0E, 0x00, 0x02, 0x08]`

POWR (0x0F)
- Encoding: `[0x0F | dst | base_reg | exp_reg]`
- Operands: `A = dst`, `B = base_reg`, `C = exp_reg`
- Semantics: `R[A] = aluPow(vm, R[B], R[C])`.
- Flags/Exceptions: As set by ALU implementation.
- Example: `POWR R0, R1, R2` → `[0x0F, 0x00, 0x01, 0x02]`

ABS (0x10)
- Encoding: `[0x10 | dst | src | 0x00]`
- Operands: `A = dst`, `B = src`
- Semantics:
  - If `R[B]` interpreted as signed 32-bit is negative, compute absolute using `aluSub(vm, 0u, R[B])` to maintain consistent flags.
  - Special-case: If `R[B] == 0x80000000` (INT32_MIN) then signed absolute overflows — implementation sets `FLAG_OVERFLOW`.
  - Otherwise returns positive value and updates zero/sign flags via `updateZeroSign`.
- Flags: `FLAG_OVERFLOW` may be set for `INT32_MIN`; ALU helpers update `FLAG_ZERO` and `FLAG_SIGN`.
- Example: `ABS R0, R1` → `[0x10, 0x00, 0x01, 0x00]`

SHL (0x11) — logical left shift
- Encoding: `[0x11 | dst | value_reg | shift_reg]`
- Operands: `A = dst`, `B = value_reg`, `C = shift_reg`
- Semantics:
  - `shift = R[C] & 0x1F` (0..31)
  - `result = (shift == 0) ? R[B] : (R[B] << shift)`
  - Updates `FLAG_CARRY` if any bits are shifted out (non-zero in `value >> (32 - shift)`), clears `FLAG_OVERFLOW`.
  - Updates `FLAG_ZERO` and `FLAG_SIGN` via `updateZeroSign`.
- Exceptions: none (shifts masked).
- Example: `SHL R0, R1, R2` → `[0x11, 0x00, 0x01, 0x02]`

SHR (0x12) — logical right shift
- Encoding: `[0x12 | dst | value_reg | shift_reg]`
- Operands: same as `SHL`.
- Semantics:
  - `shift = R[C] & 0x1F`
  - `result = (shift == 0) ? R[B] : (R[B] >> shift)`
  - `FLAG_CARRY` set if any lower bits are shifted out (checked via mask), `FLAG_OVERFLOW` cleared.
  - Updates `FLAG_ZERO` and `FLAG_SIGN`.
- Example: `SHR R0, R1, R2` → `[0x12, 0x00, 0x01, 0x02]`

CMP (0x13)
- Encoding: `[0x13 | a | b | 0x00]`
- Operands: `A` and `B` are register indexes that are compared: `R[A]` vs `R[B]`.
- Semantics: Delegates to `aluCmp(vm, R[A], R[B])`.
  - ALU compare sets comparison flags (e.g., `FLAG_EQUAL`, `FLAG_GREATER`, `FLAG_LESS`) consistent with unsigned comparisons in this implementation.
- Flags: sets `FLAG_EQUAL`, `FLAG_GREATER`, `FLAG_LESS` (and possibly `FLAG_ZERO`).
- Example: `CMP R1, R2` → `[0x13, 0x01, 0x02, 0x00]`

CNE (0x14)
- Encoding: `[0x14 | ... ]`
- Semantics: Present in the enum, but not implemented as an inline function in the header. If used in bytecode dispatch it will need an implementation; until then, avoid emitting `CNE` instructions.
- Example: Not implemented — do not emit.

JMP (0x15)
- Encoding: `[0x15 | addr_lo | addr_hi | 0x00]` or `[0x15 | A | B | C]` where implementation uses `pack_u8(a, b)`.
- Operands: Instruction uses the first two operand bytes to form `IMM16 = A | (B << 8)` and sets `vm.ip = IMM16`.
- Semantics: Unconditional jump — set `ip` to immediate address. The caller code expects the VM interpreter to read `ip` as next instruction index.
- Flags: unchanged.
- Exceptions: If `IMM16` is out-of-range of `codeSize`, the VM/loader must detect; the inline `JMP` simply sets `vm.ip`.
- Example: `JMP 0x0040` → `[0x15, 0x40, 0x00, 0x00]`

JE (0x16)
- Encoding: `[0x16 | addr_lo | addr_hi | 0x00]`
- Operands: `IMM16 = A | (B << 8)`
- Semantics: If `FLAG_EQUAL` is set then `vm.ip = IMM16` (jump taken). If flag is clear, execution continues to next instruction.
- Example: `JE 0x0100` → `[0x16, 0x00, 0x01, 0x00]`

JNE (0x17)
- Encoding: `[0x17 | addr_lo | addr_hi | 0x00]`
- Semantics: Jump if not equal (`!FLAG_EQUAL`).
- Example: `JNE 0x0200` → `[0x17, 0x00, 0x02, 0x00]`

CALL (0x18)
- Encoding: `[0x18 | addr_lo | addr_hi | 0x00]` (inline uses `pack_u8(a,b)`)
- Operands: Uses the first two operand bytes to form a 16-bit target `IMM16`.
- Semantics:
  - Push return address (`vm.ip + 1`) onto stack.
  - Set `vm.ip = IMM16`.
  - Checks stack overflow; if `vm.sp >= vm.stackSize` => `EX_VM_STACK_OVERFLOW`.
- Flags: unchanged.
- Example: `CALL 0x0030` → `[0x18, 0x30, 0x00, 0x00]`

SYC (0x19)
- Encoding: `[0x19 | A | B | C]` (operands are ignored by the routine)
- Semantics: System call dispatch:
  - Reads `callIndex = vm.registers[R7]` and `arg = vm.registers[R0]`.
  - Calls `sys_handlers[callIndex](vm, arg)` if `callIndex` is in range.
  - If `callIndex` is out-of-range, sets `vm.ex = EX_MAKE(EX_VM, EX_VM_ILLEGAL_INSTRUCTION)`.
- Use: Puts the call index in `R7` and parameter(s) in `R0`.. as defined by the system handler convention.
- Example: To invoke system handler 2 with argument in `R0`:
  - `LDI R7, 2` ; `SYC` → encoded `[0x03, 0x07, 0x02, 0x00] ; [0x19, 0x00, 0x00, 0x00]`

RET (not in enum)
- There is an inline `RET(VM&)` helper defined in the header but *RET* is not listed as a bytecode enum value.
- Semantics of helper:
  - Pop return address from stack and set `vm.ip` to it.
  - If `vm.sp == 0` then `EX_VM_STACK_UNDERFLOW`.
- Note: To use `RET` as an instruction you must add an opcode mapping and dispatch case.

LODR / STOR (not in enum)
- `LODR(VM& vm, uint8_t dst, uint8_t addrReg)` — load using register indirect address (`addr = vm.registers[addrReg]`), bounds-checked same as `LOD`.
- `STOR(VM& vm, uint8_t src, uint8_t addrReg)` — store using register indirect address.
- These functions exist as helpers but are not present in the `Bytecode` enum. They can be bound to opcodes if desired.

Flags, exceptions and error handling
- Flags are single-byte bitmask (`vm.flags`) and use `setFlag/clearFlag/getFlag` helpers (see `vm.h`).
- Important flag bits include:
  - `FLAG_ZERO`, `FLAG_CARRY`, `FLAG_SIGN`, `FLAG_OVERFLOW`, `FLAG_EQUAL`, `FLAG_GREATER`, `FLAG_LESS`.
- When an operation encounters an error it sets `vm.ex` using `EX_MAKE(category, code)`.
  - Common exceptions set in the inline implementations:
    - `EX_MEM_INVALID_ADDRESS` — from `LOD`, `LOD R`, `STO`, `STOR`.
    - `EX_VM_STACK_OVERFLOW` / `EX_VM_STACK_UNDERFLOW` — from `PUSH`/`POP`/`CALL`/`RET`.
    - `EX_VM_ILLEGAL_INSTRUCTION` — system call handler invalid index or user code when a dispatch table lacks an implementation.
  - ALU routines (`aluAdd`, `aluDiv`, `aluMod`, `aluPow`, `aluCmp`, etc.) may set arithmetic exceptions (div by zero, overflow) — check `runtime.h` / ALU implementation.

Assembly style conventions (encoding examples)
- All assembly examples here use the form `OP dst, src1, src2` or `OP dst, IMM16` according to instruction.
- Example sequence:
  - Load immediate 0x10 into `R1`: `LDI R1, 0x0010` → bytes `[0x03, 0x01, 0x10, 0x00]`
  - Push `R1`: `PUSH R1` → `[0x07, 0x01, 0x00, 0x00]`
  - Call subroutine at `0x0040`: `CALL 0x0040` → `[0x18, 0x40, 0x00, 0x00]`
  - Return from subroutine: (use `RET` helper / opcode if added)

Notes & implementation details
- Many operations delegate to ALU helpers (`aluAdd`, `aluSub`, `aluMul`, `aluDiv`, `aluMod`, `aluPow`, `aluCmp`) — those functions manage flag updates and error cases. Consult `runtime.h` / ALU implementation for exact numeric semantics (signed/unsigned behavior, type promotion).
- Some helper functions exist (`LODR`, `STOR`, `RET`) but their opcodes are not present in the `Bytecode` enum — either those opcodes were intentionally omitted or will be added later. Avoid emitting unsupported opcodes until you add corresponding enum values and dispatch entries.
- The `POW` instruction uses the operand bytes as immediate base and exponent (8-bit each). `POWR` uses registers for base/exponent.
- Branch opcodes `JMP`, `JE`, `JNE`, and `CALL` use `pack_u8(a,b)` in the implementation: the code forms a 16-bit jump target from the first two operand bytes. When assembling, put the low byte in the `A` operand and the high byte in `B`.
- The VM uses 32-bit registers and memory cells (signed int32_t for memory storage). When storing a register to memory it casts to `int32_t`. When loading memory it casts to `uint32_t` into the register — values should be interpreted consistent with user code.

Checklist when adding or emitting instructions
- Ensure the opcode exists in `Bytecode` enum and the inline function or dispatch entry is implemented in `bytecode.h` / `bytecode.cpp`.
- Update the interpreter dispatch (`execute`) so that the opcode maps to the implementation (the project's dispatch table may live in `bytecode.cpp`).
- Add documentation entry here (this file) describing encoding, operands, flags, exceptions and examples.
- Add tests exercising boundary cases (stack overflow/underflow, memory bounds, arithmetic edge cases such as INT32_MIN absolute).

References
- See `bytecode.h` for inline instruction implementations.
- See `vm.h` for VM layout, flags, exceptions and helper functions.
- See `runtime.h` for ALU helper implementations (`aluAdd`, `aluDiv`, `aluPow`, etc.) which determine exact numeric semantics and which flags/exceptions they set.

If you want, I can:
- Add missing opcode implementations (e.g., `CNE`, `RET`) and update the interpreter dispatch.
- Generate a short assembler macro table showing textual mnemonics → 4-byte encodings for quick reference.