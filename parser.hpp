#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <system_error>
#include <variant>

namespace parser {
// This corresponds to Stock Locate in the itch spec
enum class InstrumentID : uint16_t {};
// This corresponds to Order Reference Number in the itch spec
enum class OrderID : uint64_t {};

enum class Type : bool { buy, sell };

enum class Quantity : uint32_t {};

enum class Price : uint32_t {};
struct OrderAdd {
    OrderID oid {};
    Quantity qty {};
    Price price {};
    InstrumentID locate {};
    Type type {};
};

// Executes order (reduces the quantity by executed count)
struct OrderExecute {
    OrderID oid {};
    InstrumentID locate {};
    Quantity executed_qty {};
};

// Action that cancels a certain amount of shares
struct OrderCancel {
    OrderID oid {};
    InstrumentID locate {};
    Quantity qty {};
};

// Deletes the order from the book (reduces the order's quantity to 0)
struct OrderDelete {
    OrderID oid {};
    InstrumentID locate {};
};

// Replaces an order by reducing the current order's quantity to 0 and then
// creating a new order with a new id
struct OrderReplace {
    OrderID oid {};
    OrderID new_oid {};
    Quantity new_qty {};
    Price new_price {};
    InstrumentID locate {};
};

// Action can be one of the defined orders
using Action = std::variant<OrderAdd, OrderExecute, OrderCancel, OrderDelete, OrderReplace>;

namespace detail {

template <size_t N> constexpr uint64_t parse_be(std::span<const std::byte> data, size_t offset) {
    static_assert(N > 0 && N <= 8);
    uint64_t value {};
    for (size_t i {}; i < N; ++i) {
        value = (value << 8) | std::to_integer<uint64_t>(data[i + offset]);
    }
    return value;
}

// Function declarations for internal parse-handling:
auto parse_add(std::span<const std::byte> msg) -> Action;
auto parse_execute(std::span<const std::byte> msg) -> Action;
auto parse_cancel(std::span<const std::byte> msg) -> Action;
auto parse_delete(std::span<const std::byte> msg) -> Action;
auto parse_replace(std::span<const std::byte> msg) -> Action;

using ParsingFunction = Action (*)(std::span<const std::byte> msg);

inline constexpr std::array<ParsingFunction, 256> parse_lut = [] consteval {
    std::array<ParsingFunction, 256> arr {};
    arr['A'] = parse_add;
    arr['F'] = parse_add;
    arr['E'] = parse_execute;
    arr['C'] = parse_execute;
    arr['X'] = parse_cancel;
    arr['D'] = parse_delete;
    arr['U'] = parse_replace;

    return arr;
}();

} // namespace detail
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
    [[nodiscard]] std::expected<Action, ParseError> parse(std::span<const std::byte> msg) noexcept;

private:
    // Count of messages that have been parsed so far
    size_t _count {};
    std::array<uint64_t, 256> _by_type {};
};

} // namespace parser
