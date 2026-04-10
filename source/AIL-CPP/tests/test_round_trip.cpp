// Round-trip tests: compile → decompile → recompile and assert bytecode or
// functional equivalence.  Mirrors C# RoundTripTests.cs exactly.
//
// Two forms of equivalence are checked (matching C# spec):
//   1. Bytecode equality  — instructions whose decompiler output re-encodes
//      with the same addressing mode produce bit-identical bytecodes.
//   2. Functional equality — programs that use interrupts may differ in the
//      addressing-mode bits of the interrupt instruction but must produce
//      identical console output.

#include "test_framework.hpp"
#include "io_capture.hpp"
#include "ail/compiler/compiler.hpp"
#include "ail/decompiler/decompiler.hpp"
#include "ail/vm/vm.hpp"
#include <vector>
#include <string>

using ail::compiler::Compiler;
using ail::decompiler::Decompiler;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::vector<uint8_t> compile(const std::string& src) {
    return Compiler(src).compile();
}

static std::string decompile(const std::vector<uint8_t>& code) {
    return Decompiler(code).decompile();
}

static std::string execute(const std::vector<uint8_t>& code) {
    ail::test::io::reset();
    ail::VM vm(code.data(), static_cast<int>(code.size()));
    vm.execute();
    return ail::test::io::output();
}

// ── Bytecode round-trip ───────────────────────────────────────────────────────

// MOV-only program: compile → decompile → recompile → bytes identical.
// Mirrors C# MovOnly_BytecodeRoundTripIsIdentical.
TEST(MovOnly_BytecodeRoundTripIsIdentical) {
    const char* src =
        "MOV AL, 42\n"
        "MOV AH, AL\n"
        "MOV BL, 0xFF\n";
    auto first  = compile(src);
    auto second = compile(decompile(first));
    REQUIRE_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i)
        REQUIRE_EQ((int)first[i], (int)second[i]);
}

// Arithmetic instructions round-trip cleanly.
// Mirrors C# ArithmeticInstructions_BytecodeRoundTripIsIdentical.
TEST(ArithmeticInstructions_BytecodeRoundTripIsIdentical) {
    const char* src =
        "MOV AL, 10\n"
        "ADD AL, 5\n"
        "SUB AL, 3\n"
        "INC AL\n"
        "DEC AL\n"
        "MUL AL, 2\n";
    auto first  = compile(src);
    auto second = compile(decompile(first));
    REQUIRE_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i)
        REQUIRE_EQ((int)first[i], (int)second[i]);
}

// Bitwise instructions round-trip cleanly.
// Mirrors C# BitwiseInstructions_BytecodeRoundTripIsIdentical.
TEST(BitwiseInstructions_BytecodeRoundTripIsIdentical) {
    const char* src =
        "MOV AL, 0xFF\n"
        "AND AL, 0x0F\n"
        "BOR AL, 0xF0\n"
        "XOR AL, 0xAA\n"
        "SHL AL, 1\n"
        "SHR AL, 1\n";
    auto first  = compile(src);
    auto second = compile(decompile(first));
    REQUIRE_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i)
        REQUIRE_EQ((int)first[i], (int)second[i]);
}

// ── Functional round-trip ─────────────────────────────────────────────────────

// HelloWorld functional round-trip: both compilations produce identical output.
// Mirrors C# HelloWorld_FunctionalRoundTripOutputMatches.
TEST(HelloWorld_FunctionalRoundTripOutputMatches) {
    const char* src =
        "MOV AL, 0x01\n"
        "MOV AH, 'H'\n" "KEI 0x01\n"
        "MOV AH, 'e'\n" "KEI 0x01\n"
        "MOV AH, 'l'\n" "KEI 0x01\n"
        "MOV AH, 'l'\n" "KEI 0x01\n"
        "MOV AH, 'o'\n" "KEI 0x01\n"
        "MOV AH, ','\n" "KEI 0x01\n"
        "MOV AH, ' '\n" "KEI 0x01\n"
        "MOV AH, 'W'\n" "KEI 0x01\n"
        "MOV AH, 'o'\n" "KEI 0x01\n"
        "MOV AH, 'r'\n" "KEI 0x01\n"
        "MOV AH, 'l'\n" "KEI 0x01\n"
        "MOV AH, 'd'\n" "KEI 0x01\n"
        "MOV AH, '!'\n" "KEI 0x01\n"
        "MOV AH, '\\n'\n" "KEI 0x01\n"
        "KEI 0x02\n";

    auto original  = compile(src);
    auto roundTrip = compile(decompile(original));

    std::string out1 = execute(original);
    std::string out2 = execute(roundTrip);

    REQUIRE_EQ(out1, out2);
    REQUIRE_EQ(out1, std::string("Hello, World!\nHalting!\n"));
}

