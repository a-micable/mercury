#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace mercury {

enum class ErrorCode {
  ok,
  end_of_input,
  invalid_magic,
  unsupported_version,
  malformed_record,
  checksum_mismatch,
  compression_error,
  io_error,
  unknown_format,
  invalid_configuration,
};

class Status {
public:
  Status();
  Status(ErrorCode code, std::string message);

  [[nodiscard]] bool ok() const;
  [[nodiscard]] ErrorCode code() const;
  [[nodiscard]] const std::string& message() const;

  static Status Ok();

private:
  ErrorCode code_;
  std::string message_;
};

template <typename T>
class Result {
public:
  Result(T value) : value_(std::move(value)) {}
  Result(Status status) : value_(std::move(status)) {}

  [[nodiscard]] bool ok() const { return std::holds_alternative<T>(value_); }
  [[nodiscard]] const T& value() const { return std::get<T>(value_); }
  [[nodiscard]] T& value() { return std::get<T>(value_); }
  [[nodiscard]] const Status& status() const { return std::get<Status>(value_); }

private:
  std::variant<T, Status> value_;
};

class BinaryView {
public:
  explicit BinaryView(std::span<const std::uint8_t> bytes);

  [[nodiscard]] std::size_t remaining() const;
  [[nodiscard]] std::size_t offset() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] std::span<const std::uint8_t> tail() const;

  Result<std::uint8_t> read_u8();
  Result<std::uint16_t> read_u16_le();
  Result<std::uint32_t> read_u32_le();
  Result<std::uint64_t> read_u64_le();
  Result<std::vector<std::uint8_t>> read_bytes(std::size_t length);
  Result<std::string> read_string(std::size_t length);
  Status skip(std::size_t length);

private:
  std::span<const std::uint8_t> bytes_;
  std::size_t offset_ = 0;
};

struct MetadataValue {
  using Value = std::variant<std::string, std::int64_t, double, bool>;
  Value value;
};

using Metadata = std::map<std::string, MetadataValue>;

enum class RecordKind : std::uint16_t {
  data = 1,
  event = 2,
  metadata = 3,
  index = 4,
  heartbeat = 5,
  attachment = 6,
  unknown = 0xffff,
};

struct Record {
  RecordKind kind = RecordKind::unknown;
  std::uint16_t stream_id = 0;
  std::uint64_t timestamp_ns = 0;
  std::uint32_t sequence = 0;
  std::vector<std::uint8_t> payload;
  Metadata metadata;
};

struct Recording {
  std::string format;
  std::uint16_t version = 0;
  Metadata metadata;
  std::vector<Record> records;
};

struct IndexEntry {
  std::uint64_t timestamp_ns = 0;
  std::uint64_t file_offset = 0;
  std::uint32_t sequence = 0;
  std::uint16_t stream_id = 0;
};

struct TimelineEvent {
  std::uint64_t timestamp_ns = 0;
  std::uint16_t stream_id = 0;
  RecordKind kind = RecordKind::unknown;
  std::string label;
};

std::uint32_t crc32(std::span<const std::uint8_t> bytes);
std::uint64_t crc64_ecma(std::span<const std::uint8_t> bytes);
std::uint32_t fnv1a32(std::span<const std::uint8_t> bytes);
std::array<std::uint8_t, 32> sha256(std::span<const std::uint8_t> bytes);
std::string hex_digest(std::span<const std::uint8_t> bytes);

std::string to_string(ErrorCode code);
std::string to_string(RecordKind kind);
RecordKind record_kind_from_wire(std::uint16_t value);

class Logger {
public:
  enum class Level { trace, debug, info, warn, error };
  using Sink = std::function<void(Level, std::string_view)>;

  static Logger& instance();
  void set_sink(Sink sink);
  void log(Level level, std::string_view message);

private:
  Sink sink_;
};

} // namespace mercury
