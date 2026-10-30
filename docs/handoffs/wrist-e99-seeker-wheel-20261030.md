# Wrist HUD size/E99 and Seeker wheel — 2026-10-30

## Accepted baseline
User physically accepted wrist candidate 06609B2D (source 8c323ebd): inward wrist presentation, hiding on outward rotation and occlusion. VFX B7481E52 was previously physically accepted and is preserved. New requests: slightly larger panel, E99 information, Seeker in the weapon wheel.

## Changes and evidence
- Default wrist scale increases 100 -> 110 percent; explicit INI overrides remain honored. No attachment, facing, depth or VFX change.
- Live session `build/wrist-main/session-06609B2D/view_matrix.log` had valid pawn health but HUD=0 and invalid optional counters. Actual decoded exports prove `sharedHUD` belongs to `ClientSetCinematicMode`, whereas `myHUD` belongs to `PlayerController`. The cached source now resolves registered `myHUD`, retaining controller/pawn identity, freshness and unavailable-field hiding. E99 Tech uses the existing native mE99TechCount producer; no fabricated values or guessed offsets.
- When the existing native Seeker slot is enabled, an unused catalog slot becomes a fifth weapon position. It uses the existing dev-seeker atlas sprite. Rendering and selection share catalog order and five-sector geometry. Ordinary four-bit inventory masks and native cycling remain unchanged.
- Selection requests the existing owned Seeker through the engine-thread selector and its current pawn/ownership/native ABI gates. Re-selecting equipped Seeker is a no-op. Back-holster toggle remains unchanged. No spawning, ammunition changes or artificial ownership.

## Offline verification
- `tools/hud/verify_wrist_hud_source_fields.py`: actual decoded RvGame/Engine export ownership PASS.
- `tools/reload/test-seeker.ps1`: real x86 native adapter, 100 selector ABI calls, ownership/lifecycle refusals, explicit wheel selection and repeat no-op PASS; original ammo bytes preserved.
- `tools/test-ui-cleanup.ps1`: catalog additions, native cycling preservation, input/layout (2..8 sectors), UI and D3D9/D3D9Ex regressions PASS.
- `tools/tests/test-hud-live.ps1`: live-source gates, 12 palette/ride cases, both-eye world/foreground glove/magazine depth pixels, failures/reset and state restoration PASS.
- Logs: build/wrist-main/{seeker-wheel-tests,ui-seeker-tests,hud-e99-tests}.log.

## Physical acceptance pending
One session: read the enlarged inward-wrist panel, verify E99 Tech against the native balance and a pickup/spend, then select an actually owned Seeker from R3 and return to an ordinary weapon. An unowned Seeker must not be selectable. No physical result or performance improvement is inferred from offline tests. Current Steam remains accepted 06609B2D until an authorized test deployment.

## Candidate identity
- Source: b9f7f42ec698368f5bd67a6d44baa71fdd552435, branch feature/wrist-e99-seeker-wheel (pushed).
- DLL: 57840426EBA3799D4E6C11557E00F8BCE16B8BF1717C41E1E4F96BD4CD207482.
- MSVC x86 PE32 /O2 PASS; tracked source clean at build. Existing unrelated untracked private build/hd-physical-followup excluded.
- Staged: build/next-weapon-reload/wrist-e99-seeker-20261030. Profile/change-plan WhatIf preflight PASS; no Steam writes or launch.
- Installed baseline remains 06609B2D64E4000EBCCD2D76EDACB569AC8A8A45E1467FAEA3B20D5E6CB46657. Installed INI has no explicit scale override, so 110 percent default applies on deployment.
- New changes: READY_FOR_PHYSICAL_ACCEPTANCE. Actual E99 balance updates and Seeker wheel appearance/retrieval remain PENDING.
