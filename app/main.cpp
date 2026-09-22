#include "orderbook_core/concurrency/SPSCQueue.hpp"
#include "orderbook_core/itch/BasicFramer.hpp"
#include "orderbook_core/itch/Parser.hpp"
#include "orderbook_core/system/MappedFile.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#define DIAG
namespace fs = std::filesystem;

int main() {
    using namespace rushevich;
    const auto path = fs::path(ITCH_ASSET_DIR) / "NOADD_ITCH_BINARY";
    auto file = system::MappedFile(path);
    const auto data = file.data();
    itch::BasicFramer framer {};
    parser::Parser parser {};
#ifdef DIAG
    std::map<InstrumentID, std::set<Price>> locateToPriceRange;
    uint32_t global_min { UINT_MAX };
    uint32_t global_max { 0 };
    uint32_t max_ticks { 0 };
    uint32_t adds {};
    uint32_t reps {};

    const auto diagnosticsFunction = [&](const OrderAction& action) {
        if (auto price = action.price;
            price.value() > 0 && (action.type == Type::Replace || action.type == Type::Add)) {
            adds += (action.type == Type::Add) ? 1 : 0;
            reps += (action.type == Type::Replace) ? 1 : 0;

            locateToPriceRange[action.locate].insert(price);
            global_min = std::min(price.value(), global_min);
            global_max = std::max(price.value(), global_max);
        }
    };
#endif
    framer.consume_bytes(data, parser
#ifdef DIAG
                         ,
                         diagnosticsFunction
#endif
    );
    std::ofstream dump_file(fs::current_path() / "ParseFile_dump");

    parser.dump_stats(dump_file);
#ifdef DIAG
    for (const auto& [loc, prices] : locateToPriceRange) {
        std::println(dump_file, "loc {}: min = {}, max = {}, tick-count = {}", loc.value(),
                     prices.begin()->value(), prices.rbegin()->value(), prices.size());
        max_ticks = std::max(max_ticks, static_cast<uint32_t>(prices.size()));
    }

    std::println(dump_file, "global range: [{},{}]", global_min, global_max);
    std::println(dump_file, "filtered: adds {}, reps {}", adds, reps);
    std::println(dump_file, "max ticks = {}", max_ticks);
    std::println(dump_file, "instrument-count = {}", locateToPriceRange.size());
#endif
    return 0;
}
