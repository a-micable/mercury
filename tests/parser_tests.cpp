#include "test_support.hpp"

namespace {

const mercury::test::Register parser_roundtrip("parser roundtrip", [] {
  auto bytes = mercury::Serializer().write_recording(mercury::test::fixture_recording());
  auto parsed = mercury::make_default_registry().parse(bytes);
  mercury::test::require(parsed.ok(), "expected parser to accept serializer output");
  mercury::test::require(parsed.value().records.size() == 3, "expected three records");
  mercury::test::require(parsed.value().records[1].payload.size() == 3, "payload was not preserved");
});

const mercury::test::Register parser_rejects_checksum("parser rejects checksum mismatch", [] {
  auto bytes = mercury::Serializer().write_recording(mercury::test::fixture_recording());
  bytes[bytes.size() - 1] ^= 0xff;
  auto parsed = mercury::make_default_registry().parse(bytes);
  mercury::test::require(!parsed.ok(), "corrupted payload should not parse");
  mercury::test::require(parsed.status().code() == mercury::ErrorCode::checksum_mismatch,
                         "expected checksum mismatch");
});

} // namespace
