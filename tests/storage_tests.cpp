#include "test_support.hpp"

#include <filesystem>

namespace {

struct TemporaryStore {
  std::filesystem::path root =
      std::filesystem::temp_directory_path() / "mercury-storage-tests";

  ~TemporaryStore() { std::filesystem::remove_all(root); }
};

const mercury::test::Register store_roundtrip_and_listing(
    "recording store roundtrip and listing", [] {
      TemporaryStore temporary;
      mercury::RecordingStore store(temporary.root, "acme");
      const auto recording = mercury::test::fixture_recording();

      mercury::test::require(store.put("flight-001", recording).ok(),
                             "store should publish recording");
      auto loaded = store.get("flight-001");
      mercury::test::require(loaded.ok(), "store should load recording");
      mercury::test::require(loaded.value().records.size() == 3,
                             "store should preserve records");

      auto summaries = store.list();
      mercury::test::require(summaries.ok(), "store should list recordings");
      mercury::test::require(summaries.value().size() == 1,
                             "store should list one recording");
      mercury::test::require(summaries.value().front().tenant == "acme",
                             "summary should include tenant");
      mercury::test::require(summaries.value().front().payload_bytes == 6,
                             "summary should calculate payload bytes");
    });

const mercury::test::Register store_rejects_unsafe_ids(
    "recording store rejects unsafe identifiers", [] {
      TemporaryStore temporary;
      mercury::RecordingStore store(temporary.root, "acme");
      const auto status = store.put("../escape", mercury::test::fixture_recording());
      mercury::test::require(!status.ok(), "store should reject path traversal");
      mercury::test::require(status.code() == mercury::ErrorCode::invalid_configuration,
                             "unsafe id should report invalid configuration");
    });

const mercury::test::Register store_isolates_tenants(
    "recording store isolates tenants", [] {
      TemporaryStore temporary;
      mercury::RecordingStore first(temporary.root, "tenant-a");
      mercury::RecordingStore second(temporary.root, "tenant-b");
      mercury::test::require(
          first.put("same-id", mercury::test::fixture_recording()).ok(),
          "first tenant should accept recording");
      auto second_listing = second.list();
      mercury::test::require(second_listing.ok(), "second tenant should list");
      mercury::test::require(second_listing.value().empty(),
                             "tenant data must remain isolated");
    });

} // namespace