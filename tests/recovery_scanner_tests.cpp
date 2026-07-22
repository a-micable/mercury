#include "test_support.hpp"

namespace {

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value & 0xff));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int shift = 0; shift < 64; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

void append_string(std::vector<std::uint8_t>& out, std::string_view value) {
  append_u16(out, static_cast<std::uint16_t>(value.size()));
  out.insert(out.end(), value.begin(), value.end());
}

void append_record(std::vector<std::uint8_t>& out,
                   mercury::RecordKind kind,
                   std::uint16_t stream,
                   std::uint64_t timestamp,
                   std::uint32_t sequence,
                   std::vector<std::uint8_t> payload,
                   bool valid_checksum = true) {
  append_u16(out, static_cast<std::uint16_t>(kind));
  append_u16(out, stream);
  append_u64(out, timestamp);
  append_u32(out, sequence);
  append_u32(out, static_cast<std::uint32_t>(payload.size()));
  out.insert(out.end(), payload.begin(), payload.end());
  append_u32(out, valid_checksum ? mercury::crc32(payload) : mercury::crc32(payload) + 1);
}

std::vector<std::uint8_t> framed_fixture(bool valid_second_checksum = true) {
  std::vector<std::uint8_t> bytes{'M', 'R', 'F', '1'};
  append_u16(bytes, 1);
  append_u16(bytes, 1);
  append_string(bytes, "source");
  append_string(bytes, "fixture");
  append_u32(bytes, 2);
  append_record(bytes, mercury::RecordKind::data, 2, 100, 1, {1, 2, 3});
  append_record(bytes, mercury::RecordKind::event, 2, 200, 2, {'o', 'k'}, valid_second_checksum);
  return bytes;
}

const mercury::test::Register recovery_scanner_recovers_clean_recording("recovery scanner recovers clean recording", [] {
  const auto recovered = mercury::RecoveryScanner().recover(framed_fixture());

  mercury::test::require(recovered.clean(), "clean fixture should not produce recovery issues");
  mercury::test::require(recovered.recording.format == "MRF1", "recovery should identify format");
  mercury::test::require(recovered.recording.metadata.size() == 1, "recovery should preserve metadata");
  mercury::test::require(recovered.recording.records.size() == 2, "recovery should preserve records");
  mercury::test::require(recovered.recording.records.back().payload.size() == 2,
                         "recovery should preserve payload bytes");
});

const mercury::test::Register recovery_scanner_skips_prefix("recovery scanner skips prefix", [] {
  auto bytes = framed_fixture();
  bytes.insert(bytes.begin(), {0, 1, 2, 3, 4});

  const auto recovered = mercury::RecoveryScanner().recover(bytes);

  mercury::test::require(recovered.recording.records.size() == 2, "recovery should find recording after prefix");
  mercury::test::require(!recovered.clean(), "skipped prefix should be reported");
  mercury::test::require(recovered.issues.front().kind == mercury::RecoveryIssueKind::skipped_prefix,
                         "first issue should report skipped prefix");
});

const mercury::test::Register recovery_scanner_drops_bad_checksums("recovery scanner drops bad checksums", [] {
  const auto recovered = mercury::RecoveryScanner().recover(framed_fixture(false));

  mercury::test::require(recovered.recording.records.size() == 1, "bad checksum record should be dropped");
  mercury::test::require(recovered.issues.size() == 1, "bad checksum should create one issue");
  mercury::test::require(recovered.issues.front().kind == mercury::RecoveryIssueKind::checksum_mismatch,
                         "issue should report checksum mismatch");
});

const mercury::test::Register recovery_scanner_can_keep_bad_checksums("recovery scanner can keep bad checksums", [] {
  mercury::RecoveryOptions options;
  options.keep_checksum_failures = true;

  const auto recovered = mercury::RecoveryScanner().recover(framed_fixture(false), options);

  mercury::test::require(recovered.recording.records.size() == 2, "option should keep checksum failures");
  mercury::test::require(recovered.issues.size() == 1, "kept checksum failure should still be reported");
});

const mercury::test::Register recovery_scanner_reports_truncation("recovery scanner reports truncation", [] {
  auto bytes = framed_fixture();
  bytes.resize(bytes.size() - 3);

  const auto recovered = mercury::RecoveryScanner().recover(bytes);

  mercury::test::require(recovered.recording.records.size() == 1, "records before truncation should survive");
  mercury::test::require(!recovered.issues.empty(), "truncation should create an issue");
  mercury::test::require(recovered.issues.front().kind == mercury::RecoveryIssueKind::truncated_payload,
                         "issue should report truncated payload");
});

} // namespace
