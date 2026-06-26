# Mercury

Mercury is a C++20 binary flight recorder and telemetry analysis framework for the kind of data that tends to arrive late, damaged, undocumented, and urgently important.

It is built around a practical problem: embedded systems, avionics rigs, industrial controllers, and monitoring appliances often emit compact binary recorder files that must be decoded long after the original device, firmware image, or operator context has disappeared. Mercury provides the foundations for reading those files, validating their integrity, indexing them for random access, reconstructing timelines, and replaying events through deterministic analysis pipelines.

This repository is an implementation seed for that system. It already contains the core library shape, command-line tools, tests, fuzz harnesses, example data, and documentation needed to evolve Mercury into a larger production codebase without changing its basic architecture.

## What Mercury Is For

Mercury treats binary recorder files as evidence. A recording is not just a byte stream; it is a time-ordered collection of measurements, events, metadata, attachments, heartbeats, and partial failures. The framework is designed to preserve that context while giving engineers the tools to ask sharper questions:

- What format and protocol version produced this file?
- Which records are intact, corrupt, truncated, or unsupported?
- What streams were active, and when did they drift, stop, or restart?
- Can this archive be indexed without fully materializing it in memory?
- Can a damaged session still yield a useful timeline?
- Can the same bytes be replayed deterministically during regression analysis?

## Current Capabilities

- CMake-based C++20 library with deterministic local builds.
- Parser registry with support for `MRF1`, `MRF2`, legacy, compact embedded, and stream-capture envelopes.
- Shared framed-record reader with version checks, payload limits, record-count limits, and per-record CRC validation.
- Archive bundle reader and writer with table-of-contents validation.
- Compression codec registry with stored and RLE codecs.
- Integrity primitives for CRC32, CRC64-ECMA, FNV-1a, and SHA-256.
- Replay engine that reconstructs sorted timelines and supports stream/time filtering.
- Incremental index builder for timestamp-oriented random access.
- Binary serializer/deserializer for normalized recording fixtures.
- Configuration parser for simple operational profiles.
- Plugin host for registering external parsers and codecs.
- CLI suite for inspection, verification, indexing, replay, conversion, export, and benchmarking.
- Unit tests, regression fixtures, fuzz targets, seed corpora, and ClusterFuzzLite configuration.

## Repository Layout

```text
include/mercury/       Public library API
src/core/              Status, binary reading, logging, integrity primitives
src/parser/            Parser registry and format detection
src/formats/           MRF1, MRF2, legacy, compact, and stream-capture readers
src/archive/           Bundle scan, validation, and extraction
src/codecs/            Pluggable compression codecs
src/index/             Random-access index construction
src/replay/            Timeline reconstruction and replay callbacks
src/serialization/     Normalized binary serialization helpers
src/config/            Configuration language parser
src/plugin/            Parser and codec plugin host
tools/                 Command-line utilities
tests/                 Unit and regression tests
fuzz/                  libFuzzer harnesses, dictionaries, and seed corpora
docs/                  Architecture and format notes
examples/              Small integration examples
benchmarks/            Deterministic parser benchmark
```

## Build

Mercury has no service dependencies and does not require credentials. A normal local build uses CMake:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
```

Fuzz targets are optional and are kept behind a build flag so day-to-day development remains fast:

```sh
cmake -S . -B build-fuzz -DMERCURY_BUILD_FUZZERS=ON -DMERCURY_BUILD_TESTS=OFF
cmake --build build-fuzz
```

## Command-Line Tools

Mercury ships as a library first, but the command-line tools make the recorder workflow visible:

- `mercury-inspect` prints format, version, metadata, and record summaries.
- `mercury-verify` parses and indexes a recording while reporting validation failures.
- `mercury-index` emits a compact timestamp index suitable for random-access workflows.
- `mercury-replay` replays records in reconstructed timeline order.
- `mercury-export` writes a small JSON summary for automation and reports.
- `mercury-convert` normalizes supported framed recordings into the internal Mercury envelope.
- `mercury-benchmark` runs a deterministic parser microbenchmark.

## Format Model

The initial framed formats use a shared envelope so version-specific readers can evolve without duplicating core safety checks:

1. Four-byte magic such as `MRF1`, `MRF2`, `LGR0`, `CER0`, or `SCAP`.
2. Little-endian `uint16` protocol version.
3. Metadata key/value table.
4. Little-endian `uint32` record count.
5. Repeated records containing kind, stream id, timestamp, sequence, payload length, payload bytes, and CRC32.

This model is intentionally conservative. Parser code validates magic, supported version ranges, metadata sizes, record counts, payload bounds, and checksums before exposing normalized records to replay or indexing layers.

## Fuzzing

The fuzzing surface is split by subsystem instead of routed through one giant harness. This makes crashes easier to triage and keeps coverage meaningful as the project grows:

- `record_fuzzer`
- `archive_fuzzer`
- `metadata_fuzzer`
- `replay_fuzzer`
- `config_fuzzer`
- `compression_fuzzer`

Seed corpora live under `fuzz/corpus/`, and `.clusterfuzzlite/` contains the ClusterFuzzLite build configuration.

## Design Principles

Mercury favors boring, inspectable machinery over cleverness. Binary recovery code should be explicit about offsets, lengths, endian conversion, integrity checks, and failure modes. The public API is small on purpose: parse bytes into normalized recordings, validate what can be validated, build indexes, reconstruct timelines, and leave enough extension points for proprietary formats without letting those formats leak into every subsystem.

The project is also intentionally dependency-light. Recorder analysis often happens in constrained environments: lab machines, incident response laptops, air-gapped networks, and CI workers with minimal packages installed. Mercury should remain useful in those places.

## Maturity Note

This checkout is not a fabricated decade of history. It is an honest foundation: coherent, buildable in shape, covered by early tests and fuzz targets, and ready for incremental engineering milestones. The intended path from here is straightforward: harden recovery behavior, expand format-specific fixtures, deepen archive compatibility, add richer metadata validation, and grow performance coverage around large recordings.
