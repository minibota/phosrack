#!/usr/bin/env bash
set -euo pipefail
install_prefix="${PHOSRACK_INSTALL_PREFIX:-$HOME/.local}"
rm -f -- "$install_prefix/bin/phosrack" "$install_prefix/bin/phosrack-eq-worker" \
  "$install_prefix/share/applications/io.phosrack.Phosrack.desktop" \
  "$install_prefix/share/metainfo/io.phosrack.Phosrack.metainfo.xml" \
  "$install_prefix/share/icons/hicolor/scalable/apps/io.phosrack.Phosrack.svg" \
  "$install_prefix/share/licenses/phosrack/LICENSE" \
  "$install_prefix/share/doc/phosrack/README.md" "$install_prefix/share/doc/phosrack/GETTING_STARTED.md" "$install_prefix/share/doc/phosrack/LEEME.md"
echo 'Phosrack removed. Personal settings and wallet entries are preserved.'
