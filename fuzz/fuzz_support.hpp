#pragma once

#include "mercury/mercury.hpp"

namespace mercury::fuzz {

inline std::span<const std::uint8_t> bytes(const std::uint8_t* data, std::size_t size) {
  return {data, size};
}

inline void exercise_recording(const Recording& recording) {
  IndexBuilder().build(recording);
  ReplayEngine().timeline(recording);
  Serializer().write_recording(recording);
}

} // namespace mercury::fuzz
