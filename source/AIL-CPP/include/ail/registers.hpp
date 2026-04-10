#pragma once
#include <cstdint>

namespace ail {

/// Register identifier bytes (0xF0–0xFE) as defined in spec §3.
struct Registers {
    static constexpr uint8_t PC = 0xF0; ///< Program Counter (8-bit)
    static constexpr uint8_t IP = 0xF1; ///< Instruction Pointer (8-bit)
    static constexpr uint8_t SP = 0xF2; ///< Stack Pointer — read-only
    static constexpr uint8_t SS = 0xF3; ///< Stack Segment
    static constexpr uint8_t A  = 0xF4; ///< General purpose 16-bit (AL + AH)
    static constexpr uint8_t AL = 0xF5; ///< Lower byte of A
    static constexpr uint8_t AH = 0xF6; ///< Higher byte of A
    static constexpr uint8_t B  = 0xF7; ///< General purpose 16-bit (BL + BH)
    static constexpr uint8_t BL = 0xF8; ///< Lower byte of B
    static constexpr uint8_t BH = 0xF9; ///< Higher byte of B
    static constexpr uint8_t C  = 0xFA; ///< General purpose 16-bit (CL + CH)
    static constexpr uint8_t CL = 0xFB; ///< Lower byte of C
    static constexpr uint8_t CH = 0xFC; ///< Higher byte of C
    static constexpr uint8_t X  = 0xFD; ///< General purpose 32-bit
    static constexpr uint8_t Y  = 0xFE; ///< General purpose 32-bit

    /// Returns true if the byte identifies a known register.
    static constexpr bool isRegister(uint8_t b) noexcept {
        return b >= 0xF0 && b <= 0xFE;
    }
};

} // namespace ail
