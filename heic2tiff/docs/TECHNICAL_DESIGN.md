# heic2tiff Technical Design

## Goal

`heic2tiff` is a small command-line converter that reads one HEIC/HEIF image, decodes it to an internal pixel buffer, normalizes pixels into a TIFF-compatible representation, and writes a standards-compliant TIFF file. The implementation should keep dependencies isolated, memory ownership explicit, and platform-specific optimizations optional.

## CLI Architecture

The executable is organized as a thin orchestration layer over independent pipeline modules:

```text
main
 ├─ cli_parse(argc, argv)              // options, input/output paths
 ├─ app_convert(config)                // high-level workflow
 │   ├─ heic_decode(input_path)
 │   ├─ pixel_convert(decoded_image, options)
 │   └─ tiff_write(output_path, image)
 └─ diagnostic formatting / exit code mapping
```

Recommended options:

- `heic2tiff INPUT OUTPUT`
- `--compression none|lzw|deflate` where supported
- `--page INDEX` for multi-image HEIF containers; default `0`
- `--preserve-alpha` / `--drop-alpha`
- `--strict` to fail on unsupported color profiles or metadata rather than approximating
- `--help`, `--version`

The CLI should not own decoding, conversion, or writing details. It validates user intent, constructs a conversion config, calls the pipeline, and maps typed errors to stable exit codes.

## Decode Pipeline

The HEIC decode layer wraps the chosen HEIF library behind a narrow adapter, for example `decode_heic_file(path, page_index) -> DecodedImage`.

Responsibilities:

1. Open and validate the container.
2. Select the requested image item/page.
3. Decode to a predictable intermediate format when possible, preferably 8-bit or 16-bit RGBA/RGB with known row stride.
4. Extract width, height, bit depth, alpha presence, orientation, color profile, and relevant metadata.
5. Return an owned `DecodedImage` or a typed decode error.

The rest of the program should not depend on HEIF library structs. Any external decoder handles are released before returning, or retained only through an explicit owner object with a destructor.

## Pixel Conversion Pipeline

The converter normalizes `DecodedImage` into `TiffImage`:

```text
DecodedImage
 ├─ apply orientation if requested/required
 ├─ validate dimensions, channels, bit depth, stride
 ├─ convert color layout: RGBA/BGRA/YUV/etc. -> RGB/RGBA/Gray
 ├─ convert sample depth: 8-bit or 16-bit integer samples
 ├─ premultiplied alpha handling if decoder emits it
 └─ produce contiguous strips/rows for TIFF writer
```

Design rules:

- Preserve precision when practical: 10/12/16-bit HEIC sources should become 16-bit TIFF samples.
- Keep conversion explicit; avoid hidden global decoder configuration.
- Treat row stride and channel order as data, not assumptions.
- Prefer streaming row conversion for large images, but allow a full-frame buffer for a first implementation.
- Validate overflow before allocating: `width * height * channels * bytes_per_sample` must be checked.

## TIFF Writer Pipeline

The TIFF writer receives a fully described `TiffImage` and writes only TIFF concerns:

1. Create the output stream.
2. Emit baseline tags: dimensions, samples per pixel, bits per sample, photometric interpretation, compression, planar configuration, rows per strip, strip offsets/counts.
3. Emit alpha tag when alpha is preserved.
4. Embed ICC profile and metadata when available and supported.
5. Write pixel strips/tiles.
6. Finalize offsets and close atomically.

For robustness, write to a temporary file in the output directory, flush/close it, then rename to the target path. If finalization fails, remove only the temporary file.

## Error Handling

Use typed errors with context instead of raw strings:

- `CLI_ERROR`: invalid arguments or unsupported option combination
- `IO_ERROR`: input/output open, read, write, flush, or rename failure
- `DECODE_ERROR`: invalid HEIC, unsupported codec feature, missing image item
- `CONVERT_ERROR`: unsupported pixel format, color conversion failure, overflow
- `TIFF_ERROR`: tag/write/finalization failure
- `INTERNAL_ERROR`: invariant violation or unexpected null/invalid state

Every public pipeline function returns either a result object or an error object containing category, user-facing message, optional source location/function, and nested cause. The CLI owns printing diagnostics and choosing exit codes.

## Memory Ownership

Use single-owner objects with explicit cleanup functions:

- `DecodedImage` owns decoded pixel memory and metadata copied out of the decoder.
- `TiffImage` owns converted pixel memory or a row-provider callback state.
- `ConversionContext` owns transient buffers and allocator references.
- External library handles are wrapped immediately and released in one place.

Rules:

- The module that allocates frees, unless ownership is explicitly transferred in the API name or documentation.
- Prefer `init/free` pairs or RAII wrappers in C++ builds.
- Zero-initialize structs before use; cleanup functions must tolerate partial initialization.
- Never store borrowed pointers to decoder memory beyond the decoder wrapper lifetime.

## Portability Strategy

Keep the portable path primary:

- Standard C11 or conservative C++17, depending on project choice.
- Use fixed-width integer types and explicit endian helpers for TIFF fields.
- Avoid assuming unaligned access, native endian, or SIMD availability.
- Hide filesystem differences behind a small path/temp-file adapter.
- Build optional features through compile-time detection: HEIF backend, TIFF backend, compression libraries, SIMD/assembly.

Tests should cover little-endian and big-endian serialization behavior even if CI only runs on little-endian hosts.

## Inline Assembly Boundary

Inline assembly is optional and must never be required for correctness. Any assembly/SIMD belongs behind a small conversion-kernel interface:

```text
pixel_kernel_select(cpu_features) -> PixelKernels
PixelKernels.rgba8_to_rgb8(...)
PixelKernels.rgba16_to_rgb16(...)
```

Boundary rules:

- The portable C implementation is the reference implementation.
- Assembly files or compiler intrinsics are isolated under `arch/` or `simd/`.
- Runtime CPU feature checks select optimized kernels; builds without them use portable kernels.
- Kernel inputs and outputs are plain buffers with lengths, strides, and format descriptors.
- Tests compare optimized output byte-for-byte with the portable path.

## Minimal Module Layout

```text
src/
  main.c
  cli.c / cli.h
  convert.c / convert.h
  error.c / error.h
  image.h
  decode/heic_decoder.c / heic_decoder.h
  pixels/pixel_convert.c / pixel_convert.h
  pixels/pixel_kernels.c / pixel_kernels.h
  tiff/tiff_writer.c / tiff_writer.h
  platform/filesystem.c / filesystem.h
```

This layout keeps the executable small, isolates third-party APIs, and makes the decode, conversion, and write stages independently testable.
