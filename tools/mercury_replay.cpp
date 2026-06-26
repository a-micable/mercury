#include "tool_common.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: mercury-replay <recording>\n";
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
  mercury::ReplayEngine engine;
  auto status = engine.replay(parsed.value(), {}, [](const mercury::TimelineEvent& event, const mercury::Record& record) {
    std::cout << event.timestamp_ns << " " << event.label
              << " payload=" << record.payload.size() << '\n';
  });
  return status.ok() ? 0 : mercury::tool::print_error(status);
}
