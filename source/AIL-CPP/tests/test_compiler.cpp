// Compiler encoding tests — verify that known AIL source produces the expected
// raw bytecode without executing the VM.
//
// Instruction encoding (spec §2):
//   byte 0   = (opcode << 2) | address_mode
//   byte 1   = param1 (8-bit register ID or immediate)
//   bytes 2-5= param2 (32-bit little-endian value or register ID in low byte)
//
// Address modes: RegReg=0x00  ValReg=0x01  RegVal=0x02  ValVal=0x03
// Register bytes: AL=0xF5  AH=0xF6  BL=0xF8

#include "test_framework.hpp"
#include "ail/compiler/compiler.hpp"
#include "ail/compiler/build_exception.hpp"
#include <vector>
#include <string>
#include <cstdint>

using ail::compiler::Compiler;
using ail::compiler::BuildException;

static std::vector<uint8_t> compile(const std::string& src) {
    return Compiler(src).compile();
}

// ── MOV ───────────────────────────────────────────────────────────────────────

// MOV AL, 0x01 — RegVal mode
//   byte 0 = (0x01 << 2) | 0x02 = 0x06
//   byte 1 = 0xF5 (AL)
//   bytes 2-5 = 0x01 0x00 0x00 0x00
TEST(MovRegVal_EncodesCorrectly) {
    auto code = compile("MOV AL, 0x01");
    REQUIRE_EQ((int)code.size(), 6);
    REQUIRE_EQ((int)code[0], 0x06);  // (MOV=0x01 << 2) | RegVal=0x02
    REQUIRE_EQ((int)code[1], 0xF5);  // AL
    REQUIRE_EQ((int)code[2], 0x01);  // param2 low byte
    REQUIRE_EQ((int)code[3], 0x00);
    REQUIRE_EQ((int)code[4], 0x00);
    REQUIRE_EQ((int)code[5], 0x00);
}

// MOV AH, AL — RegReg mode
//   byte 0 = (0x01 << 2) | 0x00 = 0x04
//   byte 1 = 0xF6 (AH),  byte 2 = 0xF5 (AL in low byte of param2)
TEST(MovRegReg_EncodesCorrectly) {
    auto code = compile("MOV AH, AL");
    REQUIRE_EQ((int)code.size(), 6);
    REQUIRE_EQ((int)code[0], 0x04);  // (MOV=0x01 << 2) | RegReg=0x00
    REQUIRE_EQ((int)code[1], 0xF6);  // AH
    REQUIRE_EQ((int)code[2], 0xF5);  // AL in low byte of param2
}

// ── KEI ───────────────────────────────────────────────────────────────────────

// KEI 0x01 — interrupt number encoded in param1 (byte 1), not param2
TEST(Kei_EncodesInterruptNumberInParam1) {
    auto code = compile("KEI 0x01");
    REQUIRE_EQ((int)code.size(), 6);
    REQUIRE_EQ(((int)code[0] >> 2) & 0x3F, 0x2B); // opcode field
    REQUIRE_EQ((int)code[1], 0x01);                // interrupt number
}

// ── ADD ───────────────────────────────────────────────────────────────────────

// ADD AL, 5 — RegVal: byte 0 = (0x04 << 2) | 0x02 = 0x12
TEST(AddRegVal_EncodesCorrectly) {
    auto code = compile("ADD AL, 5");
    REQUIRE_EQ((int)code.size(), 6);
    REQUIRE_EQ((int)code[0], 0x12);  // (ADD=0x04 << 2) | RegVal=0x02
    REQUIRE_EQ((int)code[1], 0xF5);  // AL
    REQUIRE_EQ((int)code[2], 0x05);  // 5 in low byte of param2
}

// ── Char literals ─────────────────────────────────────────────────────────────

// 'H' = 0x48
TEST(CharLiteral_EncodesCorrectByte) {
    auto code = compile("MOV AH, 'H'");
    REQUIRE_EQ((int)code[2], 0x48);
}

// '\n' = 0x0A
TEST(EscapeNewline_EncodesCorrectByte) {
    auto code = compile("MOV AH, '\\n'");
    REQUIRE_EQ((int)code[2], 0x0A);
}

