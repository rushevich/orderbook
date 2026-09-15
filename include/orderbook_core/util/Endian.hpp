#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
namespace rushevich::util {
template <size_t N> constexpr auto parse_be(std::span<const std::byte> data, size_t offset) {
    static_assert(N > 0 && N <= 8);
    uint64_t value {};
    for (size_t i {}; i < N; ++i) {
        value = (value << 8) | std::to_integer<uint64_t>(data[i + offset]);
    }
    if constexpr (N == 8) {
        return value;
    } else if constexpr (N == 4) {
        return static_cast<uint32_t>(value);
    } else if constexpr (N == 2) {
        return static_cast<uint16_t>(value);
    } else if constexpr (N == 1) {
        return static_cast<uint8_t>(value);
    }
}

} // namespace rushevich::util
