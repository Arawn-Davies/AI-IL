#pragma once
#include <cstdint>
#include <array>
#include <string>
#include "ail/address_mode.hpp"

namespace ail::compiler {

/// Maps mnemonic strings to opcode bytes and encodes 6-byte instructions.
///
/// Instruction encoding (spec §2):
///   byte 0   : (opcode << 2) | address_mode
///   byte 1   : param1  (8-bit register byte or immediate)
///   bytes 2-5: param2  (32-bit little-endian value or register byte)
class Instruction {
public:
    /// Returns true when @p token is a known mnemonic (case-insensitive).
    static bool isMnemonic(const std::string& token);

    /// Returns the opcode byte for @p mnemonic, or 0 if unknown.
    static uint8_t getOpcode(const std::string& mnemonic);

    /// Returns the mnemonic string for @p opcode, or "" if unknown.
    static std::string getName(uint8_t opcode);

    /// Encode a single 6-byte instruction.
    static std::array<uint8_t, 6> encode(uint8_t opcode, AddressMode mode,
                                          uint8_t param1, int32_t param2);
};

} // namespace ail::compiler
