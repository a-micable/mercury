#include "tool_common.hpp"

#include <fstream>

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "usage: mercury-convert <input> <output>\n";
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
  auto normalized = mercury::Serializer().write_recording(parsed.value());
  std::ofstream output(argv[2], std::ios::binary);
  output.write(reinterpret_cast<const char*>(normalized.data()), static_cast<std::streamsize>(normalized.size()));
  return output ? 0 : 1;
}
