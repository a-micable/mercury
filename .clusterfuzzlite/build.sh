#!/usr/bin/env bash
set -eu

cmake -S . -B build-fuzz \
  -DMERCURY_BUILD_FUZZERS=ON \
  -DMERCURY_BUILD_TESTS=OFF \
  -DMERCURY_BUILD_BENCHMARKS=OFF \
  -DCMAKE_CXX_COMPILER="${CXX:-clang++}" \
  -DCMAKE_CXX_FLAGS="${CXXFLAGS:-} ${LIB_FUZZING_ENGINE:-}"
cmake --build build-fuzz --parallel
