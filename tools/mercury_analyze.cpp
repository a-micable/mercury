#include "tool_common.hpp"

#include <iomanip>

namespace {

void print_streams(const mercury::RecordingAnalysis& analysis) {
  std::cout << "streams:\n";
  for (const auto& stream : analysis.streams) {
    std::cout << "  stream=" << stream.stream_id << " records=" << stream.records
              << " payload_bytes=" << stream.payload_bytes << " first_ns=" << stream.first_timestamp_ns
              << " last_ns=" << stream.last_timestamp_ns << " first_seq=" << stream.first_sequence
              << " last_seq=" << stream.last_sequence
              << " monotonic=" << (stream.timestamps_monotonic ? "yes" : "no")
              << " contiguous=" << (stream.sequences_contiguous ? "yes" : "no") << '\n';
  }
}

void print_kinds(const mercury::RecordingAnalysis& analysis) {
  std::cout << "kinds:\n";
  for (const auto& [kind, count] : analysis.records_by_kind) {
    std::cout << "  " << mercury::to_string(kind) << "=" << count << '\n';
  }
}

void print_warnings(const mercury::RecordingAnalysis& analysis) {
  if (analysis.warnings.empty()) {
    std::cout << "warnings=0\n";
    return;
  }

  std::cout << "warnings=" << analysis.warnings.size() << '\n';
  for (const auto& warning : analysis.warnings) {
    std::cout << "  " << warning << '\n';
  }
}

} // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    std::cerr << "usage: mercury-analyze <recording> [gap-threshold-ns]\n";
    return 2;
  }

  mercury::RecordingAnalysisOptions options;
  if (argc == 3) {
    try {
      options.timeline_gap_threshold_ns = std::stoull(argv[2]);
    } catch (const std::exception&) {
      std::cerr << "error: gap threshold must be an unsigned integer\n";
      return 2;
    }
  }

  auto bytes = mercury::tool::read_file(argv[1]);
  if (!bytes.ok()) {
    return mercury::tool::print_error(bytes.status());
  }

  auto parsed = mercury::make_default_registry().parse(bytes.value());
  if (!parsed.ok()) {
    return mercury::tool::print_error(parsed.status());
  }

  const auto analysis = mercury::RecordingAnalyzer().analyze(parsed.value(), options);
  std::cout << "records=" << analysis.records << '\n';
  std::cout << "payload_bytes=" << analysis.payload_bytes << '\n';
  std::cout << "first_ns=" << analysis.first_timestamp_ns << '\n';
  std::cout << "last_ns=" << analysis.last_timestamp_ns << '\n';
  print_kinds(analysis);
  print_streams(analysis);
  print_warnings(analysis);
  return analysis.warnings.empty() ? 0 : 3;
}
