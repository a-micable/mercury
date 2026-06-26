#include "fuzz_support.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  auto parsed = mercury::make_default_registry().parse(mercury::fuzz::bytes(data, size));
  if (!parsed.ok()) {
    return 0;
  }
  mercury::ReplayOptions options;
  if (size > 0) {
    options.stream_filter.insert(data[0]);
  }
  mercury::ReplayEngine engine;
  (void)engine.replay(parsed.value(), options, [](const mercury::TimelineEvent&, const mercury::Record&) {});
  return 0;
}
