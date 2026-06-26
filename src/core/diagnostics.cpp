#include "mercury/core.hpp"

#include <algorithm>

namespace mercury {

void DiagnosticCollector::add(Diagnostic diagnostic) {
  entries_.push_back(std::move(diagnostic));
}

void DiagnosticCollector::note(std::string component, std::string message, std::uint64_t offset) {
  add(Diagnostic{DiagnosticSeverity::note, ErrorCode::ok, std::move(component), std::move(message), offset});
}

void DiagnosticCollector::warning(ErrorCode code, std::string component, std::string message, std::uint64_t offset) {
  add(Diagnostic{DiagnosticSeverity::warning, code, std::move(component), std::move(message), offset});
}

void DiagnosticCollector::error(ErrorCode code, std::string component, std::string message, std::uint64_t offset) {
  add(Diagnostic{DiagnosticSeverity::error, code, std::move(component), std::move(message), offset});
}

bool DiagnosticCollector::has_errors() const {
  return std::any_of(entries_.begin(), entries_.end(), [](const Diagnostic& diagnostic) {
    return diagnostic.severity == DiagnosticSeverity::error;
  });
}

std::size_t DiagnosticCollector::size() const {
  return entries_.size();
}

const std::vector<Diagnostic>& DiagnosticCollector::entries() const {
  return entries_;
}

void DiagnosticCollector::clear() {
  entries_.clear();
}

std::string to_string(DiagnosticSeverity severity) {
  switch (severity) {
  case DiagnosticSeverity::note:
    return "note";
  case DiagnosticSeverity::warning:
    return "warning";
  case DiagnosticSeverity::error:
    return "error";
  }
  return "unknown";
}

} // namespace mercury
