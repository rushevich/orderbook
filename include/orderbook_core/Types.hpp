#pragma once
#include "orderbook_core/containers/IntrusiveNode.hpp"

#include <compare> // Compiler is annoyingly warning
#include <concepts>
#include <cstdint>
#include <functional> // std::hash
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
    constexpr StrongInt& operator-=(StrongInt other) noexcept {
        _underlying -= other._underlying;
        return *this;
    }
    constexpr StrongInt& operator+=(StrongInt other) noexcept {
        _underlying += other._underlying;
        return *this;
    }

private:
    T _underlying {};
};

// This corresponds to Stock Locate in the itch spec
using InstrumentID = StrongInt<uint16_t, struct InstrumentIDTag>;

// This corresponds to Order Reference Number in the itch spec
using OrderID = StrongInt<uint64_t, struct OrderIDTag>;

using Quantity = StrongInt<uint32_t, struct QuantityTag>;

using Price = StrongInt<uint32_t, struct PriceTag>;

enum class Side : bool { buy, sell };

enum class Type : uint8_t { Add, Execute, Cancel, Delete, Replace };

struct OrderMeta : public containers::IntrusiveNode<OrderMeta*> {
    OrderID oid {};
    Quantity qty {};
    Price priceTick {};
    InstrumentID locateIdx {}; // Index for orderbook array.
    Side side {};
};

struct PriceLevel {
    OrderMeta* headHandle; // corresponds to handles that are given out by the orderPool
    OrderMeta* tailHandle;
    Quantity volume;
};

} // namespace rushevich

template <typename T, typename Tag> struct std::hash<rushevich::StrongInt<T, Tag>> {
    size_t operator()(const rushevich::StrongInt<T, Tag>& val) const noexcept {
        return std::hash<T> {}(val.value());
    }
};
