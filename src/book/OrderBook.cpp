#include "orderbook_core/book/OrderBook.hpp"

#include "orderbook_core/Actions.hpp"
#include "orderbook_core/Types.hpp"
#include "orderbook_core/containers/ObjectPool.hpp"

namespace rushevich::book {
// struct OrderMeta : public container::IntrusiveNode<> {
//     uint64_t oid;
//     uint32_t qty;
//     uint32_t priceTick;
//     uint16_t locateIdx; // Index for orderbook array.
//     uint8_t isBid;
// };

using containers::IntrusiveNode;
void OrderBook::_addOrder(const OrderAction& action) {
    const auto& [oid, _1_, price, qty, loc, side, _2_] = action;
    const auto bid = side == Side::buy;
    auto& sideMap = _sideMap(bid);

    // Default allocate a new order
    auto* orderMeta = _orderPool.allocate();
    if (orderMeta == nullptr) {
        // OOM: Do some logging feature then abort the program
    }

    // Initializing the fields
    orderMeta->oid = oid.value();
    orderMeta->qty = qty.value();
    orderMeta->priceTick = price.value();
    orderMeta->locateIdx = loc.value();
    orderMeta->isBid = bid;
    _linkToPrice(orderMeta, sideMap[orderMeta->priceTick]); // Handles priceLevel.volume increment
    _oidMetaMap[oid.value()] = orderMeta;
}

void OrderBook::_execOrCancelOrder(const OrderAction& action) {
    const auto& [oid, _1_, _2_, qty, _3_, side, _4_] = action;
    auto orderMetaIterator = _oidMetaMap.find(oid.value());
    assert(orderMetaIterator != _oidMetaMap.end());

    auto* orderMeta = orderMetaIterator->second;
    assert(orderMeta != nullptr);
    auto& sideMap = _sideMap(orderMeta->isBid);

    // Case: we have to remove the order entirely from the book:
    if ((orderMeta->qty - qty.value()) == 0) {
        _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
        _oidMetaMap.erase(orderMetaIterator);
        _orderPool.free(orderMeta);
        return;
    }
    orderMeta->qty -= qty.value(); // We have to make sure to do this anyway
    sideMap[orderMeta->priceTick].volume -= qty.value();
}

void OrderBook::_deleteOrder(const OrderAction& action) {
    const auto& [oid, _1_, _2_, _3_, _4_, side, _5_] = action;
    auto orderMetaIterator = _oidMetaMap.find(oid.value());
    assert(orderMetaIterator != _oidMetaMap.end());

    auto* orderMeta = orderMetaIterator->second;
    assert(orderMeta != nullptr);
    auto& sideMap = _sideMap(orderMeta->isBid);

    _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
    _oidMetaMap.erase(orderMetaIterator);
    _orderPool.free(orderMeta);
}

void OrderBook::_replaceOrder(const OrderAction& action) {
    const auto& [oid, repl_oid, price, qty, _1_, side, _2_] = action;
    auto orderMetaIterator = _oidMetaMap.find(oid.value());
    assert(orderMetaIterator != _oidMetaMap.end());

    auto* orderMeta = orderMetaIterator->second;
    assert(orderMeta != nullptr);
    auto& sideMap = _sideMap(orderMeta->isBid);

    _oidMetaMap.erase(orderMetaIterator);
    _unlinkOrder(orderMeta, sideMap[orderMeta->priceTick]);
    orderMeta->oid = repl_oid.value();
    orderMeta->priceTick = price.value();
    orderMeta->qty = qty.value();
    _linkToPrice(orderMeta, sideMap[orderMeta->priceTick]);
    _oidMetaMap[orderMeta->oid] = orderMeta; // Create a new map entry
}

void OrderBook::_unlinkOrder(OrderMeta* orderMeta, PriceLevel& priceLevel) {
    priceLevel.volume -= orderMeta->qty;
    // If it is head, we have to bump head up
    if (orderMeta == priceLevel.headHandle) {
        priceLevel.headHandle = orderMeta->nextHandle;
    } else { // This means it’s within the list or a tail, in which case
        assert(orderMeta->prevHandle != IntrusiveNode<OrderMeta*>::NULL_HANDLE);
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

void OrderBook::_linkToPrice(OrderMeta* orderMeta, PriceLevel& priceLevel) {
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
} // namespace rushevich::book