// Conditional-jump functional round-trip.
// Mirrors C# ConditionalJump_FunctionalRoundTripOutputMatches.
TEST(ConditionalJump_FunctionalRoundTripOutputMatches) {
    const char* src =
        "MOV AL, 0x01\n"
        "MOV AH, 0x01\n"
        "TEQ AL, AH\n"
        "JMT equal\n"
        "MOV AH, 'N'\n" "KEI 0x01\n"
        "equal:\n"
        "MOV AH, 'Y'\n" "KEI 0x01\n"
        "KEI 0x02\n";

    auto original  = compile(src);
    auto roundTrip = compile(decompile(original));

    std::string out1 = execute(original);
    std::string out2 = execute(roundTrip);

    REQUIRE_EQ(out1, out2);
    // AL == AH → JMT fires → 'N' is skipped → only 'Y' is printed
    REQUIRE_EQ(out1, std::string("YHalting!\n"));
}

// ── Decompiler output format ──────────────────────────────────────────────────

// Decompiled output must include the standard header comment.
// Mirrors C# Decompiler_IncludesHeaderComment.
TEST(Decompiler_IncludesHeaderComment) {
    auto code = compile("MOV AL, 1\nKEI 0x02\n");
    std::string asm2 = decompile(code);
    REQUIRE(asm2.find("; Decompiled by AIL C++ port") != std::string::npos);
}

// Decompiled output must contain recognisable mnemonics.
// Mirrors C# Decompiler_ContainsMnemonics.
TEST(Decompiler_ContainsMnemonics) {
    auto code = compile("MOV AL, 1\nADD AL, 2\nKEI 0x02\n");
    std::string asm2 = decompile(code);
    REQUIRE(asm2.find("MOV") != std::string::npos);
    REQUIRE(asm2.find("ADD") != std::string::npos);
    REQUIRE(asm2.find("KEI") != std::string::npos);
}

// ── Cross-implementation binary compatibility ─────────────────────────────────
// These tests verify that bytecode produced by the C++ compiler matches the
// exact encoding expected by the AIL spec, ensuring a binary compiled here will
// execute correctly on the C# VM (and vice versa).

// Verify the exact 6-byte encoding of a MOV RegVal instruction so the C# VM
// can parse the same binary without modification.
TEST(CrossCompat_MovRegVal_BytesMatchSpec) {
    auto code = compile("MOV AL, 0x01");
    // byte 0 = (MOV=0x01 << 2) | RegVal=0x02 = 0x06
    // byte 1 = AL = 0xF5
    // bytes 2-5 = 0x01 0x00 0x00 0x00  (little-endian 1)
    REQUIRE_EQ((int)code[0], 0x06);
    REQUIRE_EQ((int)code[1], 0xF5);
    REQUIRE_EQ((int)code[2], 0x01);
    REQUIRE_EQ((int)code[3], 0x00);
    REQUIRE_EQ((int)code[4], 0x00);
    REQUIRE_EQ((int)code[5], 0x00);
}

// Verify param2 is always little-endian (cross-platform endianness safety).
TEST(CrossCompat_Param2_IsLittleEndian) {
    // MOV X, 0x12345678 — a multi-byte value that exposes endianness
    auto code = compile("MOV X, 0x12345678");
    REQUIRE_EQ((int)code[2], 0x78); // low byte first
    REQUIRE_EQ((int)code[3], 0x56);
    REQUIRE_EQ((int)code[4], 0x34);
    REQUIRE_EQ((int)code[5], 0x12); // high byte last
}

// Verify the .ila container format used by wrapIla() is spec-compliant so the
// C# Executable.Run() can load a binary produced by the C++ compiler.
TEST(CrossCompat_IlaFormat_MatchesSpec) {
    auto code = compile("MOV AL, 1\nKEI 0x02\n");
    auto ila  = Compiler::wrapIla(code);

    // Magic "AIL\0"
    REQUIRE_EQ((int)ila[0], 0x41);
    REQUIRE_EQ((int)ila[1], 0x49);
    REQUIRE_EQ((int)ila[2], 0x4C);
    REQUIRE_EQ((int)ila[3], 0x00);
    // Version field (bytes 4-5), value ignored by the C# loader
    // Section count = 1 (little-endian)
    REQUIRE_EQ((int)ila[6], 0x01);
    REQUIRE_EQ((int)ila[7], 0x00);
    // Section type = 0x0001 (code)
    REQUIRE_EQ((int)ila[8],  0x01);
    REQUIRE_EQ((int)ila[9],  0x00);
    // Section length = code.size() (little-endian)
    uint32_t sLen = static_cast<uint32_t>(ila[10])
                  | (static_cast<uint32_t>(ila[11]) << 8)
                  | (static_cast<uint32_t>(ila[12]) << 16)
                  | (static_cast<uint32_t>(ila[13]) << 24);
    REQUIRE_EQ(static_cast<size_t>(sLen), code.size());
}
