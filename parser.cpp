#include "parser.hpp"

#include <format>
#include <iostream>
#include <ranges>

namespace parser {
// struct OrderAdd {
//     OrderID oid {};
//     Quantity qty {};
//     Price price {};
//     InstrumentID locate {};
//     Type type {};
// };
namespace detail {
Action parse_add(std::span<const std::byte> msg) {
    return OrderAdd { .oid = OrderID { parse_be<8>(msg, 11) },
                      .qty = Quantity { parse_be<4>(msg, 20) },
                      .price = Price { parse_be<4>(msg, 32) },
                      .locate = InstrumentID { parse_be<2>(msg, 1) },
                      .type = static_cast<char>(msg[19]) == 'B' ? Type::buy : Type::sell };
}

// struct OrderExecute {
//     OrderID oid {};
//     Quantity executed_qty {};
//     InstrumentID locate {};
// };
Action parse_execute(std::span<const std::byte> msg) {
    return OrderExecute { .oid = OrderID { parse_be<8>(msg, 11) },
                          .executed_qty = Quantity { parse_be<4>(msg, 19) },
                          .locate = InstrumentID { parse_be<2>(msg, 1) } };
}
// struct OrderCancel {
//     OrderID oid {};
//     Quantity qty {};
//     InstrumentID locate {};
// };
Action parse_cancel(std::span<const std::byte> msg) {
    return OrderCancel { .oid = OrderID { parse_be<8>(msg, 11) },
                         .qty = Quantity { parse_be<4>(msg, 19) },
                         .locate = InstrumentID { parse_be<2>(msg, 1) } };
}

// struct OrderDelete {
//     OrderID oid {};
//     InstrumentID locate {};
// };
Action parse_delete(std::span<const std::byte> msg) {
    return OrderDelete { .oid = OrderID { parse_be<8>(msg, 11) },
                         .locate = InstrumentID { parse_be<2>(msg, 1) } };
}

// struct OrderReplace {
//     OrderID oid {};
//     OrderID new_oid {};
//     Quantity new_qty {};
//     Price new_price {};
//     InstrumentID locate {};
// };
Action parse_replace(std::span<const std::byte> msg) {
    return OrderReplace { .oid = OrderID { parse_be<8>(msg, 11) },
                          .new_oid = OrderID { parse_be<8>(msg, 19) },
                          .new_qty = Quantity { parse_be<4>(msg, 27) },
                          .new_price = Price { parse_be<4>(msg, 31) },
                          .locate = InstrumentID { parse_be<2>(msg, 1) } };
}
} // namespace detail
[[nodiscard]] std::expected<Action, ParseError>
Parser::parse(std::span<const std::byte> msg) noexcept {
    if (msg.empty()) {
        return std::unexpected { ParseError::empty };
    }
    const auto identifier = detail::parse_be<1>(msg, 0);
    const auto func = detail::parse_lut[identifier];
    if (func == nullptr) {
        return std::unexpected { ParseError::unknown_type };
    }
    _count++;
    _by_type[identifier]++;
    return func(msg);
}

void Parser::dump_stats(std::ostream& out) {
    std::println(out, "Total count: {}", _count);
    for (size_t idx {}; idx < _by_type.size(); ++idx) {
        const auto count = _by_type[idx];
        if (count == 0) {
            continue;
        }
        switch (idx) {
        case 'F':
            out << std::format("Order Adds (MPID): {}", count) << '\n';
            break;
        case 'A':
            out << std::format("Order Adds: {}", count) << '\n';
            break;
        case 'C':
            out << std::format("Order Executes (w/ price): {}", count) << '\n';
            break;
        case 'E':
            out << std::format("Order Executes: {}", count) << '\n';
            break;
        case 'X':
            out << std::format("Order Cancels: {}", count) << '\n';
            break;
        case 'D':
            out << std::format("Order Deletes: {}", count) << '\n';
            break;
        case 'U':
            out << std::format("Order Replaces: {}", count) << '\n';
            break;
        default:
            out << std::format("Unregistered parsing operation??? {}", count) << '\n';
        }
    }
}
} // namespace parser
