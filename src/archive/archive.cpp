#include "mercury/mercury.hpp"

#include <algorithm>

namespace mercury {

namespace {

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value & 0xffu));
  out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int i = 0; i < 4; ++i) {
    out.push_back(static_cast<std::uint8_t>((value >> (i * 8)) & 0xffu));
  }
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<std::uint8_t>((value >> (i * 8)) & 0xffu));
  }
}

} // namespace

Result<std::vector<ArchiveMember>> ArchiveReader::scan(std::span<const std::uint8_t> bytes) const {
  BinaryView view(bytes);
  auto magic = view.read_bytes(4);
  if (!magic.ok() || std::string(magic.value().begin(), magic.value().end()) != "MBND") {
    return Status(ErrorCode::invalid_magic, "archive bundle magic mismatch");
  }

  auto count = view.read_u32_le();
  if (!count.ok()) {
    return count.status();
  }
  if (count.value() > 100000) {
    return Status(ErrorCode::malformed_record, "archive member count exceeds sanity limit");
  }

  std::vector<ArchiveMember> members;
  members.reserve(count.value());
  for (std::uint32_t i = 0; i < count.value(); ++i) {
    auto name_length = view.read_u16_le();
    if (!name_length.ok()) {
      return name_length.status();
    }
    auto name = view.read_string(name_length.value());
    auto offset = view.read_u64_le();
    auto size = view.read_u64_le();
    auto checksum = view.read_u32_le();
    if (!name.ok() || !offset.ok() || !size.ok() || !checksum.ok()) {
      return Status(ErrorCode::end_of_input, "archive table is truncated");
    }
    if (offset.value() + size.value() > bytes.size()) {
      return Status(ErrorCode::malformed_record, "archive member points outside bundle");
    }
    auto member_bytes = bytes.subspan(static_cast<std::size_t>(offset.value()), static_cast<std::size_t>(size.value()));
    if (crc32(member_bytes) != checksum.value()) {
      return Status(ErrorCode::checksum_mismatch, "archive member checksum failed");
    }
    members.push_back(ArchiveMember{name.value(), offset.value(), size.value(), checksum.value()});
  }
  return members;
}

Result<std::vector<std::uint8_t>> ArchiveReader::extract(std::span<const std::uint8_t> bytes, std::string_view name) const {
  auto members = scan(bytes);
  if (!members.ok()) {
    return members.status();
  }
  for (const auto& member : members.value()) {
    if (member.name == name) {
      auto span = bytes.subspan(static_cast<std::size_t>(member.offset), static_cast<std::size_t>(member.size));
      return std::vector<std::uint8_t>(span.begin(), span.end());
    }
  }
  return Status(ErrorCode::malformed_record, "archive member was not found");
}

void ArchiveWriter::add(std::string name, std::vector<std::uint8_t> bytes) {
  members_.push_back({std::move(name), std::move(bytes)});
}

std::vector<std::uint8_t> ArchiveWriter::finish() const {
  std::vector<std::uint8_t> header{'M', 'B', 'N', 'D'};
  append_u32(header, static_cast<std::uint32_t>(members_.size()));

  std::uint64_t table_size = header.size();
  for (const auto& [name, bytes] : members_) {
    table_size += 2 + name.size() + 8 + 8 + 4;
  }

  std::vector<std::uint8_t> table = header;
  std::vector<std::uint8_t> payload;
  std::uint64_t offset = table_size;
  for (const auto& [name, bytes] : members_) {
    append_u16(table, static_cast<std::uint16_t>(name.size()));
    table.insert(table.end(), name.begin(), name.end());
    append_u64(table, offset);
    append_u64(table, bytes.size());
    append_u32(table, crc32(bytes));
    payload.insert(payload.end(), bytes.begin(), bytes.end());
    offset += bytes.size();
  }
  table.insert(table.end(), payload.begin(), payload.end());
  return table;
}

} // namespace mercury
