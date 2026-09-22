#pragma once
#include "orderbook_core/Actions.hpp"
#include "orderbook_core/Types.hpp"
#include "orderbook_core/containers/ObjectPool.hpp"

#include <unordered_map>

namespace rushevich::book {
using namespace containers;
class OrderBook {
public:
    // Pessimistic defaults
    static constexpr auto DEFAULT_TICK_COUNT = 6000UZ;
    static constexpr auto DEFAULT_ORDER_COUNT = 10'000'000UZ;

    OrderBook(ObjectPool<OrderMeta, DEFAULT_ORDER_COUNT>& orderPool,
              std::unordered_map<uint64_t, OrderMeta*> oidMetaMap)
        : _orderPool { orderPool },
          _oidMetaMap { oidMetaMap },
          _buySide(DEFAULT_TICK_COUNT),
          _sellSide(DEFAULT_TICK_COUNT) {}

private:
    // Storage for all the orders within the system. Passed in by the orderbook orchestrator on
    // construction.
    ObjectPool<OrderMeta, DEFAULT_ORDER_COUNT>& _orderPool;
    // Maps OrderID to the index within the object pool. Chose to use indices instead of pointers
    // for size reduction purposes. To get the data, simply operator[] with the value
    // It is static because this map is shared across all instruments
    using OidMetaMap = std::unordered_map<uint64_t, OrderMeta*>;
    OidMetaMap& _oidMetaMap;

    // This is less than ideal, but for now can serve as a naive implementation. Ultimately, the
    // best design might be a ring-buffer that can handle price volatility
    using SideMap = std::unordered_map<uint32_t, PriceLevel>;
    SideMap _buySide;
    SideMap _sellSide;

public:
    // allocates a new order within the system-wide ’_orderPool’ and ’_oidMetaMap’ structures and
    // then calls ’_linkToPrice()’
    void _addOrder(const OrderAction& action);

    // Subtracts the needed quantity from the order referred to by ’action’ and calls
    // ’_unlinkOrder()’ and cleans up the associated data structures if quantity reaches 0
    void _execOrCancelOrder(const OrderAction& action);

    // Simply calls ’_unlinkOrder()’ then cleans up the associated data structures
    void _deleteOrder(const OrderAction& action);

    // First calls ’_unlinkOrder()’ then changes the internal metadata related to the order, and
    // calls ’_linkToPrice()’ with the new price level
    void _replaceOrder(const OrderAction& action);

    // Helper for deducing the right side inline
    __attribute__((always_inline)) auto& _sideMap(bool isBid) {
        return isBid ? _buySide : _sellSide;
    }

private:
    // Slashes the links between the order and the other order’s in its pricelevel.
    // Also handles reduction in quantity from the corresponding PriceLevel’s volume field.
    void _unlinkOrder(OrderMeta* orderMeta, PriceLevel& priceLevel);

    // Links the order referred to by ’orderMeta’ into ’priceLevel’ properly.
    // Also handles increase in quantity to the corresponding PriceLevel’s volume field.
    void _linkToPrice(OrderMeta* orderMeta, PriceLevel& priceLevel);
};

} // namespace rushevich::book
