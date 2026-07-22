#include "mercury/mercury.hpp"

#include <algorithm>
#include <array>
#include <sstream>

namespace mercury {

namespace {

struct RecoverableFormat {
  std::string_view name;
  std::array<std::uint8_t, 4> magic;
  std::uint16_t min_version = 1;
  std::uint16_t max_version = 1;
};

constexpr std::array<RecoverableFormat, 5> kRecoverableFormats{{
    {"MRF1", {'M', 'R', 'F', '1'}, 1, 3},
    {"MRF2", {'M', 'R', 'F', '2'}, 2, 6},
    {"Legacy", {'L', 'G', 'R', '0'}, 1, 2},
    {"Compact", {'C', 'E', 'R', '0'}, 1, 1},
    {"StreamCapture", {'S', 'C', 'A', 'P'}, 1, 4},
}};

struct DetectedFormat {
  const RecoverableFormat* format = nullptr;
  std::size_t offset = 0;
};

void add_issue(std::vector<RecoveryIssue>& issues,
               RecoveryIssueKind kind,
               ErrorCode code,
               std::uint64_t offset,
               std::string message) {
  RecoveryIssue issue;
  issue.kind = kind;
  issue.code = code;
  issue.offset = offset;
  issue.message = std::move(message);
  issues.push_back(std::move(issue));
}

std::optional<DetectedFormat> find_format(std::span<const std::uint8_t> bytes) {
  for (std::size_t offset = 0; offset + 4 <= bytes.size(); ++offset) {
    for (const auto& format : kRecoverableFormats) {
      if (std::equal(format.magic.begin(), format.magic.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset))) {
        return DetectedFormat{&format, offset};
      }
    }
  }
  return std::nullopt;
}

Result<std::string> read_recovery_string(BinaryView& view) {
  auto length = view.read_u16_le();
  if (!length.ok()) {
    return length.status();
  }
  if (length.value() > 4096) {
    return Status(ErrorCode::malformed_record, "metadata string is too large");
  }
  return view.read_string(length.value());
}

Status read_recovery_metadata(BinaryView& view, Metadata& metadata) {
  auto count = view.read_u16_le();
  if (!count.ok()) {
    return count.status();
  }

  for (std::uint16_t index = 0; index < count.value(); ++index) {
    auto key = read_recovery_string(view);
    if (!key.ok()) {
      return key.status();
    }
    auto value = read_recovery_string(view);
    if (!value.ok()) {
      return value.status();
    }
    metadata[key.value()] = MetadataValue{value.value()};
  }

  return Status::Ok();
}

std::string invalid_version_message(std::string_view format, std::uint16_t version) {
  std::ostringstream out;
  out << format << " version " << version << " is outside supported recovery range";
  return out.str();
}

bool read_record_header(BinaryView& view,
                        std::uint16_t& kind,
                        std::uint16_t& stream,
                        std::uint64_t& timestamp,
                        std::uint32_t& sequence,
                        std::uint32_t& payload_length) {
  auto parsed_kind = view.read_u16_le();
  auto parsed_stream = view.read_u16_le();
  auto parsed_timestamp = view.read_u64_le();
  auto parsed_sequence = view.read_u32_le();
  auto parsed_payload_length = view.read_u32_le();
  if (!parsed_kind.ok() || !parsed_stream.ok() || !parsed_timestamp.ok() || !parsed_sequence.ok() ||
      !parsed_payload_length.ok()) {
    return false;
  }

  kind = parsed_kind.value();
  stream = parsed_stream.value();
  timestamp = parsed_timestamp.value();
  sequence = parsed_sequence.value();
  payload_length = parsed_payload_length.value();
  return true;
}

Record make_record(std::uint16_t kind,
                   std::uint16_t stream,
                   std::uint64_t timestamp,
                   std::uint32_t sequence,
                   std::vector<std::uint8_t> payload) {
  Record record;
  record.kind = record_kind_from_wire(kind);
  record.stream_id = stream;
  record.timestamp_ns = timestamp;
  record.sequence = sequence;
  record.payload = std::move(payload);
  return record;
}

void recover_records(BinaryView& view,
                     Recording& recording,
                     std::vector<RecoveryIssue>& issues,
                     std::uint32_t record_count,
                     const RecoveryOptions& options) {
  const auto bounded_count = std::min(record_count, options.max_records);
  for (std::uint32_t index = 0; index < bounded_count; ++index) {
    const auto record_offset = view.offset();

    std::uint16_t kind = 0;
    std::uint16_t stream = 0;
    std::uint64_t timestamp = 0;
    std::uint32_t sequence = 0;
    std::uint32_t payload_length = 0;
    if (!read_record_header(view, kind, stream, timestamp, sequence, payload_length)) {
      add_issue(issues, RecoveryIssueKind::malformed_record_header, ErrorCode::end_of_input, record_offset,
                "record header is truncated");
      return;
    }

    if (payload_length > options.max_payload_bytes) {
      add_issue(issues, RecoveryIssueKind::payload_too_large, ErrorCode::malformed_record, record_offset,
                "record payload exceeds recovery limit");
      return;
    }

    auto payload = view.read_bytes(payload_length);
    auto expected_checksum = view.read_u32_le();
    if (!payload.ok() || !expected_checksum.ok()) {
      add_issue(issues, RecoveryIssueKind::truncated_payload, ErrorCode::end_of_input, record_offset,
                "record payload or checksum is truncated");
      return;
    }

    const auto observed_checksum = crc32(payload.value());
    if (observed_checksum != expected_checksum.value()) {
      add_issue(issues, RecoveryIssueKind::checksum_mismatch, ErrorCode::checksum_mismatch, record_offset,
                "record payload checksum failed");
      if (!options.keep_checksum_failures) {
        continue;
      }
    }

    recording.records.push_back(make_record(kind, stream, timestamp, sequence, std::move(payload.value())));
  }

  if (record_count > options.max_records) {
    add_issue(issues, RecoveryIssueKind::malformed_record_header, ErrorCode::malformed_record, view.offset(),
              "declared record count exceeds recovery limit");
  }
}

} // namespace

