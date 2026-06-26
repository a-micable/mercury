#include "fuzz_support.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  auto parsed = mercury::make_default_registry().parse(mercury::fuzz::bytes(data, size));
  if (parsed.ok()) {
    mercury::fuzz::exercise_recording(parsed.value());
  }
  return 0;
}
