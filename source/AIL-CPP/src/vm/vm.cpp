#include "ail/vm/vm.hpp"
#include "ail/registers.hpp"
#include <cstring>

namespace ail {

// ── Constructor ───────────────────────────────────────────────────────────────

VM::VM(const uint8_t* code, int codeLen) {
    memset(stackMemory, 0, sizeof(stackMemory));
    IP = 0;
    PC = 1;
    SP = 0xFF;
    SS = 0;
    AL = AH = BL = BH = CL = CH = 0;
    X  = Y  = 0;
    running   = false;
    lastLogic = false;
    lastError = VMError::None;
    ram.load(code, codeLen);
}

// ── Execution loop ────────────────────────────────────────────────────────────

VMError VM::execute() {
    running = true;
    lastError = VMError::None;
    while (running && ram.memory[IP] != 0x00) {
        uint8_t b0 = ram.memory[IP];
        uint8_t op = static_cast<uint8_t>((b0 >> 2) & 0x3F);
        if (op == 0x00) { running = false; break; }
        getAddressMode(b0);
        VMError e = parseOpcode(op);
        if (e != VMError::None) {
            lastError = e;
            running   = false;
            return e;
        }
        IP = PC;
        PC = static_cast<uint16_t>(PC + 1);
    }
    running = false;
    return lastError;
}

VMError VM::tick() {
    if (ram.memory[IP] == 0x00) { running = false; return VMError::None; }
    uint8_t b0 = ram.memory[IP];
    uint8_t op = static_cast<uint8_t>((b0 >> 2) & 0x3F);
    if (op == 0x00) { running = false; return VMError::None; }
    getAddressMode(b0);
    VMError e = parseOpcode(op);
    if (e != VMError::None) { lastError = e; running = false; return e; }
    IP = PC;
    PC = static_cast<uint16_t>(PC + 1);
    return VMError::None;
}

VMError VM::executeAt(uint16_t addr) {
    IP = addr;
    PC = static_cast<uint16_t>(addr + 1);
    return execute();
}

void VM::halt() { running = false; }

// ── Address mode decode ───────────────────────────────────────────────────────

void VM::getAddressMode(uint8_t b) {
    m_addrMode = static_cast<AddressMode>(b & 0x03);
}

// ── Little-endian 32-bit read from RAM ────────────────────────────────────────

int32_t VM::get32(int off) const {
    return static_cast<int32_t>(
          static_cast<uint32_t>(ram.memory[off    ])
        | (static_cast<uint32_t>(ram.memory[off + 1]) <<  8)
        | (static_cast<uint32_t>(ram.memory[off + 2]) << 16)
        | (static_cast<uint32_t>(ram.memory[off + 3]) << 24));
}

// ── Register access ───────────────────────────────────────────────────────────

VMError VM::setRegister(uint8_t reg, int32_t value) {
    switch (reg) {
        case Registers::PC: PC = static_cast<uint16_t>(value);       break;
        case Registers::IP: IP = static_cast<uint16_t>(value);       break;
        case Registers::SP: /* read-only — spec §3, silent ignore */   break;
        case Registers::SS: SS = static_cast<uint8_t>(value);        break;
        case Registers::A:  setSplit('A', value);                      break;
        case Registers::AL: AL = static_cast<uint8_t>(value);        break;
        case Registers::AH: AH = static_cast<uint8_t>(value);        break;
        case Registers::B:  setSplit('B', value);                      break;
        case Registers::BL: BL = static_cast<uint8_t>(value);        break;
        case Registers::BH: BH = static_cast<uint8_t>(value);        break;
        case Registers::C:  setSplit('C', value);                      break;
        case Registers::CL: CL = static_cast<uint8_t>(value);        break;
        case Registers::CH: CH = static_cast<uint8_t>(value);        break;
        case Registers::X:  X  = value;                                break;
        case Registers::Y:  Y  = value;                                break;
        default: return VMError::BadRegister;
    }
    return VMError::None;
}

int32_t VM::getRegister(uint8_t reg) const {
    switch (reg) {
        case Registers::PC: return static_cast<int32_t>(PC);
        case Registers::IP: return static_cast<int32_t>(IP);
        case Registers::SP: return static_cast<int32_t>(SP);
        case Registers::SS: return static_cast<int32_t>(SS);
        case Registers::A:  return getSplit('A');
        case Registers::AL: return static_cast<int32_t>(AL);
        case Registers::AH: return static_cast<int32_t>(AH);
        case Registers::B:  return getSplit('B');
        case Registers::BL: return static_cast<int32_t>(BL);
        case Registers::BH: return static_cast<int32_t>(BH);
        case Registers::C:  return getSplit('C');
        case Registers::CL: return static_cast<int32_t>(CL);
        case Registers::CH: return static_cast<int32_t>(CH);
        case Registers::X:  return X;
        case Registers::Y:  return Y;
        default:            return 0;
    }
}

void VM::setSplit(char which, int32_t value) {
    uint8_t lo = static_cast<uint8_t>(value & 0xFF);
    uint8_t hi = static_cast<uint8_t>((value >> 8) & 0xFF);
    if (which == 'A') { AL = lo; AH = hi; }
    else if (which == 'B') { BL = lo; BH = hi; }
    else if (which == 'C') { CL = lo; CH = hi; }
}

int32_t VM::getSplit(char which) const {
    if (which == 'A') return static_cast<int32_t>(AL) | (static_cast<int32_t>(AH) << 8);
    if (which == 'B') return static_cast<int32_t>(BL) | (static_cast<int32_t>(BH) << 8);
    if (which == 'C') return static_cast<int32_t>(CL) | (static_cast<int32_t>(CH) << 8);
    return 0;
}

} // namespace ail
