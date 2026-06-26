#include "tool_common.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: mercury-verify <recording>\n";
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
  mercury::IndexBuilder indexer;
  auto index = indexer.build(parsed.value());
  std::cout << "ok: " << parsed.value().records.size() << " records, "
            << index.size() << " index entries\n";
  return 0;
}
