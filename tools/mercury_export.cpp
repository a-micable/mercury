#include "tool_common.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: mercury-export <recording>\n";
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
  std::cout << "{\n  \"format\": \"" << parsed.value().format << "\",\n"
            << "  \"records\": " << parsed.value().records.size() << "\n}\n";
  return 0;
}
