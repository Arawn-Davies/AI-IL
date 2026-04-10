// AIL C++ test runner — registers all test translation units and runs them.
// Exit code 0 = all passed, 1 = one or more failed.

#include "test_framework.hpp"

// The TEST() macros in each file register themselves via static initializers.
// We only need to include the headers to pull in the registrations.
// (The .cpp files are compiled as separate translation units and linked in.)

int main() {
    std::printf("=== AIL C++ Test Suite ===\n\n");
    return ail::test::run_all();
}
