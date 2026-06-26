#include "fuzz_support.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  mercury::ArchiveReader reader;
  auto members = reader.scan(mercury::fuzz::bytes(data, size));
  if (members.ok()) {
    for (const auto& member : members.value()) {
      (void)reader.extract(mercury::fuzz::bytes(data, size), member.name);
    }
  }
  return 0;
}
