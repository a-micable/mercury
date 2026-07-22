#include "mercury/mercury.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace mercury {

namespace {

bool valid_component(std::string_view value) {
  if (value.empty() || value == "." || value == "..") {
    return false;
  }
  for (const char character : value) {
    const bool safe = (character >= 'a' && character <= 'z') ||
                      (character >= 'A' && character <= 'Z') ||
                      (character >= '0' && character <= '9') ||
                      character == '-' || character == '_' || character == '.';
    if (!safe) {
      return false;
    }
  }
  return true;
}

Status filesystem_error(const std::string& action,
                         const std::error_code& error) {
  return Status(ErrorCode::io_error, action + ": " + error.message());
}

Result<std::vector<std::uint8_t>> read_bytes(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return Status(ErrorCode::io_error, "failed to open recording " + path.string());
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), {});
}

} // namespace

RecordingStore::RecordingStore(std::filesystem::path root, std::string tenant)
    : root_(std::move(root)), tenant_(std::move(tenant)) {}

Status RecordingStore::initialize() const {
  if (!valid_component(tenant_)) {
    return Status(ErrorCode::invalid_configuration,
                  "tenant must be a non-empty path-safe identifier");
  }
  std::error_code error;
  std::filesystem::create_directories(root_ / tenant_, error);
  if (error) {
    return filesystem_error("failed to initialize recording store", error);
  }
  return Status::Ok();
}

Result<std::filesystem::path> RecordingStore::path_for(std::string_view id) const {
  if (!valid_component(id)) {
    return Status(ErrorCode::invalid_configuration,
                  "recording id must be a non-empty path-safe identifier");
  }
  return root_ / tenant_ / (std::string(id) + ".mrf");
}

Status RecordingStore::put(std::string_view id, const Recording& recording) const {
  auto destination = path_for(id);
  if (!destination.ok()) {
    return destination.status();
  }
  auto initialized = initialize();
  if (!initialized.ok()) {
    return initialized;
  }

  const auto bytes = Serializer().write_recording(recording);
  const auto temporary = destination.value().string() + ".tmp";
  {
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
      return Status(ErrorCode::io_error,
                    "failed to create temporary recording " + temporary);
    }
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    if (!output) {
      return Status(ErrorCode::io_error,
                    "failed to write temporary recording " + temporary);
    }
  }

  std::error_code error;
  std::filesystem::rename(temporary, destination.value(), error);
  if (error) {
    std::filesystem::remove(temporary);
    return filesystem_error("failed to publish recording", error);
  }
  return Status::Ok();
}

Result<Recording> RecordingStore::get(std::string_view id) const {
  auto path = path_for(id);
  if (!path.ok()) {
    return path.status();
  }
  auto bytes = read_bytes(path.value());
  if (!bytes.ok()) {
    return bytes.status();
  }
  return Serializer().read_recording(bytes.value());
}

Result<RecordingSummary> RecordingStore::summarize(
    std::string_view id, std::span<const std::uint8_t> bytes) const {
  auto recording = Serializer().read_recording(bytes);
  if (!recording.ok()) {
    return recording.status();
  }
  RecordingSummary summary;
  summary.tenant = tenant_;
  summary.id = std::string(id);
  summary.format = recording.value().format;
  summary.version = recording.value().version;
  summary.records = recording.value().records.size();
  summary.serialized_bytes = bytes.size();
  for (const auto& record : recording.value().records) {
    summary.payload_bytes += record.payload.size();
  }
  return summary;
}

Result<std::vector<RecordingSummary>> RecordingStore::list() const {
  auto initialized = initialize();
  if (!initialized.ok()) {
    return initialized;
  }
  std::vector<RecordingSummary> summaries;
  std::error_code error;
  for (const auto& entry : std::filesystem::directory_iterator(
           root_ / tenant_, std::filesystem::directory_options::skip_permission_denied,
           error)) {
    if (error) {
      return filesystem_error("failed to enumerate recording store", error);
    }
    if (!entry.is_regular_file() || entry.path().extension() != ".mrf") {
      continue;
    }
    auto bytes = read_bytes(entry.path());
    if (!bytes.ok()) {
      return bytes.status();
    }
    auto summary = summarize(entry.path().stem().string(), bytes.value());
    if (!summary.ok()) {
      return summary.status();
    }
    summaries.push_back(std::move(summary.value()));
  }
  std::sort(summaries.begin(), summaries.end(),
            [](const auto& left, const auto& right) { return left.id < right.id; });
  return summaries;
}

Status RecordingStore::erase(std::string_view id) const {
  auto path = path_for(id);
  if (!path.ok()) {
    return path.status();
  }
  std::error_code error;
  const bool removed = std::filesystem::remove(path.value(), error);
  if (error) {
    return filesystem_error("failed to remove recording", error);
  }
  if (!removed) {
    return Status(ErrorCode::io_error, "recording was not found");
  }
  return Status::Ok();
}

} // namespace mercury