#include "mercury/mercury.hpp"

#include <algorithm>

namespace mercury {

std::vector<IndexEntry> IndexBuilder::build(const Recording& recording) const {
  std::vector<IndexEntry> entries;
  entries.reserve(recording.records.size());
  std::uint64_t logical_offset = 0;
  for (const auto& record : recording.records) {
    entries.push_back(IndexEntry{record.timestamp_ns, logical_offset, record.sequence, record.stream_id});
    logical_offset += 20 + record.payload.size() + 4;
  }
  std::stable_sort(entries.begin(), entries.end(), [](const IndexEntry& lhs, const IndexEntry& rhs) {
    if (lhs.timestamp_ns != rhs.timestamp_ns) {
      return lhs.timestamp_ns < rhs.timestamp_ns;
    }
    return lhs.sequence < rhs.sequence;
  });
  return entries;
}

} // namespace mercury
