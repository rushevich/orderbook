#include "orderbook_core/book/Orchestrator.hpp"

#include "orderbook_core/Actions.hpp"
#include "orderbook_core/Types.hpp"

#include <utility>

namespace rushevich::book {
bool Orchestrator::consume(const OrderAction& action) {
    switch (action.type) {
        using enum Type;
    case Add:
        return _onAdd(action);
    case Execute:
        return _onExecute(action);
    case Cancel:
        return _onCancel(action);
    case Delete:
        return _onDelete(action);
    case Replace:
        return _onReplace(action);
    }
    std::unreachable();
}

bool Orchestrator::_onAdd(const OrderAction& action) {
    auto* orderMeta = _pool.allocate();
    if (orderMeta == nullptr) {
        return false;
    }
    orderMeta->oid = action.oid;
    orderMeta->qty = action.qty;
    orderMeta->priceTick = action.price;
    orderMeta->side = action.side;

    auto& book = _bookFor(action.locate);
    book.link(orderMeta);
    _oidMetaMap[action.oid] = orderMeta;
    return true;
}

bool Orchestrator::_onExecute(const OrderAction& action) {
    auto* orderMeta = _oidMetaMap[action.oid];
    if (orderMeta == nullptr) {
        return false;
    }
    auto& book = _bookFor(action.locate);
    if (orderMeta->qty <= action.qty) {
        book.unlink(orderMeta); // reduces the volume of the level and ejects the order
        _oidMetaMap.erase(action.oid);
        _pool.free(orderMeta);
    } else {
        book.reduce(orderMeta, action.qty);
    }
    return true;
}

bool Orchestrator::_onCancel(const OrderAction& action) {
    auto* orderMeta = _oidMetaMap[action.oid];
    if (orderMeta == nullptr) {
        return false;
    }
    auto& book = _bookFor(action.locate);
    if (orderMeta->qty <= action.qty) {
        book.unlink(orderMeta); // reduces the volume of the level and ejects the order
        _oidMetaMap.erase(action.oid);
        _pool.free(orderMeta);
    } else {
        book.reduce(orderMeta, action.qty);
    }
    return true;
}

bool Orchestrator::_onDelete(const OrderAction& action) {
    auto* orderMeta = _oidMetaMap[action.oid];
    if (orderMeta == nullptr) {
        return false;
    }
    auto& book = _bookFor(action.locate);
    book.unlink(orderMeta);
    _oidMetaMap.erase(action.oid);
    _pool.free(orderMeta);
    return true;
}

bool Orchestrator::_onReplace(const OrderAction& action) {
    auto* orderMeta = _oidMetaMap[action.oid];
    if (orderMeta == nullptr) {
        return false;
    }
    auto& book = _bookFor(action.locate);
    book.unlink(orderMeta);
    _oidMetaMap.erase(action.oid);
    orderMeta->oid = action.repl_oid;
    orderMeta->priceTick = action.price;
    orderMeta->qty = action.qty;
    book.link(orderMeta);
    _oidMetaMap[action.repl_oid] = orderMeta;
    return true;
}

OrderBook& Orchestrator::_bookFor(InstrumentID loc) { return _books[loc.value()]; }
} // namespace rushevich::book
