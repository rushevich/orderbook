#pragma once

#include <cstdint>

namespace rushevich::container {
template <typename HandleType = uint32_t> struct IntrusiveNode {
    static constexpr auto NULL_HANDLE { static_cast<HandleType>(-1) };
    HandleType prev_handle { NULL_HANDLE };
    HandleType next_handle { NULL_HANDLE };
};
} // namespace rushevich::container
