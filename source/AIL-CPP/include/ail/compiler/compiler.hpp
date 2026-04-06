#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "ail/compiler/build_exception.hpp"

namespace ail::compiler {

/// Two-pass AIL assembler.
///
/// Syntax accepted:
///   ; comment          // comment
///   label:
///   MNEMONIC [param1 [, param2]]
///   DB value1 [, value2 ...]
///
/// Operand forms:
///   Register name (case-insensitive): PC IP SP SS A AL AH B BL BH C CL CH X Y
///   Decimal integer:  42
///   Hex integer:      0xFF
///   Char literal:     'H'  '\n'  ' '
///   Label reference:  main   loop
///   String literal (DB only): "Hello, World"
///
/// Instruction encoding (spec §2) — every instruction is exactly 6 bytes:
///   byte 0   : (opcode << 2) | address_mode
///   byte 1   : param1  (8-bit register byte or immediate)
///   bytes 2-5: param2  (32-bit little-endian value or register byte)
///
/// DB pseudo-instruction emits raw bytes (variable length).
/// Labels are resolved to their byte offsets in pass 1.
///
/// Throws BuildException on any syntax or semantic error.
class Compiler {
public:
    explicit Compiler(const std::string& source);

    /// Assemble the source and return the raw bytecode.
    /// Also stored in byteCode after a successful call.
    std::vector<uint8_t> compile();

    /// Wrap raw bytecode in a .ila container (spec §8) and return the binary.
    static std::vector<uint8_t> wrapIla(const std::vector<uint8_t>& code);

    /// Raw bytecode produced by compile().
    std::vector<uint8_t> byteCode;

private:
    const std::string m_source;

    // ── Pass helpers ──────────────────────────────────────────────────────────
    std::vector<uint8_t> assembleLine(
        const std::vector<std::string>& tokens,
        const std::unordered_map<std::string, int>& labels,
        int lineNum);

    std::vector<uint8_t> assembleDb(
        const std::vector<std::string>& tokens,
        int lineNum);

    int countDbBytes(const std::vector<std::string>& tokens, int lineNum);

    // ── Token / parse utilities ───────────────────────────────────────────────
    static std::string             stripComment(const std::string& line);
    static std::vector<std::string> tokenise(const std::string& line);

    static std::tuple<bool, uint8_t, int32_t> parseParam(
        const std::string& token,
        const std::unordered_map<std::string, int>& labels,
        int lineNum,
        bool isParam1);

    static bool   tryParseInteger(const std::string& tok, int32_t& out);
    static int     dbTokenByteCount(const std::string& tok, int lineNum);
};

} // namespace ail::compiler
