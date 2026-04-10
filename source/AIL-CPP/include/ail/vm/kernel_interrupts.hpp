#pragma once

namespace ail {

class VM; // forward declaration

/// Kernel interrupt handler for KEI instructions (opcode 0x2B).
///
/// KEI 0x01 — stdio (behaviour selected by register AL):
///   AL=0x01  Write the character in AH to stdout.
///   AL=0x02  Write a string from RAM: X = base address, B = byte count.
///   AL=0x03  Read one character from stdin into AH.
///   AL=0x04  Read a line from stdin into RAM at address X; B = bytes written.
///   AL=0x05  Write the 16-bit value of register B as a decimal to stdout.
///
/// KEI 0x02 — halt:
///   Writes "Halting!" to stdout and stops VM execution.
class KernelInterrupts {
public:
    /// Handle the kernel interrupt numbered @p command.
    /// @p vm is the VM instance that issued the instruction.
    static void handleInterrupt(VM& vm, int command);
};

} // namespace ail
