#include <benchmark/benchmark.h>

#include "parser.hpp"

static void BM_ParseMessage(benchmark::State &state) {
  // setup outside the loop
  for (auto _ : state) {
    // work under measurement
    benchmark::DoNotOptimize(0);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ParseMessage);
