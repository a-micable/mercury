#include "test_support.hpp"

namespace {

const mercury::test::Register config_parse("config parse", [] {
  auto parsed = mercury::ConfigLoader().parse("threads = 4\n# comment\ncodec=rle\n");
  mercury::test::require(parsed.ok(), "config should parse");
  mercury::test::require(parsed.value().get("threads") == "4", "threads mismatch");
  mercury::test::require(parsed.value().get("missing", "fallback") == "fallback", "fallback mismatch");
});

const mercury::test::Register config_rejects_bad_line("config rejects bad line", [] {
  auto parsed = mercury::ConfigLoader().parse("threads\n");
  mercury::test::require(!parsed.ok(), "config should reject missing equals");
});

} // namespace
