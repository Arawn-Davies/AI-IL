cmake_minimum_required(VERSION 3.14)
project(AIL-CPP VERSION 2.0 LANGUAGES CXX)

# ── Language standard ──────────────────────────────────────────────────────────
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

# ── VM source list (shared between production and test builds) ─────────────────
set(AIL_VM_SOURCES
    src/vm/ram.cpp
    src/vm/call_stack.cpp
    src/vm/vm.cpp
    src/vm/vm_ops.cpp
    src/vm/kernel_interrupts.cpp
    src/vm/software_interrupts.cpp
    src/executable.cpp
)

# ── ail-vm — embedded-safe VM core library ─────────────────────────────────────
add_library(ail-vm STATIC ${AIL_VM_SOURCES})
target_include_directories(ail-vm PUBLIC "${AIL_INCLUDE_DIR}")
set_target_properties(ail-vm PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

# ── ail-compiler ──────────────────────────────────────────────────────────────
set(AIL_COMPILER_SOURCES
    src/compiler/instruction.cpp
    src/compiler/compiler.cpp
)
add_library(ail-compiler STATIC ${AIL_COMPILER_SOURCES})
target_include_directories(ail-compiler PUBLIC "${AIL_INCLUDE_DIR}")
target_link_libraries(ail-compiler PUBLIC ail-vm)

# ── ail-decompiler ─────────────────────────────────────────────────────────────
set(AIL_DECOMPILER_SOURCES
    src/decompiler/decompiler.cpp
)
add_library(ail-decompiler STATIC ${AIL_DECOMPILER_SOURCES})
target_include_directories(ail-decompiler PUBLIC "${AIL_INCLUDE_DIR}")
target_link_libraries(ail-decompiler PUBLIC ail-compiler)

# ── ailcpp — command-line tool ────────────────────────────────────────────────
add_executable(ailcpp src/main.cpp)
target_link_libraries(ailcpp PRIVATE ail-vm ail-compiler ail-decompiler)

# ── ail-tests — unit test executable ─────────────────────────────────────────
# Compiled with AIL_TEST_MODE so all VM I/O is routed through io_capture.cpp
# rather than stdout, allowing tests to assert on program output.
add_executable(ail-tests
    ${AIL_VM_SOURCES}
    ${AIL_COMPILER_SOURCES}
    ${AIL_DECOMPILER_SOURCES}
    tests/io_capture.cpp
    tests/test_compiler.cpp
    tests/test_vm.cpp
    tests/test_round_trip.cpp
    tests/test_executable.cpp
    tests/main.cpp
)
target_include_directories(ail-tests PRIVATE "${AIL_INCLUDE_DIR}" tests)
target_compile_definitions(ail-tests PRIVATE AIL_TEST_MODE)
# Disable -Werror for tests so test helpers don't need perfect coverage
if(NOT MSVC)
    target_compile_options(ail-tests PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ── CTest integration ─────────────────────────────────────────────────────────
enable_testing()
add_test(NAME ail-tests COMMAND ail-tests)

# ── Optional: configurable RAM size ───────────────────────────────────────────
if(DEFINED AIL_RAM_SIZE)
    foreach(_t ail-vm ail-compiler ail-decompiler ailcpp ail-tests)
        target_compile_definitions(${_t} PUBLIC AIL_RAM_SIZE=${AIL_RAM_SIZE})
    endforeach()
endif()

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS ailcpp RUNTIME DESTINATION bin)
install(TARGETS ail-vm ail-compiler ail-decompiler ARCHIVE DESTINATION lib)
install(DIRECTORY "${AIL_INCLUDE_DIR}/" DESTINATION include)
