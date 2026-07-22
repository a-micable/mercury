#include "test_support.hpp"

namespace {

class TestParser final : public mercury::Parser {
public:
  explicit TestParser(std::string parser_name) : name_(std::move(parser_name)) {}

  std::string_view name() const override { return name_; }
  bool can_parse(std::span<const std::uint8_t>) const override { return false; }
  mercury::Result<mercury::Recording> parse(
      std::span<const std::uint8_t>) const override {
    return mercury::Status(mercury::ErrorCode::unknown_format,
                            "test parser does not parse input");
  }

private:
  std::string name_;
};

const mercury::test::Register parser_registry_ignores_null_and_duplicates(
    "parser registry ignores null and duplicate registrations", [] {
      mercury::ParserRegistry registry;
      registry.register_parser(nullptr);
      registry.register_parser(std::make_unique<TestParser>("custom"));
      registry.register_parser(std::make_unique<TestParser>("custom"));

      const auto names = registry.names();
      mercury::test::require(names.size() == 1,
                             "registry should contain one unique parser");
      mercury::test::require(names.front() == "custom",
                             "registry should preserve the first parser");
    });

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
