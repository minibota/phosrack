# Security and privacy

Refresh tokens are stored in KWallet under the Phosrack folder, keyed by Client ID.
Access tokens, PKCE verifiers and OAuth state exist only in process memory. The
Client ID is public configuration, not a secret. Never paste a Client Secret into
the application. No tokens are stored in QSettings, logs or source files.

The audio monitor runs while the visualizer is visible, following the default output
unless overridden in Settings. It stops when folded, hidden or minimized. It can
include audio from other apps on that sink. Samples are processed in memory
and are not saved or uploaded. Audio processes use fixed executables and argument
lists without a shell; capture targets are selected from discovered output nodes.

Report vulnerabilities privately through the hosting platform's security advisory
mechanism once the repository is published. Do not open a public issue containing
live tokens. Supported security fixes initially target the latest development release.
