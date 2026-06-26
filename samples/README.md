# Samples

`sample_mrf1.hex` is a readable seed representation of a minimal MRF1 recording:

- magic `MRF1`
- version 1
- one metadata pair
- three records with CRC32-checked payloads

Use `tools/mercury_benchmark.cpp` or `examples/inspect_recording.cpp` to generate binary recordings from the serializer until a fixture generator is added.
