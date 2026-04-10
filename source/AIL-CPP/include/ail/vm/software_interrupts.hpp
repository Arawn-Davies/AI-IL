#pragma once

namespace ail {

class VM;

/// Software interrupt handler for SWI instructions (opcode 0x2A).
///
/// SWI 0x01 — string utilities (operation selected by register AL):
///   AL=0x01  Strlen: scan RAM from address X until 0x00; store byte count in B.
///   AL=0x02  Strcpy: copy B bytes from address X to address Y.
class SoftwareInterrupts {
public:
    static void handleInterrupt(VM& vm, int command);
};

} // namespace ail
