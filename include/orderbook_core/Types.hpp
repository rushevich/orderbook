#pragma once
#include "orderbook_core/containers/IntrusiveNode.hpp"

#include <compare>
#include <concepts>
#include <cstdint>
namespace rushevich {

template <typename T, typename Tag>
    requires std::integral<T>
struct StrongInt {
public:
    StrongInt() = default;
    explicit constexpr StrongInt(T val) noexcept : _underlying { val } {}
    explicit constexpr operator T() const noexcept { return _underlying; }
    constexpr T value() const noexcept { return _underlying; }
    friend auto operator<=>(StrongInt, StrongInt) noexcept = default;

private:
    T _underlying {};
};

// This corresponds to Stock Locate in the itch spec
using InstrumentID = StrongInt<uint16_t, struct InstrumentIDTag>;

// This corresponds to Order Reference Number in the itch spec
using OrderID = StrongInt<uint64_t, struct OrderIDTag>;

enum class Side : bool { buy, sell };

using Quantity = StrongInt<uint32_t, struct QuantityTag>;

// enum class Price : uint32_t {};
using Price = StrongInt<uint32_t, struct PriceTag>;

struct OrderMeta : public container::IntrusiveNode<> {
    uint64_t oid;
    uint32_t qty;
    uint32_t price_tick;
    uint16_t locate_idx; // Index for orderbook array.
    uint8_t is_bid;
};

struct PriceLevel {
    uint32_t head_handle;
    uint32_t tail_handle;
    uint32_t volume;
};
} // namespace rushevich
