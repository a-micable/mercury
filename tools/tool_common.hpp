#pragma once

#include "mercury/mercury.hpp"

#include <fstream>
#include <iostream>
#include <iterator>

namespace mercury::tool {

inline mercury::Result<std::vector<std::uint8_t>> read_file(const char* path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return mercury::Status(mercury::ErrorCode::io_error, std::string("failed to open ") + path);
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), {});
}

inline int print_error(const mercury::Status& status) {
  std::cerr << "error: " << mercury::to_string(status.code()) << ": " << status.message() << '\n';
  return 1;
}

inline mercury::Recording sample_recording() {
  mercury::Recording recording;
  recording.format = "MRF1";
  recording.version = 1;
  recording.metadata["source"] = mercury::MetadataValue{std::string("generated")};
  recording.records.push_back(mercury::Record{
      mercury::RecordKind::metadata, 1, 1000, 1, {'m', 'e', 't', 'a'}, {}});
  recording.records.push_back(mercury::Record{
      mercury::RecordKind::data, 1, 2000, 2, {1, 2, 3, 4, 5}, {}});
  recording.records.push_back(mercury::Record{
      mercury::RecordKind::event, 2, 2500, 3, {'o', 'k'}, {}});
  return recording;
}

} // namespace mercury::tool
