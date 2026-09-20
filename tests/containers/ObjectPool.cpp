#include "orderbook_core/containers/ObjectPool.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <ranges>
#include <vector>
#define SUITE ObjectPoolTests
using namespace rushevich::containers;

// Test that allocates a pool of one element and then deallocates, ensuring that the pointers are
// invalidated after free
TEST(ObjectPoolTests, TrivialAllocateDeallocate) {
    ObjectPool<int, 1> pool;
    [[maybe_unused]] int* num = pool.allocate(5);
    ASSERT_EQ(*num, 5);
    pool.free(num);
    ASSERT_EQ(num, nullptr);
}

TEST(SUITE, TrivialStructAllocate100) {
    struct Trivial {
        int x {};
        int y {};
    };
    static_assert(std::is_trivially_destructible_v<Trivial>);
    ObjectPool<Trivial, 100> pool;
    std::vector<Trivial*> ptrs;
    for (auto num : std::views::iota(0, 100)) {
        ptrs.emplace_back(pool.allocate(num, num));
    }
    ASSERT_EQ(pool.allocate(1, 1), nullptr); // ObjectPool returns nullptr on allocations when full
    pool.free(*(ptrs.rbegin()));
    ptrs.pop_back();
    ASSERT_TRUE(pool.remaining() == 1);
    for (int num = 0; auto& ptr : ptrs) {
        ASSERT_EQ(ptr->x, num);
        ASSERT_EQ(ptr->y, num++);
        pool.free(ptr);
        ASSERT_EQ(ptr, nullptr);
    }
}

TEST(SUITE, TrivialStructHugePoolNoFree) {
    struct Trivial {
        uint32_t orderid;
        uint32_t quantity;
        uint32_t bookid;
    };
    ObjectPool<Trivial, 6'000'000> huge_pool;
    for (auto num : std::views::iota(0, 6'000'000)) {
        [[maybe_unused]] auto* ptr = huge_pool.allocate(num, num, num);
    }
    ASSERT_TRUE(huge_pool.allocate() == nullptr);
}

#undef suite
