#include "mercury/core.hpp"

namespace mercury {

Status::Status() : code_(ErrorCode::ok) {}

Status::Status(ErrorCode code, std::string message)
    : code_(code), message_(std::move(message)) {}

bool Status::ok() const {
  return code_ == ErrorCode::ok;
}

ErrorCode Status::code() const {
  return code_;
}

const std::string& Status::message() const {
  return message_;
}

Status Status::Ok() {
  return {};
}

std::string to_string(ErrorCode code) {
  switch (code) {
  case ErrorCode::ok:
    return "ok";
  case ErrorCode::end_of_input:
    return "end_of_input";
  case ErrorCode::invalid_magic:
    return "invalid_magic";
  case ErrorCode::unsupported_version:
    return "unsupported_version";
  case ErrorCode::malformed_record:
    return "malformed_record";
  case ErrorCode::checksum_mismatch:
    return "checksum_mismatch";
  case ErrorCode::compression_error:
    return "compression_error";
  case ErrorCode::io_error:
    return "io_error";
  case ErrorCode::unknown_format:
    return "unknown_format";
  case ErrorCode::invalid_configuration:
    return "invalid_configuration";
  }
  return "unknown_error";
}

} // namespace mercury
