# Product Requirements Document: heic2tiff

## Problem

Users often receive HEIC/HEIF images from iPhones and modern cameras, but many archival, publishing, scientific, and document workflows require TIFF. Existing conversion tools are frequently GUI-only, lossy by default, hard to automate, or unclear about metadata/color-profile preservation.

`heic2tiff` should provide a simple, scriptable CLI that reliably converts HEIC images to TIFF with predictable output and useful failure messages.

## Target User

- Developers and operators automating image conversion in scripts or pipelines.
- Photographers, archivists, and designers who need TIFF outputs from HEIC sources.
- Power users who prefer command-line batch processing over manual GUI conversion.

## CLI Use Cases

- Convert one file:
  - `heic2tiff input.heic output.tiff`
- Convert one file into an output directory using the source basename:
  - `heic2tiff input.heic --out-dir converted/`
- Batch convert multiple files:
  - `heic2tiff *.heic --out-dir converted/`
- Recursively convert a directory:
  - `heic2tiff photos/ --recursive --out-dir tiffs/`
- Control overwrite behavior:
  - `heic2tiff input.heic output.tiff --overwrite`
- Preserve metadata and color profile by default when supported.
- Emit machine-friendly exit codes and concise errors for failed conversions.

## MVP Scope

- CLI executable named `heic2tiff`.
- Accept HEIC/HEIF input file paths and produce `.tif`/`.tiff` output files.
- Single-file and multi-file batch conversion.
- Optional `--out-dir` for batch outputs.
- Optional `--recursive` for directory inputs.
- Safe default behavior: do not overwrite existing files unless `--overwrite` is provided.
- Preserve orientation, basic EXIF metadata, and embedded color profile when the underlying decoder/encoder supports it.
- Clear progress summary: converted, skipped, failed.
- Non-zero exit status when any requested conversion fails.
- Help/version flags: `--help`, `--version`.

## Non-MVP Scope

- GUI application or drag-and-drop interface.
- Editing features such as resize, crop, rotate, denoise, or watermarking.
- Advanced TIFF tuning beyond sensible defaults, such as custom compression modes, bit depth selection, tiling, or BigTIFF.
- Cloud storage integrations.
- Watch-folder/daemon mode.
- RAW camera format support.
- Lossless guarantees beyond what available HEIC decoding libraries can provide.

## Success Criteria

- A user can install and run `heic2tiff` without writing code.
- Single-file conversion succeeds for common iPhone HEIC images.
- Batch conversion handles at least hundreds of files without crashing on the first bad input.
- Existing outputs are protected by default.
- Error messages identify the failed input and likely cause.
- The CLI behavior is documented with examples.
- Automated tests cover success, overwrite protection, missing input, invalid input, batch partial failure, and output path handling.

## Risks

- HEIC/HEIF decoding support may depend on platform-specific native libraries.
- Metadata and color-profile preservation can vary by decoder/encoder implementation.
- Patent/licensing concerns around HEIC libraries may affect distribution choices.
- Large image batches may expose memory or performance issues.
- Multi-image HEIC containers and animated HEIC files may require explicit handling or documented limitations.
