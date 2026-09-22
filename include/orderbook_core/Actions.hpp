#pragma once
#include "orderbook_core/Types.hpp"

namespace rushevich {
// An order type that refers to any possible order
struct OrderAction {
    OrderID oid {};
    OrderID repl_oid {};    // Used in order replace
    Price price {};         // Used in order add and replace
    Quantity qty {};        // Used in add, execute, cancel, and replace
    InstrumentID locate {}; // Used ubiquitously
    Side side {};           // Used in order add
    Type type {};
};
struct DoNothing {};

} // namespace rushevich
