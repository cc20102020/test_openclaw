# Deployment Guide

## Local build

```bash
bash scripts/setup_ubuntu.sh
bash scripts/build.sh
bash scripts/test.sh
```

## CI

GitHub Actions installs Ubuntu dependencies, configures CMake, builds, and runs CTest. No new dependencies are required for 4K-class HEIC support.

## Runtime notes

For large iPhone/high-end camera images, ensure enough disk space for TIFF output. A compressed HEIC can expand into a much larger TIFF.
