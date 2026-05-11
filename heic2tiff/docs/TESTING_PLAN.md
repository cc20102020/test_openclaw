# Testing Plan

## Scope

The test suite should protect the command-line contract, error handling, HEIC decoding boundaries, pixel conversion behavior, and TIFF output compatibility for `heic2tiff`.

## Current CTest Coverage

CTest is wired in `CMakeLists.txt` through `tests/cli_tests.cmake`. The current tests cover:

- `--help` and `-h` print usage and exit `0`.
- `--version` and `-V` print the expected version and exit `0`.
- Invalid CLI arguments exit with `H2T_ERR_ARGS` / code `2`:
  - no arguments
  - missing output argument
  - too many arguments
  - unknown single option such as `--bogus`
  - identical literal input/output paths
- Missing input files exit with `H2T_ERR_DECODE` / code `3`, print a decode error on stderr, and do not leave an output TIFF.
- Invalid/non-HEIC input files exit with `H2T_ERR_DECODE` / code `3`, print a decode error on stderr, and do not leave an output TIFF.

Run locally:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Fixture Strategy

Keep fixtures small and deterministic:

- Use text or tiny binary files for negative tests. `tests/fixtures/not_heic.txt` is intentionally not a valid HEIC and should remain tiny.
- `tests/fixtures/example.heic` and `tests/fixtures/sample1.heic` is a real sample HEIC fixture from the upstream libheif gh-pages example, used for positive conversion smoke tests. For future pixel-precise regression tests, prefer generated 1x1 or 2x2 images with known RGB/RGBA pixels.
- If valid HEIC fixtures are too large or have licensing concerns, generate them in a test helper script from a checked-in source image when required tools are available, and skip those tests when generation tools are missing.
- Store expected metadata rather than large golden output files when possible: width, height, channel count, TIFF tags, and selected pixel values.

## Planned Unit and Integration Tests

### CLI argument tests

Already covered through CTest. Future additions:

- Empty input/output path behavior if a shell-independent test harness is added.
- Output path pointing to a non-writable directory.
- Input and output paths with spaces/unicode.

### Invalid and missing file tests

Already covered through CTest. Future additions:

- Directory passed as input.
- Truncated HEIC header fixture.
- Valid HEIC with unsupported feature, if libheif reports one reliably.

### Decode and conversion tests

Add focused C tests around internal functions where possible:

- `checked_size` overflow boundaries indirectly through decode fixtures or by extracting it into a testable helper.
- RGB image decode produces expected dimensions, channels, stride, and byte values.
- RGBA image decode preserves alpha.
- `h2t_prepare_pixels_for_tiff` handles 3-channel and 4-channel images, rejects invalid dimensions/channels, and is stable across architecture-specific implementations.

### TIFF writer tests

Add integration tests that convert a tiny valid HEIC to TIFF, then inspect the TIFF with libtiff or `tiffinfo` when available:

- Image width/height tags match source.
- Samples per pixel is 3 or 4 as expected.
- Bits per sample is 8.
- Output file is readable by libtiff.
- Pixel smoke check for a 1x1 or 2x2 fixture.

## Valgrind / Memory Checks

If Valgrind is installed, run the current CTest suite under it:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure \
  -T memcheck \
  --overwrite MemoryCheckCommand=valgrind \
  --overwrite MemoryCheckCommandOptions="--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=99"
```

For a single negative-path smoke check:

```sh
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=99 \
  ./build/heic2tiff tests/fixtures/not_heic.txt /tmp/heic2tiff-invalid.tiff
```

The command should report no memory errors. The program itself is expected to exit with code `3` for invalid input unless Valgrind finds an error and returns `99`.

## Regression Plan

Before releases or larger refactors:

1. Run the normal CTest suite on Linux x86_64.
2. Run the suite with `CMAKE_BUILD_TYPE=Debug` and compiler warnings enabled.
3. Run Valgrind memcheck when available.
4. Run the checked-in `valid_sample_conversion` test using `tests/fixtures/example.heic` and `tests/fixtures/sample1.heic` and verify TIFF metadata/magic.
5. Run tests on ARM/aarch64 or CI emulation when touching `src/arch/*` conversion code.
6. Add a regression fixture for every bug that reaches users: the smallest input that reproduces it, the expected exit code/output behavior, and a short note in this file if the behavior is subtle.

## CI Recommendation

Add a CI job that installs `libheif-dev`, `libtiff-dev`, `cmake`, and a C compiler, then runs:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

A second optional debug job can run Valgrind if the CI image supports it without excessive runtime.

## 4K regression coverage

The suite includes `dimension_limits`, a unit-style CTest executable covering the supported 4K-class boundary:

- 4032 × 3024 iPhone HEIC dimensions are accepted.
- 3840 × 2160 UHD dimensions are accepted.
- 4096 × 4096 is the maximum supported square image.
- Dimensions over 4096 pixels on either side are rejected.

Real smoke testing should also convert an actual iPhone/high-end camera HEIC and verify TIFF output metadata with an image reader.
