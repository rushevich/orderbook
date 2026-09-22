#pragma once
#include "orderbook_core/Types.hpp"

#include <cassert>
#include <unordered_map>

namespace rushevich::book {
using namespace containers;
class OrderBook {
public:
    // This is less than ideal, but for now can serve as a naive implementation. Ultimately, the
    // best design might be a ring-buffer that can handle price volatility
    using SideMap = std::unordered_map<Price, PriceLevel>;

    OrderBook(size_t priceTickCount = DEFAULT_TICK_COUNT)
        : _buySide(priceTickCount),
          _sellSide(priceTickCount) {}

    // All OrderBook mutation functions return a boolean indicating whether or not the order was
    // added successfully. This is tentative while I figure out a better way to handle exceptional
    // circumstances.

    // allocates a new order within the system-wide ’_orderPool’ and ’_oidMetaMap’ structures and
    // then calls ’_linkToPrice()’
    // bool _addOrder(const OrderAction& action) {
    // const auto& [oid, _1_, price, qty, loc, side, _2_] = action;
    // const auto bid = side == Side::buy;
    // auto& sideMap = _sideMap(bid);

    // // Default allocate a new order
    // auto* orderMeta = _orderPool.allocate();
    // if (orderMeta == nullptr) {
    //     return false; // indicates OOM
    // }

    // // Initializing the fields
    // orderMeta->oid = oid.value();
    // orderMeta->qty = qty.value();
    // orderMeta->priceTick = price.value();
    // orderMeta->locateIdx = loc.value();
    // orderMeta->isBid = bid;
    // _linkToPrice(orderMeta,
    //              sideMap[orderMeta->priceTick]); // Handles priceLevel.volume increment
    // _oidMetaMap[oid.value()] = orderMeta;
    //     return true;
    // }

    // Subtracts the needed quantity from the order referred to by ’action’ and calls
    // ’_unlinkOrder()’ and cleans up the associated data structures if quantity reaches 0
    // bool _execOrCancelOrder(const OrderAction& action) {
    // const auto& [oid, _1_, _2_, qty, _3_, side, _4_] = action;
    // auto orderMetaIterator = _oidMetaMap.find(oid.value());
    // if (orderMetaIterator == _oidMetaMap.end()) {
    //     return false;
    // }

    // auto* orderMeta = orderMetaIterator->second;
    // auto& sideMap = _sideMap(orderMeta->isBid);

    // // Case: we have to remove the order entirely from the book:
    // if ((orderMeta->qty - qty.value()) == 0) {
    //     _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
    //     _oidMetaMap.erase(orderMetaIterator);
    //     _orderPool.free(orderMeta);
    //     return true;
    // }
    // orderMeta->qty -= qty.value(); // We have to make sure to do this anyway
    // sideMap[orderMeta->priceTick].volume -= qty.value();
    //     return true;
    // }

    // Simply calls ’_unlinkOrder()’ then cleans up the associated data structures
    // bool _deleteOrder(const OrderAction& action) {
    // const auto& [oid, _1_, _2_, _3_, _4_, side, _5_] = action;
    // auto orderMetaIterator = _oidMetaMap.find(oid.value());
    // if (orderMetaIterator == _oidMetaMap.end()) {
    //     return false;
    // }

    // auto* orderMeta = orderMetaIterator->second;
    // auto& sideMap = _sideMap(orderMeta->isBid);

    // _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
    // _oidMetaMap.erase(orderMetaIterator);
    // _orderPool.free(orderMeta);
    // return true;
    // }

    // First calls ’_unlinkOrder()’ then changes the internal metadata related to the order, and
    // calls ’_linkToPrice()’ with the new price level
    // bool _replaceOrder(const OrderAction& action) {
    // const auto& [oid, repl_oid, price, qty, _1_, side, _2_] = action;
    // auto orderMetaIterator = _oidMetaMap.find(oid.value());
    // if (orderMetaIterator == _oidMetaMap.end()) {
    //     return false;
    // }

    // auto* orderMeta = orderMetaIterator->second;
    // auto& sideMap = _sideMap(orderMeta->isBid);

    // _oidMetaMap.erase(orderMetaIterator);
    // _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
    // orderMeta->oid = repl_oid.value();
    // orderMeta->priceTick = price.value();
    // orderMeta->qty = qty.value();
    // _linkToPrice(orderMeta, sideMap[orderMeta->priceTick]);
    // _oidMetaMap[orderMeta->oid] = orderMeta; // Create a new map entry
    // return true;
    // }

    // Helper for deducing the book side inline
    __attribute__((always_inline)) auto& sideMap(Side side) {
        return side == Side::buy ? _buySide : _sellSide;
    }

    OrderBook(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = delete;

    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook& operator=(OrderBook&&) = delete;

    ~OrderBook() = default;

    // Slashes the links between the order and the other order’s in its pricelevel.
    // Also handles reduction in quantity from the corresponding PriceLevel’s volume field.
    void unlink(OrderMeta* orderMeta);

    // Links the order referred to by ’orderMeta’ into ’priceLevel’ properly.
    // Also handles increase in quantity to the corresponding PriceLevel’s volume field.
    void link(OrderMeta* orderMeta);

    void reduce(OrderMeta* orderMeta, Quantity qty);

private:
    static constexpr auto DEFAULT_TICK_COUNT = 6000UZ;

    SideMap _buySide;
    SideMap _sellSide;
};

} // namespace rushevich::book
