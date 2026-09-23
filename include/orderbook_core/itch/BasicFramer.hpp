#pragma once
#include "orderbook_core/Actions.hpp"
#include "orderbook_core/itch/Parser.hpp"
#include "orderbook_core/util/Endian.hpp"

#include <concepts>
#include <cstddef>
#include <expected>
#include <span>
#include <utility>

namespace rushevich::itch {

class BasicFramer {
public:
    template <typename ParseFunction>
        requires std::invocable<ParseFunction, std::span<const std::byte>>
    void consumeAllBytes(std::span<const std::byte> bytes, ParseFunction& parse) {
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
    template <typename ParseFunction>
        requires std::invocable<ParseFunction, std::span<const std::byte>>
    std::expected<OrderAction, parser::ParseError> consumeMessage(std::span<const std::byte> bytes,
                                                                  ParseFunction& parse) {
        if (_pos + 2 <= bytes.size()) {
            const auto len = util::parse_be<2>(bytes, _pos);
            if (_pos + 2 + len > bytes.size()) [[unlikely]] {
                _done = true;
                return {};
            }
            auto val = parse(bytes.subspan(_pos + 2, len));
            _pos += 2 + len;
            return val;
        }
        std::unreachable();
    }

    template <typename ParseFunction, typename OtherFunction>
        requires std::invocable<ParseFunction, std::span<const std::byte>>
    void consumeAllBytes(std::span<const std::byte> bytes, ParseFunction& parse,
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

    [[nodiscard]] bool isDone() const { return _done; }

private:
    size_t _pos {};
    bool _done { false };
};
} // namespace rushevich::itch
