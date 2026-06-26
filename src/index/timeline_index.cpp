#include "mercury/mercury.hpp"

#include <algorithm>
#include <iterator>

namespace mercury {

namespace {

bool index_entry_less(const IndexEntry& lhs, const IndexEntry& rhs) {
  if (lhs.timestamp_ns != rhs.timestamp_ns) {
    return lhs.timestamp_ns < rhs.timestamp_ns;
  }
  if (lhs.sequence != rhs.sequence) {
    return lhs.sequence < rhs.sequence;
  }
  return lhs.stream_id < rhs.stream_id;
}

} // namespace

void TimelineIndex::build(std::vector<IndexEntry> entries) {
  entries_ = std::move(entries);
  std::stable_sort(entries_.begin(), entries_.end(), index_entry_less);
}

std::vector<IndexEntry> TimelineIndex::range(std::uint64_t begin_ns, std::uint64_t end_ns) const {
  if (begin_ns > end_ns) {
    return {};
  }

  const auto lower = std::lower_bound(entries_.begin(), entries_.end(), begin_ns,
                                      [](const IndexEntry& entry, std::uint64_t timestamp) {
                                        return entry.timestamp_ns < timestamp;
                                      });
  const auto upper = std::upper_bound(entries_.begin(), entries_.end(), end_ns,
                                      [](std::uint64_t timestamp, const IndexEntry& entry) {
                                        return timestamp < entry.timestamp_ns;
                                      });
  return std::vector<IndexEntry>(lower, upper);
}

std::optional<IndexEntry> TimelineIndex::nearest_at_or_before(std::uint64_t timestamp_ns) const {
  const auto iter = std::upper_bound(entries_.begin(), entries_.end(), timestamp_ns,
                                    [](std::uint64_t timestamp, const IndexEntry& entry) {
                                      return timestamp < entry.timestamp_ns;
                                    });
  if (iter == entries_.begin()) {
    return std::nullopt;
  }
  return *std::prev(iter);
}

std::optional<IndexEntry> TimelineIndex::nearest_at_or_after(std::uint64_t timestamp_ns) const {
  const auto iter = std::lower_bound(entries_.begin(), entries_.end(), timestamp_ns,
                                    [](const IndexEntry& entry, std::uint64_t timestamp) {
                                      return entry.timestamp_ns < timestamp;
                                    });
  if (iter == entries_.end()) {
    return std::nullopt;
  }
  return *iter;
}

const std::vector<IndexEntry>& TimelineIndex::entries() const {
  return entries_;
}

bool TimelineIndex::empty() const {
  return entries_.empty();
}

void TimelineIndex::clear() {
  entries_.clear();
}

} // namespace mercury
