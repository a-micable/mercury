#include "mercury/mercury.hpp"

#include <algorithm>

namespace mercury {

namespace {

bool route_less(const EventRoute& lhs, const EventRoute& rhs) {
  if (lhs.priority != rhs.priority) {
    return lhs.priority > rhs.priority;
  }
  return lhs.name < rhs.name;
}

} // namespace

Status EventRouter::add_route(EventRoute route) {
  if (route.name.empty()) {
    return Status(ErrorCode::invalid_configuration, "event route name must not be empty");
  }
  const auto duplicate = std::find_if(routes_.begin(), routes_.end(), [&](const EventRoute& existing) {
    return existing.name == route.name;
  });
  if (duplicate != routes_.end()) {
    return Status(ErrorCode::invalid_configuration, "duplicate event route: " + route.name);
  }

  routes_.push_back(std::move(route));
  std::stable_sort(routes_.begin(), routes_.end(), route_less);
  return Status::Ok();
}

std::vector<EventRoute> EventRouter::routes() const {
  return routes_;
}

std::vector<RoutedEvent> EventRouter::route_record(std::size_t ordinal, const Record& record) const {
  std::vector<RoutedEvent> routed;
  QueryEngine query_engine;
  for (const auto& route : routes_) {
    if (!query_engine.matches(record, route.query)) {
      continue;
    }
    routed.push_back(RoutedEvent{route.name, ordinal, &record});
    if (route.stop_after_match) {
      break;
    }
  }
  return routed;
}

std::vector<RoutedEvent> EventRouter::route_recording(const Recording& recording) const {
  std::vector<RoutedEvent> routed;
  for (std::size_t ordinal = 0; ordinal < recording.records.size(); ++ordinal) {
    auto matches = route_record(ordinal, recording.records[ordinal]);
    routed.insert(routed.end(), matches.begin(), matches.end());
  }
  return routed;
}

void EventRouter::clear() {
  routes_.clear();
}

} // namespace mercury
