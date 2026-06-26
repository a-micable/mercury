#include "framed_format.hpp"

#include <algorithm>
#include <array>

namespace mercury::formats {

namespace {

Result<std::string> read_metadata_string(BinaryView& view) {
  auto length = view.read_u16_le();
  if (!length.ok()) {
    return length.status();
  }
  if (length.value() > 4096) {
    return Status(ErrorCode::malformed_record, "metadata string is too large");
  }
  return view.read_string(length.value());
}

Status read_metadata(BinaryView& view, Metadata& metadata) {
  auto count = view.read_u16_le();
  if (!count.ok()) {
    return count.status();
  }
  for (std::uint16_t i = 0; i < count.value(); ++i) {
    auto key = read_metadata_string(view);
    if (!key.ok()) {
      return key.status();
    }
    auto value = read_metadata_string(view);
    if (!value.ok()) {
      return value.status();
    }
    metadata[key.value()] = MetadataValue{value.value()};
  }
  return Status::Ok();
}

} // namespace

FramedParser::FramedParser(FormatDescriptor descriptor) : descriptor_(std::move(descriptor)) {}

std::string_view FramedParser::name() const {
  return descriptor_.name;
}

bool FramedParser::can_parse(std::span<const std::uint8_t> bytes) const {
  return bytes.size() >= descriptor_.magic.size() &&
         std::equal(descriptor_.magic.begin(), descriptor_.magic.end(), bytes.begin());
}

Result<Recording> FramedParser::parse(std::span<const std::uint8_t> bytes) const {
  BinaryView view(bytes);
  auto magic = view.read_bytes(4);
  if (!magic.ok()) {
    return magic.status();
  }
  if (!std::equal(descriptor_.magic.begin(), descriptor_.magic.end(), magic.value().begin())) {
    return Status(ErrorCode::invalid_magic, "recording magic does not match parser");
  }

  auto version = view.read_u16_le();
  if (!version.ok()) {
    return version.status();
  }
  if (version.value() < descriptor_.min_version || version.value() > descriptor_.max_version) {
    return Status(ErrorCode::unsupported_version, "format version is outside supported range");
  }

  Recording recording;
  recording.format = descriptor_.name;
  recording.version = version.value();

  auto metadata_status = read_metadata(view, recording.metadata);
  if (!metadata_status.ok()) {
    return metadata_status;
  }

  auto record_count = view.read_u32_le();
  if (!record_count.ok()) {
    return record_count.status();
  }
  if (record_count.value() > 1'000'000) {
    return Status(ErrorCode::malformed_record, "record count exceeds sanity limit");
  }

  recording.records.reserve(record_count.value());
  for (std::uint32_t i = 0; i < record_count.value(); ++i) {
    auto kind = view.read_u16_le();
    auto stream = view.read_u16_le();
    auto timestamp = view.read_u64_le();
    auto sequence = view.read_u32_le();
    auto payload_length = view.read_u32_le();
    if (!kind.ok() || !stream.ok() || !timestamp.ok() || !sequence.ok() || !payload_length.ok()) {
      return Status(ErrorCode::end_of_input, "record header is truncated");
    }
    if (payload_length.value() > 64 * 1024 * 1024) {
      return Status(ErrorCode::malformed_record, "record payload exceeds sanity limit");
    }
    auto payload = view.read_bytes(payload_length.value());
    auto expected_checksum = view.read_u32_le();
    if (!payload.ok() || !expected_checksum.ok()) {
      return Status(ErrorCode::end_of_input, "record payload is truncated");
    }
    if (crc32(payload.value()) != expected_checksum.value()) {
      return Status(ErrorCode::checksum_mismatch, "record payload checksum failed");
    }

    Record record;
    record.kind = record_kind_from_wire(kind.value());
    record.stream_id = stream.value();
    record.timestamp_ns = timestamp.value();
    record.sequence = sequence.value();
    record.payload = std::move(payload.value());
    recording.records.push_back(std::move(record));
  }

  return recording;
}

} // namespace mercury::formats
