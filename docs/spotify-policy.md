# Spotify policy review

Reviewed 10 November 2026 for v0.3.3. This records the project's technical decisions
and remaining questions; it is not Spotify approval or a guarantee of compliance.

## Request limits and the 429 correction

Spotify has two distinct mechanisms:

- **Short-term rate limits:** app requests are counted in a rolling 30-second
  window. Endpoint-specific limits can also apply. Spotify does not publish one
  universal safe request count in this documentation.
- **Development quotas:** endpoint buckets are shared by every development app
  owned by the same developer account. This changed in July 2026. A quota response
  can contain `error.reason: QUOTA_EXCEEDED`. Creating a new Client ID under that
  account does not create a fresh quota.

Sources: [rate limits](https://developer.spotify.com/documentation/web-api/concepts/rate-limits),
[development quotas](https://developer.spotify.com/documentation/web-api/concepts/quota-modes#quota-counting),
[July 2026 change](https://developer.spotify.com/documentation/web-api/references/changes/july-2026#per-account-quota-counting).

v0.3.2 queried playback every three seconds even when the integrated SDK supplied
state. Two visible UI components could also poll the same queue. Those unnecessary
requests were a plausible contributor to the reported limit; the screenshot does
not prove which requests exhausted the allowance.

v0.3.3 uses SDK state for its local player, reduces remote polling and coalesces
recent queue requests. A 429 is processed even if its original search is obsolete.
The server's `Retry-After` deadline survives restart and disconnect, updates the UI
as a remaining wait, and cannot be shortened by another response. The client avoids
repeated requests during that pause. This does not clear a restriction already
imposed by Spotify or control traffic from other applications.

Spotify's [Developer Terms, IV.2.4 and VI](https://developer.spotify.com/terms)
restrict unnecessary excessive calls and permit usage restrictions. Client-ID
rotation is not a recovery mechanism.

## Account access and application credentials

Development applications support up to five approved users; their owner must have
Premium. Browser authorization alone does not put an account on the allowlist.
Extended public access requires Spotify's approval and eligibility criteria,
currently including an established organization and at least 250,000 monthly active
users. See [quota modes](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

Phosrack's Client ID field supports a personal development setup. It is not a promise
that distributing this application with each user's ID is approved. Spotify's terms
limit security codes per application, while its policy addresses replacements of
Spotify's core experience. These remain questions for Spotify to clarify in writing
before claiming approval for broader distribution. Open-source licensing does not
override Spotify's conditions.

## Audio tools and content presentation

The [Web Playback SDK policy notes](https://developer.spotify.com/documentation/web-playback-sdk/reference)
require unchanged audio content and prohibit synchronization with visual media.
The [Developer Policy, III](https://developer.spotify.com/policy) also restricts
content analysis. We found no explicit exception for a transient FFT or system EQ.
The documents do not specifically adjudicate Phosrack's operating-system DSP.

The conservative product boundary is therefore:

- **Spotify mode:** no Phosrack output capture or EQ; phosphor animation is ambient,
  not audio-driven. Song artwork and metadata remain part of the player.
- **Local audio tools:** Spotify is suspended in Phosrack, and the user must
  confirm use with non-Spotify sound. These tools have no Spotify fullscreen overlay.
  Users must also stop Spotify in other apps: a mixed system output cannot establish
  which content every application is playing.

This separation reduces the known conflict; it is not a claim that every remaining
feature has received permission. The core-experience and personal-credential
questions above remain unresolved. Do not describe OS capture as a policy exemption.

Content displays use Spotify's official monochrome logo, preserve artwork
proportions without cropping or controls over the image, and link back to Spotify.
See [branding guidelines](https://developer.spotify.com/documentation/design) and
[artwork provenance](../THIRD_PARTY.md). Metadata and artwork are temporary, and the
app does not provide audio downloads, recording or offline playback. Spotify's
[attribution and display rules](https://developer.spotify.com/policy#ii-respect-content-and-creators)
continue to apply.

## Authorization, privacy and distribution

The requested permissions cover implemented playback controls, queue, library and
playlist browsing. Streaming additionally uses the SDK's documented
[streaming and profile scopes](https://developer.spotify.com/documentation/web-playback-sdk/howtos/web-app-player#request-user-authorization).
The queue reference lists both playback-state and currently-playing permissions;
this release retains them. It requests no playlist/library write permission.

[PRIVACY.md](../PRIVACY.md) describes local storage, external browser cookies and
account removal. [TERMS.md](../TERMS.md) separates Spotify-specific obligations from
Phosrack's GPL rights. The connection flow presents both before first use.
Disconnect requests removal of the refresh token, clears account content and stops
the SDK. A persistent sign-out marker prevents reuse if protected storage refuses
deletion; the interface explains how to remove the entry manually.
Automatic restoration must not override an explicit disconnect.

The official SDK is fetched from Spotify at runtime. Spotify passwords are never
collected by Phosrack, DRM is not bypassed, and Widevine is not bundled or downloaded.
A compatible existing module is still a platform prerequisite, not a distribution
license. Technical playback tests do not establish Spotify approval.
