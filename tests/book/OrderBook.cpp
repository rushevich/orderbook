#include "orderbook_core/book/OrderBook.hpp"

#include "orderbook_core/Types.hpp"
#include "orderbook_core/containers/ObjectPool.hpp"

#include <gtest/gtest.h>

#define SUITE PlainOrderBookTests
using namespace rushevich;
using book::OrderBook;
TEST(SUITE, ConstructionDestruction) { OrderBook book; }

#undef SUITE
