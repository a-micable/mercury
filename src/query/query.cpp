#include "mercury/mercury.hpp"

#include <algorithm>
#include <sstream>

namespace mercury {

namespace {

std::string metadata_to_string(const MetadataValue& value) {
  return std::visit(
      [](const auto& item) -> std::string {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::string>) {
          return item;
        } else if constexpr (std::is_same_v<T, bool>) {
          return item ? "true" : "false";
        } else {
          std::ostringstream out;
          out << item;
          return out.str();
        }
      },
      value.value);
}

bool contains_payload(std::span<const std::uint8_t> payload, std::span<const std::uint8_t> needle) {
  if (needle.empty()) {
    return true;
  }
  if (needle.size() > payload.size()) {
    return false;
  }
  return std::search(payload.begin(), payload.end(), needle.begin(), needle.end()) != payload.end();
}

bool metadata_matches(const Metadata& metadata, const std::map<std::string, std::string>& expected) {
  for (const auto& [key, value] : expected) {
    const auto iter = metadata.find(key);
    if (iter == metadata.end() || metadata_to_string(iter->second) != value) {
      return false;
    }
  }
  return true;
}

} // namespace

bool RecordQuery::empty() const {
  return streams.empty() && kinds.empty() && !begin_ns && !end_ns && !min_sequence && !max_sequence &&
         metadata_equals.empty() && payload_contains.empty();
}

bool QueryEngine::matches(const Record& record, const RecordQuery& query) const {
  if (!query.streams.empty() && !query.streams.contains(record.stream_id)) {
    return false;
  }
  if (!query.kinds.empty() && !query.kinds.contains(record.kind)) {
    return false;
  }
  if (query.begin_ns && record.timestamp_ns < *query.begin_ns) {
    return false;
  }
  if (query.end_ns && record.timestamp_ns > *query.end_ns) {
    return false;
  }
  if (query.min_sequence && record.sequence < *query.min_sequence) {
    return false;
  }
  if (query.max_sequence && record.sequence > *query.max_sequence) {
    return false;
  }
  if (!metadata_matches(record.metadata, query.metadata_equals)) {
    return false;
  }
  return contains_payload(record.payload, query.payload_contains);
}

std::vector<QueryResult> QueryEngine::search(const Recording& recording, const RecordQuery& query) const {
  std::vector<QueryResult> results;
  for (std::size_t ordinal = 0; ordinal < recording.records.size(); ++ordinal) {
    const auto& record = recording.records[ordinal];
    if (matches(record, query)) {
      results.push_back(QueryResult{ordinal, &record});
    }
  }
  return results;
}

} // namespace mercury
