#pragma once

#include "mercury/mercury.hpp"

#include <array>

namespace mercury::formats {

struct FormatDescriptor {
  std::string name;
  std::array<std::uint8_t, 4> magic;
  std::uint16_t min_version = 1;
  std::uint16_t max_version = 1;
};

class FramedParser final : public Parser {
public:
  explicit FramedParser(FormatDescriptor descriptor);

  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] bool can_parse(std::span<const std::uint8_t> bytes) const override;
  [[nodiscard]] Result<Recording> parse(std::span<const std::uint8_t> bytes) const override;

private:
  FormatDescriptor descriptor_;
};

} // namespace mercury::formats
