#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ail::decompiler {

/// AIL binary disassembler.
///
/// Accepts either:
///   • A structured .ila binary (magic "AIL\0" + sections per spec §8)
///   • Raw bytecode (no header)
///
/// Decodes each 6-byte instruction back to assembly text.
/// Unknown opcodes are emitted as comments rather than aborting.
class Decompiler {
public:
    explicit Decompiler(const std::vector<uint8_t>& binary);

    /// Decode the binary and return assembly source text.
    std::string decompile() const;

private:
    const std::vector<uint8_t> m_binary;

    static std::vector<uint8_t> extractCode(const std::vector<uint8_t>& data);

    static std::string formatInstruction(
        const std::string& name,
        uint8_t  opcode,
        uint8_t  addrMode,
        uint8_t  p1b,
        uint8_t  p2b0,
        int32_t  p2);
};

} // namespace ail::decompiler
