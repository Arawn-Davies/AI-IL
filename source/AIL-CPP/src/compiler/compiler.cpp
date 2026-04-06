#include "ail/compiler/compiler.hpp"
#include "ail/compiler/instruction.hpp"
#include "ail/registers.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstring>

namespace ail::compiler {

// ── Register table ────────────────────────────────────────────────────────────

struct RegEntry { const char* name; uint8_t byte; };
static const RegEntry kRegs[] = {
    {"PC",0xF0},{"IP",0xF1},{"SP",0xF2},{"SS",0xF3},
    {"A", 0xF4},{"AL",0xF5},{"AH",0xF6},
    {"B", 0xF7},{"BL",0xF8},{"BH",0xF9},
    {"C", 0xFA},{"CL",0xFB},{"CH",0xFC},
    {"X", 0xFD},{"Y", 0xFE},
};
static constexpr int kRegCount = static_cast<int>(sizeof(kRegs)/sizeof(kRegs[0]));

static std::string toUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}
static bool isRegToken(const std::string& t, uint8_t& outByte) {
    std::string up = toUpper(t);
    for (int i = 0; i < kRegCount; ++i)
        if (up == kRegs[i].name) { outByte = kRegs[i].byte; return true; }
    return false;
}

// ── Char literal helpers ──────────────────────────────────────────────────────

static uint8_t escapeToChar(char e) {
    switch (e) {
        case 'n': return '\n'; case 'r': return '\r'; case 't': return '\t';
        case 'a': return '\a'; case 'b': return '\b'; case 'v': return '\v';
        case '0': return 0;   case '\'': return '\''; case '"': return '"';
        case '\\': return '\\';
        default:  return static_cast<uint8_t>(e);
    }
}
static bool isCharLiteral(const std::string& tok) {
    if (tok.size() < 3) return false;
    if (tok.front() != '\'' || tok.back() != '\'') return false;
    return true;
}
static uint8_t parseCharLiteral(const std::string& tok) {
    // tok is in the form 'X' or '\n'
    if (tok[1] == '\\' && tok.size() == 4) return escapeToChar(tok[2]);
    return static_cast<uint8_t>(tok[1]);
}

// ── Integer parsing ───────────────────────────────────────────────────────────

bool Compiler::tryParseInteger(const std::string& tok, int32_t& out) {
    if (tok.size() >= 3 &&
        tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')) {
        try {
            out = static_cast<int32_t>(std::stoul(tok.substr(2), nullptr, 16));
            return true;
        } catch (...) { return false; }
    }
    try {
        out = std::stoi(tok);
        return true;
    } catch (...) { return false; }
}

// ── Comment stripping ─────────────────────────────────────────────────────────

std::string Compiler::stripComment(const std::string& line) {
    bool inStr  = false;
    bool inChar = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inStr) {
            if (c == '\\') { ++i; continue; }
            if (c == '"')  inStr  = false;
        } else if (inChar) {
            if (c == '\\') { ++i; continue; }
            if (c == '\'') inChar = false;
        } else {
            if (c == '"')  { inStr  = true;  continue; }
            if (c == '\'') { inChar = true;  continue; }
            if (c == ';')  return line.substr(0, i);
            if (c == '/' && i + 1 < line.size() && line[i+1] == '/')
                return line.substr(0, i);
        }
    }
    return line;
}

// ── Tokeniser ─────────────────────────────────────────────────────────────────

