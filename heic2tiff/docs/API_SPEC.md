# API / CLI Specification

## Command

```bash
heic2tiff input.heic output.tiff
heic2tiff --help
heic2tiff --version
```

## Exit codes

- `0`: success
- `2`: invalid arguments
- `3`: HEIC decode failed
- `4`: pixel conversion failed
- `5`: TIFF write failed
- `6`: memory allocation failed
- `7`: image exceeds supported 4K limit

## Image limits

The decoder accepts images up to 4096 pixels per side and up to 4096 × 4096 decoded pixels. Common iPhone still images such as 4032 × 3024 and UHD 3840 × 2160 are supported. Larger images are rejected with exit code `7`.
