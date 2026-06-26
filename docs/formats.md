# Supported Recording Families

## MRF1

MRF1 is the baseline Mercury recorder envelope. Version 1 through 3 are accepted. It is intended for deterministic embedded recorders with strict checksums and fixed stream identifiers.

## MRF2

MRF2 accepts version 2 through 6 and is reserved for richer nested structures, optional extension blocks, and larger metadata sections. The current parser validates the shared envelope and leaves extension interpretation to future record decoders.

## Legacy Recorder

Legacy recordings use the `LGR0` magic and versions 1 through 2. Recovery heuristics belong in `src/formats/legacy.cpp` so they can be regression-tested without loosening modern parser validation.

## Compact Embedded Recorder

Compact embedded recordings use `CER0` and currently accept only version 1. The shared envelope gives compact devices the same checksum and indexing behavior as larger recorders.

## Stream Capture

Stream capture recordings use `SCAP` and versions 1 through 4. This family is the natural home for incremental streaming and partial recovery work.

## Archive Bundle

Archive bundles use `MBND`. Each member has a name, byte offset, length, and CRC32 checksum. The reader rejects out-of-bounds members and checksum mismatches before returning extracted bytes.
