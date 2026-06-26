#include "mercury/core.hpp"

#include <iostream>

namespace mercury {

Logger& Logger::instance() {
  static Logger logger;
  return logger;
}

void Logger::set_sink(Sink sink) {
  sink_ = std::move(sink);
}

void Logger::log(Level level, std::string_view message) {
  if (sink_) {
    sink_(level, message);
    return;
  }
  const char* prefix = "INFO";
  switch (level) {
  case Level::trace:
    prefix = "TRACE";
    break;
  case Level::debug:
    prefix = "DEBUG";
    break;
  case Level::info:
    prefix = "INFO";
    break;
  case Level::warn:
    prefix = "WARN";
    break;
  case Level::error:
    prefix = "ERROR";
    break;
  }
  std::clog << "[mercury] " << prefix << ' ' << message << '\n';
}

} // namespace mercury
