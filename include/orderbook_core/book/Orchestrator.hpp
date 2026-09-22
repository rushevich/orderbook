#pragma once
#include "orderbook_core/Actions.hpp"
#include "orderbook_core/Types.hpp"
#include "orderbook_core/book/OrderBook.hpp"
#include "orderbook_core/containers/ObjectPool.hpp"

#include <vector>

namespace rushevich::book {
// ’Orchestrates’ OrderBook objects -- owns an array of OrderBooks and the object pools and
// global orderId map that they use
namespace detail {
inline constexpr auto NUM_SYMBOLS = 10'000UZ;
inline constexpr auto ORDER_POOL_SIZE = 10'000'000UZ;
} // namespace detail
class Orchestrator {
public:
    Orchestrator(size_t instrumentCount = detail::NUM_SYMBOLS)
        : _oidMetaMap(detail::ORDER_POOL_SIZE),
          _books(instrumentCount) {}

    // Maps OrderID to the index within the object pool. Chose to use indices instead of
    // pointers for size reduction purposes. To get the data, simply operator[] with the value
    // It is static because this map is shared across all instruments
    using OidMetaMap = std::unordered_map<OrderID, OrderMeta*>;

    bool consume(const OrderAction& action);

    Orchestrator(const Orchestrator&) = delete;
    Orchestrator(Orchestrator&&) = delete;
    Orchestrator& operator=(const Orchestrator&) = delete;
    Orchestrator& operator=(Orchestrator&&) = delete;

    ~Orchestrator() = default;

private:
    ObjectPool<OrderMeta, detail::ORDER_POOL_SIZE> _pool;
    OidMetaMap _oidMetaMap;
    std::vector<OrderBook> _books;

    // Dispatched-to functions
    bool _onAdd(const OrderAction& action);
    bool _onExecute(const OrderAction& action);
    bool _onCancel(const OrderAction& action);
    bool _onDelete(const OrderAction& action);
    bool _onReplace(const OrderAction& action);

    OrderBook& _bookFor(InstrumentID loc);
};
} // namespace rushevich::book
