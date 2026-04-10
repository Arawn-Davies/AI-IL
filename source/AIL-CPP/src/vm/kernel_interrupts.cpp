#include "ail/vm/kernel_interrupts.hpp"
#include "ail/vm/vm.hpp"
#include "ail/config.hpp"
#include <cstdio> // EOF

namespace ail {

void KernelInterrupts::handleInterrupt(VM& vm, int command) {
    switch (command) {

    // ── KEI 0x01 — stdio ─────────────────────────────────────────────────────
    case 0x01:
        switch (vm.AL) {

        case 0x01: // Write the single character in AH to stdout
            AIL_PUTCHAR(static_cast<char>(vm.AH));
            break;

        case 0x02: { // Write B bytes from RAM at address X to stdout
            int  count = vm.getSplit('B');
            int  base  = static_cast<int>(vm.X);
            for (int i = 0; i < count; ++i)
                AIL_PUTCHAR(static_cast<char>(vm.ram.getByte(base + i)));
            break;
        }

        case 0x03: // Read one character from stdin into AH
            vm.AH = static_cast<uint8_t>(AIL_GETCHAR());
            break;

        case 0x04: { // Read a line from stdin into RAM at address X; B = length
            int base = static_cast<int>(vm.X);
            int n    = 0;
            int ch;
            while ((ch = AIL_GETCHAR()) != EOF && ch != '\n') {
                if (base + n < RAM::SIZE)
                    vm.ram.memory[base + n] = static_cast<uint8_t>(ch);
                ++n;
            }
            vm.setSplit('B', n);
            break;
        }

        case 0x05: // Write the 16-bit value of B as decimal
            AIL_PRINTF("%d", vm.getSplit('B'));
            break;

        default:
            break;
        }
        break;

    // ── KEI 0x02 — halt ───────────────────────────────────────────────────────
    case 0x02:
        AIL_PUTS("Halting!\n");
        vm.halt();
        break;

    default:
        AIL_PRINTF("Undocumented KEI 0x%02X — halting for data protection\n", command);
        vm.halt();
        break;
    }
}

} // namespace ail
