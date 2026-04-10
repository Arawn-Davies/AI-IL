#include "ail/decompiler/decompiler.hpp"
#include "ail/compiler/instruction.hpp"  // reuse getName() / opcode table
#include "ail/registers.hpp"
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace ail::decompiler {

static const uint8_t kMagic[4] = { 0x41, 0x49, 0x4C, 0x00 };
static constexpr uint16_t kSectionTypeCode = 0x0001;

// ── Register name lookup ──────────────────────────────────────────────────────

struct RegEntry { uint8_t byte; const char* name; };
static const RegEntry kRegs[] = {
    {0xF0,"PC"},{0xF1,"IP"},{0xF2,"SP"},{0xF3,"SS"},
    {0xF4,"A"}, {0xF5,"AL"},{0xF6,"AH"},
    {0xF7,"B"}, {0xF8,"BL"},{0xF9,"BH"},
    {0xFA,"C"}, {0xFB,"CL"},{0xFC,"CH"},
    {0xFD,"X"}, {0xFE,"Y"},
};
static constexpr int kRegCount = static_cast<int>(sizeof(kRegs)/sizeof(kRegs[0]));

static const char* regName(uint8_t b) {
    for (int i = 0; i < kRegCount; ++i)
        if (kRegs[i].byte == b) return kRegs[i].name;
    return nullptr;
}

// ── .ila stripping ────────────────────────────────────────────────────────────

std::vector<uint8_t> Decompiler::extractCode(const std::vector<uint8_t>& data) {
    if (data.size() < 8) return data;
    for (int i = 0; i < 4; ++i)
        if (data[static_cast<size_t>(i)] != kMagic[i]) return data;

    uint16_t secCount = static_cast<uint16_t>(data[6])
                      | static_cast<uint16_t>(data[7] << 8);
    int offset = 8;
    for (uint16_t s = 0; s < secCount; ++s) {
        if (offset + 6 > static_cast<int>(data.size())) break;
        uint16_t type = static_cast<uint16_t>(data[static_cast<size_t>(offset)])
                      | static_cast<uint16_t>(data[static_cast<size_t>(offset+1)] << 8);
        int len = static_cast<int>(
                    static_cast<uint32_t>(data[static_cast<size_t>(offset+2)])
                  | (static_cast<uint32_t>(data[static_cast<size_t>(offset+3)]) <<  8)
                  | (static_cast<uint32_t>(data[static_cast<size_t>(offset+4)]) << 16)
                  | (static_cast<uint32_t>(data[static_cast<size_t>(offset+5)]) << 24));
        offset += 6;
        if (type == kSectionTypeCode) {
            int end = offset + len;
            if (end > static_cast<int>(data.size())) end = static_cast<int>(data.size());
            return std::vector<uint8_t>(data.begin() + offset,
                                        data.begin() + end);
        }
        offset += len;
    }
    return data; // fallback: treat as raw
}

// ── Instruction formatting ────────────────────────────────────────────────────

std::string Decompiler::formatInstruction(const std::string& name,
    uint8_t opcode, uint8_t addrMode,
    uint8_t p1b, uint8_t p2b0, int32_t p2)
{
    std::ostringstream os;
    os << std::uppercase << std::hex;

    auto regStr = [](uint8_t b) -> std::string {
        const char* n = regName(b);
        return n ? std::string(n) : "";
    };
    bool p1isReg = regName(p1b) != nullptr;
    bool addrIsReg = (addrMode == 0 || addrMode == 2); // RegReg or RegVal

    switch (opcode) {
    // No operands
    case 0x12: // RET
        os << name;
        break;

    // Single register operand
    case 0x08: case 0x09: case 0x0D: case 0x21: // INC DEC NOT POP
        os << name << " " << regStr(p1b);
        break;

    // Single operand — register or value (jumps, PSH)
    case 0x10: case 0x11: case 0x13: case 0x14: case 0x17: case 0x18: // JMP CLL JMT JMF CLT CLF
    case 0x20: // PSH
        if (addrIsReg)
            os << name << " " << regStr(p1b);
        else
            os << name << " 0x" << std::setw(2) << std::setfill('0')
               << (static_cast<unsigned>(p2) & 0xFF);
        break;

    // Single value operand in p1b (interrupt commands)
    case 0x2A: case 0x2B: // SWI KEI
        os << name << " 0x" << std::setw(2) << std::setfill('0')
           << static_cast<unsigned>(p1b);
        break;

    // Two register operands
    case 0x02: case 0x1A: case 0x1B: case 0x1C: case 0x1D: // SWP TEQ TNE TLT TMT
        os << name << " " << regStr(p1b) << ", " << regStr(p2b0);
        break;

    // Shift / rotate: reg, immediate
    case 0x06: case 0x07: case 0x0E: case 0x0F: // SHL SHR ROL ROR
        os << name << " " << regStr(p1b) << ", " << std::dec << p2;
        break;

    // MOM src, addr
    case 0x3A: {
        std::string src = p1isReg ? regStr(p1b)
                                  : ("0x" + [&]{ std::ostringstream t;
                                       t << std::uppercase << std::hex
                                         << std::setw(2) << std::setfill('0')
                                         << static_cast<unsigned>(p1b);
                                       return t.str(); }());
        os << name << " " << src << ", 0x"
           << std::setw(4) << std::setfill('0')
           << (static_cast<unsigned>(p2) & 0xFFFF);
        break;
    }

    // MOE dest, addr
    case 0x3B:
        os << name << " " << regStr(p1b) << ", 0x"
           << std::setw(4) << std::setfill('0')
           << (static_cast<unsigned>(p2) & 0xFFFF);
        break;

    // I/O: port, reg
    case 0x24: case 0x25: case 0x26:
    case 0x27: case 0x28: case 0x29: {
        const char* rn = regName(p2b0);
        std::string r2 = rn ? std::string(rn)
                            : ("0x" + [&]{ std::ostringstream t;
                                 t << std::uppercase << std::hex
                                   << std::setw(2) << std::setfill('0')
                                   << static_cast<unsigned>(p2b0);
                                 return t.str(); }());
        os << name << " 0x" << std::setw(2) << std::setfill('0')
           << static_cast<unsigned>(p1b) << ", " << r2;
        break;
    }

    // Default: two-operand (MOV ADD SUB MUL DIV AND BOR XOR)
    default: {
        std::string p2str = (addrMode == 0) // RegReg
            ? regStr(p2b0)
            : ("0x" + [&]{ std::ostringstream t;
                   t << std::uppercase << std::hex
                     << std::setw(2) << std::setfill('0')
                     << (static_cast<unsigned>(p2) & 0xFF);
                   return t.str(); }());
        os << name << " " << regStr(p1b) << ", " << p2str;
        break;
    }
    }
    return os.str();
}

// ── Constructor + decompile ───────────────────────────────────────────────────

Decompiler::Decompiler(const std::vector<uint8_t>& binary) : m_binary(binary) {}

std::string Decompiler::decompile() const {
    if (m_binary.empty())
        return "; empty binary\n";

    std::vector<uint8_t> code = extractCode(m_binary);
    std::ostringstream out;
    out << "; Decompiled by AIL C++ port\n\n";

    for (int i = 0; i + 6 <= static_cast<int>(code.size()); i += 6) {
        uint8_t b0      = code[static_cast<size_t>(i)];
        uint8_t opcode  = static_cast<uint8_t>((b0 >> 2) & 0x3F);
        uint8_t addrMode= static_cast<uint8_t>(b0 & 0x03);

        if (opcode == 0x00) break; // null opcode = end

        uint8_t p1b  = code[static_cast<size_t>(i + 1)];
        uint8_t p2b0 = code[static_cast<size_t>(i + 2)];
        int32_t p2   = static_cast<int32_t>(
            static_cast<uint32_t>(code[static_cast<size_t>(i + 2)])
          | (static_cast<uint32_t>(code[static_cast<size_t>(i + 3)]) <<  8)
          | (static_cast<uint32_t>(code[static_cast<size_t>(i + 4)]) << 16)
          | (static_cast<uint32_t>(code[static_cast<size_t>(i + 5)]) << 24));

        std::string name = ail::compiler::Instruction::getName(opcode);
        if (name.empty()) {
            out << "; unknown opcode 0x" << std::uppercase << std::hex
                << std::setw(2) << std::setfill('0')
                << static_cast<unsigned>(opcode)
                << " at offset " << std::dec << i << "\n";
            continue;
        }
        out << "    " << formatInstruction(name, opcode, addrMode, p1b, p2b0, p2) << "\n";
    }

    return out.str();
}

} // namespace ail::decompiler
