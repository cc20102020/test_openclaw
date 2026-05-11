# heic2tiff

`heic2tiff` is a small C command-line tool for converting a single HEIC/HEIF image into a TIFF file. It uses `libheif` for decoding and `libtiff` for writing, with a deliberately narrow internal pipeline so decoding, pixel validation, and TIFF output stay separated.

> Status: early MVP. The current executable supports one input file and one output file. Batch conversion, `--out-dir`, recursive directory traversal, overwrite protection, metadata preservation, and richer conversion options are product goals but are not implemented yet.

## Features

- Decode the primary image from a HEIC/HEIF file with `libheif`.
- Write RGB or RGBA TIFF output with `libtiff`.
- Preserve alpha when `libheif` decodes an alpha channel.
- Emit concise errors and stable exit codes for common failure classes.
- Provide CTest coverage for CLI behavior and negative decode paths.

## Requirements

Build requirements:

- C11 compiler (`gcc` or `clang`)
- CMake 3.16+
- `pkg-config`
- `libheif` development headers/library
- `libtiff` development headers/library

Optional:

- Valgrind, for memory-checking tests during development

### Install dependencies on Ubuntu/Debian

```sh
sudo apt update
sudo apt install -y build-essential cmake pkg-config libheif-dev libtiff-dev valgrind
```

Or run the helper script:

```sh
./scripts/setup_ubuntu.sh
```

## Build

Using the helper script:

```sh
./scripts/build.sh
```

Equivalent CMake commands:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The executable is produced at:

```text
build/heic2tiff
```

Useful environment overrides for the helper script:

```sh
BUILD_DIR=build-debug BUILD_TYPE=Debug ./scripts/build.sh
CMAKE_BUILD_PARALLEL_LEVEL=4 ./scripts/build.sh
```

## Run

```sh
./build/heic2tiff input.heic output.tiff
```

Help and version:

```sh
./build/heic2tiff --help
./build/heic2tiff --version
```

Exit codes:

| Code | Meaning |
| ---: | --- |
| 0 | Success |
| 2 | Invalid arguments |
| 3 | HEIC decode failed |
| 4 | Pixel conversion/validation failed |
| 5 | TIFF write failed |
| 6 | Memory allocation failed |

## Examples

Convert one image:

```sh
./build/heic2tiff photo.heic photo.tiff
```

Convert a file with spaces in its name:

```sh
./build/heic2tiff "Vacation Photo.heic" "Vacation Photo.tiff"
```

A simple shell loop for batch conversion today:

```sh
mkdir -p converted
for src in *.heic *.HEIC; do
  [ -e "$src" ] || continue
  base=${src%.*}
  ./build/heic2tiff "$src" "converted/$base.tiff"
done
```

Note: the current program opens the output path for writing and can overwrite an existing file. Check for existing outputs yourself until built-in overwrite protection is implemented.

## Test

Run the normal test suite:

```sh
./scripts/test.sh
```

Equivalent commands:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The current tests cover help/version output, invalid argument handling, missing input files, invalid non-HEIC input, and real HEIC-to-TIFF smoke/regression conversions using `tests/fixtures/example.heic` and `tests/fixtures/sample1.heic`.

Optional Valgrind smoke test for the invalid-input path:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=99 \
  ./build/heic2tiff tests/fixtures/not_heic.txt /tmp/heic2tiff-invalid.tiff
```

The command should not report memory errors. The program itself is expected to return exit code `3` for that invalid input unless Valgrind finds a problem and returns `99`.

## Architecture summary

```text
src/main.c
  ├─ cli.c            parse arguments, print help/version
  ├─ heic_decode.c    wrap libheif and copy decoded RGB/RGBA pixels
  ├─ pixel_convert.c  validate TIFF-compatible pixel shape
  ├─ tiff_write.c     write scanline TIFF output via libtiff
  └─ error.c          stable status codes and messages
```

High-level flow:

1. Parse CLI arguments into `H2TOptions`.
2. Decode the primary HEIC image into an owned `H2TImage` buffer.
3. Validate dimensions, channel count, and pixel buffer state.
4. Write the image as an 8-bit RGB/RGBA TIFF using LZW compression.
5. Free owned image memory before exiting.

Architecture notes:

- `heic_decode.c` is the only module that deals directly with `libheif` handles.
- `tiff_write.c` is the only module that deals directly with `libtiff` output.
- Architecture-specific pixel conversion files exist under `src/arch/`, but the current conversion path is still minimal validation rather than a full SIMD/assembly selection layer.
- Design, testing, and security notes live under `docs/`.

## Limitations

Current implementation limitations:

- Converts exactly one input file to one output path per invocation.
- Uses the primary HEIC image only; no page/index selection for multi-image containers.
- Always writes 8-bit TIFF samples based on the decoded RGB/RGBA output from `libheif`.
- Always requests LZW TIFF compression.
- Does not currently preserve EXIF metadata, orientation metadata, ICC profiles, or other HEIC metadata.
- Does not protect existing output files from overwrite.
- Does not write through a temporary file plus atomic rename yet.
- Does not expose options for alpha handling, compression, strict mode, recursive input, or output directories.

## Troubleshooting

### CMake cannot find libheif or libtiff

Make sure development packages and `pkg-config` are installed:

```sh
sudo apt install -y pkg-config libheif-dev libtiff-dev
pkg-config --modversion libheif libtiff-4
```

If the libraries are installed in a non-standard prefix, set `PKG_CONFIG_PATH` before configuring CMake.

### `HEIC decode failed`

Common causes:

- The input path does not exist or is not readable.
- The file is not a valid HEIC/HEIF file.
- The file uses a codec feature your installed `libheif` cannot decode.
- The decoded image dimensions or stride are rejected by the program's safety checks.

Try confirming the file with another HEIC-aware tool and checking your `libheif` version:

```sh
pkg-config --modversion libheif
```

### `TIFF write failed`

Common causes:

- The output directory does not exist.
- You do not have write permission for the destination.
- The destination path points to an invalid or unavailable filesystem location.
- `libtiff` failed while writing scanlines.

### Output was overwritten

This is a known current limitation. Until overwrite protection is implemented, use a wrapper check such as:

```sh
out=photo.tiff
if [ -e "$out" ]; then
  echo "Refusing to overwrite $out" >&2
  exit 1
fi
./build/heic2tiff photo.heic "$out"
```

### Build succeeds but tests fail

Run CTest with verbose failure output:

```sh
ctest --test-dir build --output-on-failure -V
```

Then verify you are running the freshly built executable from the same build directory used by CTest.

## More documentation

- `docs/PRD.md` — product requirements and target behavior
- `docs/TECHNICAL_DESIGN.md` — desired architecture and future pipeline design
- `docs/TESTING_PLAN.md` — current and planned testing strategy
- `docs/SECURITY_CHECKLIST.md` — input, output, dependency, and hardening notes

## 4K image support

`heic2tiff` supports common iPhone and high-end camera HEIC/HEIF still images up to a bounded 4K-class limit:

- Maximum dimension: 4096 pixels per side
- Maximum decoded pixels: 4096 × 4096
- Typical iPhone 12 MP HEIC images such as 4032 × 3024 are supported

Images above this limit are rejected with a clear error instead of attempting unbounded allocation. This keeps memory use predictable for local CLI and CI runs.