std::vector<std::string> Compiler::tokenise(const std::string& line) {
    std::vector<std::string> tokens;
    std::string cur;
    size_t i = 0;
    while (i < line.size()) {
        char c = line[i];
        if (c == '\'') {
            cur += c; ++i;
            while (i < line.size()) {
                char ch = line[i]; cur += ch; ++i;
                if (ch == '\\' && i < line.size()) { cur += line[i]; ++i; }
                else if (ch == '\'') break;
            }
        } else if (c == '"') {
            cur += c; ++i;
            while (i < line.size()) {
                char ch = line[i]; cur += ch; ++i;
                if (ch == '\\' && i < line.size()) { cur += line[i]; ++i; }
                else if (ch == '"') break;
            }
        } else if (c == ' ' || c == '\t' || c == ',') {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            ++i;
        } else {
            cur += c; ++i;
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

// ── Param parsing ─────────────────────────────────────────────────────────────

std::tuple<bool, uint8_t, int32_t> Compiler::parseParam(
    const std::string& token,
    const std::unordered_map<std::string, int>& labels,
    int lineNum,
    bool isParam1)
{
    uint8_t rb = 0;
    if (isRegToken(token, rb))
        return {true, rb, static_cast<int32_t>(rb)};

    if (isCharLiteral(token)) {
        uint8_t cv = parseCharLiteral(token);
        return {false, cv, static_cast<int32_t>(cv)};
    }

    int32_t num = 0;
    if (tryParseInteger(token, num)) {
        uint8_t bv = isParam1 ? static_cast<uint8_t>(num & 0xFF) : 0;
        return {false, bv, num};
    }

    // Label reference
    auto it = labels.find(token);
    if (it != labels.end())
        return {false, 0, static_cast<int32_t>(it->second)};

    throw BuildException(
        std::string("Invalid value for parameter ") +
        (isParam1 ? "1" : "2") + ": '" + token + "'", lineNum);
}

// ── DB helpers ────────────────────────────────────────────────────────────────

int Compiler::dbTokenByteCount(const std::string& tok, int lineNum) {
    if (tok.size() >= 2 && tok.front() == '"' && tok.back() == '"') {
        const std::string content = tok.substr(1, tok.size() - 2);
        int n = 0;
        for (size_t j = 0; j < content.size(); ++j) {
            if (content[j] == '\\' && j + 1 < content.size()) ++j;
            ++n;
        }
        return n;
    }
    if (isCharLiteral(tok)) return 1;
    int32_t dummy = 0;
    if (tryParseInteger(tok, dummy)) return 1;
    throw BuildException("DB: invalid byte value '" + tok + "'", lineNum);
}

int Compiler::countDbBytes(const std::vector<std::string>& tokens, int lineNum) {
    int n = 0;
    for (size_t i = 1; i < tokens.size(); ++i)
        n += dbTokenByteCount(tokens[i], lineNum);
    return n;
}

std::vector<uint8_t> Compiler::assembleDb(
    const std::vector<std::string>& tokens, int lineNum)
{
    std::vector<uint8_t> out;
    for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string& tok = tokens[i];
        if (tok.size() >= 2 && tok.front() == '"' && tok.back() == '"') {
            const std::string content = tok.substr(1, tok.size() - 2);
            for (size_t j = 0; j < content.size(); ++j) {
                if (content[j] == '\\' && j + 1 < content.size()) {
                    out.push_back(escapeToChar(content[j + 1]));
                    ++j;
                } else {
                    out.push_back(static_cast<uint8_t>(content[j]));
                }
            }
        } else if (isCharLiteral(tok)) {
            out.push_back(parseCharLiteral(tok));
        } else {
            int32_t num = 0;
            if (tryParseInteger(tok, num)) {
                out.push_back(static_cast<uint8_t>(num & 0xFF));
            } else {
                throw BuildException("DB: invalid byte value '" + tok + "'", lineNum);
            }
        }
    }
    return out;
}

// ── Line assembly ─────────────────────────────────────────────────────────────

std::vector<uint8_t> Compiler::assembleLine(
    const std::vector<std::string>& tokens,
    const std::unordered_map<std::string, int>& labels,
    int lineNum)
{
    std::string mnemonic = toUpper(tokens[0]);
    uint8_t opcode = Instruction::getOpcode(mnemonic);
    if (opcode == 0 && !Instruction::isMnemonic(mnemonic))
        throw BuildException("Unknown mnemonic '" + tokens[0] + "'", lineNum);

    bool    reg1 = false, reg2 = false;
    uint8_t p1b  = 0;
    int32_t p1i  = 0, p2i = 0;

    if (tokens.size() >= 2)
        std::tie(reg1, p1b, p1i) = parseParam(tokens[1], labels, lineNum, true);

    if (tokens.size() >= 3) {
        uint8_t dummy = 0;
        std::tie(reg2, dummy, p2i) = parseParam(tokens[2], labels, lineNum, false);
    } else if (!reg1) {
        // Single non-register operand (e.g. JMP 0x10, KEI 0x02): propagate
        // the full integer to param2 so the VM picks it up correctly.
        p2i = p1i;
    }

    AddressMode mode;
    if      ( reg1 &&  reg2) mode = AddressMode::RegReg;
    else if (!reg1 &&  reg2) mode = AddressMode::ValReg;
    else if ( reg1 && !reg2) mode = AddressMode::RegVal;
    else                     mode = AddressMode::ValVal;

    auto arr = Instruction::encode(opcode, mode, p1b, p2i);
    return std::vector<uint8_t>(arr.begin(), arr.end());
}

// ── Constructor + compile ─────────────────────────────────────────────────────

Compiler::Compiler(const std::string& source) : m_source(source) {}

std::vector<uint8_t> Compiler::compile() {
    if (m_source.empty())
        throw BuildException("Source cannot be empty.");

    // Normalise line endings.
    std::string src = m_source;
    src.erase(std::remove(src.begin(), src.end(), '\r'), src.end()); // NOLINT

    std::vector<std::string> lines;
    std::istringstream iss(src);
    std::string ln;
    while (std::getline(iss, ln)) lines.push_back(ln);

    // ── Pass 1: resolve labels ────────────────────────────────────────────────
    std::unordered_map<std::string, int> labels;
    int byteOffset = 0;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        std::string stripped = stripComment(lines[i]);
        // trim
        size_t s = stripped.find_first_not_of(" \t");
        if (s == std::string::npos) continue;
        stripped = stripped.substr(s);
        size_t e = stripped.find_last_not_of(" \t");
        if (e != std::string::npos) stripped = stripped.substr(0, e + 1);
        if (stripped.empty()) continue;

        if (stripped.back() == ':') {
            std::string name = stripped.substr(0, stripped.size() - 1);
            // trim name
            size_t ns = name.find_first_not_of(" \t");
            if (ns != std::string::npos) name = name.substr(ns);
            if (name.empty())
                throw BuildException("Empty label name.", i + 1);
            if (labels.count(name))
                throw BuildException("Duplicate label '" + name + "'.", i + 1);
            labels[name] = byteOffset;
        } else {
            auto toks = tokenise(stripped);
            if (toks.empty()) continue;
            if (toUpper(toks[0]) == "DB")
                byteOffset += countDbBytes(toks, i + 1);
            else
                byteOffset += 6;
        }
    }

    // ── Pass 2: emit bytes ────────────────────────────────────────────────────
    std::vector<uint8_t> bytes;
    bytes.reserve(static_cast<size_t>(byteOffset));

    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        std::string stripped = stripComment(lines[i]);
        size_t s = stripped.find_first_not_of(" \t");
        if (s == std::string::npos) continue;
        stripped = stripped.substr(s);
        size_t ee = stripped.find_last_not_of(" \t");
        if (ee != std::string::npos) stripped = stripped.substr(0, ee + 1);
        if (stripped.empty() || stripped.back() == ':') continue;

        auto toks = tokenise(stripped);
        if (toks.empty()) continue;

        if (toUpper(toks[0]) == "DB") {
            auto db = assembleDb(toks, i + 1);
            bytes.insert(bytes.end(), db.begin(), db.end());
        } else {
            auto instr = assembleLine(toks, labels, i + 1);
            bytes.insert(bytes.end(), instr.begin(), instr.end());
        }
    }

    byteCode = bytes;
    return byteCode;
}

