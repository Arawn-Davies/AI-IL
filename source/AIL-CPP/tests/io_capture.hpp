#pragma once
// Output/input capture for VM tests compiled with AIL_TEST_MODE.
//
// Call io::reset() before each test run, then io::output() to retrieve
// everything written via AIL_PUTCHAR / AIL_PUTS during that run.

#include <string>

namespace ail::test::io {

void        reset();
std::string output();   // returns and clears the captured buffer

} // namespace ail::test::io
