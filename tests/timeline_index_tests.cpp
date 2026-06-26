#include "test_support.hpp"

namespace {

std::vector<mercury::IndexEntry> unsorted_entries() {
  return {
      mercury::IndexEntry{300, 30, 3, 7},
      mercury::IndexEntry{100, 10, 1, 7},
      mercury::IndexEntry{200, 20, 2, 3},
      mercury::IndexEntry{200, 21, 4, 1},
  };
}

const mercury::test::Register timeline_index_sorts_entries("timeline index sorts entries", [] {
  mercury::TimelineIndex index;
  index.build(unsorted_entries());

  mercury::test::require(!index.empty(), "timeline index should contain entries after build");
  const auto& entries = index.entries();
  mercury::test::require(entries.front().timestamp_ns == 100, "first entry should be earliest timestamp");
  mercury::test::require(entries[1].sequence == 2, "same-timestamp entries should sort by sequence");
  mercury::test::require(entries.back().timestamp_ns == 300, "last entry should be latest timestamp");
});

const mercury::test::Register timeline_index_ranges_are_inclusive("timeline index ranges are inclusive", [] {
  mercury::TimelineIndex index;
  index.build(unsorted_entries());

  const auto entries = index.range(150, 250);
  mercury::test::require(entries.size() == 2, "range should include both entries at timestamp 200");
  mercury::test::require(entries.front().timestamp_ns == 200, "range lower bound returned wrong timestamp");
  mercury::test::require(index.range(400, 100).empty(), "inverted range should be empty");
});

const mercury::test::Register timeline_index_finds_nearest_boundaries("timeline index finds nearest boundaries", [] {
  mercury::TimelineIndex index;
  index.build(unsorted_entries());

  const auto before = index.nearest_at_or_before(250);
  mercury::test::require(before && before->timestamp_ns == 200, "nearest before should return timestamp 200");

  const auto after = index.nearest_at_or_after(250);
  mercury::test::require(after && after->timestamp_ns == 300, "nearest after should return timestamp 300");

  mercury::test::require(!index.nearest_at_or_before(50), "no entry should exist before first timestamp");
  mercury::test::require(!index.nearest_at_or_after(350), "no entry should exist after last timestamp");
});

} // namespace
