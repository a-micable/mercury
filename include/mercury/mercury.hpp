#pragma once

#include "mercury/core.hpp"

#include <memory>
#include <set>

namespace mercury {

class Parser {
public:
  virtual ~Parser() = default;
  [[nodiscard]] virtual std::string_view name() const = 0;
  [[nodiscard]] virtual bool can_parse(std::span<const std::uint8_t> bytes) const = 0;
  [[nodiscard]] virtual Result<Recording> parse(std::span<const std::uint8_t> bytes) const = 0;
};

class ParserRegistry {
public:
  void register_parser(std::unique_ptr<Parser> parser);
  [[nodiscard]] const Parser* detect(std::span<const std::uint8_t> bytes) const;
  [[nodiscard]] Result<Recording> parse(std::span<const std::uint8_t> bytes) const;
  [[nodiscard]] std::vector<std::string> names() const;

private:
  std::vector<std::unique_ptr<Parser>> parsers_;
};

ParserRegistry make_default_registry();

class CompressionCodec {
public:
  virtual ~CompressionCodec() = default;
  [[nodiscard]] virtual std::string_view name() const = 0;
  [[nodiscard]] virtual Result<std::vector<std::uint8_t>> decompress(std::span<const std::uint8_t> bytes) const = 0;
  [[nodiscard]] virtual Result<std::vector<std::uint8_t>> compress(std::span<const std::uint8_t> bytes) const = 0;
};

class CodecRegistry {
public:
  void register_codec(std::unique_ptr<CompressionCodec> codec);
  [[nodiscard]] const CompressionCodec* find(std::string_view name) const;
  [[nodiscard]] std::vector<std::string> names() const;

private:
  std::vector<std::unique_ptr<CompressionCodec>> codecs_;
};

CodecRegistry make_default_codecs();

struct ArchiveMember {
  std::string name;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
  std::uint32_t checksum = 0;
};

class ArchiveReader {
public:
  [[nodiscard]] Result<std::vector<ArchiveMember>> scan(std::span<const std::uint8_t> bytes) const;
  [[nodiscard]] Result<std::vector<std::uint8_t>> extract(std::span<const std::uint8_t> bytes, std::string_view name) const;
};

class ArchiveWriter {
public:
  void add(std::string name, std::vector<std::uint8_t> bytes);
  [[nodiscard]] std::vector<std::uint8_t> finish() const;

private:
  std::vector<std::pair<std::string, std::vector<std::uint8_t>>> members_;
};

class IndexBuilder {
public:
  [[nodiscard]] std::vector<IndexEntry> build(const Recording& recording) const;
};

class TimelineIndex {
public:
  void build(std::vector<IndexEntry> entries);
  [[nodiscard]] std::vector<IndexEntry> range(std::uint64_t begin_ns, std::uint64_t end_ns) const;
  [[nodiscard]] std::optional<IndexEntry> nearest_at_or_before(std::uint64_t timestamp_ns) const;
  [[nodiscard]] std::optional<IndexEntry> nearest_at_or_after(std::uint64_t timestamp_ns) const;
  [[nodiscard]] const std::vector<IndexEntry>& entries() const;
  [[nodiscard]] bool empty() const;
  void clear();

private:
  std::vector<IndexEntry> entries_;
};

struct ReplayOptions {
  double speed = 1.0;
  std::set<std::uint16_t> stream_filter;
  std::optional<std::uint64_t> begin_ns;
  std::optional<std::uint64_t> end_ns;
};

class ReplayEngine {
public:
  using Callback = std::function<void(const TimelineEvent&, const Record&)>;
  [[nodiscard]] std::vector<TimelineEvent> timeline(const Recording& recording, const ReplayOptions& options = {}) const;
  Status replay(const Recording& recording, const ReplayOptions& options, Callback callback) const;
};

struct SequenceGap {
  std::uint16_t stream_id = 0;
  std::uint32_t expected_sequence = 0;
  std::uint32_t observed_sequence = 0;
  std::size_t record_ordinal = 0;
};

struct TimelineGap {
  std::uint16_t stream_id = 0;
  std::uint64_t previous_timestamp_ns = 0;
  std::uint64_t current_timestamp_ns = 0;
  std::uint64_t delta_ns = 0;
  std::size_t record_ordinal = 0;
};

struct StreamSummary {
  std::uint16_t stream_id = 0;
  std::size_t records = 0;
  std::size_t payload_bytes = 0;
  std::uint64_t first_timestamp_ns = 0;
  std::uint64_t last_timestamp_ns = 0;
  std::uint32_t first_sequence = 0;
  std::uint32_t last_sequence = 0;
  bool timestamps_monotonic = true;
  bool sequences_contiguous = true;
};

struct RecordingAnalysis {
  std::size_t records = 0;
  std::size_t payload_bytes = 0;
  std::uint64_t first_timestamp_ns = 0;
  std::uint64_t last_timestamp_ns = 0;
  std::map<RecordKind, std::size_t> records_by_kind;
  std::vector<StreamSummary> streams;
  std::vector<SequenceGap> sequence_gaps;
  std::vector<TimelineGap> timeline_gaps;
  std::vector<std::string> warnings;
};

struct RecordingAnalysisOptions {
  std::optional<std::uint64_t> timeline_gap_threshold_ns;
  bool require_contiguous_sequences = true;
  bool require_monotonic_stream_timestamps = true;
};

class RecordingAnalyzer {
public:
  [[nodiscard]] RecordingAnalysis analyze(const Recording& recording,
                                          const RecordingAnalysisOptions& options = {}) const;
};

struct RecordQuery {
  std::set<std::uint16_t> streams;
  std::set<RecordKind> kinds;
  std::optional<std::uint64_t> begin_ns;
  std::optional<std::uint64_t> end_ns;
  std::optional<std::uint32_t> min_sequence;
  std::optional<std::uint32_t> max_sequence;
  std::map<std::string, std::string> metadata_equals;
  std::vector<std::uint8_t> payload_contains;

