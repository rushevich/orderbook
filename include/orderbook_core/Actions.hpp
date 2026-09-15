#pragma once
#include "orderbook_core/Types.hpp"

#include <variant>

namespace rushevich {
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
    Quantity executed_qty {};
    InstrumentID locate {};
};

// Action that cancels a certain amount of shares
struct OrderCancel {
    OrderID oid {};
    Quantity qty {};
    InstrumentID locate {};
};

// Deletes the order from the book (reduces the order's quantity to 0)
struct OrderDelete {
    OrderID oid {};
    InstrumentID locate {};
};

// Replaces an order by reducing the current order's quantity to 0 and then
// creating a new order with a new id
struct OrderReplace {
    OrderID oid {};
    OrderID new_oid {};
    Quantity new_qty {};
    Price new_price {};
    InstrumentID locate {};
};

struct DoNothing {};

// Action can be one of the defined orders
using Action
    = std::variant<OrderAdd, OrderExecute, OrderCancel, OrderDelete, OrderReplace, DoNothing>;

} // namespace rushevich
