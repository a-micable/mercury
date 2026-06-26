#include "tool_common.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: mercury-index <recording>\n";
    return 2;
  }
  auto bytes = mercury::tool::read_file(argv[1]);
  if (!bytes.ok()) {
    return mercury::tool::print_error(bytes.status());
  }
  auto parsed = mercury::make_default_registry().parse(bytes.value());
  if (!parsed.ok()) {
    return mercury::tool::print_error(parsed.status());
  }
  for (const auto& entry : mercury::IndexBuilder().build(parsed.value())) {
    std::cout << entry.timestamp_ns << ',' << entry.stream_id << ','
              << entry.sequence << ',' << entry.file_offset << '\n';
  }
  return 0;
}
