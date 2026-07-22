#include "tool_common.hpp"

namespace {

std::string issue_kind_name(mercury::RecoveryIssueKind kind) {
  switch (kind) {
  case mercury::RecoveryIssueKind::skipped_prefix:
    return "skipped_prefix";
  case mercury::RecoveryIssueKind::invalid_version:
    return "invalid_version";
  case mercury::RecoveryIssueKind::malformed_metadata:
    return "malformed_metadata";
  case mercury::RecoveryIssueKind::malformed_record_header:
    return "malformed_record_header";
  case mercury::RecoveryIssueKind::payload_too_large:
    return "payload_too_large";
  case mercury::RecoveryIssueKind::truncated_payload:
    return "truncated_payload";
  case mercury::RecoveryIssueKind::checksum_mismatch:
    return "checksum_mismatch";
  case mercury::RecoveryIssueKind::trailing_bytes:
    return "trailing_bytes";
  }
  return "unknown";
}

} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: mercury-recover <recording>\n";
    return 2;
  }

  auto bytes = mercury::tool::read_file(argv[1]);
  if (!bytes.ok()) {
    return mercury::tool::print_error(bytes.status());
  }

  mercury::RecoveryOptions options;
  options.keep_checksum_failures = false;
  const auto recovered = mercury::RecoveryScanner().recover(bytes.value(), options);

  std::cout << "format=" << recovered.recording.format << " version=" << recovered.recording.version
            << " records=" << recovered.recording.records.size()
            << " bytes_consumed=" << recovered.bytes_consumed << '\n';
  for (const auto& issue : recovered.issues) {
    std::cout << issue_kind_name(issue.kind) << " offset=" << issue.offset
              << " code=" << mercury::to_string(issue.code) << " message=\"" << issue.message << "\"\n";
  }

  return recovered.recovered_anything() ? 0 : 1;
}
