#pragma once
#include "ail/config.hpp"

namespace ail {

/// Flat RAM backed by a fixed-size array of AIL_RAM_SIZE bytes.
///
/// Memory layout:
///   [0 .. ramLimit-1]       : read-only loaded executable (code guard)
///   [ramLimit .. RAM_SIZE-1]: read/write data area
///
/// On embedded targets (AIL_PLATFORM_EMBEDDED) instantiate VM as a global or
/// static object so the array lives in BSS rather than on the call stack.
class RAM {
public:
    static constexpr int SIZE = AIL_RAM_SIZE;

    uint8_t memory[SIZE] = {};

    /// One past the last byte of the loaded executable.
    /// setByte() refuses writes below this address.
    int ramLimit = 0;

    /// Copy @p len bytes from @p data into memory[0..] and set ramLimit.
    void load(const uint8_t* data, int len);

    /// Write @p value to @p address (must be >= ramLimit).
    /// On violation: asserts in debug builds; no-ops in release.
    void setByte(int address, uint8_t value);

    /// Read the byte at @p address.
    uint8_t getByte(int address) const;

    /// Copy @p length bytes starting at @p address into @p out.
    void getSection(int address, int length, uint8_t* out) const;

    /// Write @p length bytes from @p data into memory at @p address.
    void setSection(int address, const uint8_t* data, int length);
};

} // namespace ail
