#include "test_support.hpp"

namespace {

const mercury::test::Register replay_filters_streams("replay filters streams", [] {
  mercury::ReplayOptions options;
  options.stream_filter.insert(1);
  auto events = mercury::ReplayEngine().timeline(mercury::test::fixture_recording(), options);
  mercury::test::require(events.size() == 2, "expected two stream-one events");
  mercury::test::require(events.front().timestamp_ns == 100, "timeline ordering mismatch");
});

} // namespace
