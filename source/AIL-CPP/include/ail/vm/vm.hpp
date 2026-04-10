#pragma once
#include "ail/config.hpp"
#include "ail/vm/ram.hpp"
#include "ail/vm/call_stack.hpp"
#include "ail/address_mode.hpp"

namespace ail {

/// VM error codes returned by execute() / tick().
enum class VMError : uint8_t {
    None         = 0,
    BadOpcode    = 1,  ///< Unrecognised opcode
    BadAddrMode  = 2,  ///< Invalid addressing mode for this opcode
    BadRegister  = 3,  ///< Byte is not a valid register identifier
    DivByZero    = 4,  ///< DIV with a zero divisor
    MemFault     = 5,  ///< Write to read-only code region
    StackUnder   = 6,  ///< POP or RET on empty stack
    StackOver    = 7,  ///< PSH or CLL overflows the hardware stack / call stack
};

/// AIL virtual machine — heap-free, exception-free, I/O overrideable.
///
/// Addressing (spec §2 + §3):
///   PC and IP are held as uint16_t so the full 16-bit address space (64 KB)
///   is reachable.  Existing binaries compiled by the C# toolchain use only
///   8-bit addresses and work unchanged.
///
/// Instruction layout (6 bytes):
///   byte 0    = (opcode << 2) | addrmode
///   byte 1    = param1  (8-bit register ID or immediate)
///   bytes 2-5 = param2  (32-bit little-endian value or register byte)
///
/// Stack (spec §4):
///   A 256-byte hardware stack separate from RAM, indexed by SP (0xFF → 0x00).
///   SP is read-only to programs.
///
/// Instantiation on embedded:
///   Declare VM as a global or static object so the RAM array (AIL_RAM_SIZE
///   bytes) is placed in BSS, not on the call stack.
class VM {
public:
    /// Construct a VM and load @p codeLen bytes from @p code.
    VM(const uint8_t* code, int codeLen);

    // ── Registers (spec §3) ──────────────────────────────────────────────────
    uint16_t PC = 1;    ///< Program Counter — address of *next* instruction
    uint16_t IP = 0;    ///< Instruction Pointer — current instruction address
    uint8_t  SP = 0xFF; ///< Stack Pointer (read-only to programs, grows ↓)
    uint8_t  SS = 0;    ///< Stack Segment base
    uint8_t  AL = 0;    ///< Lower byte of A (16-bit)
    uint8_t  AH = 0;    ///< Higher byte of A
    uint8_t  BL = 0;    ///< Lower byte of B (16-bit)
    uint8_t  BH = 0;    ///< Higher byte of B
    uint8_t  CL = 0;    ///< Lower byte of C (16-bit)
    uint8_t  CH = 0;    ///< Higher byte of C
    int32_t  X  = 0;    ///< 32-bit general-purpose
    int32_t  Y  = 0;    ///< 32-bit general-purpose

    bool    running  = false;
    VMError lastError = VMError::None; ///< Set when execution halts abnormally

    RAM                  ram;
    uint8_t              stackMemory[256] = {}; ///< Hardware stack (separate from RAM)
    bool                 lastLogic = false;     ///< Result of last TEQ/TNE/TLT/TMT

    // ── Execution ────────────────────────────────────────────────────────────

    /// Run until halt (opcode 0x00 or KEI 0x02) or error.
    /// Returns the final error code (VMError::None on clean halt).
    VMError execute();

    /// Execute exactly one instruction.
    /// Returns VMError::None if the step succeeded.
    VMError tick();

    /// Jump to @p addr and execute from there.
    VMError executeAt(uint16_t addr);

    /// Request the VM to stop after the current instruction.
    void halt();

    // ── Register helpers (used by interrupt handlers) ────────────────────────

    /// Write @p value into the register identified by @p reg (0xF0–0xFE).
    /// SP writes are silently ignored (read-only per spec §3).
    VMError setRegister(uint8_t reg, int32_t value);

    /// Read the current value of the register identified by @p reg.
    int32_t getRegister(uint8_t reg) const;

    /// Set both halves of split register 'A', 'B', or 'C'.
    void setSplit(char which, int32_t value);

    /// Get the combined 16-bit value of split register 'A', 'B', or 'C'.
    int32_t getSplit(char which) const;

    // ── Instruction dispatch (called from execute/tick) ───────────────────────
    VMError parseOpcode(uint8_t opcode);

private:
    AddressMode m_addrMode = AddressMode::RegReg;
    CallStack   m_callStack;

    void    getAddressMode(uint8_t b);

    /// Read a little-endian 32-bit integer from RAM at byte offset @p off.
    int32_t get32(int off) const;
};

} // namespace ail
