#pragma once
#include "orderbook_core/Actions.hpp"
#include "orderbook_core/Types.hpp"
#include "orderbook_core/containers/ObjectPool.hpp"

#include <unordered_map>

namespace rushevich::book {
using namespace containers;
namespace detail {
static constexpr auto DEFAULT_TICK_COUNT = 6000UZ;

}; // namespace detail
template <size_t PoolSize> class OrderBook {
public:
    // Maps OrderID to the index within the object pool. Chose to use indices instead of pointers
    // for size reduction purposes. To get the data, simply operator[] with the value
    // It is static because this map is shared across all instruments
    using OidMetaMap = std::unordered_map<uint64_t, OrderMeta*>;

    // This is less than ideal, but for now can serve as a naive implementation. Ultimately, the
    // best design might be a ring-buffer that can handle price volatility
    using SideMap = std::unordered_map<uint32_t, PriceLevel>;

    OrderBook(ObjectPool<OrderMeta, PoolSize>& orderPool,
              std::unordered_map<uint64_t, OrderMeta*> oidMetaMap)
        : _orderPool { orderPool },
          _oidMetaMap { oidMetaMap },
          _buySide(detail::DEFAULT_TICK_COUNT),
          _sellSide(detail::DEFAULT_TICK_COUNT) {}

    // All OrderBook mutation functions return a boolean indicating whether or not the order was
    // added successfully. This is tentative while I figure out a better way to handle exceptional
    // circumstances.

    // allocates a new order within the system-wide ’_orderPool’ and ’_oidMetaMap’ structures and
    // then calls ’_linkToPrice()’
    bool _addOrder(const OrderAction& action) {
        const auto& [oid, _1_, price, qty, loc, side, _2_] = action;
        const auto bid = side == Side::buy;
        auto& sideMap = _sideMap(bid);

        // Default allocate a new order
        auto* orderMeta = _orderPool.allocate();
        if (orderMeta == nullptr) {
            return false; // indicates OOM
        }

        // Initializing the fields
        orderMeta->oid = oid.value();
        orderMeta->qty = qty.value();
        orderMeta->priceTick = price.value();
        orderMeta->locateIdx = loc.value();
        orderMeta->isBid = bid;
        _linkToPrice(orderMeta,
                     sideMap[orderMeta->priceTick]); // Handles priceLevel.volume increment
        _oidMetaMap[oid.value()] = orderMeta;
        return true;
    }

    // Subtracts the needed quantity from the order referred to by ’action’ and calls
    // ’_unlinkOrder()’ and cleans up the associated data structures if quantity reaches 0
    bool _execOrCancelOrder(const OrderAction& action) {
        const auto& [oid, _1_, _2_, qty, _3_, side, _4_] = action;
        auto orderMetaIterator = _oidMetaMap.find(oid.value());
        if (orderMetaIterator == _oidMetaMap.end()) {
            return false;
        }

        auto* orderMeta = orderMetaIterator->second;
        auto& sideMap = _sideMap(orderMeta->isBid);

        // Case: we have to remove the order entirely from the book:
        if ((orderMeta->qty - qty.value()) == 0) {
            _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
            _oidMetaMap.erase(orderMetaIterator);
            _orderPool.free(orderMeta);
            return true;
        }
        orderMeta->qty -= qty.value(); // We have to make sure to do this anyway
        sideMap[orderMeta->priceTick].volume -= qty.value();
        return true;
    }

    // Simply calls ’_unlinkOrder()’ then cleans up the associated data structures
    bool _deleteOrder(const OrderAction& action) {
        const auto& [oid, _1_, _2_, _3_, _4_, side, _5_] = action;
        auto orderMetaIterator = _oidMetaMap.find(oid.value());
        if (orderMetaIterator == _oidMetaMap.end()) {
            return false;
        }

        auto* orderMeta = orderMetaIterator->second;
        auto& sideMap = _sideMap(orderMeta->isBid);

        _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
        _oidMetaMap.erase(orderMetaIterator);
        _orderPool.free(orderMeta);
        return true;
    }

    // First calls ’_unlinkOrder()’ then changes the internal metadata related to the order, and
    // calls ’_linkToPrice()’ with the new price level
    bool _replaceOrder(const OrderAction& action) {
        const auto& [oid, repl_oid, price, qty, _1_, side, _2_] = action;
        auto orderMetaIterator = _oidMetaMap.find(oid.value());
        if (orderMetaIterator == _oidMetaMap.end()) {
            return false;
        }

        auto* orderMeta = orderMetaIterator->second;
        auto& sideMap = _sideMap(orderMeta->isBid);

        _oidMetaMap.erase(orderMetaIterator);
        _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
        orderMeta->oid = repl_oid.value();
        orderMeta->priceTick = price.value();
        orderMeta->qty = qty.value();
        _linkToPrice(orderMeta, sideMap[orderMeta->priceTick]);
        _oidMetaMap[orderMeta->oid] = orderMeta; // Create a new map entry
    }

    // Helper for deducing the right side inline
    __attribute__((always_inline)) auto& _sideMap(bool isBid) {
        return isBid ? _buySide : _sellSide;
    }

    OrderBook(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = delete;

    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook& operator=(OrderBook&&) = delete;

    ~OrderBook() = default;

private:
    // Storage for all the orders within the system. Passed in by the orderbook orchestrator on
    // construction.
    ObjectPool<OrderMeta, PoolSize>& _orderPool;
    OidMetaMap& _oidMetaMap;

    SideMap _buySide;
    SideMap _sellSide;

    // Slashes the links between the order and the other order’s in its pricelevel.
    // Also handles reduction in quantity from the corresponding PriceLevel’s volume field.
    void _unlinkOrder(OrderMeta* orderMeta, PriceLevel& priceLevel) {
        priceLevel.volume -= orderMeta->qty;
        // If it is head, we have to bump head up
        if (orderMeta == priceLevel.headHandle) {
            priceLevel.headHandle = orderMeta->nextHandle;
        } else { // This means it’s within the list or a tail, in which case
            assert(orderMeta->prevHandle
                   != IntrusiveNode<OrderMeta*>::NULL_HANDLE); // redundant, tbh
            orderMeta->prevHandle->nextHandle = orderMeta->nextHandle;
        }
        // If it is tail, we have to move tail back one
        if (orderMeta == priceLevel.tailHandle) { // Also means that next should be null
            priceLevel.tailHandle = orderMeta->prevHandle;
        } else {
            assert(orderMeta->nextHandle != IntrusiveNode<OrderMeta*>::NULL_HANDLE);
            orderMeta->nextHandle->prevHandle = orderMeta->prevHandle;
        }
        orderMeta->prevHandle = IntrusiveNode<OrderMeta*>::NULL_HANDLE;
        orderMeta->nextHandle = IntrusiveNode<OrderMeta*>::NULL_HANDLE;
    }

    // Links the order referred to by ’orderMeta’ into ’priceLevel’ properly.
    // Also handles increase in quantity to the corresponding PriceLevel’s volume field.
    void _linkToPrice(OrderMeta* orderMeta, PriceLevel& priceLevel) {
        priceLevel.volume += orderMeta->qty;
        // Case: price doesn’t exist yet
        if (auto& [head, tail, _] = priceLevel; head == IntrusiveNode<OrderMeta*>::NULL_HANDLE
                                                && tail == IntrusiveNode<OrderMeta*>::NULL_HANDLE) {
            // The added order is the only one in this price level -> it’s the head AND the tail
            head = orderMeta;
            tail = orderMeta;
        }
        // Case: price level aready exists
        else {
            assert(tail != nullptr);
            tail->nextHandle = orderMeta;
            orderMeta->prevHandle = tail;
            tail = orderMeta;
        }
    }
};

// Deduction guide
template <size_t PoolSize>
OrderBook(ObjectPool<OrderMeta, PoolSize>& pool, std::unordered_map<uint64_t, OrderMeta*>& map)
    -> OrderBook<PoolSize>;

} // namespace rushevich::book
