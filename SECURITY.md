# Security and privacy

Refresh tokens use KWallet on Linux (Phosrack folder) and Windows Credential Manager
on Windows, namespaced by the public Client ID.
Access tokens, PKCE verifiers and OAuth state exist only in process memory. The
Client ID is public configuration, not a secret. Release builds include the project
application ID; this grants no access to the developer's account. Each user authorizes
their own account, subject to Spotify's application access restrictions. Never paste a Client Secret into
the application. No tokens are stored in QSettings, logs or source files.

The audio monitor runs while the visualizer is visible, following the default output
unless overridden in Settings. It stops when folded, hidden or minimized. It can
include audio from other apps on that sink. Samples are processed in memory
and are not saved or uploaded. Audio processes use fixed executables and argument
lists without a shell; capture targets are selected from discovered output nodes.

Use the repository Security tab for available private reporting options. If no
private channel is listed, open an issue asking for one without disclosing the vulnerability. Do not open a public issue containing
live tokens. Supported security fixes initially target the latest development release.
