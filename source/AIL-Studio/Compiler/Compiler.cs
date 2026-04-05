using System;
using System.Collections.Generic;
using System.Text;

namespace AIL_Studio.Compiler
{
    /// <summary>
    /// Two-pass assembler for Artemis-IL.
    ///
    /// Syntax (per line):
    ///   ; or // comment
    ///   label:
    ///   MNEMONIC [param1 [, param2]]
    ///
    /// Values: decimal, 0x hex, or 'X' / '\n' char literals.
    /// Registers: PC IP SP SS A AL AH B BL BH C CL CH X Y (case-insensitive).
    /// Labels may be used as jump targets.
    /// </summary>
    public class Compiler
    {
        private readonly string _source;

        /// <summary>Raw bytecode produced after <see cref="Compile"/>.</summary>
        public byte[] ByteCode { get; private set; } = Array.Empty<byte>();

        public Compiler(string source)
        {
            _source = source ?? throw new ArgumentNullException(nameof(source));
        }

        // ── Public API ────────────────────────────────────────────────────────

        /// <summary>
        /// Assembles the source and returns the raw bytecode.
        /// Throws <see cref="BuildException"/> on error.
        /// </summary>
        public byte[] Compile()
        {
            if (string.IsNullOrWhiteSpace(_source))
                throw new BuildException("Source cannot be empty.");

            string[] lines = _source.Replace("\r", "").Split('\n');

            // Pass 1 — discover labels
            var labels = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            int instrIndex = 0;
            for (int i = 0; i < lines.Length; i++)
            {
                string stripped = StripComment(lines[i]).Trim();
                if (stripped.Length == 0)
                    continue;
                if (stripped.EndsWith(':'))
                {
                    string name = stripped[..^1].Trim();
                    if (name.Length == 0)
                        throw new BuildException("Empty label name.", i + 1);
                    if (labels.ContainsKey(name))
                        throw new BuildException($"Duplicate label '{name}'.", i + 1);
                    labels[name] = instrIndex * 6;
                }
                else
                {
                    instrIndex++;
                }
            }

            // Pass 2 — emit bytecode
            var bytes = new List<byte>(instrIndex * 6);
            for (int i = 0; i < lines.Length; i++)
            {
                string stripped = StripComment(lines[i]).Trim();
                if (stripped.Length == 0 || stripped.EndsWith(':'))
                    continue;

                string[] tokens = Tokenise(stripped);
                if (tokens.Length == 0)
                    continue;

                byte[] instr = AssembleLine(tokens, labels, i + 1);
                bytes.AddRange(instr);
            }

            ByteCode = bytes.ToArray();
            return ByteCode;
        }

        // ── Line assembly ─────────────────────────────────────────────────────

        private static byte[] AssembleLine(string[] tokens, Dictionary<string, int> labels, int lineNum)
        {
            string mnemonic = tokens[0].ToUpperInvariant();
            byte opcode = Instruction.GetOpcode(mnemonic);
            if (opcode == 0 && !Instruction.IsMnemonic(mnemonic))
                throw new BuildException($"Unknown mnemonic '{tokens[0]}'.", lineNum);

            // Classify each operand
            bool reg1 = false, reg2 = false;
            byte  p1b = 0;
            int   p2i = 0;

            if (tokens.Length >= 2)
                (reg1, p1b, _) = ParseParam(tokens[1], labels, lineNum, isParam1: true);

            if (tokens.Length >= 3)
                (reg2, _, p2i) = ParseParam(tokens[2], labels, lineNum, isParam1: false);

            // Determine addressing mode
            AddressMode mode = (reg1, reg2) switch
            {
                (true,  true)  => AddressMode.RegisterRegister,
                (false, true)  => AddressMode.ValueRegister,
                (true,  false) => AddressMode.RegisterValue,
                _              => AddressMode.ValueValue,
            };

            return Instruction.Encode(opcode, mode, p1b, p2i);
        }

        /// <summary>
        /// Parses a single operand token.
        /// Returns (isRegister, byteValue, intValue).
        /// </summary>
        private static (bool isReg, byte byteVal, int intVal) ParseParam(
            string token, Dictionary<string, int> labels, int lineNum, bool isParam1)
        {
            // Register?
            if (Registers.IsRegister(token))
            {
                byte rb = Registers.GetByte(token);
                return (true, rb, rb);
            }

            // Char literal?
            if (CharValue.Check(token))
            {
                byte cv = CharValue.Parse(token);
                return (false, cv, cv);
            }

            // Hex / decimal literal?
            if (TryParseInteger(token, out int numVal))
            {
                byte bv = isParam1 ? (byte)(numVal & 0xFF) : (byte)0;
                return (false, bv, numVal);
            }

            // Label reference?
            if (labels.TryGetValue(token, out int addr))
                return (false, 0, addr);

            string paramNum = isParam1 ? "1" : "2";
            throw new BuildException($"Invalid value for parameter {paramNum}: '{token}'.", lineNum);
        }

        // ── Helpers ───────────────────────────────────────────────────────────

        /// <summary>Removes everything from the first comment marker to end-of-line.</summary>
        private static string StripComment(string line)
        {
            bool inChar = false;
            for (int i = 0; i < line.Length; i++)
            {
                char c = line[i];
                if (!inChar)
                {
                    if (c == '\'') { inChar = true; continue; }
                    if (c == ';') return line[..i];
                    if (c == '/' && i + 1 < line.Length && line[i + 1] == '/') return line[..i];
                }
                else
                {
                    // Inside a char literal: skip an escaped character then wait for closing '
                    if (c == '\\') { i++; continue; }  // skip next char
                    if (c == '\'') inChar = false;
                }
            }
            return line;
        }

        /// <summary>
        /// Splits a line into tokens, treating commas as whitespace.
        /// </summary>
        private static string[] Tokenise(string line)
        {
            var tokens = new List<string>();
            // Replace commas with spaces, then split — but preserve char literals intact.
            var sb = new StringBuilder();
            bool inChar = false;
            foreach (char c in line)
            {
                if (c == '\'' ) inChar = !inChar;
                if (!inChar && c == ',') sb.Append(' ');
                else sb.Append(c);
            }
            foreach (string part in sb.ToString().Split(' ', StringSplitOptions.RemoveEmptyEntries))
                tokens.Add(part);
            return tokens.ToArray();
        }

        private static bool TryParseInteger(string token, out int value)
        {
            if (token.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
            {
                if (int.TryParse(token[2..], System.Globalization.NumberStyles.HexNumber,
                    null, out value))
                    return true;
            }
            else if (int.TryParse(token, out value))
            {
                return true;
            }
            value = 0;
            return false;
        }
    }
}
