# Security Checklist

This checklist tracks security-sensitive behavior for `heic2tiff`, especially around untrusted HEIC inputs and filesystem output handling.

## Input Parsing and Decoder Boundary

- [x] Treat every HEIC/HEIF input as untrusted, including malformed containers, truncated files, unusual metadata, and unsupported codecs.
- [x] Decode through libheif only behind `h2t_decode_heic`; do not leak libheif-owned pointers outside the decode layer.
- [x] Release libheif handles and call `heif_deinit()` on all decode return paths observed in the current flow.
- [x] Reject invalid decoded dimensions (`width <= 0`, `height <= 0`) before allocation or pointer arithmetic.
- [x] Validate decoded plane pointer and stride before copying pixels.
- [x] Ensure source stride is at least the expected row byte count before `memcpy` to avoid out-of-bounds reads from malformed or unexpected decoder output.
- [ ] Add regression tests with malformed/truncated HEIC fixtures once test fixtures are available.

## Huge Dimensions, Integer Overflow, and Allocation Failure

- [x] Check `width * height * channels` with `SIZE_MAX` guards before allocation.
- [x] Reject decoded frames larger than the configured dimension/pixel caps (`H2T_MAX_DIMENSION` and `H2T_MAX_PIXELS`, currently 4096 px per side and 4096 × 4096 total pixels) to reduce denial-of-service risk from decompression bombs or extreme dimensions. This bounds the full-frame decoded RGBA buffer to about 64 MiB before libheif/libtiff overhead while supporting 4K-class image workflows.
- [x] Check row byte count against `INT_MAX` before comparing with libheif's `int` stride.
- [x] Handle `malloc` failure and return `H2T_ERR_ALLOC` without dereferencing null.
- [ ] Consider making the dimension/pixel limits configurable for trusted batch workflows.
- [ ] Add unit tests or mocks for overflow boundaries and allocation-limit rejection.

## Pixel Copying and String Safety

- [x] Current code uses bounded `memcpy` with validated row sizes.
- [x] No unsafe string-copy/format functions found in the reviewed source (`strcpy`, `strcat`, `sprintf`, `gets`, unbounded `scanf`).
- [x] CLI path arguments are borrowed from `argv` and not copied into fixed-size buffers.

## Output Path Handling

- [x] Current implementation passes the user-provided output path directly to libtiff and does not perform shell expansion or command execution.
- [ ] MVP requirement says existing outputs should be protected by default; current implementation opens with `TIFFOpen(path, "w")` and may overwrite existing files, though identical literal input/output paths are rejected. Add explicit overwrite policy before release.
- [ ] For robust writes, write to a temporary file in the destination directory, close it, then atomically rename to the final path.
- [ ] Ensure temporary-file cleanup only removes files created by this process.
- [ ] Document symlink behavior and decide whether to reject symlink outputs in high-security contexts.

## TIFF Writer Robustness

- [x] Reject null image/path/pixel input before opening output.
- [x] Return an error if any scanline write fails.
- [x] Check return values from required `TIFFSetField` calls.
- [ ] Consider BigTIFF or explicit rejection if future dimensions exceed classic TIFF limits.
- [x] Use unassociated alpha TIFF tagging for libheif RGBA output unless a future decoder path explicitly premultiplies alpha.

## Dependency CVE Awareness

- [ ] Track libheif security advisories and distro package updates.
- [ ] Track libtiff security advisories and distro package updates.
- [ ] In CI/release builds, record dependency versions (`pkg-config --modversion libheif libtiff-4`).
- [ ] Consider running a dependency scanner appropriate to the packaging target before releases.

## Fuzzing Recommendation

- [ ] Add a libFuzzer/AFL++ harness for the decode path using in-memory HEIF input where libheif supports it, or a temporary-file harness otherwise.
- [ ] Seed fuzzing with valid iPhone HEICs, truncated files, random bytes, oversized metadata, multi-image containers, alpha/no-alpha variants, and unusual dimensions.
- [ ] Run sanitizer builds (`ASan`, `UBSan`) during fuzzing and on CI smoke tests.
- [ ] Add corpus minimization and crash repro instructions under `docs/` or `tests/fuzz/`.

## Build Hardening Recommendations

- [ ] Enable sanitizer presets for development builds.
- [ ] For release packaging, consider hardening flags such as stack protector, PIE, RELRO/NOW, and `_FORTIFY_SOURCE` where supported by the platform/toolchain.
- [ ] Keep the portable C conversion path as the correctness reference for any future SIMD/assembly optimizations.

## 4K limit security notes

- Decode rejects images above `H2T_MAX_DIMENSION` or `H2T_MAX_PIXELS` before allocating the output buffer.
- The limit reduces risk from oversized or malicious image dimensions.
- Integer overflow checks remain required before width × height × channels calculations.
- The limit is product behavior, not a substitute for libheif/libtiff CVE monitoring.
