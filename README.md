# Mercury

Mercury is a C++ framework for parsing, validating, indexing, replaying, and analyzing binary recorder files produced by embedded and industrial systems.

This repository currently contains an initial production-oriented foundation:

- A CMake-based C++20 library and CLI tool suite.
- Parsers for MRF1, MRF2, legacy, compact embedded, and stream capture envelopes.
- Archive bundle reader/writer with checksum validation.
- Compression codec registry with stored and RLE codecs.
- Replay timeline generation and filtered replay callbacks.
- Serializer, configuration loader, plugin host, tests, benchmarks, and libFuzzer harnesses.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
```

Fuzz targets are optional:

```sh
cmake -S . -B build-fuzz -DMERCURY_BUILD_FUZZERS=ON -DMERCURY_BUILD_TESTS=OFF
cmake --build build-fuzz
```

## Tools

- `mercury-inspect`: print recording metadata and record summaries.
- `mercury-verify`: parse and index a recording, reporting validation failures.
- `mercury-index`: emit a CSV-like random-access index.
- `mercury-replay`: replay records in timeline order.
- `mercury-export`: export a small JSON summary.
- `mercury-convert`: normalize any supported framed recording into the internal MRF envelope.
- `mercury-benchmark`: run a deterministic parser microbenchmark.

## Format Envelope

The initial framed formats use a shared envelope:

1. Four-byte magic (`MRF1`, `MRF2`, `LGR0`, `CER0`, or `SCAP`).
2. Little-endian `uint16` version.
3. Metadata key/value table.
4. Little-endian `uint32` record count.
5. Repeated records containing kind, stream id, nanosecond timestamp, sequence, payload length, payload, and CRC32.

The parser enforces version ranges, payload limits, record-count limits, and payload checksums. Format-specific modules are separate compilation units so recovery heuristics and version-specific extensions can grow independently.

## Repository Note

The current checkout is an honest implementation seed, not a fabricated multi-year history. Future changes should be committed as real engineering milestones: parser hardening, recovery behavior, format-specific fixtures, archive compatibility, and performance work.
