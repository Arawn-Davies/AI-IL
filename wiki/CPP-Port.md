# AIL C++ Port

The `source/AIL-CPP/` directory contains a fully self-contained C++ implementation of the AIL (Artemis Intermediate Language) toolchain.  It targets the same [binary format](Specification.md) as the C# reference implementation so compiled `.ila` files run unchanged on either runtime.

---

## Contents

| Component | Description |
|---|---|
| `ail-vm` | Embedded-safe VM core library (C++11, no heap, no exceptions) |
| `ail-compiler` | Two-pass assembler — `.ail` source → raw bytecode or `.ila` binary |
| `ail-decompiler` | Disassembler — `.ila`/raw bytecode → readable assembly source |
| `ailcpp` | Command-line tool combining all three |
| `ail-tests` | Self-contained unit-test executable (71 tests, zero dependencies) |

---

## Building

### Requirements

| Toolchain | Minimum version |
|---|---|
| CMake | 3.14 |
| C++ compiler | GCC 7, Clang 5, or MSVC 2017 (C++11 for VM core; C++17 for tools) |

No external libraries or package managers are required.

### Desktop (Linux / macOS / Windows)

```bash
cd source/AIL-CPP
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Artifacts produced in `build/`:

| File | Description |
|---|---|
| `ailcpp` | CLI tool |
| `ail-tests` | Test runner |
| `libail-vm.a` | VM library (link into your own project) |
| `libail-compiler.a` | Compiler library |
| `libail-decompiler.a` | Decompiler library |

### Windows (MSVC)

```bat
cd source\AIL-CPP
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Reduced RAM size (embedded / CI)

Pass `-DAIL_RAM_SIZE=<bytes>` to override the default 64 KB RAM:

```bash
cmake -B build -DAIL_RAM_SIZE=4096
```

---

## Usage

### Run a compiled binary

```bash
ailcpp run program.ila
```

Also accepts raw bytecode files (no `.ila` header).

### Compile assembly source

```bash
ailcpp compile program.ail              # writes program.ila
ailcpp compile program.ail -o out.ila   # explicit output path
```

### Decompile a binary

```bash
ailcpp decompile program.ila            # prints assembly to stdout
ailcpp decompile program.ila -o out.ail # writes to file
```

---

## Running the tests

```bash
# From inside the build directory:
./ail-tests

# Or via CTest (shows pass/fail counts):
ctest --output-on-failure
```

Expected output:

```
=== AIL C++ Test Suite ===

  PASS  MovRegVal_EncodesCorrectly
  PASS  Add_ImmediateToRegister_ProducesCorrectResult
  ...

71 passed, 0 failed
```

### What the tests cover

| File | C# equivalent | Description |
|---|---|---|
| `tests/test_compiler.cpp` | `CompilerTests.cs` | Instruction encoding, label resolution, DB directive, error handling |
| `tests/test_vm.cpp` | `VmExecutionTests.cs` | Arithmetic, bitwise, stack, conditional jumps, CLL/RET, KEI I/O, SWI string utilities, full programs |
| `tests/test_round_trip.cpp` | `RoundTripTests.cs` | Compile → decompile → recompile bytecode/functional equivalence; binary cross-compatibility checks |
| `tests/test_executable.cpp` | *(C++ only)* | `.ila` format parsing, raw-bytecode fallback, `Executable::run()` end-to-end |

---

## Binary cross-compatibility

Binaries are identical across implementations.  A `.ila` file compiled by `ailcpp compile` runs unchanged on the C# runtime, and vice versa.

### How it works

* **Identical instruction encoding** (spec §2): every instruction is exactly 6 bytes.  Byte 0 carries `(opcode << 2) | addrmode`; byte 1 is `param1`; bytes 2–5 are a 32-bit little-endian `param2`.  The C++ encoder writes these bytes explicitly, never relying on host endianness.
* **Identical `.ila` container** (spec §8): magic `AIL\0`, version (2 bytes LE), section count (2 bytes LE), then sections of type + length + data.  The C++ `Compiler::wrapIla()` and `Executable::extractCode()` use the same offsets as the C# `Executable` class.
* **Identical register bytes** (spec §3): `PC=0xF0 … Y=0xFE`.
* **Identical opcode bytes** (spec §7): all 38 opcodes.

### Spec compliance notes

