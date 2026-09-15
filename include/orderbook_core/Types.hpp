#pragma once
#include <cstdint>

namespace rushevich {
// This corresponds to Stock Locate in the itch spec
enum class InstrumentID : uint16_t {};
// This corresponds to Order Reference Number in the itch spec
enum class OrderID : uint64_t {};

enum class Type : bool { buy, sell };

enum class Quantity : uint32_t {};

enum class Price : uint32_t {};
} // namespace rushevich
