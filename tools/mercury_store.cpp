#include "tool_common.hpp"

#include <filesystem>

namespace {

void print_usage() {
  std::cerr << "usage: mercury-store <root> <tenant> list\n"
               "       mercury-store <root> <tenant> put <id> <recording>\n"
               "       mercury-store <root> <tenant> remove <id>\n";
}

} // namespace

int main(int argc, char** argv) {
  if (argc < 4 || argc > 6) {
    print_usage();
    return 2;
  }
  mercury::RecordingStore store(argv[1], argv[2]);
  const std::string command = argv[3];
  if (command == "list" && argc == 4) {
    auto summaries = store.list();
    if (!summaries.ok()) {
      return mercury::tool::print_error(summaries.status());
    }
    for (const auto& summary : summaries.value()) {
      std::cout << summary.id << ' ' << summary.format << ' '
                << summary.records << " records " << summary.serialized_bytes
                << " bytes\n";
    }
    return 0;
  }
  if (command == "put" && argc == 6) {
    auto bytes = mercury::tool::read_file(argv[5]);
    if (!bytes.ok()) {
      return mercury::tool::print_error(bytes.status());
    }
    auto recording = mercury::Serializer().read_recording(bytes.value());
    if (!recording.ok()) {
      return mercury::tool::print_error(recording.status());
    }
    const auto status = store.put(argv[4], recording.value());
    return status.ok() ? 0 : mercury::tool::print_error(status);
  }
  if (command == "remove" && argc == 5) {
    const auto status = store.erase(argv[4]);
    return status.ok() ? 0 : mercury::tool::print_error(status);
  }
  print_usage();
  return 2;
}