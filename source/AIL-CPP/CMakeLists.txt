cmake_minimum_required(VERSION 3.14)
project(AIL-CPP VERSION 2.0 LANGUAGES CXX)

# ── Language standard ──────────────────────────────────────────────────────────
# C++17 for the desktop CLI; the VM core (ail-vm) is written to C++11.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler warnings ──────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic -Werror
        -Wno-unused-parameter
    )
endif()

# ── Include directories ────────────────────────────────────────────────────────
set(AIL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# ── ail-vm — embedded-safe VM core library ─────────────────────────────────────
# This target intentionally avoids STL containers and exceptions so it can be
# compiled for AVR, RP2040, or bare-metal ARM with minimal changes.
add_library(ail-vm STATIC
    src/vm/ram.cpp
    src/vm/call_stack.cpp
    src/vm/vm.cpp
    src/vm/vm_ops.cpp
    src/vm/kernel_interrupts.cpp
    src/executable.cpp
)
target_include_directories(ail-vm PUBLIC "${AIL_INCLUDE_DIR}")
# Keep to C++11 for the VM core so it compiles on every embedded toolchain.
set_target_properties(ail-vm PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

# ── ail-compiler — two-pass assembler (desktop, C++17) ────────────────────────
add_library(ail-compiler STATIC
    src/compiler/instruction.cpp
    src/compiler/compiler.cpp
)
target_include_directories(ail-compiler PUBLIC "${AIL_INCLUDE_DIR}")
target_link_libraries(ail-compiler PUBLIC ail-vm)

# ── ail-decompiler — disassembler (desktop, C++17) ────────────────────────────
add_library(ail-decompiler STATIC
    src/decompiler/decompiler.cpp
)
target_include_directories(ail-decompiler PUBLIC "${AIL_INCLUDE_DIR}")
target_link_libraries(ail-decompiler PUBLIC ail-compiler)

# ── ailcpp — command-line tool ────────────────────────────────────────────────
add_executable(ailcpp src/main.cpp)
target_link_libraries(ailcpp PRIVATE ail-vm ail-compiler ail-decompiler)

# ── Optional: configurable RAM size (useful for embedded cross-builds) ─────────
# Pass -DAIL_RAM_SIZE=512 to cmake to limit RAM for small targets.
if(DEFINED AIL_RAM_SIZE)
    target_compile_definitions(ail-vm     PUBLIC AIL_RAM_SIZE=${AIL_RAM_SIZE})
    target_compile_definitions(ail-compiler PUBLIC AIL_RAM_SIZE=${AIL_RAM_SIZE})
    target_compile_definitions(ail-decompiler PUBLIC AIL_RAM_SIZE=${AIL_RAM_SIZE})
    target_compile_definitions(ailcpp     PUBLIC AIL_RAM_SIZE=${AIL_RAM_SIZE})
endif()

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS ailcpp RUNTIME DESTINATION bin)
install(TARGETS ail-vm ail-compiler ail-decompiler ARCHIVE DESTINATION lib)
install(DIRECTORY "${AIL_INCLUDE_DIR}/" DESTINATION include)
