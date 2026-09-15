#include "orderbook_core/concurrency/SPSCQueue.hpp"
#include "orderbook_core/itch/BasicFramer.hpp"
#include "orderbook_core/itch/Parser.hpp"
#include "orderbook_core/system/MappedFile.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

int main() {
    using namespace rushevich;
    const auto path = fs::path(ITCH_ASSET_DIR) / "NOADD_ITCH_BINARY";
    auto file = system::MappedFile(path);
    const auto data = file.data();
    itch::BasicFramer framer {};
    parser::Parser parser {};
    framer.consume_bytes(data, parser);
    std::ofstream dump_file(fs::current_path() / "ParseFile_dump");
    parser.dump_stats(dump_file);

    {
        rushevich::SPSCQueue<int> q { 50UZ };
    }

    return 0;
}
