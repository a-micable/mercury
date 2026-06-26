#include "test_support.hpp"

namespace {

const mercury::test::Register serialization_roundtrip("serialization roundtrip", [] {
  mercury::Serializer serializer;
  auto bytes = serializer.write_recording(mercury::test::fixture_recording());
  auto parsed = serializer.read_recording(bytes);
  mercury::test::require(parsed.ok(), "serializer output should parse");
  mercury::test::require(parsed.value().metadata.contains("airframe"), "metadata missing");
});

const mercury::test::Register crc64_standard_vector("crc64 ecma standard vector", [] {
  const std::array<std::uint8_t, 9> bytes{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  mercury::test::require(mercury::crc64_ecma(bytes) == 0x6c40df5f0b497347ull,
                         "CRC64-ECMA did not match the standard vector");
});

const mercury::test::Register sha256_standard_vectors("sha256 standard vectors", [] {
  const std::array<std::uint8_t, 0> empty{};
  const auto empty_digest = mercury::sha256(empty);
  mercury::test::require(
      mercury::hex_digest(empty_digest) ==
          "e3b0c44298fc1c149afbf4c8996fb924"
          "27ae41e4649b934ca495991b7852b855",
      "SHA-256 empty digest mismatch");

  const std::array<std::uint8_t, 3> abc{'a', 'b', 'c'};
  const auto abc_digest = mercury::sha256(abc);
  mercury::test::require(
      mercury::hex_digest(abc_digest) ==
          "ba7816bf8f01cfea414140de5dae2223"
          "b00361a396177a9cb410ff61f20015ad",
      "SHA-256 abc digest mismatch");
});

} // namespace
