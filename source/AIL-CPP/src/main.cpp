#include "ail/executable.hpp"
#include "ail/compiler/compiler.hpp"
#include "ail/decompiler/decompiler.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

// ── File I/O helpers ──────────────────────────────────────────────────────────

static std::vector<uint8_t> readFile(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) { std::fprintf(stderr, "error: cannot open '%s'\n", path); return {}; }
    auto sz = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

static bool writeFile(const char* path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) { std::fprintf(stderr, "error: cannot write '%s'\n", path); return false; }
    f.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
    return f.good();
}

static std::string readTextFile(const char* path) {
    std::ifstream f(path);
    if (!f) { std::fprintf(stderr, "error: cannot open '%s'\n", path); return {}; }
    return std::string(std::istreambuf_iterator<char>(f),
                       std::istreambuf_iterator<char>());
}

// ── Usage ─────────────────────────────────────────────────────────────────────

static void printUsage(const char* prog) {
    std::fprintf(stderr,
        "Usage:\n"
        "  %s run       <file.ila>              Run a compiled binary\n"
        "  %s compile   <file.ail> [-o out.ila] Assemble source to binary\n"
        "  %s decompile <file.ila> [-o out.ail] Disassemble binary to source\n",
        prog, prog, prog);
}

// ── Sub-commands ──────────────────────────────────────────────────────────────

static int cmdRun(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "error: 'run' requires a file argument\n");
        return 1;
    }
    auto data = readFile(argv[0]);
    if (data.empty()) return 1;
    if (!ail::Executable::run(data.data(), static_cast<int>(data.size()))) {
        std::fprintf(stderr, "error: malformed or unsupported binary '%s'\n", argv[0]);
        return 1;
    }
    return 0;
}

static int cmdCompile(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "error: 'compile' requires an input file\n");
        return 1;
    }
    const char* input  = argv[0];
    const char* output = nullptr;

    for (int i = 1; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], "-o") == 0) {
            output = argv[i + 1];
            break;
        }
    }

    // Default output: replace/append .ila extension
    std::string defaultOut;
    if (!output) {
        defaultOut = input;
        size_t dot = defaultOut.rfind('.');
        if (dot != std::string::npos) defaultOut.resize(dot);
        defaultOut += ".ila";
        output = defaultOut.c_str();
    }

    std::string src = readTextFile(input);
    if (src.empty()) return 1;

    try {
        ail::compiler::Compiler c(src);
        auto bytecode = c.compile();
        auto ila      = ail::compiler::Compiler::wrapIla(bytecode);
        if (!writeFile(output, ila)) return 1;
        std::printf("Compiled '%s' → '%s' (%zu bytes)\n",
                    input, output, ila.size());
    } catch (const ail::compiler::BuildException& e) {
        std::fprintf(stderr, "compile error: %s\n", e.what());
        return 1;
    }
    return 0;
}

static int cmdDecompile(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "error: 'decompile' requires an input file\n");
        return 1;
    }
    const char* input  = argv[0];
    const char* output = nullptr;

    for (int i = 1; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], "-o") == 0) {
            output = argv[i + 1];
            break;
        }
    }

    auto data = readFile(input);
    if (data.empty()) return 1;

    ail::decompiler::Decompiler d(data);
    std::string src = d.decompile();

    if (output) {
        std::ofstream f(output);
        if (!f) { std::fprintf(stderr, "error: cannot write '%s'\n", output); return 1; }
        f << src;
        std::printf("Decompiled '%s' → '%s'\n", input, output);
    } else {
        std::cout << src;
    }
    return 0;
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    if (argc < 2) { printUsage(argv[0]); return 1; }

    const char* cmd  = argv[1];
    char**      rest = argv + 2;
    int         rem  = argc - 2;

    if (std::strcmp(cmd, "run")       == 0) return cmdRun(rem, rest);
    if (std::strcmp(cmd, "compile")   == 0) return cmdCompile(rem, rest);
    if (std::strcmp(cmd, "decompile") == 0) return cmdDecompile(rem, rest);

    std::fprintf(stderr, "error: unknown command '%s'\n", cmd);
    printUsage(argv[0]);
    return 1;
}
