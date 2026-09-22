#include "orderbook_core/Actions.hpp"
#include "orderbook_core/Types.hpp"
#include "orderbook_core/book/Orchestrator.hpp"
#include "orderbook_core/itch/Parser.hpp"
#include "orderbook_core/itch/Spec.hpp"
#include "orderbook_core/system/MappedFile.hpp"
#include "orderbook_core/util/Endian.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

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

using namespace rushevich;
TEST(Parser, ParseAdd) {
    parser::Parser par {};
    const auto result = par(kAdd);
    ASSERT_TRUE(result.has_value());
    const auto add = *result;
    EXPECT_EQ(add.oid.value(), 4886718345ULL);
    EXPECT_EQ(add.qty.value(), 100U);
    EXPECT_EQ(add.price.value(), 123400U);
    EXPECT_EQ(add.locate.value(), 1234U);
    EXPECT_EQ(add.side, Side::buy);
}

TEST(Parser, ParseExecute) {
    parser::Parser par {};
    const auto result = par(kExecute);
    ASSERT_TRUE(result.has_value());
    const auto exec = *result;
    EXPECT_EQ(exec.oid.value(), 4886718345ULL);
    EXPECT_EQ(exec.qty.value(), 50U);
    EXPECT_EQ(exec.locate.value(), 1234U);
}

TEST(Parser, ParseCancel) {
    parser::Parser par {};
    const auto result = par(kCancel);
    ASSERT_TRUE(result.has_value());
    const auto cancel = *result;
    EXPECT_EQ(cancel.oid.value(), 4886718345ULL);
    EXPECT_EQ(cancel.qty.value(), 25U);
    EXPECT_EQ(cancel.locate.value(), 1234U);
}

TEST(Parser, ParseDelete) {
    parser::Parser par {};
    const auto result = par(kDelete);
    ASSERT_TRUE(result.has_value());
    const auto del = *result;
    EXPECT_EQ(del.oid.value(), 4886718345ULL);
    EXPECT_EQ(del.locate.value(), 1234U);
}

TEST(Parser, ParseReplace) {
    parser::Parser par {};
    const auto result = par(kReplace);
    ASSERT_TRUE(result.has_value());
    const auto rep = *result;
    EXPECT_EQ(rep.oid.value(), 4886718345ULL);
    EXPECT_EQ(rep.repl_oid.value(), 12841944963ULL);
    EXPECT_EQ(rep.qty.value(), 200U);
    EXPECT_EQ(rep.price.value(), 567800U);
    EXPECT_EQ(rep.locate.value(), 1234U);
}

TEST(Parser, ParseFile) {
    // GTEST_SKIP(); // Test is slow and proven to pass
    const auto path = fs::path(ITCH_ASSET_DIR) / "NOADD_ITCH_BINARY";
    if (!fs::exists(path)) {
        GTEST_SKIP() << "no local ITCH binary found at " << path;
    }
    auto file = system::MappedFile(path);
    const auto data = file.data();
    ASSERT_TRUE(!data.empty()); // we shouldn’t proceed if this is empty
    parser::Parser par;
    size_t pos { 0 };
    book::Orchestrator orchestrator;
    while (pos + 2 <= data.size()) {
        const auto len = util::parse_be<2>(data, pos);
        ASSERT_LE(pos + 2 + len, data.size());
        const auto type = util::parse_be<1>(data, pos + 2);
        ASSERT_EQ(len, itch::message_lengths[type]);
        [[maybe_unused]] auto val = par(file.data().subspan(pos + 2, len));
        if (val) {
            orchestrator.consume(val.value());
        }
        pos += 2 + len;
    }
    ASSERT_TRUE(!par.histogram().empty());
    std::ofstream dump_file(fs::current_path() / "ParseFile_dump");
    par.dump_stats(dump_file);
}
