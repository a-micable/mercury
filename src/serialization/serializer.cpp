#include "mercury/mercury.hpp"

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

void append_string(std::vector<std::uint8_t>& out, const std::string& value) {
  append_u16(out, static_cast<std::uint16_t>(value.size()));
  out.insert(out.end(), value.begin(), value.end());
}

std::uint16_t wire_kind(RecordKind kind) {
  return static_cast<std::uint16_t>(kind == RecordKind::unknown ? RecordKind::data : kind);
}

} // namespace

std::vector<std::uint8_t> Serializer::write_recording(const Recording& recording) const {
  std::vector<std::uint8_t> out;
  const std::string magic = recording.format == "MRF2" ? "MRF2" : "MRF1";
  out.insert(out.end(), magic.begin(), magic.end());
  append_u16(out, recording.version == 0 ? 1 : recording.version);
  append_u16(out, static_cast<std::uint16_t>(recording.metadata.size()));
  for (const auto& [key, value] : recording.metadata) {
    append_string(out, key);
    if (const auto* str = std::get_if<std::string>(&value.value)) {
      append_string(out, *str);
    } else {
      append_string(out, "<typed>");
    }
  }
  append_u32(out, static_cast<std::uint32_t>(recording.records.size()));
  for (const auto& record : recording.records) {
    append_u16(out, wire_kind(record.kind));
    append_u16(out, record.stream_id);
    append_u64(out, record.timestamp_ns);
    append_u32(out, record.sequence);
    append_u32(out, static_cast<std::uint32_t>(record.payload.size()));
    out.insert(out.end(), record.payload.begin(), record.payload.end());
    append_u32(out, crc32(record.payload));
  }
  return out;
}

Result<Recording> Serializer::read_recording(std::span<const std::uint8_t> bytes) const {
  auto registry = make_default_registry();
  return registry.parse(bytes);
}

} // namespace mercury
