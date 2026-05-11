#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-$repo_root/build}"
build_type="${BUILD_TYPE:-Release}"
parallel_jobs="${CMAKE_BUILD_PARALLEL_LEVEL:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)}"

cmake -S "$repo_root" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type"
cmake --build "$build_dir" --parallel "$parallel_jobs"
