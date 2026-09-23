cmake_minimum_required(VERSION 3.25)
project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ORDERBOOK_BUILD_TESTS "Build unit tests" ON)
option(ORDERBOOK_BUILD_BENCHMARKS "Build benchmarks" ON)

# ---------- Sanitizers ----------
set(ORDERBOOK_SANITIZER "" CACHE STRING
  "Sanitizers to enable: address, undefined, thread (semicolon-separated)")
set_property(CACHE ORDERBOOK_SANITIZER PROPERTY STRINGS
  "" "address" "undefined" "address;undefined" "thread")

if(ORDERBOOK_SANITIZER)
  if("thread" IN_LIST ORDERBOOK_SANITIZER AND "address" IN_LIST ORDERBOOK_SANITIZER)
    message(FATAL_ERROR "TSan cannot be combined with ASan")
  endif()
  list(JOIN ORDERBOOK_SANITIZER "," _orderbook_san)
  add_compile_options(-fsanitize=${_orderbook_san}
                      -fno-omit-frame-pointer
                      -fno-optimize-sibling-calls)
  add_link_options(-fsanitize=${_orderbook_san})
  if("undefined" IN_LIST ORDERBOOK_SANITIZER)
    add_compile_options(-fno-sanitize-recover=all)
  endif()
endif()

# ---------- Common flags ----------
add_library(orderbook_warnings INTERFACE)
target_compile_options(orderbook_warnings INTERFACE
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -Wno-interference-size)

# ---------- Core library ----------
add_library(orderbook_core
  src/itch/Parser.cpp
  src/system/MappedFile.cpp
  src/feed/FileReader.cpp
)
target_include_directories(orderbook_core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(orderbook_core PRIVATE orderbook_warnings)

# ---------- Executable ----------
add_executable(orderbook app/main.cpp src/book/Orchestrator.cpp src/book/OrderBook.cpp)
target_link_libraries(orderbook PRIVATE orderbook_core orderbook_warnings)
target_compile_definitions(orderbook PRIVATE
  ITCH_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

# ---------- Tests ----------
if(ORDERBOOK_BUILD_TESTS)
  find_package(GTest REQUIRED)
  enable_testing()
  include(GoogleTest)

  add_executable(orderbook_tests
    tests/test.cpp
    tests/containers/ObjectPool.cpp
    tests/book/OrderBook.cpp
    src/book/OrderBook.cpp
    src/book/Orchestrator.cpp
  )
  target_link_libraries(orderbook_tests
    PRIVATE orderbook_core orderbook_warnings GTest::gtest_main)
  target_compile_definitions(orderbook_tests PRIVATE
    ITCH_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
  gtest_discover_tests(orderbook_tests DISCOVERY_MODE PRE_TEST)
endif()

# ---------- Benchmarks ----------
if(ORDERBOOK_BUILD_BENCHMARKS)
  find_package(benchmark REQUIRED)

  add_executable(orderbook_bench bench/bench.cpp)
  target_link_libraries(orderbook_bench
    PRIVATE orderbook_core orderbook_warnings benchmark::benchmark_main)

  add_executable(orderbook_perfbench bench/queue.cpp)
  target_link_libraries(orderbook_perfbench PRIVATE orderbook_core orderbook_warnings)
  target_compile_options(orderbook_perfbench PRIVATE -O2 -g -fno-omit-frame-pointer)
  target_compile_definitions(orderbook_perfbench PRIVATE NDEBUG)
endif()
