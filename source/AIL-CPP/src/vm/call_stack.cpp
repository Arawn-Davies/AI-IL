#include "ail/vm/call_stack.hpp"

namespace ail {

bool CallStack::call(int returnAddress) {
    if (m_index >= kCapacity) return false;
    m_locations[m_index++] = returnAddress;
    return true;
}

int CallStack::ret() {
    if (m_index <= 0) return -1; // underflow
    return m_locations[--m_index];
}

} // namespace ail
