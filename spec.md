# ITCH Order Book Engine — Statement of Work & Design Document

A resume-grade C++23 limit order book reconstructed from NASDAQ TotalView-ITCH,
built measurement-first: correct baseline, then iterated toward a hand-tuned,
benchmarked, cache-aware engine.

**Guiding principle:** correctness before speed, measurement before optimization.
The interview value is the *iteration story* (baseline → profiled → optimized with
numbers), not raw cleverness.

---

## Phase 0 — Foundation & Tooling

- [ ] CMake project targeting C++23 (`CMAKE_CXX_STANDARD 23`)
- [ ] GitHub Actions CI (build + test on push)
- [ ] Sanitizer build configs: ASan, UBSan, TSan
- [ ] `clang-tidy` + `clang-format` configured and enforced in CI
- [ ] Unit test framework wired in (Catch2 or GoogleTest)
- [ ] Benchmark harness wired in (Google Benchmark)
- [ ] Document target machine characteristics (cache sizes, core topology, CPU model)
- [ ] Establish a `perf` workflow (stat/record/report) and confirm it runs

**Exit criteria:** empty project builds under C++23, CI is green, a trivial
benchmark and a trivial test both run.

---

## Phase 1 — ITCH Parsing

- [ ] Obtain NASDAQ TotalView-ITCH 5.0 spec and a sample data file
- [ ] Implement message framing (2-byte big-endian length prefix over the stream)
- [ ] Byte-swap handling (`std::byteswap`)
- [ ] Zero-copy struct interpretation over the buffer (`std::span`, `std::bit_cast`)
- [ ] Parse core message types: Add, Cancel, Delete, Replace, Execute
- [ ] Correctness check: parse full sample file, tally message-type counts, validate against known totals

**Exit criteria:** the sample feed parses cleanly end-to-end with a validated
per-message-type histogram.

---

## Phase 2 — Core Order Book (correctness first)

- [ ] Model price levels with a per-level FIFO queue (price-time priority)
- [ ] Handle Add / Cancel / Delete / Replace / Execute against the book
- [ ] Order-ID → location map for O(1) cancel and modify
- [ ] Top-of-book queries (best bid/ask, sizes)
- [ ] Validate: reconstruct book from ITCH, compare top-of-book against a reference

**Exit criteria:** book reconstruction is provably correct against reference data.
No optimization yet.

---

## Phase 3 — Data Structure Design

- [ ] Naive baseline: `std::map<Price, Level>` (something to beat)
- [ ] Benchmark the baseline and record numbers
- [ ] Design a hand-rolled structure: dense array indexed by price ticks for the
      hot region, intrusive linked lists for order queues
- [ ] Free-list / pooled allocation for orders
- [ ] Write up the tradeoff analysis: sorted map (log n, cache-unfriendly) vs
      array-of-levels (O(1) index, memory cost, needs bounded/normalized prices)
- [ ] Benchmark the new structure against the baseline

**Exit criteria:** hand-rolled structure measurably beats the map baseline, with
the delta recorded.

---

## Phase 4 — Benchmarking & Profiling Discipline

- [ ] End-to-end throughput benchmark (messages/sec)
- [ ] Per-operation latency distributions: p50 / p99 / p99.9 (not just mean)
- [ ] `perf stat`: cache misses, branch mispredicts, IPC
- [ ] `perf record` / `perf report` to locate hotspots
- [ ] Benchmark log committed to the repo (for the "reduced X by Y%" story)

**Exit criteria:** a reproducible benchmark suite and a committed log establishing
the pre-optimization baseline.

---

## Phase 5 — Optimization Passes

*Only optimize what Phase 4 flagged.*

- [ ] Memory: custom allocators, object pools, arena allocation
- [ ] Cache-line alignment; struct packing to shrink footprint
- [ ] Branch reduction in the hot path; branchless updates where measurable
- [ ] Prefetching where profiling justifies it
- [ ] Evaluate SoA vs AoS for level data
- [ ] Audit for false sharing (before any threading)
- [ ] Re-benchmark after each pass; keep only wins

**Exit criteria:** each committed optimization is backed by a measured improvement.

---

## Phase 6 — Multithreading

*Last, and only if it measurably helps.*

- [ ] Understand the constraint: matching is sequential per symbol; parallelism
      comes from partitioning by symbol or splitting parse/decode from matching
- [ ] Implement an SPSC lock-free ring buffer for parser → matcher handoff
- [ ] Memory model study: `std::atomic`, acquire/release, why `seq_cst` is rarely needed
- [ ] Benchmark threaded vs single-threaded; keep the threaded version only if it wins

**Exit criteria:** a correct concurrent handoff, with a measured verdict on whether
it beats the tuned single thread.

---

## Phase 7 — Resume Polish

- [ ] README: architecture diagram, benchmark numbers, design rationale
- [ ] Clean commit history showing baseline → optimized progression
- [ ] Write-up of key engineering decisions and their measured impact

**Exit criteria:** a stranger can read the README and understand what was built,
why, and how much faster it got.

---

## C++23 Feature Targets

Concrete places to apply modern standards, so it isn't C++23 in name only:

- `std::byteswap` — endianness handling in the parser
- `std::bit_cast` — safe reinterpretation of packed message structs
- `std::span` — non-owning views over the feed buffer
- `std::mdspan` — candidate for level/depth data (Phase 3+)
- `std::expected` — parser and validation error handling
- `constexpr`/`consteval` — compile-time message-layout tables
- Ranges — validation and analysis tooling (keep out of the hot path unless measured)

This document is AI generated. The scope is my idea though.
