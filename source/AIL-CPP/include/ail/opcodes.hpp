#pragma once
#include <cstdint>

namespace ail {

/// Opcode constants as defined in spec §7.
struct Opcodes {
    // Register / Memory
    static constexpr uint8_t MOV = 0x01;
    static constexpr uint8_t SWP = 0x02;
    // Arithmetic
    static constexpr uint8_t ADD = 0x04;
    static constexpr uint8_t SUB = 0x05;
    // Bitwise
    static constexpr uint8_t SHL = 0x06;
    static constexpr uint8_t SHR = 0x07;
    // Arithmetic (cont.)
    static constexpr uint8_t INC = 0x08;
    static constexpr uint8_t DEC = 0x09;
    // Bitwise (cont.)
    static constexpr uint8_t AND = 0x0A;
    static constexpr uint8_t BOR = 0x0B;
    static constexpr uint8_t XOR = 0x0C;
    static constexpr uint8_t NOT = 0x0D;
    static constexpr uint8_t ROL = 0x0E;
    static constexpr uint8_t ROR = 0x0F;
    // Flow Control
    static constexpr uint8_t JMP = 0x10;
    static constexpr uint8_t CLL = 0x11;
    static constexpr uint8_t RET = 0x12;
    static constexpr uint8_t JMT = 0x13;
    static constexpr uint8_t JMF = 0x14;
    static constexpr uint8_t CLT = 0x17;
    static constexpr uint8_t CLF = 0x18;
    // Test
    static constexpr uint8_t TEQ = 0x1A;
    static constexpr uint8_t TNE = 0x1B;
    static constexpr uint8_t TLT = 0x1C;
    static constexpr uint8_t TMT = 0x1D;
    // Stack
    static constexpr uint8_t PSH = 0x20;
    static constexpr uint8_t POP = 0x21;
    // I/O
    static constexpr uint8_t INB = 0x24;
    static constexpr uint8_t INW = 0x25;
    static constexpr uint8_t IND = 0x26;
    static constexpr uint8_t OUB = 0x27;
    static constexpr uint8_t OUW = 0x28;
    static constexpr uint8_t OUD = 0x29;
    // Interrupts
    static constexpr uint8_t SWI = 0x2A;
    static constexpr uint8_t KEI = 0x2B;
    // Arithmetic (cont.)
    static constexpr uint8_t MUL = 0x30;
    static constexpr uint8_t DIV = 0x31;
    // Register / Memory (cont.)
    static constexpr uint8_t MOM = 0x3A;
    static constexpr uint8_t MOE = 0x3B;
};

} // namespace ail