bool RecoveryResult::recovered_anything() const {
  return !recording.records.empty() || !recording.metadata.empty();
}

bool RecoveryResult::clean() const {
  return issues.empty();
}

RecoveryResult RecoveryScanner::recover(std::span<const std::uint8_t> bytes, const RecoveryOptions& options) const {
  RecoveryResult result;
  const auto detected = find_format(bytes);
  if (!detected) {
    add_issue(result.issues, RecoveryIssueKind::skipped_prefix, ErrorCode::unknown_format, 0,
              "no recoverable format magic was found");
    return result;
  }

  if (detected->offset > 0) {
    add_issue(result.issues, RecoveryIssueKind::skipped_prefix, ErrorCode::malformed_record, 0,
              "discarded bytes before recording magic");
  }

  BinaryView view(bytes.subspan(detected->offset));
  (void)view.read_bytes(4);

  auto version = view.read_u16_le();
  if (!version.ok()) {
    add_issue(result.issues, RecoveryIssueKind::invalid_version, version.status().code(), view.offset(),
              version.status().message());
    return result;
  }

  result.recording.format = std::string(detected->format->name);
  result.recording.version = version.value();
  if (version.value() < detected->format->min_version || version.value() > detected->format->max_version) {
    add_issue(result.issues, RecoveryIssueKind::invalid_version, ErrorCode::unsupported_version, view.offset(),
              invalid_version_message(detected->format->name, version.value()));
    return result;
  }

  const auto metadata_status = read_recovery_metadata(view, result.recording.metadata);
  if (!metadata_status.ok()) {
    add_issue(result.issues, RecoveryIssueKind::malformed_metadata, metadata_status.code(), view.offset(),
              metadata_status.message());
    return result;
  }

  auto record_count = view.read_u32_le();
  if (!record_count.ok()) {
    add_issue(result.issues, RecoveryIssueKind::malformed_record_header, record_count.status().code(), view.offset(),
              record_count.status().message());
    return result;
  }

  recover_records(view, result.recording, result.issues, record_count.value(), options);
  result.bytes_consumed = detected->offset + view.offset();

  if (result.bytes_consumed < bytes.size()) {
    add_issue(result.issues, RecoveryIssueKind::trailing_bytes, ErrorCode::malformed_record, result.bytes_consumed,
              "input contains trailing bytes after recovered recording");
  }
  return result;
}

} // namespace mercury
