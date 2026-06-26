#include "fuzz_support.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  auto codecs = mercury::make_default_codecs();
  for (const auto& name : codecs.names()) {
    const auto* codec = codecs.find(name);
    auto compressed = codec->compress(mercury::fuzz::bytes(data, size));
    if (compressed.ok()) {
      (void)codec->decompress(compressed.value());
    }
    (void)codec->decompress(mercury::fuzz::bytes(data, size));
  }
  return 0;
}
