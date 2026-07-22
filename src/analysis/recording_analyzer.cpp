#include "mercury/mercury.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace mercury {

namespace {

struct MutableStreamSummary {
  StreamSummary summary;
  bool initialized = false;
  std::uint64_t previous_timestamp_ns = 0;
  std::uint32_t previous_sequence = 0;
};

std::string describe_sequence_gap(const SequenceGap& gap) {
  std::ostringstream out;
  out << "stream " << gap.stream_id << " expected sequence " << gap.expected_sequence << " but observed "
      << gap.observed_sequence << " at record " << gap.record_ordinal;
  return out.str();
}

std::string describe_non_monotonic_timestamp(std::uint16_t stream_id,
                                             std::uint64_t previous_timestamp_ns,
                                             std::uint64_t timestamp_ns,
                                             std::size_t ordinal) {
  std::ostringstream out;
  out << "stream " << stream_id << " timestamp moved backward from " << previous_timestamp_ns << " to "
      << timestamp_ns << " at record " << ordinal;
  return out.str();
}

std::string describe_timeline_gap(const TimelineGap& gap) {
  std::ostringstream out;
  out << "stream " << gap.stream_id << " has " << gap.delta_ns << " ns gap before record "
      << gap.record_ordinal;
  return out.str();
}

void observe_record_kind(RecordingAnalysis& analysis, RecordKind kind) {
  auto [iter, inserted] = analysis.records_by_kind.emplace(kind, 0);
  (void)inserted;
  ++iter->second;
}

void initialize_stream(MutableStreamSummary& stream, const Record& record) {
  stream.initialized = true;
  stream.summary.stream_id = record.stream_id;
  stream.summary.first_timestamp_ns = record.timestamp_ns;
  stream.summary.last_timestamp_ns = record.timestamp_ns;
  stream.summary.first_sequence = record.sequence;
  stream.summary.last_sequence = record.sequence;
  stream.previous_timestamp_ns = record.timestamp_ns;
  stream.previous_sequence = record.sequence;
}

void observe_payload(StreamSummary& summary, const Record& record) {
  ++summary.records;
  summary.payload_bytes += record.payload.size();
  summary.last_timestamp_ns = record.timestamp_ns;
  summary.last_sequence = record.sequence;
}

void observe_sequence(RecordingAnalysis& analysis,
                      MutableStreamSummary& stream,
                      const Record& record,
                      std::size_t ordinal,
                      const RecordingAnalysisOptions& options) {
  if (!options.require_contiguous_sequences || !stream.initialized) {
    return;
  }

  const auto expected = stream.previous_sequence + 1;
  if (record.sequence != expected) {
    stream.summary.sequences_contiguous = false;
    SequenceGap gap;
    gap.stream_id = record.stream_id;
    gap.expected_sequence = expected;
    gap.observed_sequence = record.sequence;
    gap.record_ordinal = ordinal;
    analysis.sequence_gaps.push_back(gap);
    analysis.warnings.push_back(describe_sequence_gap(gap));
  }
}

void observe_timestamp(RecordingAnalysis& analysis,
                       MutableStreamSummary& stream,
                       const Record& record,
                       std::size_t ordinal,
                       const RecordingAnalysisOptions& options) {
  if (!stream.initialized) {
    return;
  }

  if (options.require_monotonic_stream_timestamps && record.timestamp_ns < stream.previous_timestamp_ns) {
    stream.summary.timestamps_monotonic = false;
    analysis.warnings.push_back(describe_non_monotonic_timestamp(
        record.stream_id, stream.previous_timestamp_ns, record.timestamp_ns, ordinal));
  }

  if (options.timeline_gap_threshold_ns && record.timestamp_ns >= stream.previous_timestamp_ns) {
    const auto delta = record.timestamp_ns - stream.previous_timestamp_ns;
    if (delta > *options.timeline_gap_threshold_ns) {
      TimelineGap gap;
      gap.stream_id = record.stream_id;
      gap.previous_timestamp_ns = stream.previous_timestamp_ns;
      gap.current_timestamp_ns = record.timestamp_ns;
      gap.delta_ns = delta;
      gap.record_ordinal = ordinal;
      analysis.timeline_gaps.push_back(gap);
      analysis.warnings.push_back(describe_timeline_gap(gap));
    }
  }
}

void finalize_streams(RecordingAnalysis& analysis,
                      const std::unordered_map<std::uint16_t, MutableStreamSummary>& streams) {
  analysis.streams.reserve(streams.size());
  for (const auto& [stream_id, stream] : streams) {
    (void)stream_id;
    analysis.streams.push_back(stream.summary);
  }
  std::sort(analysis.streams.begin(), analysis.streams.end(), [](const StreamSummary& lhs,
                                                                 const StreamSummary& rhs) {
    return lhs.stream_id < rhs.stream_id;
  });
}

} // namespace

RecordingAnalysis RecordingAnalyzer::analyze(const Recording& recording,
                                             const RecordingAnalysisOptions& options) const {
  RecordingAnalysis analysis;
  analysis.records = recording.records.size();

  if (recording.records.empty()) {
    return analysis;
  }

  analysis.first_timestamp_ns = std::numeric_limits<std::uint64_t>::max();
  analysis.last_timestamp_ns = 0;

  std::unordered_map<std::uint16_t, MutableStreamSummary> streams;
  for (std::size_t ordinal = 0; ordinal < recording.records.size(); ++ordinal) {
    const auto& record = recording.records[ordinal];

    analysis.payload_bytes += record.payload.size();
    analysis.first_timestamp_ns = std::min(analysis.first_timestamp_ns, record.timestamp_ns);
    analysis.last_timestamp_ns = std::max(analysis.last_timestamp_ns, record.timestamp_ns);
    observe_record_kind(analysis, record.kind);

    auto& stream = streams[record.stream_id];
    observe_sequence(analysis, stream, record, ordinal, options);
    observe_timestamp(analysis, stream, record, ordinal, options);
    if (!stream.initialized) {
      initialize_stream(stream, record);
    }
    observe_payload(stream.summary, record);
    stream.previous_timestamp_ns = record.timestamp_ns;
    stream.previous_sequence = record.sequence;
  }

  finalize_streams(analysis, streams);
  return analysis;
}

} // namespace mercury
