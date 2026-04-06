#include "ail/executable.hpp"
#include "ail/vm/vm.hpp"

namespace ail {

static const uint8_t kMagic[4] = { 0x41, 0x49, 0x4C, 0x00 }; // "AIL\0"
static constexpr uint16_t kSectionTypeCode = 0x0001;

bool Executable::hasIlaMagic(const uint8_t* data, int len) {
    if (len < 8) return false;
    for (int i = 0; i < 4; ++i)
        if (data[i] != kMagic[i]) return false;
    return true;
}

bool Executable::extractCode(const uint8_t* data, int len,
                              const uint8_t*& out, int& outLen) {
    if (!hasIlaMagic(data, len)) {
        // Raw bytecode — treat the whole buffer as code.
        out    = data;
        outLen = len;
        return true;
    }

    // Parse .ila header: bytes 4-5 = version (ignored), bytes 6-7 = section count.
    uint16_t sectionCount = static_cast<uint16_t>(data[6])
                          | static_cast<uint16_t>(data[7] << 8);
    int offset = 8;

    for (uint16_t s = 0; s < sectionCount; ++s) {
        if (offset + 6 > len) return false; // truncated section table

        uint16_t type = static_cast<uint16_t>(data[offset    ])
                      | static_cast<uint16_t>(data[offset + 1] << 8);
        int sLen      = static_cast<int>(
                          static_cast<uint32_t>(data[offset + 2])
                        | (static_cast<uint32_t>(data[offset + 3]) <<  8)
                        | (static_cast<uint32_t>(data[offset + 4]) << 16)
                        | (static_cast<uint32_t>(data[offset + 5]) << 24));
        offset += 6;

        if (offset + sLen > len) return false; // section data truncated

        if (type == kSectionTypeCode) {
            out    = data + offset;
            outLen = sLen;
            return true;
        }
        offset += sLen;
    }

    return false; // no code section found
}

bool Executable::run(const uint8_t* data, int len) {
    const uint8_t* code = nullptr;
    int            codeLen = 0;
    if (!extractCode(data, len, code, codeLen)) return false;
    VM vm(code, codeLen);
    vm.execute();
    return true;
}

} // namespace ail
