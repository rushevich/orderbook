// There are many other important bookkeeping messages that are
// important to track, but for now we will only worry about
// the trivial ones like add order and modify order

// There are also trade messages, however these are related
// to non-displayable messages. For the time being, and for
// simplicity's sake, we may also simply ignore them.
// I think this is ok, since they will usually just boil down
// to deletions or more nuanced changes in a level.

// I've opted to create a stateful parser that will allow me to
// to capture useful metrics related to trading volume, and also
// avoid smelly free-standing functions

// Since messages are obviously not sent one at at time, we will
// wrap a single message parser in a parser that parses full
// files from the wire
#include <cstdint>
#include <flat_map>
#include <string_view>
#include <vector>
#include <cassert>

namespace parser {

// This corresponds to Stock Locate in the itch spec
enum class InstrumentID : uint16_t {};
// This corresponds to Order Reference Number in the itch spec
enum class OrderID : uint64_t {};

enum class Type : bool { buy, sell };

enum class Quantity : uint32_t {};

enum class Price : uint32_t {};

// the parser will consume stuff from the messages and output corresponding actions upon the
// orderbook

// order reference numbers are unique per day, which means we dont have to track duplicates

// i believe also the performant way to do this will be to explicitly consume every byte in an order
// instead of deferring to a standard library facility

// it will also be useful to write the time of receiving the order in a log. we can track the id and
// time of receiving the order then later on, when we seek to get history of the orderbook during
// runtime, we can dump the orders using reflection and the id from the logbook (we will keep our
// orders around to form a graveyard)

template <size_t B> constexpr uint64_t parse_uint(std::string_view str);

// 8-byte specialization (64-bit)
template <> constexpr uint64_t parse_uint<8>(std::string_view s) {
    return (static_cast<uint64_t>(static_cast<unsigned char>(s[0])) << 56) |
           (static_cast<uint64_t>(static_cast<unsigned char>(s[1])) << 48) |
           (static_cast<uint64_t>(static_cast<unsigned char>(s[2])) << 40) |
           (static_cast<uint64_t>(static_cast<unsigned char>(s[3])) << 32) |
           (static_cast<uint64_t>(static_cast<unsigned char>(s[4])) << 24) |
           (static_cast<uint64_t>(static_cast<unsigned char>(s[5])) << 16) |
           (static_cast<uint64_t>(static_cast<unsigned char>(s[6])) << 8) |
           (static_cast<uint64_t>(static_cast<unsigned char>(s[7])));
}

// 4-byte specialization (32-bit)
template <> constexpr uint64_t parse_uint<4>(std::string_view s) {
    return (static_cast<uint32_t>(static_cast<unsigned char>(s[0])) << 24) |
           (static_cast<uint32_t>(static_cast<unsigned char>(s[1])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(s[2])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(s[3])));
}

// 2-byte specialization (16-bit)
template <> constexpr uint64_t parse_uint<2>(std::string_view s) {
    return (static_cast<uint16_t>(static_cast<unsigned char>(s[0])) << 8) |
           (static_cast<uint16_t>(static_cast<unsigned char>(s[1])));
}

// 1-byte specialization (8-bit)
template <> constexpr uint64_t parse_uint<1>(std::string_view s) {
    return static_cast<uint8_t>(static_cast<unsigned char>(s[0]));
}

static const std::flat_map<char, Type> type_map { { 'B', Type::buy }, { 'S', Type::sell } };

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
};

// Deletes the order from the book (reduces the order's quantity to 0)
struct OrderDelete {
    OrderID oid {};
    InstrumentID locate {};
};

// Replaces an order by reducing the current order's quantity to 0 and then creating a new order
// with a new id
struct OrderReplace {
    OrderID oid {};
    InstrumentID locate {};
};

// Action can be one of the defined orders
// std::monostate for a default value semantically equivalent to nullptr
using Action =
    std::variant<std::monostate, OrderAdd, OrderExecute, OrderCancel, OrderDelete, OrderReplace>;

// Function declarations for internal parse-handling:
auto parse_add(std::span<std::byte> msg) -> Action;
auto parse_execute(std::span<std::byte> msg) -> Action;
auto parse_cancel(std::span<std::byte> msg) -> Action;
auto parse_delete(std::span<std::byte> msg) -> Action;
auto parse_replace(std::span<std::byte> msg) -> Action;

using ParsingFunction = std::function<Action(std::span<std::byte> msg)>;

// While the out parameter seems like a poor choice, it permits bypassing branches and instead using
// a lookup table
// we get the function, then call using the correct offsets / bounds for the string_view
// e.g. parse_map('A')({message.begin() + current_start, message.begin() + current_start +
// parse_add_offset});
static const std::unordered_map<char, ParsingFunction> parse_map {
    { 'A', parse_add },    { 'F', parse_add },    { 'E', parse_execute }, { 'C', parse_execute },
    { 'X', parse_cancel }, { 'D', parse_delete }, { 'U', parse_add }
};

constexpr bool valid_identifier(char c) {
    return c == 'A' || c == 'F' || c == 'E' || c == 'C' || c == 'X' || c == 'D' || c == 'U';
}

class Parser {
public:
    // Parses the arbitrary size message and outputs a collection of actions to perform
    // TODO: ensure that we can elide the move / copy
    static Action parse(std::span<std::byte> msg);

private:
    // Count of messages that have been parsed so far
    size_t _count {};
};
// This operates on the assumption that msg consists of one binary message. we will depend on the
// reader to maintain this
    // Later on we can use contracts
 Action Parser::parse(std::span<std::byte> msg) {
    assert(msg.size() > 0 && valid_identifier(static_cast<char>(msg[0])));

    const auto parsing_function = parse_map.at(static_cast<char>(msg[0]));
    return parsing_function(msg); // the parsing functions will encpasulate all the offset logic so we do not crowd namespaces
    }

} // namespace parser

// The idea:
// auto actions = parser.parse(message) (arbitrary size message)
// std::visit(actions);
//
