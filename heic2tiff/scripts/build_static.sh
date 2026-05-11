#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-static"
DEPS_DIR="${BUILD_DIR}/deps"
SRC_DIR="${BUILD_DIR}/src"
INSTALL_DIR="${BUILD_DIR}/static-prefix"
LIBDE265_VERSION="1.0.15"
LIBHEIF_VERSION="1.17.6"

mkdir -p "${SRC_DIR}" "${INSTALL_DIR}"

fetch() {
  local url="$1"
  local out="$2"
  if [[ ! -f "${out}" ]]; then
    curl -L --fail --retry 3 -o "${out}" "${url}"
  fi
}

fetch "https://github.com/strukturag/libde265/releases/download/v${LIBDE265_VERSION}/libde265-${LIBDE265_VERSION}.tar.gz" \
  "${SRC_DIR}/libde265-${LIBDE265_VERSION}.tar.gz"
fetch "https://github.com/strukturag/libheif/releases/download/v${LIBHEIF_VERSION}/libheif-${LIBHEIF_VERSION}.tar.gz" \
  "${SRC_DIR}/libheif-${LIBHEIF_VERSION}.tar.gz"

if [[ ! -d "${SRC_DIR}/libde265-${LIBDE265_VERSION}" ]]; then
  tar -xf "${SRC_DIR}/libde265-${LIBDE265_VERSION}.tar.gz" -C "${SRC_DIR}"
fi
if [[ ! -d "${SRC_DIR}/libheif-${LIBHEIF_VERSION}" ]]; then
  tar -xf "${SRC_DIR}/libheif-${LIBHEIF_VERSION}.tar.gz" -C "${SRC_DIR}"
fi

cmake -S "${SRC_DIR}/libde265-${LIBDE265_VERSION}" -B "${DEPS_DIR}/libde265" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  -DENABLE_DECODER=OFF \
  -DENABLE_ENCODER=OFF
cmake --build "${DEPS_DIR}/libde265" --parallel "$(nproc)"
cmake --install "${DEPS_DIR}/libde265"

PKG_CONFIG_PATH="${INSTALL_DIR}/lib/pkgconfig" cmake -S "${SRC_DIR}/libheif-${LIBHEIF_VERSION}" -B "${DEPS_DIR}/libheif" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  -DENABLE_PLUGIN_LOADING=OFF \
  -DWITH_LIBDE265=ON \
  -DWITH_X265=OFF \
  -DWITH_AOM_DECODER=OFF \
  -DWITH_AOM_ENCODER=OFF \
  -DWITH_DAV1D=OFF \
  -DWITH_EXAMPLES=OFF \
  -DWITH_GDK_PIXBUF=OFF \
  -DWITH_REDUCED_VISIBILITY=ON
cmake --build "${DEPS_DIR}/libheif" --parallel "$(nproc)"
cmake --install "${DEPS_DIR}/libheif"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DH2T_STATIC_STANDALONE=ON \
  -DH2T_STATIC_PREFIX="${INSTALL_DIR}"
cmake --build "${BUILD_DIR}" --parallel "$(nproc)"

file "${BUILD_DIR}/heic2tiff"
