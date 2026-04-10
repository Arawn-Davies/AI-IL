# Artemis-IL Roadmap

This document tracks planned milestones and work items for the Artemis-IL project. Each item below maps to a GitHub Issue and is tracked in the **[Projects pane](../../projects)**.

---

## Milestone 1 — Complete AIL v2.0 Compliance

These items bring the .NET VM implementation into full compliance with the AIL v2.0 specification.

| # | Title | Description |
|---|-------|-------------|
| 1 | **Implement I/O port instructions** | `INB` (0x24), `INW` (0x25), `IND` (0x26), `OUB` (0x27), `OUW` (0x28), and `OUD` (0x29) are currently no-ops. Implement the I/O port infrastructure and wire these opcodes up in `op.cs`. |
| 2 | **Expand kernel interrupt table (KEI)** | `KEI 0x01` supports stdio and `KEI 0x02` halts. Define and implement additional kernel interrupts (e.g. memory operations, timer, random). |
| 3 | **Expand software interrupt table (SWI)** | `SWI 0x01` supports `strlen` and `strcpy`. Define and implement additional software interrupts (e.g. math utilities, memory comparison, sorting). |
| 4 | **PC/IP register correctness audit** | Verify that `PC` and `IP` advance correctly for every instruction, including edge cases for conditional jumps, calls, and returns. |

---

## Milestone 2 — Tooling Improvements

| # | Title | Description |
|---|-------|-------------|
| 5 | **Stand-alone assembler CLI** | Extract the assembler from AIL Studio into a cross-platform command-line tool so `.ail` source files can be compiled without the Windows IDE. |
| 6 | **Stand-alone decompiler CLI** | Expose the decompiler as a cross-platform command-line tool for disassembling `.ila` binaries outside of AIL Studio. |
| 7 | **Publish VM library as NuGet package** | Publish `Artemis_IL` (`netstandard2.0`) to NuGet so third-party .NET projects can embed the VM without referencing this repository directly. |
| 8 | **Structured error/exit codes in AIL-Runtime** | Define and document exit codes for normal halt, VM fault, invalid executable, and other runtime conditions. |

---

## Milestone 3 — IDE Improvements (AIL Studio)

| # | Title | Description |
|---|-------|-------------|
| 9  | **Cross-platform IDE** | AIL Studio currently targets `net8.0-windows` via WinForms. Investigate an Avalonia or MAUI port to support macOS and Linux. |
| 10 | **Syntax highlighting** | Add keyword, register, and opcode highlighting in the Studio editor. |
| 11 | **Improved assembler error messages** | Surface line numbers and descriptive messages for all `BuildException` failures so developers can locate errors quickly. |
| 12 | **Watch window in the debugger** | Allow users to add specific registers or memory addresses to a watch list in `DebugForm` for easier step-debugging. |

---

## Milestone 4 — Testing & Quality

| # | Title | Description |
|---|-------|-------------|
| 13 | **Instruction-level unit tests** | Add xUnit tests in `AIL-Tests` covering every opcode in the spec, including all addressing modes and edge cases (overflow, zero, invalid mode). |
| 14 | **Standard library tests** | Add unit tests for every `KEI` and `SWI` command. |
| 15 | **Conformance test suite** | Create a set of canonical `.ila` binaries and expected outputs that any compliant VM implementation must pass. |
| 16 | **More example programs** | Expand `/examples` with programs demonstrating: subroutine calls (`CLL`/`RET`), conditional logic (`JMT`/`JMF`), stack manipulation, string operations, and I/O once ports are implemented. |

---

## Milestone 5 — AIL v3.0 Specification

These items are exploratory and subject to specification design work before implementation begins.

| # | Title | Description |
|---|-------|-------------|
| 17 | **Define AIL v3.0 scope** | Hold a design discussion to agree on what new instructions, addressing modes, or memory model changes (if any) should appear in the next major spec revision. |
| 18 | **Extended addressing (32-bit address space)** | Evaluate widening the address bus beyond the current 16-bit limit to support programs larger than 64 KB. |
| 19 | **Floating-point instruction set extension** | Define optional FPU instructions (e.g. `FADD`, `FMUL`) as a spec extension for numeric-intensive workloads. |

---

## How to use this roadmap

1. Open the **[Projects pane](../../projects)** and select the *Artemis-IL Roadmap* project.
2. Each item in this document corresponds to a GitHub Issue. Use the **New issue** button and reference the item number and title.
3. Assign the issue to the appropriate milestone and add it to the project board.
4. Move cards across columns (Backlog → In Progress → Done) as work proceeds.
