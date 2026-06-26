#pragma once

#include "mercury/mercury.hpp"

#include <stdexcept>

namespace mercury::test {

void add(std::string name, std::function<void()> fn);

struct Register {
  Register(std::string name, std::function<void()> fn) { add(std::move(name), std::move(fn)); }
};

inline void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

inline Recording fixture_recording() {
  Recording recording;
  recording.format = "MRF1";
  recording.version = 1;
  recording.metadata["airframe"] = MetadataValue{std::string("N42MX")};
  recording.records.push_back(Record{RecordKind::metadata, 1, 100, 1, {'a'}, {}});
  recording.records.push_back(Record{RecordKind::data, 2, 200, 2, {1, 2, 3}, {}});
  recording.records.push_back(Record{RecordKind::event, 1, 300, 3, {'e', 'v'}, {}});
  return recording;
}

} // namespace mercury::test
