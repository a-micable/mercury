#include "mercury/mercury.hpp"

#include <sstream>

namespace mercury {

namespace {

std::string trim(std::string_view text) {
  const auto begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string_view::npos) {
    return {};
  }
  const auto end = text.find_last_not_of(" \t\r\n");
  return std::string(text.substr(begin, end - begin + 1));
}

} // namespace

std::string Config::get(std::string_view key, std::string fallback) const {
  auto iter = values.find(std::string(key));
  return iter == values.end() ? std::move(fallback) : iter->second;
}

Result<Config> ConfigLoader::parse(std::string_view text) const {
  Config config;
  std::istringstream stream{std::string(text)};
  std::string line;
  std::size_t line_no = 0;
  while (std::getline(stream, line)) {
    ++line_no;
    auto stripped = trim(line);
    if (stripped.empty() || stripped[0] == '#') {
      continue;
    }
    auto equals = stripped.find('=');
    if (equals == std::string::npos) {
      return Status(ErrorCode::invalid_configuration, "line " + std::to_string(line_no) + " is missing '='");
    }
    auto key = trim(std::string_view(stripped).substr(0, equals));
    auto value = trim(std::string_view(stripped).substr(equals + 1));
    if (key.empty()) {
      return Status(ErrorCode::invalid_configuration, "line " + std::to_string(line_no) + " has an empty key");
    }
    config.values[std::move(key)] = std::move(value);
  }
  return config;
}

} // namespace mercury
