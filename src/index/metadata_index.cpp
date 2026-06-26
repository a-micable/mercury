#include "mercury/mercury.hpp"

#include "metadata/metadata_values.hpp"

#include <algorithm>

namespace mercury {

namespace {

void append_metadata_hits(std::vector<MetadataHit>& hits, const Metadata& metadata,
                          std::optional<std::size_t> record_ordinal) {
  for (const auto& [key, value] : metadata) {
    hits.push_back(MetadataHit{key, metadata::value_to_string(value), record_ordinal});
  }
}

} // namespace

void MetadataIndex::add_recording_metadata(const Metadata& metadata) {
  append_metadata_hits(hits_, metadata, std::nullopt);
}

void MetadataIndex::add_record_metadata(std::size_t ordinal, const Metadata& metadata) {
  append_metadata_hits(hits_, metadata, ordinal);
}

void MetadataIndex::build(const Recording& recording) {
  clear();
  add_recording_metadata(recording.metadata);
  for (std::size_t ordinal = 0; ordinal < recording.records.size(); ++ordinal) {
    add_record_metadata(ordinal, recording.records[ordinal].metadata);
  }
}

std::vector<MetadataHit> MetadataIndex::find_key(std::string_view key) const {
  std::vector<MetadataHit> matches;
  for (const auto& hit : hits_) {
    if (hit.key == key) {
      matches.push_back(hit);
    }
  }
  return matches;
}

std::vector<MetadataHit> MetadataIndex::find_exact(std::string_view key, std::string_view value) const {
  std::vector<MetadataHit> matches;
  for (const auto& hit : hits_) {
    if (hit.key == key && hit.value == value) {
      matches.push_back(hit);
    }
  }
  return matches;
}

std::vector<std::string> MetadataIndex::keys() const {
  std::vector<std::string> out;
  out.reserve(hits_.size());
  for (const auto& hit : hits_) {
    out.push_back(hit.key);
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

std::size_t MetadataIndex::size() const {
  return hits_.size();
}

void MetadataIndex::clear() {
  hits_.clear();
}

} // namespace mercury
