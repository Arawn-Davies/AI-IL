#include "ail/vm/ram.hpp"
#include <cstring>

namespace ail {

void RAM::load(const uint8_t* data, int len) {
    int copyLen = len < SIZE ? len : SIZE;
    memcpy(memory, data, static_cast<size_t>(copyLen));
    ramLimit = copyLen;
}

void RAM::setByte(int address, uint8_t value) {
    if (address < 0 || address >= SIZE) return;
    if (address < ramLimit) return; // code-guard: refuse overwrite of executable
    memory[address] = value;
}

uint8_t RAM::getByte(int address) const {
    if (address < 0 || address >= SIZE) return 0;
    return memory[address];
}

void RAM::getSection(int address, int length, uint8_t* out) const {
    for (int i = 0; i < length; ++i) {
        int addr = address + i;
        out[i] = (addr >= 0 && addr < SIZE) ? memory[addr] : 0;
    }
}

void RAM::setSection(int address, const uint8_t* data, int length) {
    for (int i = 0; i < length; ++i) {
        int addr = address + i;
        if (addr >= 0 && addr < SIZE)
            memory[addr] = data[i];
    }
}

} // namespace ail
