// VM execution tests — compile AIL source, run it in the VM, and assert on
// register state and/or captured stdout.
//
// All output (AIL_PUTCHAR, AIL_PUTS) is routed to io_capture.cpp via the
// AIL_TEST_MODE compile-time flag and retrieved with io::output().

#include "test_framework.hpp"
#include "io_capture.hpp"
#include "ail/compiler/compiler.hpp"
#include "ail/vm/vm.hpp"
#include <string>
#include <vector>

using ail::compiler::Compiler;

// ── Test helpers ──────────────────────────────────────────────────────────────

// Compile source and run it in a fresh VM.  Returns the VM so callers can
// inspect register state.  Captured output is stored in io_capture's buffer.
static ail::VM compileAndRun(const std::string& src) {
    ail::test::io::reset();
    auto code = Compiler(src).compile();
    ail::VM vm(code.data(), static_cast<int>(code.size()));
    vm.execute();
    return vm;
}

// Compile, run, and return captured output only.
static std::string runForOutput(const std::string& src) {
    compileAndRun(src);
    return ail::test::io::output();
}

// ── Hello, World ──────────────────────────────────────────────────────────────

TEST(HelloWorld_CharByChar_OutputsHelloWorld) {
    std::string out = runForOutput(
        "MOV AL, 0x01\n"
        "MOV AH, 'H'\n KEI 0x01\n"
        "MOV AH, 'e'\n KEI 0x01\n"
        "MOV AH, 'l'\n KEI 0x01\n"
        "MOV AH, 'l'\n KEI 0x01\n"
        "MOV AH, 'o'\n KEI 0x01\n"
        "MOV AH, '!'\n KEI 0x01\n"
        "MOV AH, '\\n'\n KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("Hello!\nHalting!\n"));
}

// ── Arithmetic — register state ───────────────────────────────────────────────

// ADD: AL = 10 + 5 → 15
TEST(Add_ImmediateToRegister_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV AL, 10\nADD AL, 5\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 15);
}

// ADD register to register: BL = 3, AL = 7, ADD BL, AL → BL = 10
TEST(Add_RegReg_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV BL, 3\nMOV AL, 7\nADD BL, AL\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.BL, 10);
}

// SUB: AL = 20 − 7 → 13
TEST(Sub_ImmediateFromRegister_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV AL, 20\nSUB AL, 7\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 13);
}

// MUL: AL = 6 × 7 → 42
TEST(Mul_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV AL, 6\nMUL AL, 7\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 42);
}

// DIV: AL = 20 ÷ 4 → 5
TEST(Div_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV AL, 20\nDIV AL, 4\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 5);
}

