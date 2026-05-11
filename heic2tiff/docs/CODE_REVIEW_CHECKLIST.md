# Code Review Checklist

Use this checklist for changes to `heic2tiff`, especially decoder, pixel buffer, TIFF writer, and CLI work.

## Build and dependency gates

- [ ] Configure with CMake succeeds on a clean build directory.
- [ ] `cmake --build` succeeds with `-Wall -Wextra -Wpedantic` and no new warnings.
- [ ] `ctest --output-on-failure` passes.
- [ ] If a gate cannot run, record the missing dependency/tool and the exact failing command.

## Memory, ownership, and error paths

- [ ] Every allocated libheif object has one clear release owner on all success and failure paths.
- [ ] Heap pixel buffers are freed exactly once through `h2t_image_free`.
- [ ] `H2TImage` instances are initialized before use and left safe after free.
- [ ] Integer conversions and size calculations are checked before allocation, pointer arithmetic, and library calls.
- [ ] Partial output files are not treated as successful writes after a TIFF setup or scanline error.

## HEIC decode and pixel handling

- [ ] Reject invalid dimensions, channel counts, strides, and oversized decoded images.
- [ ] Copy only valid row bytes from libheif planes; do not assume stride equals width * channels.
- [ ] Keep generic C as the correctness path. Architecture-specific helpers must be optional, tested, and free of inline assembly unless there is a measured need.
- [ ] Document alpha semantics when choosing TIFF `EXTRASAMPLE_*` values.

## TIFF writing

- [ ] Include all headers required for fixed-width and size types instead of relying on transitive includes.
- [ ] Validate image shape and stride again at the writer boundary.
- [ ] Check required `TIFFSetField` calls and scanline writes for failure.
- [ ] Use portable TIFF types and avoid narrowing conversions without range checks.

## CLI UX and portability

- [ ] `--help`/`-h` and `--version`/`-V` are side-effect-free and return success.
- [ ] Invalid invocations print diagnostics and usage to stderr with a non-zero exit code.
- [ ] Input and output paths are both required, non-empty, and not the same literal path.
- [ ] Scripts avoid GNU-only assumptions where practical or include a documented fallback.
- [ ] No destructive file operations are added without explicit user confirmation or a recoverable path.

## Review notes from this pass

- `./scripts/test.sh` passed locally: 11/11 CTest CLI tests.
- Valgrind negative-path smoke test passed with all heap blocks freed and 0 errors.
- No inline assembly is currently present; the architecture files are wrappers around the generic copy path.
- CLI diagnostics were moved to stderr for invalid invocations, with short help/version aliases added.
- TIFF writing now validates image shape/stride and checks required TIFF tag setup.
