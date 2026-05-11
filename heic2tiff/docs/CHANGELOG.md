# Changelog

## Unreleased

- Added a standalone static build path that links HEIC decoding support into the executable and avoids dynamic library loading.
- Removed libtiff build/runtime dependency by replacing it with a built-in uncompressed baseline TIFF writer.
- Added explicit 4K-class image support limits: up to 4096 pixels per side and 4096 × 4096 decoded pixels.
- Added `H2T_ERR_TOO_LARGE` for images above the supported limit.
- Added dimension boundary tests for common iPhone/high-end camera dimensions such as 4032 × 3024 and UHD 3840 × 2160.
- Documented memory, security, performance, API, and deployment notes for the 4K-class update.
