#include "tool_common.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: mercury-inspect <recording>\n";
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
  const auto& recording = parsed.value();
  std::cout << "format=" << recording.format << " version=" << recording.version
            << " records=" << recording.records.size() << '\n';
  for (const auto& record : recording.records) {
    std::cout << record.timestamp_ns << " stream=" << record.stream_id
              << " kind=" << mercury::to_string(record.kind)
              << " bytes=" << record.payload.size() << '\n';
  }
  return 0;
}
