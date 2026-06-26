cmake_minimum_required(VERSION 3.20)

project(Mercury
  VERSION 0.1.0
  DESCRIPTION "Binary recorder ingestion, replay, and analysis framework"
  LANGUAGES CXX)

option(MERCURY_BUILD_TESTS "Build Mercury unit and integration tests" ON)
option(MERCURY_BUILD_FUZZERS "Build libFuzzer harnesses" OFF)
option(MERCURY_BUILD_BENCHMARKS "Build Mercury benchmarks" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mercury
  src/archive/archive.cpp
  src/codecs/compression.cpp
  src/config/config.cpp
  src/core/binary.cpp
  src/core/checksum.cpp
  src/core/diagnostics.cpp
  src/core/logging.cpp
  src/core/status.cpp
  src/formats/compact.cpp
  src/formats/legacy.cpp
  src/formats/mrf1.cpp
  src/formats/mrf2.cpp
  src/formats/stream_capture.cpp
  src/index/index.cpp
  src/index/metadata_index.cpp
  src/index/timeline_index.cpp
  src/metadata/metadata_validator.cpp
  src/parser/parser.cpp
  src/plugin/plugin.cpp
  src/query/query.cpp
  src/replay/replay.cpp
  src/routing/event_router.cpp
  src/serialization/serializer.cpp
)

target_include_directories(mercury PUBLIC include PRIVATE src)
target_compile_features(mercury PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(mercury PRIVATE /W4 /permissive-)
else()
  target_compile_options(mercury PRIVATE -Wall -Wextra -Wpedantic)
endif()

function(mercury_tool name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE mercury)
endfunction()

mercury_tool(mercury-inspect tools/mercury_inspect.cpp)
mercury_tool(mercury-verify tools/mercury_verify.cpp)
mercury_tool(mercury-index tools/mercury_index.cpp)
mercury_tool(mercury-replay tools/mercury_replay.cpp)
mercury_tool(mercury-export tools/mercury_export.cpp)
mercury_tool(mercury-convert tools/mercury_convert.cpp)
mercury_tool(mercury-benchmark tools/mercury_benchmark.cpp)

if(MERCURY_BUILD_TESTS)
  enable_testing()
  add_executable(mercury-tests
    tests/archive_tests.cpp
    tests/config_tests.cpp
    tests/diagnostics_tests.cpp
    tests/event_router_tests.cpp
    tests/metadata_index_tests.cpp
    tests/parser_tests.cpp
    tests/query_tests.cpp
    tests/replay_tests.cpp
    tests/serialization_tests.cpp
    tests/timeline_index_tests.cpp
    tests/test_main.cpp
  )
  target_link_libraries(mercury-tests PRIVATE mercury)
  add_test(NAME mercury-tests COMMAND mercury-tests)
endif()

if(MERCURY_BUILD_FUZZERS)
  foreach(target record archive replay metadata config compression)
    add_executable(${target}_fuzzer fuzz/${target}_fuzzer.cpp)
    target_link_libraries(${target}_fuzzer PRIVATE mercury)
  endforeach()
endif()

if(MERCURY_BUILD_BENCHMARKS)
  add_executable(mercury-parser-bench benchmarks/parser_benchmark.cpp)
  target_link_libraries(mercury-parser-bench PRIVATE mercury)
endif()
