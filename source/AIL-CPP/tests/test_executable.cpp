// Executable format tests — verify .ila parsing, raw-bytecode fallback,
// and end-to-end Executable::run() behaviour.

#include "test_framework.hpp"
#include "io_capture.hpp"
#include "ail/executable.hpp"
#include "ail/compiler/compiler.hpp"
#include <vector>
#include <cstdint>

using ail::compiler::Compiler;

// ── .ila format parsing ───────────────────────────────────────────────────────

// A properly-formed .ila file must be parsed and code extracted successfully.
TEST(Executable_ExtractCode_ValidIla) {
    auto code = Compiler("MOV AL, 1\nKEI 0x02\n").compile();
    auto ila  = Compiler::wrapIla(code);

    const uint8_t* out    = nullptr;
    int            outLen = 0;
    bool ok = ail::Executable::extractCode(ila.data(), static_cast<int>(ila.size()),
                                            out, outLen);
    REQUIRE(ok);
    REQUIRE_EQ(outLen, (int)code.size());
    for (int i = 0; i < outLen; ++i)
        REQUIRE_EQ((int)out[i], (int)code[i]);
}

// A buffer without the AIL magic header is treated as raw bytecode.
TEST(Executable_ExtractCode_RawBytecode_ReturnedAsIs) {
    auto code = Compiler("MOV AL, 1\nKEI 0x02\n").compile();

    const uint8_t* out    = nullptr;
    int            outLen = 0;
    bool ok = ail::Executable::extractCode(code.data(), static_cast<int>(code.size()),
                                            out, outLen);
    REQUIRE(ok);
    REQUIRE_EQ(outLen, (int)code.size());
    REQUIRE(out == code.data()); // same pointer — no copy
}

// A buffer too short to hold any header must be treated as raw bytecode.
TEST(Executable_ExtractCode_TooShortBuffer_TreatedAsRaw) {
    std::vector<uint8_t> tiny = { 0x01, 0x02 };
    const uint8_t* out    = nullptr;
    int            outLen = 0;
    bool ok = ail::Executable::extractCode(tiny.data(), static_cast<int>(tiny.size()),
                                            out, outLen);
    REQUIRE(ok);
    REQUIRE_EQ(outLen, 2);
}

// A file with the AIL magic but no code section must return false.
TEST(Executable_ExtractCode_NoCodeSection_ReturnsFalse) {
    // Minimal .ila with 0 sections.
    std::vector<uint8_t> bad = {
        0x41, 0x49, 0x4C, 0x00, // magic
        0x02, 0x00,              // version
        0x00, 0x00,              // section count = 0
    };
    const uint8_t* out    = nullptr;
    int            outLen = 0;
    bool ok = ail::Executable::extractCode(bad.data(), static_cast<int>(bad.size()),
                                            out, outLen);
    REQUIRE(!ok);
}

// A truncated section table must return false.
TEST(Executable_ExtractCode_TruncatedSection_ReturnsFalse) {
    // Header says 1 section but there are no section bytes.
    std::vector<uint8_t> bad = {
        0x41, 0x49, 0x4C, 0x00, // magic
        0x02, 0x00,              // version
        0x01, 0x00,              // section count = 1
        // no section header bytes at all
    };
    const uint8_t* out    = nullptr;
    int            outLen = 0;
    bool ok = ail::Executable::extractCode(bad.data(), static_cast<int>(bad.size()),
                                            out, outLen);
    REQUIRE(!ok);
}

// ── End-to-end Executable::run() ─────────────────────────────────────────────

// Run a .ila binary via Executable::run() and verify output.
TEST(Executable_Run_IlaBinary_ProducesOutput) {
    ail::test::io::reset();
    auto code = Compiler("MOV AL, 0x01\nMOV AH, 'Z'\nKEI 0x01\nKEI 0x02\n").compile();
    auto ila  = Compiler::wrapIla(code);
    bool ok = ail::Executable::run(ila.data(), static_cast<int>(ila.size()));
    REQUIRE(ok);
    REQUIRE_EQ(ail::test::io::output(), std::string("ZHalting!\n"));
}

// Run a raw bytecode buffer via Executable::run().
TEST(Executable_Run_RawBytecode_ProducesOutput) {
    ail::test::io::reset();
    auto code = Compiler("MOV AL, 0x01\nMOV AH, 'Q'\nKEI 0x01\nKEI 0x02\n").compile();
    bool ok = ail::Executable::run(code.data(), static_cast<int>(code.size()));
    REQUIRE(ok);
    REQUIRE_EQ(ail::test::io::output(), std::string("QHalting!\n"));
}

// Executable::run() on a malformed .ila must return false.
TEST(Executable_Run_MalformedIla_ReturnsFalse) {
    // Magic present, 1 section declared, but section header is truncated.
    std::vector<uint8_t> bad = {
        0x41, 0x49, 0x4C, 0x00,
        0x02, 0x00,
        0x01, 0x00,
        // no section header
    };
    bool ok = ail::Executable::run(bad.data(), static_cast<int>(bad.size()));
    REQUIRE(!ok);
}
