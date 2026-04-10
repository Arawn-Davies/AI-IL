#include "io_capture.hpp"
#include <string>
#include <cstdio> // EOF

// ── Capture buffer ────────────────────────────────────────────────────────────

static std::string g_output;

namespace ail::test::io {

void reset() {
    g_output.clear();
}

std::string output() {
    std::string tmp;
    tmp.swap(g_output);
    return tmp;
}

} // namespace ail::test::io

// ── C-linkage hooks called by AIL_PUTCHAR / AIL_PUTS / AIL_GETCHAR ───────────

extern "C" void ail_test_write(char c) {
    g_output += c;
}

extern "C" int ail_test_read(void) {
    return EOF; // stdin not used in current tests
}

extern "C" void ail_test_puts(const char* s) {
    while (*s) g_output += *s++;
}
