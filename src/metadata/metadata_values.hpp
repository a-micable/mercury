#pragma once

#include "mercury/core.hpp"

#include <sstream>
#include <type_traits>

namespace mercury::metadata {

inline std::string value_to_string(const MetadataValue& value) {
  return std::visit(
      [](const auto& item) -> std::string {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::string>) {
          return item;
        } else if constexpr (std::is_same_v<T, bool>) {
          return item ? "true" : "false";
        } else {
          std::ostringstream out;
          out << item;
          return out.str();
        }
      },
      value.value);
}

} // namespace mercury::metadata
