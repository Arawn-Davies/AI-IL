#include "ail/vm/software_interrupts.hpp"
#include "ail/vm/vm.hpp"
#include "ail/config.hpp"

namespace ail {

void SoftwareInterrupts::handleInterrupt(VM& vm, int command) {
    if (command == 0x01) {
        if (vm.AL == 0x01) {
            // Strlen: count bytes at address X until a zero byte; store length in B.
            int addr  = static_cast<int>(vm.X);
            int limit = RAM::SIZE;
            int len   = 0;
            while (addr + len < limit && vm.ram.memory[addr + len] != 0x00)
                ++len;
            vm.setSplit('B', len);
        } else if (vm.AL == 0x02) {
            // Strcpy: copy B bytes from address X to address Y.
            int src   = static_cast<int>(vm.X);
            int dst   = static_cast<int>(vm.Y);
            int count = vm.getSplit('B');
            int limit = RAM::SIZE;
            if (src < 0 || src + count > limit || dst < 0 || dst + count > limit) {
                AIL_PUTS("SWI 0x01: address out of range\nHalting for protection of data\n");
                vm.halt();
                return;
            }
            // Use direct memory access to bypass the code-write guard, matching
            // the C# SetSection() behaviour which also bypasses RAMLimit.
            for (int i = 0; i < count; ++i)
                vm.ram.memory[dst + i] = vm.ram.memory[src + i];
        } else {
            AIL_PRINTF("SWI 0x01: unknown mode 0x%02X\nHalting for protection of data\n",
                       static_cast<unsigned>(vm.AL));
            vm.halt();
        }
    } else {
        AIL_PRINTF("Undocumented SWI: %d\nHalting for protection of data\n", command);
        vm.halt();
    }
}

} // namespace ail
