#include "ail/compiler/instruction.hpp"
#include "ail/opcodes.hpp"
#include <algorithm>
#include <cctype>

namespace ail::compiler {

// ── Opcode table ──────────────────────────────────────────────────────────────

struct OpcodeEntry { const char* mnemonic; uint8_t opcode; };

static const OpcodeEntry kTable[] = {
    { "MOV", Opcodes::MOV }, { "MOM", Opcodes::MOM }, { "MOE", Opcodes::MOE },
    { "SWP", Opcodes::SWP },
    { "TEQ", Opcodes::TEQ }, { "TNE", Opcodes::TNE },
    { "TLT", Opcodes::TLT }, { "TMT", Opcodes::TMT },
    { "ADD", Opcodes::ADD }, { "SUB", Opcodes::SUB },
    { "INC", Opcodes::INC }, { "DEC", Opcodes::DEC },
    { "MUL", Opcodes::MUL }, { "DIV", Opcodes::DIV },
    { "SHL", Opcodes::SHL }, { "SHR", Opcodes::SHR },
    { "ROL", Opcodes::ROL }, { "ROR", Opcodes::ROR },
    { "AND", Opcodes::AND }, { "BOR", Opcodes::BOR },
    { "XOR", Opcodes::XOR }, { "NOT", Opcodes::NOT },
    { "JMP", Opcodes::JMP }, { "CLL", Opcodes::CLL }, { "RET", Opcodes::RET },
    { "JMT", Opcodes::JMT }, { "JMF", Opcodes::JMF },
    { "CLT", Opcodes::CLT }, { "CLF", Opcodes::CLF },
    { "PSH", Opcodes::PSH }, { "POP", Opcodes::POP },
    { "INB", Opcodes::INB }, { "INW", Opcodes::INW }, { "IND", Opcodes::IND },
    { "OUB", Opcodes::OUB }, { "OUW", Opcodes::OUW }, { "OUD", Opcodes::OUD },
    { "SWI", Opcodes::SWI }, { "KEI", Opcodes::KEI },
};
static constexpr int kTableSize = static_cast<int>(sizeof(kTable) / sizeof(kTable[0]));

// ── Reverse table (opcode → name) ─────────────────────────────────────────────

static const char* kOpcodeNames[64] = {}; // indexed by opcode byte
struct TableInit {
    TableInit() {
        for (int i = 0; i < kTableSize; ++i)
            kOpcodeNames[kTable[i].opcode & 0x3F] = kTable[i].mnemonic;
    }
} static s_init;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string toUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool Instruction::isMnemonic(const std::string& token) {
    std::string up = toUpper(token);
    for (int i = 0; i < kTableSize; ++i)
        if (up == kTable[i].mnemonic) return true;
    return false;
}

uint8_t Instruction::getOpcode(const std::string& mnemonic) {
    std::string up = toUpper(mnemonic);
    for (int i = 0; i < kTableSize; ++i)
        if (up == kTable[i].mnemonic) return kTable[i].opcode;
    return 0;
}

std::string Instruction::getName(uint8_t opcode) {
    const char* n = kOpcodeNames[opcode & 0x3F];
    return n ? std::string(n) : std::string();
}

std::array<uint8_t, 6> Instruction::encode(uint8_t opcode, AddressMode mode,
                                            uint8_t param1, int32_t param2) {
    std::array<uint8_t, 6> instr{};
    instr[0] = static_cast<uint8_t>((opcode << 2) | static_cast<uint8_t>(mode));
    instr[1] = param1;
    // Little-endian param2 (explicit, host-endian independent)
    instr[2] = static_cast<uint8_t>( static_cast<uint32_t>(param2)        & 0xFF);
    instr[3] = static_cast<uint8_t>((static_cast<uint32_t>(param2) >>  8) & 0xFF);
    instr[4] = static_cast<uint8_t>((static_cast<uint32_t>(param2) >> 16) & 0xFF);
    instr[5] = static_cast<uint8_t>((static_cast<uint32_t>(param2) >> 24) & 0xFF);
    return instr;
}

} // namespace ail::compiler
