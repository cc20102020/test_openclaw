# Technical Design: heic2tiff

## Architecture

`heic2tiff` is a small C11 CLI with isolated modules:

- `main.c`: pipeline orchestration and exit codes.
- `cli.c/.h`: argument parsing and help/version output.
- `heic_decode.c/.h`: libheif context, dimension checks, decode, owned pixel buffer.
- `pixel_convert.c/.h`: validates decoded RGB/RGBA pixels and future conversion boundary.
- `tiff_write.c/.h`: libtiff tags and scanline writing.
- `arch/*`: optional architecture-specific pixel helper boundary; generic C remains correctness path.
- `error.c/.h`: stable status codes and messages.

## Decode pipeline

1. Open HEIC/HEIF with libheif.
2. Get primary image handle.
3. Check handle dimensions before decode:
   - width/height > 0
   - each side <= `H2T_MAX_DIMENSION` (4096)
   - pixels <= `H2T_MAX_PIXELS` (4096 × 4096)
4. Decode to interleaved RGB or RGBA.
5. Re-check transformed decoded dimensions.
6. Validate row stride and overflow before allocation/copy.
7. Copy into an owned contiguous `H2TImage` buffer.

## Pixel conversion pipeline

Current TIFF output accepts decoded RGB/RGBA 8-bit pixels directly. `pixel_convert` validates shape/channels. Architecture files are documented stubs that call generic C; no assembly is required for correctness.

## TIFF writer pipeline

1. Validate image pointer, dimensions, channels, and stride.
2. Open output path with libtiff.
3. Set baseline TIFF tags: dimensions, 8 bits/sample, samples/pixel, RGB photometric, contiguous planar config, LZW compression.
4. Mark alpha as unassociated when RGBA is present.
5. Write one scanline per decoded row.
6. Close TIFF handle on all paths.

## Error handling

Public functions return `H2TStatus`. The 4K update adds `H2T_ERR_TOO_LARGE` with message `image exceeds supported 4K limit`. The CLI prints the input path and user-facing error.

## Memory ownership

- `H2TImage.pixels` is heap-owned by the caller after successful decode.
- `h2t_image_free` releases and resets the image.
- libheif context, handle, and image are released in `heic_decode.c` cleanup.
- No global mutable state is introduced.

## Portability strategy

- C11, fixed-width integers, explicit size checks.
- No hard-coded paths.
- Generic C remains primary.
- x86_64/aarch64 files are optional boundaries only.

## 4K limit strategy

The limit supports common iPhone/high-end camera 4K-class still images while bounding decoded RGBA memory to roughly 64 MiB before decoder/writer overhead. Native 48 MP images above 4096 pixels per side are intentionally out of scope for this update.
