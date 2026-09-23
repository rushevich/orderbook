#include "orderbook_core/book/Orchestrator.hpp"
#include "orderbook_core/concurrency/SPSCQueue.hpp"
#include "orderbook_core/itch/BasicFramer.hpp"
#include "orderbook_core/itch/Parser.hpp"
#include "orderbook_core/system/Linux.hpp"
#include "orderbook_core/system/MappedFile.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <print>
#include <thread>
namespace fs = std::filesystem;
namespace cr = std::chrono;
using namespace rushevich;

concurrency::SPSCQueue<OrderAction> inboundQueue(100'000);
std::atomic<bool> shutdownSignal;
std::ofstream out { "APP_DUMP" };

void producer() {
    system::pinThread(0);
    itch::BasicFramer framer {};
    parser::Parser parser {};

    const auto path = fs::path(ITCH_ASSET_DIR) / "NOADD_ITCH_BINARY";
    auto file = system::MappedFile(path);
    const auto data = file.data();
    while (!framer.isDone()) {
        auto parseResult = framer.consumeMessage(data, parser);
        if (parseResult.has_value()) {
            inboundQueue.emplace(parseResult.value());
        }
    }

    parser.dump_stats(out);
    shutdownSignal.store(true, std::memory_order_release);
}

void consumer() {
    system::pinThread(3);
    book::Orchestrator engine;
    while (shutdownSignal.load(std::memory_order_acquire) == false) {
        while (inboundQueue.front() == nullptr) {
        }
        auto* action = inboundQueue.front();
        inboundQueue.pop();
        engine.consume(*action);
    }
}

int main() {

    auto start = cr::steady_clock::now();
    std::jthread con { consumer };
    std::jthread prod { producer };
    con.join();
    prod.join();
    auto elapsed = cr::duration_cast<cr::milliseconds>(cr::steady_clock::now() - start);
    std::println(out, "Elapsed time: {} ms", elapsed);
    return 0;
}
