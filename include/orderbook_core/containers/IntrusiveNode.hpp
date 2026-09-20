#pragma once

#include <cstdint>

namespace rushevich::container {
template <typename HandleType = uint32_t> struct IntrusiveNode {
    static constexpr auto NULL_HANDLE { static_cast<HandleType>(-1) };
    HandleType prevHandle { NULL_HANDLE };
    HandleType nextHandle { NULL_HANDLE };
};

template <typename HandleType> struct IntrusiveNode<HandleType*> {
    static constexpr auto NULL_HANDLE { nullptr };
    HandleType* prevHandle { NULL_HANDLE };
    HandleType* nextHandle { NULL_HANDLE };
};
} // namespace rushevich::container
