# Beagle Media + Routing Changes

This document summarizes the non-MVP integration work completed for Beagle media/file handling and channel routing.

## Scope

- Accept media/files from Beagle clients (including Beagle iOS JSON/base64 payloads).
- Persist inbound media on sidecar host for downstream model/image tools.
- Send media/files back from OpenClaw replies through Beagle channel.
- Normalize peer IDs so `beagle:<id>` and raw `<id>` do not break outbound routing.

## `packages/beagle-sidecar`

- Added real Carrier file transfer send/receive integration (`carrier_session` + `carrier_filetransfer`), not text-only fallback.
- Implemented payload support paths:
  - native file-transfer path
  - beaglechat packed file payload
  - inline JSON media payload (`type=image|file`, base64 `data`)
- Added media persistence and metadata propagation:
  - stores files under sidecar media directory
  - exposes `mediaPath`, `mediaType`, `filename`, `size` in inbound events
- Added send-media path from sidecar API to Carrier transfer path.
- Added filename/media-type inference, payload size caps, and safer parsing helpers.

## `packages/beagle-channel`

- Added peer ID normalization (`beagle://`, `beagle:` prefixes stripped) for outbound send paths.
- Added media-aware inbound context wiring:
  - `MediaUrl`, `MediaPath`, `MediaType`
  - `MediaUrls`, `MediaPaths`, `MediaTypes`
  - `Filename`, `MediaSize`
- Added media-only message hinting (`Image attached...`) when text body is empty.
- Updated reply delivery path to support both URL and local-path media payloads when sending back to Beagle.

## Runtime/Operations Notes

- Beagle sessions can persist per-session model pins. If model behavior diverges from global defaults, clear Beagle session entries from:
  - `~/.openclaw/agents/main/sessions/sessions.json` keys starting with `beagle:`
- Restart gateway after session/model cleanup:
  - `systemctl --user restart openclaw-gateway.service`

## Validation Performed

- `beagle-sidecar` CMake build completed.
- `beagle-channel` TypeScript build completed.
- Live gateway logs confirmed Beagle inbound events, media metadata flow, and outbound media/text dispatch path activation.
