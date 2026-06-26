#include "mercury/mercury.hpp"

#include <algorithm>

namespace mercury {

namespace {

bool selected(const Record& record, const ReplayOptions& options) {
  if (!options.stream_filter.empty() && !options.stream_filter.contains(record.stream_id)) {
    return false;
  }
  if (options.begin_ns && record.timestamp_ns < *options.begin_ns) {
    return false;
  }
  if (options.end_ns && record.timestamp_ns > *options.end_ns) {
    return false;
  }
  return true;
}

TimelineEvent make_event(const Record& record) {
  TimelineEvent event;
  event.timestamp_ns = record.timestamp_ns;
  event.stream_id = record.stream_id;
  event.kind = record.kind;
  event.label = "stream " + std::to_string(record.stream_id) + " " + to_string(record.kind);
  return event;
}

} // namespace

std::vector<TimelineEvent> ReplayEngine::timeline(const Recording& recording, const ReplayOptions& options) const {
  std::vector<TimelineEvent> events;
  for (const auto& record : recording.records) {
    if (selected(record, options)) {
      events.push_back(make_event(record));
    }
  }
  std::stable_sort(events.begin(), events.end(), [](const TimelineEvent& lhs, const TimelineEvent& rhs) {
    if (lhs.timestamp_ns != rhs.timestamp_ns) {
      return lhs.timestamp_ns < rhs.timestamp_ns;
    }
    return lhs.stream_id < rhs.stream_id;
  });
  return events;
}

Status ReplayEngine::replay(const Recording& recording, const ReplayOptions& options, Callback callback) const {
  if (options.speed <= 0.0) {
    return Status(ErrorCode::invalid_configuration, "replay speed must be positive");
  }
  std::vector<const Record*> records;
  for (const auto& record : recording.records) {
    if (selected(record, options)) {
      records.push_back(&record);
    }
  }
  std::stable_sort(records.begin(), records.end(), [](const Record* lhs, const Record* rhs) {
    return lhs->timestamp_ns < rhs->timestamp_ns;
  });
  for (const Record* record : records) {
    callback(make_event(*record), *record);
  }
  return Status::Ok();
}

} // namespace mercury
