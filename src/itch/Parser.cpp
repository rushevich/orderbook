#include "orderbook_core/itch/Parser.hpp"

#include "orderbook_core/Actions.hpp"
#include "orderbook_core/Types.hpp"
#include "orderbook_core/util/Endian.hpp"

#include <ranges>
#include <span>

namespace rushevich::parser {

OrderAction parse_add(std::span<const std::byte> msg) {
    return { .oid = OrderID { util::parse_be<8>(msg, 11) },
             .price = Price { util::parse_be<4>(msg, 32) },
             .qty = Quantity { util::parse_be<4>(msg, 20) },
             .locate = InstrumentID { util::parse_be<2>(msg, 1) },
             .type = static_cast<char>(msg[19]) == 'B' ? Type::buy : Type::sell };
}

OrderAction parse_execute(std::span<const std::byte> msg) {
    return { .oid = OrderID { util::parse_be<8>(msg, 11) },
             .qty = Quantity { util::parse_be<4>(msg, 19) },
             .locate = InstrumentID { util::parse_be<2>(msg, 1) } };
}

OrderAction parse_cancel(std::span<const std::byte> msg) {
    return { .oid = OrderID { util::parse_be<8>(msg, 11) },
             .qty = Quantity { util::parse_be<4>(msg, 19) },
             .locate = InstrumentID { util::parse_be<2>(msg, 1) } };
}

OrderAction parse_delete(std::span<const std::byte> msg) {
    return { .oid = OrderID { util::parse_be<8>(msg, 11) },
             .locate = InstrumentID { util::parse_be<2>(msg, 1) } };
}

OrderAction parse_replace(std::span<const std::byte> msg) {
    return OrderAction { .oid = OrderID { util::parse_be<8>(msg, 11) },
                         .repl_oid = OrderID { util::parse_be<8>(msg, 19) },
                         .price = Price { util::parse_be<4>(msg, 31) },
                         .qty = Quantity { util::parse_be<4>(msg, 27) },
                         .locate = InstrumentID { util::parse_be<2>(msg, 1) } };
}

OrderAction parse_do_nothing([[maybe_unused]] std::span<const std::byte>) { return {}; }

[[nodiscard]] std::expected<OrderAction, ParseError>
Parser::operator()(std::span<const std::byte> msg) noexcept {
    if (msg.empty()) {
        return std::unexpected { ParseError::empty };
    }
    const auto identifier = util::parse_be<1>(msg, 0);
    const auto func = parse_lut[identifier];
    if (func == nullptr) {
        return std::unexpected { ParseError::unknown_type };
    }
    _count++;
    _by_type[identifier]++;
    return func(msg);
}

void Parser::dump_stats(std::ostream& out) {
    std::println(out, "Total count: {}", _count);
    for (const auto& [idx, count] : std::views::enumerate(_by_type)) {
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
            out << std::format("Unhandled parsing operation: {}", count) << '\n';
        }
    }
}
} // namespace rushevich::parser
