# Contributing

Build and run `ctest --test-dir build --output-on-failure` before proposing changes.
Run `./build/phosrack` for UI work; no desktop-shell restart is needed. Keep QML
presentation separate from Spotify protocol and audio analysis. Add regression
coverage for protocol, concurrency and DSP changes rather than snapshot-only tests.

Do not commit credentials, local configuration, build outputs or account data.
Include supported Qt/distribution versions and reproduction steps in bug reports. A
network log must have authorization headers, callback codes and tokens removed.

All contributions must be compatible with GPL-3.0-or-later. Mark new source files
with `SPDX-License-Identifier: GPL-3.0-or-later`. Use original or properly licensed
assets. Code/documentation and source UI strings use English; Spanish and French
catalogs ship alongside them. Project hosting is https://github.com/minibota/phosrack.

For translations, use English source strings with `qsTr`/`tr` and update both
`translations/es.json` and `translations/fr.json`. Preserve `%1`/`%2` placeholders.
Test unsupported-language fallback as well as all supported locales. Build RPM,
DEB and Flatpak with the target toolchain; do not convert binaries across ABIs.
