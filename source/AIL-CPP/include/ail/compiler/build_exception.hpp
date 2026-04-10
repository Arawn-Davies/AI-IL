#pragma once
#include <stdexcept>
#include <string>

namespace ail::compiler {

/// Thrown by the assembler when it encounters invalid source code.
class BuildException : public std::runtime_error {
public:
    /// Construct with a message but no source-line context.
    explicit BuildException(const std::string& msg)
        : std::runtime_error(msg), m_line(-1) {}

    /// Construct with a message and a 1-based source-line number.
    BuildException(const std::string& msg, int line)
        : std::runtime_error(
              "Line " + std::to_string(line) + ": " + msg)
        , m_line(line) {}

    /// 1-based source-line number, or -1 if not available.
    int line() const noexcept { return m_line; }

private:
    int m_line;
};

} // namespace ail::compiler
