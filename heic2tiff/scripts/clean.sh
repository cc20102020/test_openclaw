#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-$repo_root/build}"

case "$build_dir" in
  "$repo_root"|"$repo_root"/*) ;;
  *)
    if [[ "${ALLOW_EXTERNAL_BUILD_DIR:-0}" != "1" ]]; then
      echo "Refusing to remove BUILD_DIR outside repo: $build_dir" >&2
      echo "Set ALLOW_EXTERNAL_BUILD_DIR=1 to override." >&2
      exit 2
    fi
    ;;
esac

if [[ -d "$build_dir" ]]; then
  rm -rf -- "$build_dir"
  echo "Removed $build_dir"
else
  echo "Nothing to clean: $build_dir does not exist"
fi