// '\t' = 0x09
TEST(EscapeTab_EncodesCorrectByte) {
    auto code = compile("MOV AH, '\\t'");
    REQUIRE_EQ((int)code[2], 0x09);
}

// ── Multi-instruction programs ────────────────────────────────────────────────

// Two instructions must produce exactly 12 bytes.
TEST(TwoInstructions_ProducesTwelveBytes) {
    auto code = compile("MOV AL, 1\nKEI 0x02");
    REQUIRE_EQ((int)code.size(), 12);
}

// ── Label resolution ──────────────────────────────────────────────────────────

// JMP target (target is the 3rd instruction, byte offset 12)
TEST(LabelResolution_ForwardJump_EncodesCorrectOffset) {
    const char* src =
        "JMP target\n"
        "MOV AL, 0xFF\n"
        "target:\n"
        "KEI 0x02\n";
    auto code = compile(src);
    int target = (int)code[2] | ((int)code[3] << 8)
               | ((int)code[4] << 16) | ((int)code[5] << 24);
    REQUIRE_EQ(target, 12);
}

// ── Comments & whitespace ─────────────────────────────────────────────────────

// Pure comment lines must not produce bytes.
TEST(CommentsAndBlanks_DoNotGenerateBytecode) {
    const char* src =
        "; comment\n"
        "// another\n"
        "\n"
        "MOV AL, 1\n";
    auto code = compile(src);
    REQUIRE_EQ((int)code.size(), 6);
}

// Inline comment after an instruction must be stripped.
TEST(InlineComment_DoesNotCorruptInstruction) {
    auto code = compile("MOV AL, 5 ; set AL");
    REQUIRE_EQ((int)code.size(), 6);
    REQUIRE_EQ((int)code[2], 0x05);
}

// ── DB pseudo-instruction ─────────────────────────────────────────────────────

// DB "Hi", 0x0A, 0x00 → 4 raw bytes: 0x48 0x69 0x0A 0x00
TEST(Db_StringLiteral_EmitsRawBytes) {
    auto code = compile("DB \"Hi\", 0x0A, 0x00");
    REQUIRE_EQ((int)code.size(), 4);
    REQUIRE_EQ((int)code[0], 0x48); // 'H'
    REQUIRE_EQ((int)code[1], 0x69); // 'i'
    REQUIRE_EQ((int)code[2], 0x0A); // newline
    REQUIRE_EQ((int)code[3], 0x00); // null
}

// DB 'A', '\n', 0x00 → 3 bytes: 0x41 0x0A 0x00
TEST(Db_CharLiterals_EmitsRawBytes) {
    auto code = compile("DB 'A', '\\n', 0x00");
    REQUIRE_EQ((int)code.size(), 3);
    REQUIRE_EQ((int)code[0], 0x41); // 'A'
    REQUIRE_EQ((int)code[1], 0x0A); // '\n'
    REQUIRE_EQ((int)code[2], 0x00); // null
}

// Label after a DB block must resolve to the correct byte offset.
// Layout: JMP main (6 bytes) | hello: DB "AB" (2 bytes) | main: KEI 0x02 (6 bytes)
// JMP param2 must equal 8 (= 6 + 2).
TEST(Db_LabelAfterDb_ResolvesToCorrectOffset) {
    const char* src =
        "JMP main\n"
        "hello:\n"
        "DB \"AB\"\n"
        "main:\n"
        "KEI 0x02\n";
    auto code = compile(src);
    int target = (int)code[2] | ((int)code[3] << 8)
               | ((int)code[4] << 16) | ((int)code[5] << 24);
    REQUIRE_EQ(target, 8);
}

// ── Error handling ────────────────────────────────────────────────────────────

// Unknown mnemonic must throw BuildException.
TEST(Compiler_ThrowsOnUnknownMnemonic) {
    REQUIRE_THROWS(compile("FOOBAR AL, 1"));
}

// Empty source must throw BuildException.
TEST(Compiler_ThrowsOnEmptySource) {
    REQUIRE_THROWS(Compiler("").compile());
}

// Duplicate label must throw BuildException.
TEST(Compiler_ThrowsOnDuplicateLabel) {
    REQUIRE_THROWS(compile("lbl:\nlbl:\nKEI 0x02\n"));
}
