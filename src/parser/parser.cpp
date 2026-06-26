#include "mercury/mercury.hpp"

#include "formats/framed_format.hpp"

#include <algorithm>

namespace mercury {

void ParserRegistry::register_parser(std::unique_ptr<Parser> parser) {
  parsers_.push_back(std::move(parser));
}

const Parser* ParserRegistry::detect(std::span<const std::uint8_t> bytes) const {
  for (const auto& parser : parsers_) {
    if (parser->can_parse(bytes)) {
      return parser.get();
    }
  }
  return nullptr;
}

Result<Recording> ParserRegistry::parse(std::span<const std::uint8_t> bytes) const {
  const Parser* parser = detect(bytes);
  if (parser == nullptr) {
    return Status(ErrorCode::unknown_format, "no registered parser accepted the input");
  }
  return parser->parse(bytes);
}

std::vector<std::string> ParserRegistry::names() const {
  std::vector<std::string> out;
  out.reserve(parsers_.size());
  for (const auto& parser : parsers_) {
    out.emplace_back(parser->name());
  }
  std::sort(out.begin(), out.end());
  return out;
}

ParserRegistry make_default_registry() {
  ParserRegistry registry;
  registry.register_parser(std::make_unique<formats::FramedParser>(
      formats::FormatDescriptor{"MRF1", {'M', 'R', 'F', '1'}, 1, 3}));
  registry.register_parser(std::make_unique<formats::FramedParser>(
      formats::FormatDescriptor{"MRF2", {'M', 'R', 'F', '2'}, 2, 6}));
  registry.register_parser(std::make_unique<formats::FramedParser>(
      formats::FormatDescriptor{"Legacy", {'L', 'G', 'R', '0'}, 1, 2}));
  registry.register_parser(std::make_unique<formats::FramedParser>(
      formats::FormatDescriptor{"Compact", {'C', 'E', 'R', '0'}, 1, 1}));
  registry.register_parser(std::make_unique<formats::FramedParser>(
      formats::FormatDescriptor{"StreamCapture", {'S', 'C', 'A', 'P'}, 1, 4}));
  return registry;
}

} // namespace mercury
