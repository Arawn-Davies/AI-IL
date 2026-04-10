#pragma once

// ============================================================================
// AIL C++ Port — Compile-time configuration
//
// Override any of these before including AIL headers, e.g. in your build
// system or a platform-specific prefix header.
// ============================================================================

// ----------------------------------------------------------------------------
// Platform detection
// ----------------------------------------------------------------------------
#if defined(__AVR__)
#  define AIL_PLATFORM_EMBEDDED 1
#  define AIL_PLATFORM_AVR      1
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_BOARD) || \
      (defined(__arm__) && defined(ARDUINO))
#  define AIL_PLATFORM_EMBEDDED 1
#  define AIL_PLATFORM_RP2040   1
#elif defined(__arm__) && !defined(__linux__) && !defined(__APPLE__)
#  define AIL_PLATFORM_EMBEDDED 1
#  define AIL_PLATFORM_ARM_BARE 1
#else
#  define AIL_PLATFORM_DESKTOP  1
#endif

// ----------------------------------------------------------------------------
// RAM size (bytes)
// Reduce for memory-constrained targets; must be > the largest binary you run.
// ----------------------------------------------------------------------------
#ifndef AIL_RAM_SIZE
#  if defined(AIL_PLATFORM_AVR)
#    define AIL_RAM_SIZE  512       // Arduino Uno: 2 KB total SRAM
#  elif defined(AIL_PLATFORM_RP2040)
#    define AIL_RAM_SIZE  32768     // Pi Pico: 264 KB total SRAM
#  elif defined(AIL_PLATFORM_ARM_BARE)
#    define AIL_RAM_SIZE  16384     // generic Cortex-M
#  else
#    define AIL_RAM_SIZE  65536     // 64 KB — full spec §1 address space
#  endif
#endif

// ----------------------------------------------------------------------------
// Code base address
// Where the VM loads bytecode into RAM.  Keep at 0 for binary compatibility
// with .ila files produced by the existing C# toolchain.  Change to 0x0200
// for strict spec §1 compliance (program memory starts after the IVT).
// ----------------------------------------------------------------------------
#ifndef AIL_CODE_BASE
#  define AIL_CODE_BASE 0
#endif

// ----------------------------------------------------------------------------
// Exception support
// Disable automatically on embedded targets.
// ----------------------------------------------------------------------------
#if defined(AIL_PLATFORM_EMBEDDED) && !defined(AIL_NO_EXCEPTIONS)
#  define AIL_NO_EXCEPTIONS 1
#endif

// ----------------------------------------------------------------------------
// Test-mode I/O capture
// Defining AIL_TEST_MODE replaces all I/O macros with calls into a small
// capture buffer so unit tests can assert on VM output without touching stdio.
// The buffer is implemented in tests/io_capture.cpp.
// ----------------------------------------------------------------------------
#ifdef AIL_TEST_MODE
extern "C" void ail_test_write(char c);
extern "C" int  ail_test_read(void);
extern "C" void ail_test_puts(const char* s);
#  ifndef AIL_PUTCHAR
#    define AIL_PUTCHAR(c)   ail_test_write(static_cast<char>(c))
#  endif
#  ifndef AIL_GETCHAR
#    define AIL_GETCHAR()    ail_test_read()
#  endif
#  ifndef AIL_PUTS
#    define AIL_PUTS(s)      ail_test_puts(s)
#  endif
#  ifndef AIL_PRINTF
#    include <cstdio>
#    define AIL_PRINTF(fmt, ...) \
         do { char _b[512]; snprintf(_b, sizeof(_b), fmt, ##__VA_ARGS__); \
              ail_test_puts(_b); } while(0)
#  endif
#endif

// ----------------------------------------------------------------------------
// I/O hooks
// Override for bare-metal targets that lack stdio (e.g. route through UART).
//
//   #define AIL_PUTCHAR(c)     uart_write_byte(c)
//   #define AIL_GETCHAR()      uart_read_byte()
//   #define AIL_PUTS(s)        uart_write_str(s)
//   #define AIL_PRINTF(...)    /* no-op or custom printf */
// ----------------------------------------------------------------------------
#ifndef AIL_PUTCHAR
#  include <cstdio>
#  define AIL_PUTCHAR(c)   putchar(static_cast<int>(c))
#endif
#ifndef AIL_GETCHAR
#  include <cstdio>
#  define AIL_GETCHAR()    getchar()
#endif
#ifndef AIL_PUTS
#  include <cstdio>
#  define AIL_PUTS(s)      fputs((s), stdout)
#endif
#ifndef AIL_PRINTF
#  include <cstdio>
#  define AIL_PRINTF(...)  printf(__VA_ARGS__)
#endif

// ----------------------------------------------------------------------------
// Static assert helper (safe for C++11 and later)
// ----------------------------------------------------------------------------
#include <cstdint>
static_assert(sizeof(uint8_t)  == 1, "uint8_t must be 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
static_assert(sizeof(int32_t)  == 4, "int32_t must be 4 bytes");
