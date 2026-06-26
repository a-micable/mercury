#include "mercury/mercury.hpp"

#include <algorithm>

namespace mercury {

namespace {

class StoredCodec final : public CompressionCodec {
public:
  std::string_view name() const override { return "stored"; }

  Result<std::vector<std::uint8_t>> decompress(std::span<const std::uint8_t> bytes) const override {
    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
  }

  Result<std::vector<std::uint8_t>> compress(std::span<const std::uint8_t> bytes) const override {
    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
  }
};

class RleCodec final : public CompressionCodec {
public:
  std::string_view name() const override { return "rle"; }

  Result<std::vector<std::uint8_t>> decompress(std::span<const std::uint8_t> bytes) const override {
    if (bytes.size() % 2 != 0) {
      return Status(ErrorCode::compression_error, "rle stream has an odd number of bytes");
    }
    std::vector<std::uint8_t> out;
    for (std::size_t i = 0; i < bytes.size(); i += 2) {
      out.insert(out.end(), bytes[i], bytes[i + 1]);
      if (out.size() > 64 * 1024 * 1024) {
        return Status(ErrorCode::compression_error, "rle expansion exceeds sanity limit");
      }
    }
    return out;
  }

  Result<std::vector<std::uint8_t>> compress(std::span<const std::uint8_t> bytes) const override {
    std::vector<std::uint8_t> out;
    for (std::size_t i = 0; i < bytes.size();) {
      const std::uint8_t value = bytes[i];
      std::uint8_t run = 1;
      while (i + run < bytes.size() && bytes[i + run] == value && run < 255) {
        ++run;
      }
      out.push_back(run);
      out.push_back(value);
      i += run;
    }
    return out;
  }
};

} // namespace

void CodecRegistry::register_codec(std::unique_ptr<CompressionCodec> codec) {
  codecs_.push_back(std::move(codec));
}

const CompressionCodec* CodecRegistry::find(std::string_view name) const {
  for (const auto& codec : codecs_) {
    if (codec->name() == name) {
      return codec.get();
    }
  }
  return nullptr;
}

std::vector<std::string> CodecRegistry::names() const {
  std::vector<std::string> out;
  out.reserve(codecs_.size());
  for (const auto& codec : codecs_) {
    out.emplace_back(codec->name());
  }
  std::sort(out.begin(), out.end());
  return out;
}

CodecRegistry make_default_codecs() {
  CodecRegistry registry;
  registry.register_codec(std::make_unique<StoredCodec>());
  registry.register_codec(std::make_unique<RleCodec>());
  return registry;
}

} // namespace mercury
