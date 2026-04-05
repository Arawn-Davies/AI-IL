using System;
using Xunit;
using Artemis_IL;
using AIL_Studio.Compiler;

namespace AIL_Tests
{
    /// <summary>
    /// Tests that compile AIL assembly programs, execute them in the VM, and assert on
    /// console output or register state. All tests are run sequentially (via
    /// <see cref="VmCollection"/>) because they share the process-wide
    /// <see cref="Artemis_IL.Globals.console"/> singleton.
    /// </summary>
    [Collection("VM")]
    public sealed class VmExecutionTests
    {
        private readonly TestConsole _console = new TestConsole();

        // ── Helpers ──────────────────────────────────────────────────────────────

        /// <summary>
        /// Compiles <paramref name="source"/>, loads it into a fresh VM and executes it.
        /// Returns the VM so callers can inspect registers.
        /// </summary>
        private VM CompileAndRun(string source)
        {
            byte[] code = new Compiler(source).Compile();

            Artemis_IL.Globals.console = _console;
            Artemis_IL.Globals.DebugMode = false;
            _console.Reset();

            var vm = new VM(code, 65536);
            vm.Execute();
            return vm;
        }

        // ── Hello, World! ────────────────────────────────────────────────────────

        /// <summary>
        /// Classic "Hello, World!" program — prints each character individually via
        /// KEI 0x01 in write-char mode (AL = 0x01, character in AH).
        /// </summary>
        [Fact]
        public void HelloWorld_OutputsHelloWorldNewline()
        {
            const string source = @"
MOV AL, 0x01
MOV AH, 'H'
KEI 0x01
MOV AH, 'e'
KEI 0x01
MOV AH, 'l'
KEI 0x01
MOV AH, 'l'
KEI 0x01
MOV AH, 'o'
KEI 0x01
MOV AH, ','
KEI 0x01
MOV AH, ' '
KEI 0x01
MOV AH, 'W'
KEI 0x01
MOV AH, 'o'
KEI 0x01
MOV AH, 'r'
KEI 0x01
MOV AH, 'l'
KEI 0x01
MOV AH, 'd'
KEI 0x01
MOV AH, '!'
KEI 0x01
MOV AH, '\n'
KEI 0x01
KEI 0x02
";
            CompileAndRun(source);

            // KEI 0x02 always writes "Halting!\n" before stopping.
            Assert.Equal("Hello, World!\nHalting!\n", _console.Output);
        }

        // ── Arithmetic ───────────────────────────────────────────────────────────

        /// <summary>ADD: AL = 10 + 5 → register should contain 15 after execution.</summary>
        [Fact]
        public void Add_ProducesCorrectResult()
        {
            const string source = @"
MOV AL, 10
ADD AL, 5
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(15, vm.AL);
        }

        /// <summary>SUB: AL = 20 − 7 → register should contain 13.</summary>
        [Fact]
        public void Subtract_ProducesCorrectResult()
        {
            const string source = @"
MOV AL, 20
SUB AL, 7
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(13, vm.AL);
        }

        /// <summary>MUL: AL = 6 × 7 → register should contain 42.</summary>
        [Fact]
        public void Multiply_ProducesCorrectResult()
        {
            const string source = @"
MOV AL, 6
MUL AL, 7
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(42, vm.AL);
        }

        /// <summary>INC / DEC: AL starts at 5, incremented to 6 then decremented to 5.</summary>
        [Fact]
        public void IncDec_RoundTripsValue()
        {
            const string source = @"
MOV AL, 5
INC AL
DEC AL
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(5, vm.AL);
        }

        // ── Register copy ────────────────────────────────────────────────────────

        /// <summary>MOV reg, reg: copy AL into AH and verify both hold the same value.</summary>
        [Fact]
        public void MovRegReg_CopiesValue()
        {
            const string source = @"
MOV AL, 0x42
MOV AH, AL
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(0x42, vm.AL);
            Assert.Equal(0x42, vm.AH);
        }

        // ── Conditional jump ─────────────────────────────────────────────────────

