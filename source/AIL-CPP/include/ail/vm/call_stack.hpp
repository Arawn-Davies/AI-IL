#pragma once
#include "ail/config.hpp"

namespace ail {

/// Subroutine call stack used by CLL/CLT/CLF (push) and RET (pop).
/// Fixed-size, no dynamic allocation.
class CallStack {
public:
    CallStack() = default;

    /// Push @p returnAddress. Returns false if the stack is full.
    bool call(int returnAddress);

    /// Pop and return the top return address.
    /// Returns -1 if the stack is empty (underflow).
    int ret();

    /// True when nothing has been pushed.
    bool empty() const { return m_index == 0; }

    /// True when no more entries can be pushed.
    bool full()  const { return m_index >= kCapacity; }

private:
    static constexpr int kCapacity = 255;
    int m_locations[kCapacity] = {};
    int m_index = 0;
};

} // namespace ail
