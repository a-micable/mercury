#include "mercury/mercury.hpp"

namespace mercury {

void PluginHost::register_parser(std::unique_ptr<Parser> parser) {
  parsers_.register_parser(std::move(parser));
}

void PluginHost::register_codec(std::unique_ptr<CompressionCodec> codec) {
  codecs_.register_codec(std::move(codec));
}

ParserRegistry& PluginHost::parsers() {
  return parsers_;
}

CodecRegistry& PluginHost::codecs() {
  return codecs_;
}

} // namespace mercury
