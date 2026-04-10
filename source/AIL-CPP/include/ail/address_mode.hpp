#pragma once
#include "ail/config.hpp"

namespace ail {

/// 2-bit addressing mode field (bits 1-0 of instruction byte 0).
/// Controls how the VM interprets each parameter.
enum class AddressMode : uint8_t {
    RegReg = 0x00, ///< Both operands are register identifiers.
    ValReg = 0x01, ///< param1 is an immediate value, param2 is a register.
    RegVal = 0x02, ///< param1 is a register, param2 is an immediate value.
    ValVal = 0x03, ///< Both operands are immediate values / addresses.
};

} // namespace ail
