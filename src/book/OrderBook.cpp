#include "orderbook_core/book/OrderBook.hpp"

#include "orderbook_core/Actions.hpp"
#include "orderbook_core/Types.hpp"

namespace rushevich::book {
// struct OrderMeta : public container::IntrusiveNode<> {
//     uint64_t oid;
//     uint32_t qty;
//     uint32_t priceTick;
//     uint16_t locateIdx; // Index for orderbook array.
//     uint8_t isBid;
// };

// Orders to handle
using containers::IntrusiveNode;
void OrderBook::_addOrder(const OrderAction& action) {
    const auto& [oid, _1_, price, qty, loc, side, _2_] = action;
    auto& sideMap = _sideMap(side);

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
    orderMeta->isBid = true;
    _linkToPrice(orderMeta, sideMap[price.value()]);
    sideMap[price.value()].volume += orderMeta->qty;
    _oidMetaMap[oid.value()] = orderMeta;
}

void OrderBook::_execOrCancelOrder(const OrderAction& action) {
    const auto& [oid, _1_, _2_, qty, _3_, side, _4_] = action;
    auto orderMetaIterator = _oidMetaMap.find(oid.value());
    assert(orderMetaIterator != _oidMetaMap.end());
    auto& orderMeta = orderMetaIterator->second;
    auto& sideMap = side == Side::buy ? _buySide : _sellSide;
    sideMap[orderMeta->priceTick].volume -= qty.value();
    // Case: we have to remove the order entirely from the book:
    if ((orderMeta->qty -= qty.value()) == 0) {
        _unlinkOrder(orderMeta, sideMap); // Handles freeing too
        _oidMetaMap.erase(orderMetaIterator);
        _orderPool.free(orderMeta);
    }
}

void OrderBook::_deleteOrder(const OrderAction& action) {
    const auto& [oid, _1_, _2_, _3_, _4_, side, _5_] = action;
    auto orderMetaIterator = _oidMetaMap.find(oid.value());
    auto& sideMap = _sideMap(side);
    auto* orderMeta = orderMetaIterator->second; // Copy the pool ptr to defend against invalidation
    assert(orderMetaIterator != _oidMetaMap.end() && orderMeta != nullptr);
    sideMap[orderMeta->priceTick].volume -= orderMeta->qty;
    _unlinkOrder(orderMeta, sideMap);
    _oidMetaMap.erase(orderMetaIterator);
    _orderPool.free(orderMeta);
}

void OrderBook::_replaceOrder(const OrderAction& action) {
    const auto& [oid, repl_oid, price, qty, _1_, side, _2_] = action;
    // The efficient way to do this is not to delete and then create a new order. But rather to
    // modify the existing order in-place.
    auto orderMetaIterator = _oidMetaMap.find(oid.value());
    auto& sideMap = _sideMap(side);
    auto* orderMeta = orderMetaIterator->second; // Create a copy of the info
    // We first erase it from the orderId map. Then we can unlink it within its price level
    // Then create a new entry, initialize the entry, and connect it within its new price level
    assert(orderMetaIterator != _oidMetaMap.end() && orderMeta != nullptr);
    sideMap[orderMeta->priceTick].volume -= orderMeta->qty;
    _oidMetaMap.erase(orderMetaIterator);
    _unlinkOrder(orderMeta, sideMap);
    orderMeta->oid = repl_oid.value();
    orderMeta->priceTick = price.value();
    orderMeta->qty = qty.value();
    _linkToPrice(orderMeta, sideMap[price.value()]);
    sideMap[price.value()].volume += orderMeta->qty;
    _oidMetaMap[orderMeta->oid] = orderMeta; // Create a new map entry
}

void OrderBook::_unlinkOrder(OrderMeta* orderMeta, SideMap& sideMap) {
    // - if orderHandle was head, set head to orderHandle->next
    // - if orderHandle was tail, set tail to orderHandle->prev
    // - link its prev and next if they are not nullptr
    // - we erase it from the oid map
    // - we erase it from our object pool
    auto& priceLevel = sideMap[orderMeta->priceTick];
    if (orderMeta == priceLevel.headHandle) {
        priceLevel.headHandle = orderMeta->nextHandle;
    }
    if (orderMeta == priceLevel.tailHandle) {
        priceLevel.tailHandle = orderMeta->prevHandle;
    }
    if (orderMeta->prevHandle != nullptr) {
        orderMeta->prevHandle->nextHandle = orderMeta->nextHandle;
    }
    if (orderMeta->nextHandle != nullptr) {
        orderMeta->nextHandle->prevHandle = orderMeta->prevHandle;
    }
}

void OrderBook::_linkToPrice(OrderMeta* orderMeta, PriceLevel& priceLevel) {
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
