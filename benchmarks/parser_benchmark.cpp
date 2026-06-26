#include "mercury/mercury.hpp"

#include <chrono>
#include <iostream>

int main() {
  mercury::Recording recording;
  recording.format = "MRF1";
  recording.version = 1;
  for (std::uint32_t i = 0; i < 50000; ++i) {
    recording.records.push_back(mercury::Record{
        mercury::RecordKind::data, static_cast<std::uint16_t>(i % 8),
        static_cast<std::uint64_t>(i) * 1000, i, {1, 2, 3, 4}, {}});
  }
  auto bytes = mercury::Serializer().write_recording(recording);
  auto parser = mercury::make_default_registry();
  auto start = std::chrono::steady_clock::now();
  auto parsed = parser.parse(bytes);
  auto elapsed = std::chrono::steady_clock::now() - start;
  if (!parsed.ok()) {
    std::cerr << parsed.status().message() << '\n';
    return 1;
  }
  std::cout << "records=" << parsed.value().records.size()
            << " elapsed_us="
            << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()
            << '\n';
}
