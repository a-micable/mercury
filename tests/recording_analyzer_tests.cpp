#include "test_support.hpp"

namespace {

const mercury::test::Register recording_analyzer_summarizes_streams("recording analyzer summarizes streams", [] {
  const auto recording = mercury::test::fixture_recording();
  const auto analysis = mercury::RecordingAnalyzer().analyze(recording);

  mercury::test::require(analysis.records == 3, "analysis should count all records");
  mercury::test::require(analysis.payload_bytes == 6, "analysis should sum payload bytes");
  mercury::test::require(analysis.first_timestamp_ns == 100, "analysis should keep first timestamp");
  mercury::test::require(analysis.last_timestamp_ns == 300, "analysis should keep last timestamp");
  mercury::test::require(analysis.records_by_kind.at(mercury::RecordKind::metadata) == 1,
                         "analysis should count metadata records");
  mercury::test::require(analysis.records_by_kind.at(mercury::RecordKind::data) == 1,
                         "analysis should count data records");
  mercury::test::require(analysis.records_by_kind.at(mercury::RecordKind::event) == 1,
                         "analysis should count event records");
  mercury::test::require(analysis.streams.size() == 2, "analysis should summarize two streams");
  mercury::test::require(analysis.streams.front().stream_id == 1, "stream summaries should be sorted");
  mercury::test::require(analysis.streams.front().records == 2, "stream 1 should contain two records");
});

const mercury::test::Register recording_analyzer_detects_sequence_gaps("recording analyzer detects sequence gaps", [] {
  mercury::Recording recording;
  recording.format = "MRF1";
  recording.version = 1;
  recording.records.push_back(mercury::Record{mercury::RecordKind::data, 7, 100, 10, {1}, {}});
  recording.records.push_back(mercury::Record{mercury::RecordKind::data, 7, 200, 11, {2}, {}});
  recording.records.push_back(mercury::Record{mercury::RecordKind::data, 7, 300, 15, {3}, {}});

  const auto analysis = mercury::RecordingAnalyzer().analyze(recording);

  mercury::test::require(analysis.sequence_gaps.size() == 1, "analysis should detect one sequence gap");
  mercury::test::require(analysis.sequence_gaps.front().stream_id == 7, "sequence gap should name stream");
  mercury::test::require(analysis.sequence_gaps.front().expected_sequence == 12,
                         "sequence gap should report expected sequence");
  mercury::test::require(analysis.sequence_gaps.front().observed_sequence == 15,
                         "sequence gap should report observed sequence");
  mercury::test::require(!analysis.streams.front().sequences_contiguous,
                         "stream summary should mark non-contiguous sequences");
  mercury::test::require(!analysis.warnings.empty(), "sequence gap should create a warning");
});

const mercury::test::Register recording_analyzer_detects_time_anomalies("recording analyzer detects time anomalies", [] {
  auto recording = mercury::test::fixture_recording();
  recording.records.push_back(mercury::Record{mercury::RecordKind::data, 2, 900, 3, {1}, {}});
  recording.records.push_back(mercury::Record{mercury::RecordKind::data, 2, 800, 4, {2}, {}});

  mercury::RecordingAnalysisOptions options;
  options.timeline_gap_threshold_ns = 500;
  options.require_contiguous_sequences = false;
  const auto analysis = mercury::RecordingAnalyzer().analyze(recording, options);

  mercury::test::require(analysis.timeline_gaps.size() == 1, "analysis should detect one timeline gap");
  mercury::test::require(analysis.timeline_gaps.front().delta_ns == 700,
                         "timeline gap should report observed delta");
  mercury::test::require(!analysis.streams.back().timestamps_monotonic,
                         "stream summary should mark non-monotonic timestamps");
  mercury::test::require(analysis.warnings.size() == 2, "gap and backward timestamp should both warn");
});

} // namespace
