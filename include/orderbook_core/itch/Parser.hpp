#pragma once

#include "orderbook_core/Actions.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iostream>
#include <span>

namespace rushevich::parser {

// Function declarations for internal parse-handling:
OrderAction parse_add(std::span<const std::byte> msg);
OrderAction parse_execute(std::span<const std::byte> msg);
OrderAction parse_cancel(std::span<const std::byte> msg);
OrderAction parse_delete(std::span<const std::byte> msg);
OrderAction parse_replace(std::span<const std::byte> msg);
OrderAction parse_do_nothing(std::span<const std::byte> msg);

using ParsingFunction = OrderAction (*)(std::span<const std::byte> msg);

inline constexpr std::array<ParsingFunction, 256> parse_lut = [] consteval {
    std::array<ParsingFunction, 256> arr {};
    arr['A'] = parse_add;
    arr['F'] = parse_add;
    arr['E'] = parse_execute;
    arr['C'] = parse_execute;
    arr['X'] = parse_cancel;
    arr['D'] = parse_delete;
    arr['U'] = parse_replace;
    arr['S'] = parse_do_nothing; // System Event
    arr['R'] = parse_do_nothing; // Stock Directory
    arr['H'] = parse_do_nothing; // Stock Trading Action
    arr['Y'] = parse_do_nothing; // Reg SHO Restriction
    arr['L'] = parse_do_nothing; // Market Participant Position
    arr['V'] = parse_do_nothing; // MWCB Decline Level
    arr['W'] = parse_do_nothing; // MWCB Status
    arr['K'] = parse_do_nothing; // IPO Quoting Period Update
    arr['J'] = parse_do_nothing; // LULD Auction Collar
    arr['h'] = parse_do_nothing; // Operational Halt
    arr['P'] = parse_do_nothing; // Trade (non-cross)
    arr['Q'] = parse_do_nothing; // Cross Trade
    arr['B'] = parse_do_nothing; // Broken Trade
    arr['I'] = parse_do_nothing; // NOII
    arr['N'] = parse_do_nothing; // RPII
    arr['O'] = parse_do_nothing; // Direct Listing with Capital Raise

    return arr;
}();

enum class ParseError : uint8_t {
    empty,
    unknown_type,
    truncated,
};

class Parser {
public:
    // Parses the arbitrary size message and outputs a collection of actions to
    // perform
    // TODO: ensure that we can elide the move / copy
    [[nodiscard]] std::expected<OrderAction, ParseError>
    operator()(std::span<const std::byte> msg) noexcept;

    [[nodiscard]] auto count() const { return _count; }

    [[nodiscard]] auto histogram() { return _by_type; }

    // Pass in the histogram and a desired output stream
    void dump_stats(std::ostream& out);

private:
    // Count of messages that have been parsed so far
    size_t _count {};
    std::array<uint64_t, 256> _by_type {};
};

} // namespace rushevich::parser
