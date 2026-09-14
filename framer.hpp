#pragma once
#include "parser.hpp"
#include "reader.hpp"

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace framer {

class MessageFramer {
public:
    template <typename ParseFunction>
        requires std::invocable<ParseFunction, std::span<const std::byte>>
    void consume_bytes(std::span<const std::byte> bytes, ParseFunction& parse) {
        size_t pos {};
        while (pos + 2 <= bytes.size()) {
            const auto len = parser::detail::parse_be<2>(bytes, pos);
            parse(bytes.subspan(pos + 2, len));
            pos += 2 + len;
        }
    }

private:
};
} // namespace framer