  [[nodiscard]] bool empty() const;
};

struct QueryResult {
  std::size_t ordinal = 0;
  const Record* record = nullptr;
};

class QueryEngine {
public:
  [[nodiscard]] bool matches(const Record& record, const RecordQuery& query) const;
  [[nodiscard]] std::vector<QueryResult> search(const Recording& recording, const RecordQuery& query) const;
};

struct EventRoute {
  std::string name;
  RecordQuery query;
  int priority = 0;
  bool stop_after_match = false;
};

struct RoutedEvent {
  std::string route_name;
  std::size_t ordinal = 0;
  const Record* record = nullptr;
};

class EventRouter {
public:
  Status add_route(EventRoute route);
  [[nodiscard]] std::vector<EventRoute> routes() const;
  [[nodiscard]] std::vector<RoutedEvent> route_record(std::size_t ordinal, const Record& record) const;
  [[nodiscard]] std::vector<RoutedEvent> route_recording(const Recording& recording) const;
  void clear();

private:
  std::vector<EventRoute> routes_;
};

struct MetadataHit {
  std::string key;
  std::string value;
  std::optional<std::size_t> record_ordinal;
};

class MetadataIndex {
public:
  void add_recording_metadata(const Metadata& metadata);
  void add_record_metadata(std::size_t ordinal, const Metadata& metadata);
  void build(const Recording& recording);
  [[nodiscard]] std::vector<MetadataHit> find_key(std::string_view key) const;
  [[nodiscard]] std::vector<MetadataHit> find_exact(std::string_view key, std::string_view value) const;
  [[nodiscard]] std::vector<std::string> keys() const;
  [[nodiscard]] std::size_t size() const;
  void clear();

private:
  std::vector<MetadataHit> hits_;
};

enum class MetadataType {
  string,
  integer,
  floating,
  boolean,
};

struct MetadataRule {
  std::string key;
  MetadataType type = MetadataType::string;
  bool required = false;
  bool non_empty = false;
};

struct MetadataValidationIssue {
  std::string key;
  std::string message;
  std::optional<std::size_t> record_ordinal;
};

class MetadataValidator {
public:
  void add_rule(MetadataRule rule);
  [[nodiscard]] const std::vector<MetadataRule>& rules() const;
  [[nodiscard]] std::vector<MetadataValidationIssue> validate_recording(const Recording& recording) const;
  [[nodiscard]] std::vector<MetadataValidationIssue> validate_record(std::size_t ordinal, const Record& record) const;
  void clear();

private:
  std::vector<MetadataRule> rules_;
};

class Serializer {
public:
  [[nodiscard]] std::vector<std::uint8_t> write_recording(const Recording& recording) const;
  [[nodiscard]] Result<Recording> read_recording(std::span<const std::uint8_t> bytes) const;
};

struct Config {
  std::unordered_map<std::string, std::string> values;
  [[nodiscard]] std::string get(std::string_view key, std::string fallback = {}) const;
};

class ConfigLoader {
public:
  [[nodiscard]] Result<Config> parse(std::string_view text) const;
};

class PluginHost {
public:
  void register_parser(std::unique_ptr<Parser> parser);
  void register_codec(std::unique_ptr<CompressionCodec> codec);
  [[nodiscard]] ParserRegistry& parsers();
  [[nodiscard]] CodecRegistry& codecs();

private:
  ParserRegistry parsers_;
  CodecRegistry codecs_;
};

} // namespace mercury
