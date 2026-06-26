#include "test_support.hpp"

namespace {

const mercury::test::Register archive_roundtrip("archive roundtrip", [] {
  mercury::ArchiveWriter writer;
  writer.add("recording.mrf", mercury::Serializer().write_recording(mercury::test::fixture_recording()));
  writer.add("notes.txt", {'o', 'k'});
  auto bundle = writer.finish();
  auto members = mercury::ArchiveReader().scan(bundle);
  mercury::test::require(members.ok(), "archive should scan");
  mercury::test::require(members.value().size() == 2, "expected two archive members");
  auto notes = mercury::ArchiveReader().extract(bundle, "notes.txt");
  mercury::test::require(notes.ok(), "expected notes member");
  mercury::test::require(notes.value().size() == 2, "expected notes payload");
});

} // namespace
