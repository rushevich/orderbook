#include "orderbook_core/concurrency/SPSCQueue.hpp"

#include <chrono>
#include <print>
#include <pthread.h>
#include <sched.h>
#include <system_error>
#include <thread>
#include <unistd.h>
namespace {
void pinThread(int cpu) {
    if (cpu < 0) {
        return;
    }
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to set thread affinity");
    }
}

static constexpr auto q_sz = 10000000ll;
static constexpr auto q_iter = 10000000ll;

inline __attribute__((always_inline)) void DoNotOptimize(auto& value) {
    asm volatile("" : "+r,m"(value) : : "memory");
}

} // namespace

static void throughput_bench() {
    using rushevich::concurrency::SPSCQueue;
    SPSCQueue<long long> queue(q_sz);
    std::thread consumer { [&queue] {
        pinThread(0);
        long long val = 0;
        for (long long i = 0; i < q_iter; i++) {
            while (queue.front() == nullptr)
                ;
            DoNotOptimize(val = *queue.front());
            queue.pop();
        }
    } };

    pinThread(2);
    const auto start = std::chrono::steady_clock::now();
    for (long long i = 0; i < q_iter; i++) {
        queue.emplace(i);
    }
    consumer.join();
    const std::chrono::nanoseconds runtime = std::chrono::steady_clock::now() - start;
    std::println("SPSC Throughput: {} ops/ms", q_iter * 1000000 / runtime.count());
}

int main() {
    throughput_bench();
    return 0;
}
