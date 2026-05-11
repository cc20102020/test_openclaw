#!/usr/bin/env bash
set -euo pipefail

sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  libheif-dev \
  libtiff-dev \
  valgrind