| Item | C# behaviour | C++ behaviour | Notes |
|---|---|---|---|
| PC / IP width | `byte` (8-bit) | `uint16_t` (16-bit) | C++ is forward-compatible; existing 8-bit binaries run unchanged |
| Call stack `RET` | Off-by-one bug (reads wrong index) | Fixed — correct LIFO | Binaries that use `CLL`/`RET` will work correctly on C++; the C# bug means those binaries were also broken on C# |
| `SWI 0x01` | Implemented (strlen / strcpy) | Implemented identically | Byte-compatible |
| `SWI` other | Halts with message | Halts with message | Compatible |
| Program load address | Offset 0 | Offset 0 (default) | Set `AIL_CODE_BASE 0x0200` in `config.hpp` for strict IVT compliance |

---

## Embedding in your project

### As a CMake sub-directory

```cmake
add_subdirectory(source/AIL-CPP)
target_link_libraries(myapp PRIVATE ail-vm)          # VM only
target_link_libraries(myapp PRIVATE ail-compiler)    # compiler + VM
```

### As a static library

Copy the output of `cmake --install build --prefix /usr/local` and link against `libail-vm.a` / `libail-compiler.a`.

### Minimal VM-only integration (no STL)

```cpp
#include "ail/vm/vm.hpp"

// code[] is your AIL bytecode
ail::VM vm(code, codeLen);
ail::VMError err = vm.execute();
```

The `ail-vm` library has no STL container dependencies and requires only `<cstdint>` and `<cstring>` at runtime.

---

## Configuration reference

All compile-time knobs live in `include/ail/config.hpp`.  Override them *before* including any AIL header or via your build system.

| Macro | Default | Description |
|---|---|---|
| `AIL_RAM_SIZE` | 65536 (desktop) / auto-detected on embedded | VM RAM size in bytes |
| `AIL_CODE_BASE` | `0` | Address at which bytecode is loaded into RAM |
| `AIL_NO_EXCEPTIONS` | Defined on embedded targets | Disable exception handling in the VM |
| `AIL_PUTCHAR(c)` | `putchar(c)` | Output a single character |
| `AIL_GETCHAR()` | `getchar()` | Read a single character from input |
| `AIL_PUTS(s)` | `fputs(s, stdout)` | Output a null-terminated string |
| `AIL_PRINTF(...)` | `printf(...)` | Formatted output (error/debug messages only) |
| `AIL_TEST_MODE` | *(not defined)* | Route all I/O through capture buffer for unit tests |

### Example: Arduino / AVR

```cpp
// Before including any AIL header:
#define AIL_RAM_SIZE   256
#define AIL_NO_EXCEPTIONS 1
#define AIL_PUTCHAR(c) Serial.write(c)
#define AIL_GETCHAR()  Serial.read()
#define AIL_PUTS(s)    Serial.print(s)
#define AIL_PRINTF(...)  /* no-op */

#include "ail/vm/vm.hpp"
```

### Example: Raspberry Pi Pico

```cpp
#define AIL_RAM_SIZE  16384
#define AIL_PUTCHAR(c) uart_putc(uart0, c)
#define AIL_GETCHAR()  uart_getc(uart0)
#include "ail/vm/vm.hpp"
```

---

## Project structure

```
source/AIL-CPP/
├── CMakeLists.txt
├── include/ail/
│   ├── config.hpp            Compile-time platform knobs
│   ├── address_mode.hpp      AddressMode enum (RegReg/ValReg/RegVal/ValVal)
│   ├── registers.hpp         Register byte constants (0xF0–0xFE)
│   ├── opcodes.hpp           Opcode byte constants
│   ├── executable.hpp        .ila binary parser
│   ├── vm/
│   │   ├── ram.hpp           Fixed-size RAM (AIL_RAM_SIZE bytes)
│   │   ├── call_stack.hpp    Subroutine call stack (no heap)
│   │   ├── vm.hpp            VM class — registers, execute(), tick()
│   │   ├── kernel_interrupts.hpp   KEI handler
│   │   └── software_interrupts.hpp SWI handler (strlen, strcpy)
│   ├── compiler/
│   │   ├── build_exception.hpp
│   │   ├── instruction.hpp   Opcode table + 6-byte encoder
│   │   └── compiler.hpp      Two-pass assembler
│   └── decompiler/
│       └── decompiler.hpp    Disassembler
├── src/                      Implementation (.cpp) files
└── tests/
    ├── test_framework.hpp    Zero-dependency assert macros + test registry
    ├── io_capture.hpp/.cpp   stdout capture for VM tests
    ├── test_compiler.cpp     17 compiler encoding tests
    ├── test_vm.cpp           35 VM execution + output tests
    ├── test_round_trip.cpp   13 round-trip + cross-compatibility tests
    ├── test_executable.cpp   8 .ila format tests
    └── main.cpp              Test runner entry point
```
