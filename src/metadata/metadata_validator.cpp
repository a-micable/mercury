#include "mercury/mercury.hpp"

#include <algorithm>

namespace mercury {

namespace {

bool has_type(const MetadataValue& value, MetadataType type) {
  switch (type) {
  case MetadataType::string:
    return std::holds_alternative<std::string>(value.value);
  case MetadataType::integer:
    return std::holds_alternative<std::int64_t>(value.value);
  case MetadataType::floating:
    return std::holds_alternative<double>(value.value);
  case MetadataType::boolean:
    return std::holds_alternative<bool>(value.value);
  }
  return false;
}

bool is_empty_string(const MetadataValue& value) {
  const auto text = std::get_if<std::string>(&value.value);
  return text != nullptr && text->empty();
}

std::vector<MetadataValidationIssue> validate_metadata(const Metadata& metadata, const std::vector<MetadataRule>& rules,
                                                       std::optional<std::size_t> record_ordinal) {
  std::vector<MetadataValidationIssue> issues;
  for (const auto& rule : rules) {
    const auto iter = metadata.find(rule.key);
    if (iter == metadata.end()) {
      if (rule.required) {
        issues.push_back(MetadataValidationIssue{rule.key, "required metadata key is missing", record_ordinal});
      }
      continue;
    }
    if (!has_type(iter->second, rule.type)) {
      issues.push_back(MetadataValidationIssue{rule.key, "metadata value has unexpected type", record_ordinal});
    }
    if (rule.non_empty && is_empty_string(iter->second)) {
      issues.push_back(MetadataValidationIssue{rule.key, "metadata string must not be empty", record_ordinal});
    }
  }
  return issues;
}

} // namespace

void MetadataValidator::add_rule(MetadataRule rule) {
  const auto existing = std::find_if(rules_.begin(), rules_.end(), [&](const MetadataRule& current) {
    return current.key == rule.key;
  });
  if (existing == rules_.end()) {
    rules_.push_back(std::move(rule));
  } else {
    *existing = std::move(rule);
  }
}

const std::vector<MetadataRule>& MetadataValidator::rules() const {
  return rules_;
}

std::vector<MetadataValidationIssue> MetadataValidator::validate_recording(const Recording& recording) const {
  return validate_metadata(recording.metadata, rules_, std::nullopt);
}

std::vector<MetadataValidationIssue> MetadataValidator::validate_record(std::size_t ordinal, const Record& record) const {
  return validate_metadata(record.metadata, rules_, ordinal);
}

void MetadataValidator::clear() {
  rules_.clear();
}

} // namespace mercury
