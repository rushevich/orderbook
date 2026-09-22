#include "orderbook_core/book/OrderBook.hpp"

#include "orderbook_core/Types.hpp"
#include "orderbook_core/containers/ObjectPool.hpp"

#include <gtest/gtest.h>
#include <unordered_map>

#define SUITE PlainOrderBookTests
using namespace rushevich;
using book::OrderBook;
using containers::ObjectPool;
TEST(SUITE, ConstructionDestruction) {
    ObjectPool<OrderMeta, 100> pool;
    std::unordered_map<uint64_t, OrderMeta*> map(100);
    OrderBook book(pool, map);
}
#undef SUITE
