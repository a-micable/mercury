#include "test_support.hpp"

namespace {

const mercury::test::Register diagnostics_collect_findings("diagnostics collect findings", [] {
  mercury::DiagnosticCollector diagnostics;

  diagnostics.note("parser", "record count accepted", 12);
  diagnostics.warning(mercury::ErrorCode::malformed_record, "parser", "optional extension skipped", 48);
  diagnostics.error(mercury::ErrorCode::checksum_mismatch, "archive", "member checksum mismatch", 128);

  mercury::test::require(diagnostics.size() == 3, "diagnostic collector should retain every finding");
  mercury::test::require(diagnostics.has_errors(), "diagnostic collector should report errors");
  mercury::test::require(diagnostics.entries()[1].severity == mercury::DiagnosticSeverity::warning,
                         "warning severity should be preserved");
  mercury::test::require(diagnostics.entries()[2].offset == 128, "diagnostic offset should be preserved");
});

const mercury::test::Register diagnostics_clear_and_format("diagnostics clear and format", [] {
  mercury::DiagnosticCollector diagnostics;
  diagnostics.error(mercury::ErrorCode::invalid_magic, "format", "invalid magic");
  diagnostics.clear();

  mercury::test::require(diagnostics.size() == 0, "clear should remove diagnostics");
  mercury::test::require(!diagnostics.has_errors(), "empty diagnostics should not report errors");
  mercury::test::require(mercury::to_string(mercury::DiagnosticSeverity::note) == "note",
                         "note severity should format");
  mercury::test::require(mercury::to_string(mercury::DiagnosticSeverity::warning) == "warning",
                         "warning severity should format");
  mercury::test::require(mercury::to_string(mercury::DiagnosticSeverity::error) == "error",
                         "error severity should format");
});

} // namespace
