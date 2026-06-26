#include "test_support.hpp"

namespace {

mercury::EventRoute route_for_stream(std::string name, std::uint16_t stream, int priority, bool stop = false) {
  mercury::EventRoute route;
  route.name = std::move(name);
  route.query.streams.insert(stream);
  route.priority = priority;
  route.stop_after_match = stop;
  return route;
}

const mercury::test::Register event_router_rejects_invalid_routes("event router rejects invalid routes", [] {
  mercury::EventRouter router;

  auto status = router.add_route(mercury::EventRoute{});
  mercury::test::require(!status.ok(), "empty route name should be rejected");

  status = router.add_route(route_for_stream("primary", 1, 10));
  mercury::test::require(status.ok(), "valid route should be accepted");

  status = router.add_route(route_for_stream("primary", 2, 20));
  mercury::test::require(!status.ok(), "duplicate route name should be rejected");
});

const mercury::test::Register event_router_orders_by_priority("event router orders by priority", [] {
  mercury::EventRouter router;
  mercury::test::require(router.add_route(route_for_stream("low", 1, 1)).ok(), "low route should be accepted");
  mercury::test::require(router.add_route(route_for_stream("high", 1, 100)).ok(), "high route should be accepted");

  const auto routed = router.route_record(0, mercury::test::fixture_recording().records.front());
  mercury::test::require(routed.size() == 2, "both routes should match stream 1");
  mercury::test::require(routed.front().route_name == "high", "higher priority route should run first");
});

const mercury::test::Register event_router_honors_stop_after_match("event router honors stop after match", [] {
  mercury::EventRouter router;
  mercury::test::require(router.add_route(route_for_stream("terminal", 1, 100, true)).ok(),
                         "terminal route should be accepted");
  mercury::test::require(router.add_route(route_for_stream("audit", 1, 10)).ok(), "audit route should be accepted");

  const auto recording = mercury::test::fixture_recording();
  const auto routed = router.route_recording(recording);

  mercury::test::require(routed.size() == 2, "terminal route should match both stream 1 records");
  mercury::test::require(routed.front().route_name == "terminal", "terminal route should be first");
  mercury::test::require(routed.front().record->stream_id == 1, "routed event should reference source record");
});

} // namespace