        /// <summary>
        /// TEQ + JMT: when AL == AH the jump fires, skipping the MOV that would
        /// change AL to 0xFF.  After execution AL must still equal 3.
        /// </summary>
        [Fact]
        public void ConditionalJump_SkipsInstructionWhenEqual()
        {
            const string source = @"
MOV AL, 3
MOV AH, 3
TEQ AL, AH
JMT skip
MOV AL, 0xFF
skip:
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(3, vm.AL);
        }

        /// <summary>
        /// TEQ + JMF: when AL != AH (values differ) JMF does NOT fire, the instruction
        /// after the jump executes and sets AL to 0xFF.
        /// </summary>
        [Fact]
        public void ConditionalJump_FallsThroughWhenNotEqual()
        {
            const string source = @"
MOV AL, 1
MOV AH, 2
TEQ AL, AH
JMF skip
MOV AL, 0xFF
skip:
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(0xFF, vm.AL);
        }

        // ── Stack ────────────────────────────────────────────────────────────────

        /// <summary>
        /// PSH / POP: push 0x42 onto the stack, zero AL, then pop back.
        /// AL must equal 0x42 after the pop.
        /// </summary>
        [Fact]
        public void StackPushPop_RestoresValue()
        {
            const string source = @"
MOV AL, 0x42
PSH AL
MOV AL, 0x00
POP AL
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(0x42, vm.AL);
        }

        // ── Bitwise ──────────────────────────────────────────────────────────────

        /// <summary>AND: 0xFF &amp; 0x0F = 0x0F.</summary>
        [Fact]
        public void BitwiseAnd_ProducesCorrectResult()
        {
            const string source = @"
MOV AL, 0xFF
AND AL, 0x0F
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(0x0F, vm.AL);
        }

        /// <summary>BOR: 0xF0 | 0x0F = 0xFF.</summary>
        [Fact]
        public void BitwiseOr_ProducesCorrectResult()
        {
            const string source = @"
MOV AL, 0xF0
BOR AL, 0x0F
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(0xFF, vm.AL);
        }

        /// <summary>XOR: 0xFF ^ 0xFF = 0x00.</summary>
        [Fact]
        public void BitwiseXor_ProducesCorrectResult()
        {
            const string source = @"
MOV AL, 0xFF
XOR AL, 0xFF
KEI 0x02
";
            VM vm = CompileAndRun(source);
            Assert.Equal(0x00, vm.AL);
        }

        // ── Write-string mode ────────────────────────────────────────────────────

        /// <summary>
        /// KEI 0x01 in write-string mode (AL = 0x02): stores "Hi" in RAM via MOM,
        /// points X at the data address, sets B = 2, then issues KEI 0x01.
        /// Output (excluding halt) must equal "Hi".
        /// </summary>
        [Fact]
        public void WriteString_OutputsCorrectText()
        {
            // We'll store "Hi" at address 0x60 (96) in RAM using MOM (move-to-memory).
            // MOM src, addr  – writes the byte value of src to the given memory address.
            const string source = @"
; Store 'H' at 0x60, 'i' at 0x61
MOM 0x48, 0x60
MOM 0x69, 0x61
; Set up registers: AL=0x02 (write-string mode), X=base address, B=length
MOV AL, 0x02
MOV X, 0x60
MOV BL, 2
KEI 0x01
KEI 0x02
";
            CompileAndRun(source);
            Assert.Equal("HiHalting!\n", _console.Output);
        }

        // ── Compiler error handling ──────────────────────────────────────────────

        /// <summary>The compiler must reject an unknown mnemonic with a <see cref="BuildException"/>.</summary>
        [Fact]
        public void Compiler_ThrowsOnUnknownMnemonic()
        {
            Assert.Throws<BuildException>(() => new Compiler("FOOBAR AL, 1").Compile());
        }

        /// <summary>An empty source string must throw a <see cref="BuildException"/>.</summary>
        [Fact]
        public void Compiler_ThrowsOnEmptySource()
        {
            Assert.Throws<BuildException>(() => new Compiler("").Compile());
        }
    }
}
