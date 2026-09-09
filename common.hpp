#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace common {
constexpr inline std::array<size_t, 256> message_lengths { [] consteval -> std::array<size_t, 256> {
    std::array<size_t, 256> arr {};
    arr['S'] = 12; // System Event
    arr['R'] = 39; // Stock Directory
    arr['H'] = 25; // Stock Trading Action
    arr['Y'] = 20; // Reg SHO Restriction
    arr['L'] = 26; // Market Participant Position
    arr['V'] = 35; // MWCB Decline Level
    arr['W'] = 12; // MWCB Status
    arr['K'] = 28; // IPO Quoting Period Update
    arr['J'] = 35; // LULD Auction Collar
    arr['h'] = 21; // Operational Halt
    arr['A'] = 36; // Add Order, no MPID
    arr['F'] = 40; // Add Order with MPID
    arr['E'] = 31; // Order Executed
    arr['C'] = 36; // Order Executed With Price
    arr['X'] = 23; // Order Cancel
    arr['D'] = 19; // Order Delete
    arr['U'] = 35; // Order Replace
    arr['P'] = 44; // Trade (non-cross)
    arr['Q'] = 40; // Cross Trade
    arr['B'] = 19; // Broken Trade
    arr['I'] = 50; // NOII
    arr['N'] = 20; // RPII
    arr['O'] = 48; // Direct Listing with Capital Raise
    return arr;
}() };
} // namespace common
