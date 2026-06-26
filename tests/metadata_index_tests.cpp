#include "test_support.hpp"

namespace {

const mercury::test::Register metadata_index_builds_recording_and_record_hits(
    "metadata index builds recording and record hits", [] {
      auto recording = mercury::test::fixture_recording();
      recording.metadata["site"] = mercury::MetadataValue{std::string("hangar-7")};
      recording.records[0].metadata["phase"] = mercury::MetadataValue{std::string("startup")};
      recording.records[1].metadata["phase"] = mercury::MetadataValue{std::string("capture")};
      recording.records[1].metadata["valid"] = mercury::MetadataValue{true};

      mercury::MetadataIndex index;
      index.build(recording);

      mercury::test::require(index.size() == 5, "metadata index should include recording and record metadata");
      const auto site = index.find_exact("site", "hangar-7");
      mercury::test::require(site.size() == 1, "recording metadata should be searchable by exact value");
      mercury::test::require(!site.front().record_ordinal, "recording metadata should not have a record ordinal");

      const auto phase = index.find_key("phase");
      mercury::test::require(phase.size() == 2, "record metadata keys should include all matching records");
      mercury::test::require(phase[1].record_ordinal && *phase[1].record_ordinal == 1,
                             "record metadata should preserve source ordinal");
    });

const mercury::test::Register metadata_index_returns_sorted_unique_keys("metadata index returns sorted unique keys", [] {
  auto recording = mercury::test::fixture_recording();
  recording.metadata["zulu"] = mercury::MetadataValue{std::string("last")};
  recording.records[0].metadata["alpha"] = mercury::MetadataValue{std::string("first")};
  recording.records[1].metadata["alpha"] = mercury::MetadataValue{std::string("again")};

  mercury::MetadataIndex index;
  index.build(recording);

  const auto keys = index.keys();
  mercury::test::require(keys.size() == 3, "keys should be unique");
  mercury::test::require(keys[0] == "airframe", "keys should be sorted");
  mercury::test::require(keys[1] == "alpha", "duplicate record keys should collapse");
  mercury::test::require(keys[2] == "zulu", "last key missing from sorted output");
});

} // namespace
