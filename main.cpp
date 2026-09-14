#include "SPSCQueue.hpp"
#include "framer.hpp"
#include "parser.hpp"
#include "reader.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

int main() {
    const auto path = fs::path(ITCH_ASSET_DIR) / "NOADD_ITCH_BINARY";
    auto file = reader::detail::MappedFile(path);
    const auto data = file.data();
    framer::MessageFramer framer {};
    parser::Parser parser {};
    framer.consume_bytes(data, parser);
    std::ofstream dump_file(fs::current_path() / "ParseFile_dump");
    parser.dump_stats(dump_file);

    {
        rushevich::SPSCQueue<int> q { 50UZ };
    }

    return 0;
}
