#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-$project_dir/build}"
install_prefix="${PHOSRACK_INSTALL_PREFIX:-$HOME/.local}"
if [[ ! -x "$build_dir/phosrack" ]]; then
  echo 'Build first: cmake -S . -B build -G Ninja && cmake --build build' >&2
  exit 1
fi
cmake --install "$build_dir" --prefix "$install_prefix"
echo "Phosrack installed under $install_prefix. Open it from the applications menu."
