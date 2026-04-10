#pragma once
// Minimal zero-dependency test framework for AIL C++ tests.
//
// Usage:
//   TEST("my test") { REQUIRE(1 + 1 == 2); REQUIRE_EQ(42, answer()); }
//
// In main.cpp:
//   return ail::test::run_all();

#include <cstdio>
#include <cstring>
#include <vector>
#include <functional>
#include <string>

namespace ail::test {

struct Case {
    const char*            name;
    std::function<void()>  fn;
};

// ── Global test registry ──────────────────────────────────────────────────────

inline std::vector<Case>& registry() {
    static std::vector<Case> r;
    return r;
}

// Per-test failure flag (set by REQUIRE macros, read by run_all).
inline bool& failed_flag() {
    static bool f = false;
    return f;
}

// ── Registration helper ───────────────────────────────────────────────────────

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) {
        registry().push_back({name, fn});
    }
};

// ── Runner ────────────────────────────────────────────────────────────────────

inline int run_all() {
    int passed = 0, failed = 0;
    for (auto& tc : registry()) {
        failed_flag() = false;
        tc.fn();
        if (failed_flag()) {
            std::fprintf(stderr, "  FAIL  %s\n", tc.name);
            ++failed;
        } else {
            std::printf("  PASS  %s\n", tc.name);
            ++passed;
        }
    }
    std::printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}

} // namespace ail::test

// ── Macros ────────────────────────────────────────────────────────────────────

// Register a named test.  Body follows as a lambda block { ... }.
#define TEST(name)                                                         \
    static void ail_test_body_##name();                                    \
    static ail::test::Registrar ail_test_reg_##name(#name, ail_test_body_##name); \
    static void ail_test_body_##name()

// Assert a boolean expression; stop this test on failure.
#define REQUIRE(expr)                                                      \
    do {                                                                   \
        if (!(expr)) {                                                     \
            std::fprintf(stderr, "    REQUIRE(" #expr ") at %s:%d\n",     \
                         __FILE__, __LINE__);                               \
            ail::test::failed_flag() = true;                               \
            return;                                                        \
        }                                                                  \
    } while (0)

// Assert equality; prints both sides on failure.
#define REQUIRE_EQ(actual, expected)                                       \
    do {                                                                   \
        auto _a = (actual);                                                \
        auto _e = (expected);                                              \
        if (!(_a == _e)) {                                                 \
            std::fprintf(stderr,                                           \
                "    REQUIRE_EQ(" #actual ", " #expected ") at %s:%d\n",  \
                __FILE__, __LINE__);                                       \
            ail::test::failed_flag() = true;                               \
            return;                                                        \
        }                                                                  \
    } while (0)

// Assert that an expression throws any exception.
#define REQUIRE_THROWS(expr)                                               \
    do {                                                                   \
        bool _threw = false;                                               \
        try { (void)(expr); } catch (...) { _threw = true; }              \
        if (!_threw) {                                                     \
            std::fprintf(stderr,                                           \
                "    REQUIRE_THROWS(" #expr ") did not throw at %s:%d\n", \
                __FILE__, __LINE__);                                       \
            ail::test::failed_flag() = true;                               \
            return;                                                        \
        }                                                                  \
    } while (0)
