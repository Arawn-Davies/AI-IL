#include "ail/vm/vm.hpp"
#include "ail/vm/kernel_interrupts.hpp"
#include "ail/vm/software_interrupts.hpp"
#include "ail/registers.hpp"
#include "ail/opcodes.hpp"

// All instructions are exactly 6 bytes (spec §2).
// On entry to parseOpcode:
//   IP  = address of byte 0 (the opcode/mode byte already read)
//   PC  = IP + 1 (set by execute() before the call)
// For sequential instructions PC is advanced by +5 (so PC = IP + 6 = next instr).
// For jump/call instructions PC is set directly to the target address.
// On return execute() does: IP = PC; PC = PC + 1.

namespace ail {

// p1IsReg: param1 is a register identifier (RegReg or RegVal mode).
// Used by jump/call/PSH instructions where the first operand selects
// either a register or the instruction encodes the target in param2.
static inline bool p1IsReg(AddressMode m) {
    return m == AddressMode::RegReg || m == AddressMode::RegVal;
}

VMError VM::parseOpcode(uint8_t opcode) {
    switch (opcode) {

    // ── Register / Memory ────────────────────────────────────────────────────

    case Opcodes::MOV: { // 0x01 — dest = src  (RegReg | RegVal)
        uint8_t dest = ram.memory[IP + 1];
        if (!Registers::isRegister(dest)) return VMError::BadRegister;
        int32_t val  = (m_addrMode == AddressMode::RegVal)
                       ? get32(IP + 2)
                       : getRegister(ram.memory[IP + 2]);
        setRegister(dest, val);
        PC += 5; break;
    }

    case Opcodes::SWP: { // 0x02 — swap two registers
        uint8_t r1 = ram.memory[IP + 1];
        uint8_t r2 = ram.memory[IP + 2];
        int32_t tmp = getRegister(r1);
        setRegister(r1, getRegister(r2));
        setRegister(r2, tmp);
        PC += 5; break;
    }

    case Opcodes::MOM: { // 0x3A — write register/value to memory address
        int32_t  destAddr = get32(IP + 2);
        uint8_t  srcByte  = ram.memory[IP + 1];
        uint8_t  srcVal   = (m_addrMode == AddressMode::RegVal)
                             ? static_cast<uint8_t>(getRegister(srcByte))
                             : srcByte;
        ram.setByte(destAddr, srcVal);
        PC += 5; break;
    }

    case Opcodes::MOE: { // 0x3B — read memory into register
        uint8_t destReg = ram.memory[IP + 1];
        int32_t srcAddr = get32(IP + 2);
        setRegister(destReg, static_cast<int32_t>(ram.getByte(srcAddr)));
        PC += 5; break;
    }

    // ── Arithmetic ────────────────────────────────────────────────────────────

    case Opcodes::ADD: { // 0x04
        uint8_t dest = ram.memory[IP + 1];
        int32_t src  = (m_addrMode == AddressMode::RegReg)
                       ? getRegister(ram.memory[IP + 2])
                       : get32(IP + 2);
        setRegister(dest, getRegister(dest) + src);
        PC += 5; break;
    }

    case Opcodes::SUB: { // 0x05
        uint8_t dest = ram.memory[IP + 1];
        int32_t src  = (m_addrMode == AddressMode::RegReg)
                       ? getRegister(ram.memory[IP + 2])
                       : get32(IP + 2);
        setRegister(dest, getRegister(dest) - src);
        PC += 5; break;
    }

    case Opcodes::INC: { // 0x08
        uint8_t reg = ram.memory[IP + 1];
        setRegister(reg, getRegister(reg) + 1);
        PC += 5; break;
    }

    case Opcodes::DEC: { // 0x09
        uint8_t reg = ram.memory[IP + 1];
        setRegister(reg, getRegister(reg) - 1);
        PC += 5; break;
    }

    case Opcodes::MUL: { // 0x30
        uint8_t dest = ram.memory[IP + 1];
        int32_t src  = (m_addrMode == AddressMode::RegReg)
                       ? getRegister(ram.memory[IP + 2])
                       : get32(IP + 2);
        setRegister(dest, getRegister(dest) * src);
        PC += 5; break;
    }

    case Opcodes::DIV: { // 0x31 — integer division; returns VMError::DivByZero
        uint8_t dest = ram.memory[IP + 1];
        int32_t src  = (m_addrMode == AddressMode::RegReg)
                       ? getRegister(ram.memory[IP + 2])
                       : get32(IP + 2);
        if (src == 0) return VMError::DivByZero;
        setRegister(dest, getRegister(dest) / src);
        PC += 5; break;
    }

    // ── Bitwise ───────────────────────────────────────────────────────────────

    case Opcodes::SHL: { // 0x06 — logical shift left
        uint8_t  src = ram.memory[IP + 1];
        uint32_t n   = static_cast<uint32_t>(get32(IP + 2)) & 31u;
        setRegister(src, static_cast<int32_t>(
            static_cast<uint32_t>(getRegister(src)) << n));
        PC += 5; break;
    }

    case Opcodes::SHR: { // 0x07 — logical shift right (unsigned)
        uint8_t  src = ram.memory[IP + 1];
        uint32_t n   = static_cast<uint32_t>(get32(IP + 2)) & 31u;
        setRegister(src, static_cast<int32_t>(
            static_cast<uint32_t>(getRegister(src)) >> n));
        PC += 5; break;
    }

    case Opcodes::ROL: { // 0x0E — rotate left
        uint8_t  src = ram.memory[IP + 1];
        uint32_t n   = static_cast<uint32_t>(get32(IP + 2)) & 31u;
        uint32_t v   = static_cast<uint32_t>(getRegister(src));
        setRegister(src, static_cast<int32_t>((v << n) | (v >> (32u - n))));
        PC += 5; break;
    }

    case Opcodes::ROR: { // 0x0F — rotate right
        uint8_t  src = ram.memory[IP + 1];
        uint32_t n   = static_cast<uint32_t>(get32(IP + 2)) & 31u;
        uint32_t v   = static_cast<uint32_t>(getRegister(src));
        setRegister(src, static_cast<int32_t>((v >> n) | (v << (32u - n))));
        PC += 5; break;
    }

    case Opcodes::AND: { // 0x0A
        uint8_t dest = ram.memory[IP + 1];
        int32_t src  = (m_addrMode == AddressMode::RegReg)
                       ? getRegister(ram.memory[IP + 2]) : get32(IP + 2);
        setRegister(dest, getRegister(dest) & src);
        PC += 5; break;
    }

    case Opcodes::BOR: { // 0x0B
        uint8_t dest = ram.memory[IP + 1];
        int32_t src  = (m_addrMode == AddressMode::RegReg)
                       ? getRegister(ram.memory[IP + 2]) : get32(IP + 2);
        setRegister(dest, getRegister(dest) | src);
        PC += 5; break;
    }

    case Opcodes::XOR: { // 0x0C
        uint8_t dest = ram.memory[IP + 1];
        int32_t src  = (m_addrMode == AddressMode::RegReg)
                       ? getRegister(ram.memory[IP + 2]) : get32(IP + 2);
        setRegister(dest, getRegister(dest) ^ src);
        PC += 5; break;
    }

    case Opcodes::NOT: { // 0x0D
        uint8_t src = ram.memory[IP + 1];
        setRegister(src, ~getRegister(src));
        PC += 5; break;
    }

    // ── Flow control ─────────────────────────────────────────────────────────

    case Opcodes::JMP: { // 0x10 — unconditional jump
        if (p1IsReg(m_addrMode))
            PC = static_cast<uint16_t>(getRegister(ram.memory[IP + 1]));
        else
            PC = static_cast<uint16_t>(get32(IP + 2));
        // No PC += 5: execute() sets IP = PC; PC++ after return
        break;
    }

    case Opcodes::CLL: { // 0x11 — call subroutine (push return addr, jump)
        // Return address = start of the instruction after this 6-byte one.
        // At this point PC = IP+1; the full instruction spans IP..IP+5,
        // so the next instruction starts at IP+6.
        if (!m_callStack.call(static_cast<int>(IP) + 6))
            return VMError::StackOver;
        if (p1IsReg(m_addrMode))
            PC = static_cast<uint16_t>(getRegister(ram.memory[IP + 1]));
        else
            PC = static_cast<uint16_t>(get32(IP + 2));
        break;
    }

    case Opcodes::RET: { // 0x12 — return from subroutine
        int ret = m_callStack.ret();
        if (ret < 0) return VMError::StackUnder;
        PC = static_cast<uint16_t>(ret);
        break;
    }

    case Opcodes::JMT: { // 0x13 — jump if last test was true
        if (lastLogic) {
            if (p1IsReg(m_addrMode))
                PC = static_cast<uint16_t>(getRegister(ram.memory[IP + 1]));
            else
                PC = static_cast<uint16_t>(get32(IP + 2));
        } else {
            PC += 5;
        }
        break;
    }

    case Opcodes::JMF: { // 0x14 — jump if last test was false
        if (!lastLogic) {
            if (p1IsReg(m_addrMode))
                PC = static_cast<uint16_t>(getRegister(ram.memory[IP + 1]));
            else
                PC = static_cast<uint16_t>(get32(IP + 2));
        } else {
            PC += 5;
        }
        break;
    }

    case Opcodes::CLT: { // 0x17 — call if true
        if (lastLogic) {
            if (!m_callStack.call(static_cast<int>(IP) + 6))
                return VMError::StackOver;
            if (p1IsReg(m_addrMode))
                PC = static_cast<uint16_t>(getRegister(ram.memory[IP + 1]));
            else
                PC = static_cast<uint16_t>(get32(IP + 2));
        } else {
            PC += 5;
        }
        break;
    }

    case Opcodes::CLF: { // 0x18 — call if false
        if (!lastLogic) {
            if (!m_callStack.call(static_cast<int>(IP) + 6))
                return VMError::StackOver;
            if (p1IsReg(m_addrMode))
                PC = static_cast<uint16_t>(getRegister(ram.memory[IP + 1]));
            else
                PC = static_cast<uint16_t>(get32(IP + 2));
        } else {
            PC += 5;
        }
        break;
    }

    // ── Test (set lastLogic flag) ─────────────────────────────────────────────

    case Opcodes::TEQ: // 0x1A
        lastLogic = getRegister(ram.memory[IP + 1]) == getRegister(ram.memory[IP + 2]);
        PC += 5; break;

    case Opcodes::TNE: // 0x1B
        lastLogic = getRegister(ram.memory[IP + 1]) != getRegister(ram.memory[IP + 2]);
        PC += 5; break;

    case Opcodes::TLT: // 0x1C
        lastLogic = getRegister(ram.memory[IP + 1]) <  getRegister(ram.memory[IP + 2]);
        PC += 5; break;

    case Opcodes::TMT: // 0x1D
        lastLogic = getRegister(ram.memory[IP + 1]) >  getRegister(ram.memory[IP + 2]);
        PC += 5; break;

    // ── Stack ─────────────────────────────────────────────────────────────────

    case Opcodes::PSH: { // 0x20 — push (SP grows downward)
        if (SP == 0) return VMError::StackOver;
        uint8_t val = p1IsReg(m_addrMode)
                      ? static_cast<uint8_t>(getRegister(ram.memory[IP + 1]))
                      : ram.memory[IP + 1];
        --SP;
        stackMemory[SP] = val;
        PC += 5; break;
    }

    case Opcodes::POP: { // 0x21 — pop
        if (SP == 0xFF) return VMError::StackUnder;
        uint8_t val = stackMemory[SP];
        ++SP;
        setRegister(ram.memory[IP + 1], static_cast<int32_t>(val));
        PC += 5; break;
    }

    // ── I/O (reserved; no-ops per current spec) ────────────────────────────────

    case Opcodes::INB: case Opcodes::INW: case Opcodes::IND:
    case Opcodes::OUB: case Opcodes::OUW: case Opcodes::OUD:
        PC += 5; break;

    // ── Interrupts ─────────────────────────────────────────────────────────────

    case Opcodes::SWI: { // 0x2A — software interrupt
        int cmd = static_cast<int>(ram.memory[IP + 1]);
        SoftwareInterrupts::handleInterrupt(*this, cmd);
        PC += 5; break;
    }

    case Opcodes::KEI: { // 0x2B — kernel interrupt
        int cmd = static_cast<int>(ram.memory[IP + 1]);
        KernelInterrupts::handleInterrupt(*this, cmd);
        PC += 5; break;
    }

    default:
        return VMError::BadOpcode;
    }

    return VMError::None;
}

} // namespace ail
