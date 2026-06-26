#include "tool_common.hpp"

#include <chrono>

int main() {
  auto bytes = mercury::Serializer().write_recording(mercury::tool::sample_recording());
  auto registry = mercury::make_default_registry();
  const auto start = std::chrono::steady_clock::now();
  std::size_t parsed_records = 0;
  for (int i = 0; i < 10000; ++i) {
    auto parsed = registry.parse(bytes);
    if (!parsed.ok()) {
      return mercury::tool::print_error(parsed.status());
    }
    parsed_records += parsed.value().records.size();
  }
  const auto elapsed = std::chrono::steady_clock::now() - start;
  std::cout << "parsed_records=" << parsed_records
            << " elapsed_ms="
            << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
            << '\n';
  return 0;
}
