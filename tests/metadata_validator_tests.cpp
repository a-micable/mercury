#include "test_support.hpp"

namespace {

const mercury::test::Register metadata_validator_checks_recording_metadata(
    "metadata validator checks recording metadata", [] {
      auto recording = mercury::test::fixture_recording();

      mercury::MetadataValidator validator;
      validator.add_rule(mercury::MetadataRule{"airframe", mercury::MetadataType::string, true, true});
      validator.add_rule(mercury::MetadataRule{"mission", mercury::MetadataType::string, true, true});

      const auto issues = validator.validate_recording(recording);
      mercury::test::require(issues.size() == 1, "missing required recording metadata should be reported");
      mercury::test::require(issues.front().key == "mission", "wrong metadata key reported");
      mercury::test::require(!issues.front().record_ordinal, "recording issue should not have record ordinal");
    });

const mercury::test::Register metadata_validator_checks_record_metadata("metadata validator checks record metadata", [] {
  auto recording = mercury::test::fixture_recording();
  recording.records[1].metadata["sensor"] = mercury::MetadataValue{std::string("")};
  recording.records[1].metadata["quality"] = mercury::MetadataValue{std::string("high")};

  mercury::MetadataValidator validator;
  validator.add_rule(mercury::MetadataRule{"sensor", mercury::MetadataType::string, true, true});
  validator.add_rule(mercury::MetadataRule{"quality", mercury::MetadataType::integer, true, false});

  const auto issues = validator.validate_record(1, recording.records[1]);
  mercury::test::require(issues.size() == 2, "record metadata should report type and non-empty failures");
  mercury::test::require(issues[0].record_ordinal && *issues[0].record_ordinal == 1,
                         "record issue should preserve ordinal");
});

const mercury::test::Register metadata_validator_replaces_existing_rules(
    "metadata validator replaces existing rules", [] {
      mercury::MetadataValidator validator;
      validator.add_rule(mercury::MetadataRule{"airframe", mercury::MetadataType::string, true, true});
      validator.add_rule(mercury::MetadataRule{"airframe", mercury::MetadataType::string, false, false});

      mercury::test::require(validator.rules().size() == 1, "duplicate rule keys should replace existing rules");
      validator.clear();
      mercury::test::require(validator.rules().empty(), "clear should remove validation rules");
    });

} // namespace
