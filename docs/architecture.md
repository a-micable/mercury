# Architecture

Mercury is organized around a small set of stable library contracts.

## Parser Framework

`Parser` implementations are registered in `ParserRegistry`. Detection is magic-based for the initial formats, while parsing returns a normalized `Recording` containing metadata and records. Shared framing keeps validation behavior consistent; format-specific units remain separate so each family can add extensions without contaminating the common reader.

## Archive Bundles

`ArchiveWriter` emits deterministic Mercury bundles with a table of contents and CRC32 per member. `ArchiveReader` validates member offsets and checksums before extraction.

## Integrity

The core library exposes small deterministic integrity primitives used by parsers, archives, and command-line verification. CRC32 remains the fast per-record checksum, CRC64-ECMA is available for wider archive and transport manifests, and SHA-256 provides a stable content digest for offline audit trails and regression fixtures. The implementations are internal and dependency-free so Mercury can run in restricted recovery environments.

## Replay

`ReplayEngine` converts records into sorted `TimelineEvent` values, applies stream and time filters, and exposes callback-based replay. The API accepts speed scaling now even though wall-clock pacing is intentionally left to a scheduler layer.

## Query and Routing

`QueryEngine` evaluates structured predicates over normalized records. It supports stream filters, record-kind filters, time windows, sequence ranges, exact metadata matches, and byte-subsequence payload searches. The query layer is deliberately independent of CLI parsing so applications can construct filters from configuration files, scripts, user interfaces, or stored investigations.

`EventRouter` builds on that query layer. Routes are named, prioritized filters that turn matching records into routed analysis events. A route may stop evaluation after it matches, which lets production pipelines model terminal classifications such as "drop", "quarantine", or "handled by a specialized decoder" without hard-coding policy into the replay engine.

## Indexes

`IndexBuilder` produces compact timestamp-oriented entries from a recording. `TimelineIndex` owns those entries, sorts them into deterministic order, and provides inclusive timestamp ranges plus nearest before/after lookups. `MetadataIndex` stores recording-level and record-level metadata hits separately, preserving record ordinals so tools can jump from a key/value match back to the source record.

## Plugins

`PluginHost` owns parser and compression registries. Third-party modules can register decoders or codecs through these stable interfaces without depending on CLI implementation details.

## Testing and Fuzzing

Tests use the public library API. Fuzzers are intentionally thin: they feed bytes into parser, archive, replay, config, and compression logic rather than duplicating parsing rules in harnesses.
