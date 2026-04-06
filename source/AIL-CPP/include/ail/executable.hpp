#pragma once
#include "ail/config.hpp"

namespace ail {

/// Parses and executes an AIL executable (.ila or raw bytecode).
///
/// .ila binary layout (spec §8):
///   Offset 0 : 4 bytes  Magic  = 0x41 0x49 0x4C 0x00 ("AIL\0")
///   Offset 4 : 2 bytes  Format version (little-endian)
///   Offset 6 : 2 bytes  Section count  (little-endian)
///   Each section:
///     Offset 0 : 2 bytes  Section type   (0x0001 = code)
///     Offset 2 : 4 bytes  Section length (little-endian)
///     Offset 6 : N bytes  Section data
///
/// Files without the magic header are treated as raw bytecode.
class Executable {
public:
    /// Extract the code section from @p data (length @p len) into @p out.
    /// @p outLen must equal the number of bytes written on return.
    /// Returns true on success, false if the file is malformed.
    static bool extractCode(const uint8_t* data, int len,
                            const uint8_t*& out, int& outLen);

    /// Load @p data and run it in a fresh VM.
    /// Returns false if the binary is malformed.
    static bool run(const uint8_t* data, int len);

private:
    static bool hasIlaMagic(const uint8_t* data, int len);
};

} // namespace ail
