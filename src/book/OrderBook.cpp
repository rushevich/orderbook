#include "orderbook_core/book/OrderBook.hpp"

namespace rushevich::book {

void OrderBook::unlink(OrderMeta* orderMeta) {
    auto& side = sideMap(orderMeta->side);
    auto& priceLevel = side[orderMeta->priceTick];
    priceLevel.volume -= orderMeta->qty;
    // If it is head, we have to bump head up
    if (orderMeta == priceLevel.headHandle) {
        priceLevel.headHandle = orderMeta->nextHandle;
    } else { // This means it’s within the list or a tail, in which case
        assert(orderMeta->prevHandle != IntrusiveNode<OrderMeta*>::NULL_HANDLE); // redundant, tbh
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

void OrderBook::link(OrderMeta* orderMeta) {
    auto& side = sideMap(orderMeta->side);
    auto& priceLevel = side[orderMeta->priceTick];
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

void OrderBook::reduce(OrderMeta* orderMeta, Quantity qty) {
    auto& side = sideMap(orderMeta->side);
    auto& priceLevel = side[orderMeta->priceTick];
    priceLevel.volume -= qty;
    orderMeta->qty -= qty;
}

} // namespace rushevich::book
