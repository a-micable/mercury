#include "test_support.hpp"

namespace {

const mercury::test::Register query_empty_matches_all("query empty matches all records", [] {
  const auto recording = mercury::test::fixture_recording();
  mercury::RecordQuery query;

  mercury::test::require(query.empty(), "new query should report empty");
  const auto results = mercury::QueryEngine().search(recording, query);
  mercury::test::require(results.size() == recording.records.size(), "empty query should match every record");
  mercury::test::require(results[2].ordinal == 2, "query results should preserve record ordinals");
});

const mercury::test::Register query_combines_record_predicates("query combines record predicates", [] {
  auto recording = mercury::test::fixture_recording();
  recording.records[1].metadata["sensor"] = mercury::MetadataValue{std::string("imu-left")};
  recording.records[1].metadata["calibrated"] = mercury::MetadataValue{true};

  mercury::RecordQuery query;
  query.streams.insert(2);
  query.kinds.insert(mercury::RecordKind::data);
  query.begin_ns = 150;
  query.end_ns = 250;
  query.min_sequence = 2;
  query.max_sequence = 4;
  query.metadata_equals["sensor"] = "imu-left";
  query.metadata_equals["calibrated"] = "true";
  query.payload_contains = {2, 3};

  const auto results = mercury::QueryEngine().search(recording, query);
  mercury::test::require(results.size() == 1, "combined query should match one data record");
  mercury::test::require(results.front().ordinal == 1, "combined query matched the wrong record");
  mercury::test::require(results.front().record->sequence == 2, "query result should point at the source record");
});

const mercury::test::Register query_rejects_missing_payload_sequence("query rejects missing payload sequence", [] {
  const auto recording = mercury::test::fixture_recording();

  mercury::RecordQuery query;
  query.streams.insert(2);
  query.payload_contains = {3, 2};

  const auto results = mercury::QueryEngine().search(recording, query);
  mercury::test::require(results.empty(), "payload subsequence matching should respect byte order");
});

} // namespace