// INC then DEC round-trips back to the original value.
TEST(IncDec_RoundTripsValue) {
    auto vm = compileAndRun("MOV AL, 5\nINC AL\nDEC AL\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 5);
}

// ── Register copy ─────────────────────────────────────────────────────────────

// MOV reg, reg: copy AL into AH.
TEST(MovRegReg_CopiesValue) {
    auto vm = compileAndRun("MOV AL, 0x42\nMOV AH, AL\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 0x42);
    REQUIRE_EQ((int)vm.AH, 0x42);
}

// SWP: swap AL and BL.
TEST(Swp_ExchangesRegisters) {
    auto vm = compileAndRun("MOV AL, 10\nMOV BL, 20\nSWP AL, BL\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 20);
    REQUIRE_EQ((int)vm.BL, 10);
}

// ── Bitwise ───────────────────────────────────────────────────────────────────

// AND: 0xFF & 0x0F = 0x0F
TEST(BitwiseAnd_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV AL, 0xFF\nAND AL, 0x0F\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 0x0F);
}

// BOR: 0xF0 | 0x0F = 0xFF
TEST(BitwiseOr_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV AL, 0xF0\nBOR AL, 0x0F\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 0xFF);
}

// XOR: 0xFF ^ 0xFF = 0x00
TEST(BitwiseXor_SelfXorProducesZero) {
    auto vm = compileAndRun("MOV AL, 0xFF\nXOR AL, 0xFF\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 0x00);
}

// NOT: ~0x00 = 0xFF (byte mask applied by SetRegister → 0xFF)
TEST(BitwiseNot_InvertsAllBits) {
    auto vm = compileAndRun("MOV AL, 0x00\nNOT AL\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 0xFF);
}

// SHL: 1 << 3 = 8
TEST(ShiftLeft_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV AL, 1\nSHL AL, 3\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 8);
}

// SHR: 16 >> 2 = 4
TEST(ShiftRight_ProducesCorrectResult) {
    auto vm = compileAndRun("MOV AL, 16\nSHR AL, 2\nKEI 0x02\n");
    REQUIRE_EQ((int)vm.AL, 4);
}

// ── Stack ─────────────────────────────────────────────────────────────────────

// PSH / POP: push 0x42, zero AL, pop back → AL = 0x42
TEST(StackPushPop_RestoresValue) {
    auto vm = compileAndRun(
        "MOV AL, 0x42\n"
        "PSH AL\n"
        "MOV AL, 0x00\n"
        "POP AL\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ((int)vm.AL, 0x42);
}

// Multiple pushes/pops (LIFO order).
TEST(StackMultiplePushPop_LIFOOrder) {
    auto vm = compileAndRun(
        "MOV AL, 1\nPSH AL\n"
        "MOV AL, 2\nPSH AL\n"
        "MOV AL, 3\nPSH AL\n"
        "POP AL\n"   // should pop 3
        "POP BL\n"   // should pop 2
        "KEI 0x02\n"
    );
    REQUIRE_EQ((int)vm.AL, 3);
    REQUIRE_EQ((int)vm.BL, 2);
}

// ── Conditional jumps ─────────────────────────────────────────────────────────

// TEQ + JMT: equal values → jump fires → MOV AL, 0xFF is skipped
TEST(ConditionalJump_JMT_SkipsWhenEqual) {
    auto vm = compileAndRun(
        "MOV AL, 3\n"
        "MOV AH, 3\n"
        "TEQ AL, AH\n"
        "JMT skip\n"
        "MOV AL, 0xFF\n"
        "skip:\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ((int)vm.AL, 3);
}

// TEQ + JMT: unequal values → jump does NOT fire → MOV AL, 0xFF executes
TEST(ConditionalJump_JMT_FallsThroughWhenNotEqual) {
    auto vm = compileAndRun(
        "MOV AL, 1\n"
        "MOV AH, 2\n"
        "TEQ AL, AH\n"
        "JMT skip\n"
        "MOV AL, 0xFF\n"
        "skip:\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ((int)vm.AL, 0xFF);
}

// TLT + JMT: AL < AH → flag set → jump fires
TEST(ConditionalJump_TLT_JumpsWhenLessThan) {
    auto vm = compileAndRun(
        "MOV AL, 1\n"
        "MOV AH, 5\n"
        "TLT AL, AH\n"
        "JMT skip\n"
        "MOV AL, 0xFF\n"
        "skip:\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ((int)vm.AL, 1);
}

// JMF: jump fires when flag is false
TEST(ConditionalJump_JMF_JumpsWhenFalse) {
    auto vm = compileAndRun(
        "MOV AL, 1\n"
        "MOV AH, 2\n"
        "TEQ AL, AH\n"   // false (1 != 2)
        "JMF skip\n"
        "MOV AL, 0xFF\n"
        "skip:\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ((int)vm.AL, 1);
}

// ── Call / return ─────────────────────────────────────────────────────────────

// CLL + RET: call a subroutine that sets BL = 99, then returns.
TEST(CallReturn_ExecutesSubroutineAndReturns) {
    auto vm = compileAndRun(
        "CLL sub\n"       // offset 0
        "KEI 0x02\n"      // offset 6  (return lands here)
        "sub:\n"          // offset 12
        "MOV BL, 99\n"
        "RET\n"
    );
    REQUIRE_EQ((int)vm.BL, 99);
}

// ── Memory instructions ───────────────────────────────────────────────────────

// MOM: write value to memory address, MOE: read it back.
TEST(MomMoe_WriteReadMemory) {
    auto vm = compileAndRun(
        "MOM 0x42, 0x60\n"   // write 0x42 to address 0x60
        "MOE AL, 0x60\n"     // read it back into AL
        "KEI 0x02\n"
    );
    REQUIRE_EQ((int)vm.AL, 0x42);
}

// ── KEI I/O modes — output capture ───────────────────────────────────────────

// KEI 0x01 AL=0x01: write character in AH to output
TEST(KeiWriteChar_OutputsSingleCharacter) {
    std::string out = runForOutput(
        "MOV AL, 0x01\n"
        "MOV AH, 'X'\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("XHalting!\n"));
}

// KEI 0x01 AL=0x05: write register B as decimal integer
TEST(KeiWriteInt_OutputsDecimalValue) {
    std::string out = runForOutput(
        "MOV BL, 42\n"
        "MOV AL, 0x05\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("42Halting!\n"));
}

// KEI 0x01 AL=0x02: write-string mode — B bytes from RAM at address X
TEST(KeiWriteString_OutputsCorrectText) {
    std::string out = runForOutput(
        "MOM 0x48, 0x60\n"   // 'H'
        "MOM 0x69, 0x61\n"   // 'i'
        "MOV AL, 0x02\n"
        "MOV X, 0x60\n"
        "MOV BL, 2\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("HiHalting!\n"));
}

// ── Full programs ─────────────────────────────────────────────────────────────

// Hello, World via DB string — matches examples/hello_world_db.ail
TEST(HelloWorldDb_OutputsHelloWorld) {
    std::string out = runForOutput(
        "JMP main\n"
        "hello:\n"
        "DB \"Hello, World\", 0x0A, 0x00\n"
        "main:\n"
        "MOV AL, 0x02\n"
        "MOV X, hello\n"
        "MOV BL, 13\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("Hello, World\nHalting!\n"));
}

// Calculator addition: 3 + 4 = 7 printed as integer
TEST(Calculator_Addition_PrintsCorrectResult) {
    std::string out = runForOutput(
        "MOV BL, 3\n"
        "ADD BL, 4\n"
        "MOV AL, 0x05\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("7Halting!\n"));
}

// Calculator subtraction: 10 - 3 = 7
TEST(Calculator_Subtraction_PrintsCorrectResult) {
    std::string out = runForOutput(
        "MOV BL, 10\n"
        "SUB BL, 3\n"
        "MOV AL, 0x05\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("7Halting!\n"));
}

// Calculator multiplication: 6 * 7 = 42
TEST(Calculator_Multiplication_PrintsCorrectResult) {
    std::string out = runForOutput(
        "MOV BL, 6\n"
        "MUL BL, 7\n"
        "MOV AL, 0x05\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("42Halting!\n"));
}

// Calculator division: 20 / 4 = 5
TEST(Calculator_Division_PrintsCorrectResult) {
    std::string out = runForOutput(
        "MOV BL, 20\n"
        "DIV BL, 4\n"
        "MOV AL, 0x05\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("5Halting!\n"));
}

// ── VM error codes ────────────────────────────────────────────────────────────

// Division by zero must return VMError::DivByZero without crashing.
TEST(DivByZero_ReturnsError) {
    auto code = Compiler("MOV AL, 10\nDIV AL, 0\nKEI 0x02\n").compile();
    ail::VM vm(code.data(), static_cast<int>(code.size()));
    ail::VMError err = vm.execute();
    REQUIRE_EQ((int)err, (int)ail::VMError::DivByZero);
}

// ── Full Hello, World! matching C# VmExecutionTests ──────────────────────────

// Exact equivalent of C# HelloWorld_OutputsHelloWorldNewline
TEST(HelloWorld_Full_OutputsHelloWorldNewline) {
    std::string out = runForOutput(
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
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("Hello, World!\nHalting!\n"));
}

// ── SWI 0x01 — string utilities (matching C# SwiStrlen / SwiStrcpy) ──────────

// SWI 0x01 AL=0x01: strlen of "AIL\0" at 0x80 → B = 3
TEST(SwiStrlen_ReturnsCorrectLength) {
    auto vm = compileAndRun(
        "MOM 0x41, 0x80\n"   // 'A'
        "MOM 0x49, 0x81\n"   // 'I'
        "MOM 0x4C, 0x82\n"   // 'L'
        "MOM 0x00, 0x83\n"   // '\0'
        "MOV AL, 0x01\n"
        "MOV X, 0x80\n"
        "SWI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(vm.getSplit('B'), 3);
}

// SWI 0x01 AL=0x02: strcpy 3 bytes from 0x80 to 0x90, print via KEI write-string
TEST(SwiStrcpy_CopiesBytes) {
    std::string out = runForOutput(
        "MOM 0x48, 0x80\n"   // 'H'
        "MOM 0x69, 0x81\n"   // 'i'
        "MOM 0x21, 0x82\n"   // '!'
        "MOV AL, 0x02\n"
        "MOV X, 0x80\n"
        "MOV Y, 0x90\n"
        "MOV BL, 3\n"
        "SWI 0x01\n"
        "MOV AL, 0x02\n"
        "MOV X, 0x90\n"
        "MOV BL, 3\n"
        "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("Hi!Halting!\n"));
}

// ── Calculator — all operations in one program ────────────────────────────────

// Exact equivalent of C# Calculator_AllOperations_PrintsAllResults
TEST(Calculator_AllOperations_PrintsAllResults) {
    std::string out = runForOutput(
        "MOV BL, 3\n"  "ADD BL, 4\n"  "MOV AL, 0x05\n" "KEI 0x01\n"
        "MOV AL, 0x01\n" "MOV AH, 0x0A\n" "KEI 0x01\n"
        "MOV BL, 10\n" "SUB BL, 3\n"  "MOV AL, 0x05\n" "KEI 0x01\n"
        "MOV AL, 0x01\n" "MOV AH, 0x0A\n" "KEI 0x01\n"
        "MOV BL, 6\n"  "MUL BL, 7\n"  "MOV AL, 0x05\n" "KEI 0x01\n"
        "MOV AL, 0x01\n" "MOV AH, 0x0A\n" "KEI 0x01\n"
        "MOV BL, 20\n" "DIV BL, 4\n"  "MOV AL, 0x05\n" "KEI 0x01\n"
        "MOV AL, 0x01\n" "MOV AH, 0x0A\n" "KEI 0x01\n"
        "KEI 0x02\n"
    );
    REQUIRE_EQ(out, std::string("7\n7\n42\n5\nHalting!\n"));
}
