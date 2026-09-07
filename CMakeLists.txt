cmake_minimum_required(VERSION 3.25)
project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ORDERBOOK_BUILD_TESTS "Build unit tests" ON)
option(ORDERBOOK_BUILD_BENCHMARKS "Build benchmarks" ON)

# ---------- Common flags ----------
add_library(orderbook_warnings INTERFACE)
target_compile_options(orderbook_warnings INTERFACE
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

# ---------- Core library ----------
add_library(orderbook_core
  parser.cpp
)
target_include_directories(orderbook_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(orderbook_core PRIVATE orderbook_warnings)

# ---------- Executable ----------
add_executable(orderbook main.cpp)
target_link_libraries(orderbook PRIVATE orderbook_core orderbook_warnings)

# ---------- Tests ----------
if(ORDERBOOK_BUILD_TESTS)
  find_package(GTest REQUIRED)
  enable_testing()
  include(GoogleTest)

  add_executable(orderbook_tests test.cpp)
  target_link_libraries(orderbook_tests
    PRIVATE orderbook_core orderbook_warnings GTest::gtest_main)
  gtest_discover_tests(orderbook_tests)
endif()

# ---------- Benchmarks ----------
if(ORDERBOOK_BUILD_BENCHMARKS)
  find_package(benchmark REQUIRED)

  add_executable(orderbook_bench bench.cpp)
  target_link_libraries(orderbook_bench
    PRIVATE orderbook_core orderbook_warnings benchmark::benchmark_main)
endif()