// ── .ila wrapper ──────────────────────────────────────────────────────────────

std::vector<uint8_t> Compiler::wrapIla(const std::vector<uint8_t>& code) {
    // Header: magic(4) + version(2) + sectionCount(2) = 8 bytes
    // Section: type(2) + length(4) + data(N)
    uint32_t codeLen = static_cast<uint32_t>(code.size());
    std::vector<uint8_t> out;
    out.reserve(8 + 6 + codeLen);

    // Magic "AIL\0"
    out.push_back(0x41); out.push_back(0x49);
    out.push_back(0x4C); out.push_back(0x00);
    // Version 2.0 (little-endian)
    out.push_back(0x02); out.push_back(0x00);
    // Section count = 1
    out.push_back(0x01); out.push_back(0x00);
    // Section type = 0x0001 (code)
    out.push_back(0x01); out.push_back(0x00);
    // Section length (little-endian)
    out.push_back(static_cast<uint8_t>( codeLen        & 0xFF));
    out.push_back(static_cast<uint8_t>((codeLen >>  8) & 0xFF));
    out.push_back(static_cast<uint8_t>((codeLen >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((codeLen >> 24) & 0xFF));
    // Section data
    out.insert(out.end(), code.begin(), code.end());
    return out;
}

} // namespace ail::compiler
