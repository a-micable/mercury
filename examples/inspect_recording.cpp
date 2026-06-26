#include "mercury/mercury.hpp"

#include <iostream>

int main() {
  mercury::Recording recording;
  recording.format = "MRF1";
  recording.version = 1;
  recording.records.push_back(mercury::Record{
      mercury::RecordKind::event, 7, 42, 1, {'b', 'o', 'o', 't'}, {}});

  auto bytes = mercury::Serializer().write_recording(recording);
  auto parsed = mercury::make_default_registry().parse(bytes);
  if (!parsed.ok()) {
    std::cerr << parsed.status().message() << '\n';
    return 1;
  }

  for (const auto& event : mercury::ReplayEngine().timeline(parsed.value())) {
    std::cout << event.timestamp_ns << " " << event.label << '\n';
  }
}
