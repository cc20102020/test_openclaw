#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-$repo_root/build}"

"$repo_root/scripts/build.sh"
ctest --test-dir "$build_dir" --output-on-failure
