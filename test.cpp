#include "parser.hpp"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <utility>
#include <variant>

// Contains names to this TU
namespace {
// Function that constructs an array of bytes from arbitrary & variadic input
template <typename... Ts> constexpr auto bytes(Ts... vs) {
    return std::array<std::byte, sizeof...(Ts)> { static_cast<std::byte>(vs)... };
}

// Add Order, 36 bytes.
//  0     type          'A'
//  1..2  stock locate  1234
//  3..4  tracking      0
//  5..10 timestamp     arbitrary filler, not parsed
// 11..18 order ref     4886718345 (0x1'2345'6789)
// 19     buy/sell      'B'
// 20..23 shares        100
// 24..31 stock         "AAPL    "
// 32..35 price         123400 (i.e. $12.34)
constexpr auto kAdd = bytes(0x41,                                           //
                            0x04, 0xD2,                                     //
                            0x00, 0x00,                                     //
                            0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A,             //
                            0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, //
                            0x42,                                           //
                            0x00, 0x00, 0x00, 0x64,                         //
                            'A', 'A', 'P', 'L', ' ', ' ', ' ', ' ',         //
                            0x00, 0x01, 0xE2, 0x08);                        //

static_assert(kAdd.size() == 36);

// Order Executed ('E'), 31 bytes.
//  0     type            'E'
//  1..2  stock locate    1234
//  3..4  tracking        0
//  5..10 timestamp       filler, not parsed
// 11..18 order ref       4886718345 (0x1'2345'6789)
// 19..22 executed shares 50
// 23..30 match number    filler, not parsed
constexpr auto kExecute = bytes(0x45,                                           //
                                0x04, 0xD2,                                     //
                                0x00, 0x00,                                     //
                                0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A,             //
                                0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, //
                                0x00, 0x00, 0x00, 0x32,                         //
                                0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA);
static_assert(kExecute.size() == 31);

// Order Cancel ('X'), 23 bytes.
//  0     type             'X'
//  1..2  stock locate     1234
//  3..4  tracking         0
//  5..10 timestamp        filler, not parsed
// 11..18 order ref        4886718345 (0x1'2345'6789)
// 19..22 cancelled shares 25
constexpr auto kCancel = bytes(0x58,                                           //
                               0x04, 0xD2,                                     //
                               0x00, 0x00,                                     //
                               0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A,             //
                               0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, //
                               0x00, 0x00, 0x00, 0x19);
static_assert(kCancel.size() == 23);

// Order Delete ('D'), 19 bytes.
//  0     type          'D'
//  1..2  stock locate  1234
//  3..4  tracking      0
//  5..10 timestamp     filler, not parsed
// 11..18 order ref     4886718345 (0x1'2345'6789)
constexpr auto kDelete = bytes(0x44,                               //
                               0x04, 0xD2,                         //
                               0x00, 0x00,                         //
                               0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, //
                               0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89);
static_assert(kDelete.size() == 19);

// Order Replace ('U'), 35 bytes.
//  0     type          'U'
//  1..2  stock locate  1234
//  3..4  tracking      0
//  5..10 timestamp     filler, not parsed
// 11..18 original ref  4886718345 (0x1'2345'6789)
// 19..26 new order ref 12841944963 (0x2'FEDC'BA43)
// 27..30 new shares    200
// 31..34 new price     567800 (i.e. $56.78)
constexpr auto kReplace = bytes(0x55,                                           //
                                0x04, 0xD2,                                     //
                                0x00, 0x00,                                     //
                                0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A,             //
                                0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, //
                                0x00, 0x00, 0x00, 0x02, 0xFD, 0x70, 0x87, 0x83, //
                                0x00, 0x00, 0x00, 0xC8,                         //
                                0x00, 0x08, 0xA9, 0xF8);
static_assert(kReplace.size() == 35);

} // namespace

TEST(Parser, ParseAdd) {
    parser::Parser par {};
    const auto result = par.parse(kAdd);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<parser::OrderAdd>(*result));
    const auto add = std::get<parser::OrderAdd>(*result);
    EXPECT_EQ(std::to_underlying(add.oid), 4886718345ULL);
    EXPECT_EQ(std::to_underlying(add.qty), 100U);
    EXPECT_EQ(std::to_underlying(add.price), 123400U);
    EXPECT_EQ(std::to_underlying(add.locate), 1234U);
    EXPECT_EQ(add.type, parser::Type::buy);
}

TEST(Parser, ParseExecute) {
    parser::Parser par {};
    const auto result = par.parse(kExecute);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<parser::OrderExecute>(*result));
    const auto exec = std::get<parser::OrderExecute>(*result);
    EXPECT_EQ(std::to_underlying(exec.oid), 4886718345ULL);
    EXPECT_EQ(std::to_underlying(exec.executed_qty), 50U);
    EXPECT_EQ(std::to_underlying(exec.locate), 1234U);
}

TEST(Parser, ParseCancel) {
    parser::Parser par {};
    const auto result = par.parse(kCancel);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<parser::OrderCancel>(*result));
    const auto cancel = std::get<parser::OrderCancel>(*result);
    EXPECT_EQ(std::to_underlying(cancel.oid), 4886718345ULL);
    EXPECT_EQ(std::to_underlying(cancel.qty), 25U);
    EXPECT_EQ(std::to_underlying(cancel.locate), 1234U);
}

TEST(Parser, ParseDelete) {
    parser::Parser par {};
    const auto result = par.parse(kDelete);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<parser::OrderDelete>(*result));
    const auto del = std::get<parser::OrderDelete>(*result);
    EXPECT_EQ(std::to_underlying(del.oid), 4886718345ULL);
    EXPECT_EQ(std::to_underlying(del.locate), 1234U);
}

TEST(Parser, ParseReplace) {
    parser::Parser par {};
    const auto result = par.parse(kReplace);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<parser::OrderReplace>(*result));
    const auto rep = std::get<parser::OrderReplace>(*result);
    EXPECT_EQ(std::to_underlying(rep.oid), 4886718345ULL);
    EXPECT_EQ(std::to_underlying(rep.new_oid), 12841944963ULL);
    EXPECT_EQ(std::to_underlying(rep.new_qty), 200U);
    EXPECT_EQ(std::to_underlying(rep.new_price), 567800U);
    EXPECT_EQ(std::to_underlying(rep.locate), 1234U);
}
