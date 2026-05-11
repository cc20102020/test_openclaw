# Product Requirements Document: heic2tiff

## Problem

Users need a Linux CLI tool that converts HEIC/HEIF still images from iPhones and modern cameras into TIFF without GUI steps or hard-coded paths. The tool must handle common 4K-class images safely while avoiding unbounded memory allocation.

## Target user

- Linux users converting iPhone/high-end camera HEIC still images.
- Developers/operators automating image conversion in scripts.
- Photographers/archivists needing TIFF output from HEIC sources.

## Current CLI use cases

```bash
heic2tiff input.heic output.tiff
heic2tiff --help
heic2tiff --version
```

## MVP scope

- Single HEIC/HEIF input to single TIFF output.
- Decode primary image with libheif to RGB/RGBA.
- Write RGB/RGBA TIFF with libtiff.
- Support common 4K-class still images:
  - maximum dimension: 4096 pixels per side
  - maximum decoded pixels: 4096 × 4096
  - common iPhone 12 MP stills such as 4032 × 3024
  - UHD 3840 × 2160
- Reject images above the configured limit with a clear error.
- Keep portable C11 path as correctness path.
- Keep inline assembly optional and non-required.

## Non-MVP scope

- Batch/directory conversion.
- Resizing, cropping, rotation UI, metadata editing, or cloud integrations.
- 48 MP+ native still support above 4096 pixels per side.
- BigTIFF or custom compression/bit-depth controls.
- Pixel-perfect color-management guarantees beyond libheif/libtiff behavior.

## Acceptance criteria

- Project builds with CMake on Linux.
- `bash scripts/test.sh` passes.
- Real iPhone 4032 × 3024 HEIC smoke test converts to TIFF.
- Valgrind on the real smoke path reports no leaks/errors.
- Oversized dimensions are rejected before output buffer allocation.
- Documentation describes the 4K-class limit and limitations.

## Risks

- HEIC decoder behavior depends on installed libheif codec support.
- TIFF output can be much larger than compressed HEIC input.
- 48 MP/high-resolution camera images may exceed the 4K-class product cap.
- Color profile and HDR handling are limited by current 8-bit RGB/RGBA decode path.
