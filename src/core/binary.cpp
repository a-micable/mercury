#include "mercury/core.hpp"

#include <algorithm>

namespace mercury {

BinaryView::BinaryView(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

std::size_t BinaryView::remaining() const {
  return bytes_.size() - offset_;
}

std::size_t BinaryView::offset() const {
  return offset_;
}

bool BinaryView::empty() const {
  return remaining() == 0;
}

std::span<const std::uint8_t> BinaryView::tail() const {
  return bytes_.subspan(offset_);
}

Result<std::uint8_t> BinaryView::read_u8() {
  if (remaining() < 1) {
    return Status(ErrorCode::end_of_input, "expected one byte");
  }
  return bytes_[offset_++];
}

Result<std::uint16_t> BinaryView::read_u16_le() {
  auto bytes = read_bytes(2);
  if (!bytes.ok()) {
    return bytes.status();
  }
  return static_cast<std::uint16_t>(bytes.value()[0] | (bytes.value()[1] << 8));
}

Result<std::uint32_t> BinaryView::read_u32_le() {
  auto bytes = read_bytes(4);
  if (!bytes.ok()) {
    return bytes.status();
  }
  std::uint32_t value = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    value |= static_cast<std::uint32_t>(bytes.value()[i]) << (i * 8);
  }
  return value;
}

Result<std::uint64_t> BinaryView::read_u64_le() {
  auto bytes = read_bytes(8);
  if (!bytes.ok()) {
    return bytes.status();
  }
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < 8; ++i) {
    value |= static_cast<std::uint64_t>(bytes.value()[i]) << (i * 8);
  }
  return value;
}

Result<std::vector<std::uint8_t>> BinaryView::read_bytes(std::size_t length) {
  if (remaining() < length) {
    return Status(ErrorCode::end_of_input, "requested byte range exceeds input");
  }
  std::vector<std::uint8_t> out(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
                               bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + length));
  offset_ += length;
  return out;
}

Result<std::string> BinaryView::read_string(std::size_t length) {
  auto bytes = read_bytes(length);
  if (!bytes.ok()) {
    return bytes.status();
  }
  return std::string(bytes.value().begin(), bytes.value().end());
}

Status BinaryView::skip(std::size_t length) {
  if (remaining() < length) {
    return Status(ErrorCode::end_of_input, "skip exceeds input");
  }
  offset_ += length;
  return Status::Ok();
}

RecordKind record_kind_from_wire(std::uint16_t value) {
  switch (value) {
  case 1:
    return RecordKind::data;
  case 2:
    return RecordKind::event;
  case 3:
    return RecordKind::metadata;
  case 4:
    return RecordKind::index;
  case 5:
    return RecordKind::heartbeat;
  case 6:
    return RecordKind::attachment;
  default:
    return RecordKind::unknown;
  }
}

std::string to_string(RecordKind kind) {
  switch (kind) {
  case RecordKind::data:
    return "data";
  case RecordKind::event:
    return "event";
  case RecordKind::metadata:
    return "metadata";
  case RecordKind::index:
    return "index";
  case RecordKind::heartbeat:
    return "heartbeat";
  case RecordKind::attachment:
    return "attachment";
  case RecordKind::unknown:
    return "unknown";
  }
  return "unknown";
}

} // namespace mercury
