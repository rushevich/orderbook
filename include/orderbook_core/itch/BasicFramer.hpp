#pragma once
#include "orderbook_core/util/Endian.hpp"

#include <concepts>
#include <cstddef>
#include <span>

namespace rushevich::itch {

class BasicFramer {
public:
    template <typename ParseFunction>
        requires std::invocable<ParseFunction, std::span<const std::byte>>
    void consume_bytes(std::span<const std::byte> bytes, ParseFunction& parse) {
        size_t pos {};
        while (pos + 2 <= bytes.size()) {
            const auto len = util::parse_be<2>(bytes, pos);
            if (pos + 2 + len > bytes.size()) [[unlikely]] {
                return;
            }
            [[maybe_unused]] auto val = parse(bytes.subspan(pos + 2, len));
            pos += 2 + len;
        }
    }

    template <typename ParseFunction, typename OtherFunction>
        requires std::invocable<ParseFunction, std::span<const std::byte>>
    void consume_bytes(std::span<const std::byte> bytes, ParseFunction& parse,
                       OtherFunction& other) {
        size_t pos {};
        while (pos + 2 <= bytes.size()) {
            const auto len = util::parse_be<2>(bytes, pos);
            if (pos + 2 + len > bytes.size()) [[unlikely]] {
                return;
            }
            auto val = parse(bytes.subspan(pos + 2, len));
            other(val.value());
            pos += 2 + len;
        }
    }

private:
};
} // namespace rushevich::itch
