#include "fuzz_support.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  std::string text(reinterpret_cast<const char*>(data), size);
  auto parsed = mercury::ConfigLoader().parse(text);
  if (parsed.ok()) {
    (void)parsed.value().get("codec", "stored");
  }
  return 0;
}
