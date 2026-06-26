#include "fuzz_support.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  auto parsed = mercury::make_default_registry().parse(mercury::fuzz::bytes(data, size));
  if (parsed.ok()) {
    for (const auto& [key, value] : parsed.value().metadata) {
      (void)key;
      (void)value;
    }
  }
  return 0;
}
