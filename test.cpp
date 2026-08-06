#include <catch2/catch_test_macros.hpp>

auto function_that_returns_one() -> int { return 1; }

TEST_CASE("Trivial test", "[test1]") {
    REQUIRE(function_that_returns_one() == 1);
}
