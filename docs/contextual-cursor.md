# Contextual cursor: confirmation, notes and pickup dialogs

The 2026-10-18 PSVR2 test accepted save-slot/confirmation separation but still
showed an arrow and laser offset in contextual windows. This follow-up is
L1_PASS/L2_PASS and READY_FOR_PHYSICAL_ACCEPTANCE; no runtime/pixel acceptance
is claimed for the new projection or cursor packages.

## Proven producers and mechanism

The preserved Steam PID 36352 log identifies `Dialogs` RawData as 118739 bytes,
FNV64 `28B71A137B55BB7A`, with null RenderTexture and RenderTextureMode=0.
Independent decoding of the installed `RvGame.xxx` finds that exact movie and
its exported Mouse sprite 3 / exclusive DefineShape leaf 2: the original arrow.
The installed single-player package's iis/SPMenus movies already contain the
older static ring. SPMenus also supplies SPNotepad. A menu-only shape change
therefore missed the Dialogs producer in the separate script package.

The pointer plane used class-4 inset 1.0, while `HudStereoPair` uses the shared
menu-safe cap 0.84. Notes use an independent 0.40 inset. A centre-only horizontal
eye shift cannot make those different panel extents agree at their edges.
The laser also independently reread UV after the reticle endpoint was computed.

## Controlled change

`Render/ContextPointerProjection=0` is the default. When explicitly enabled,
class-4 movies must prove their direct path by reflected properties before the
new mapping runs. Native GFx's source clip matrix is composed with the same
tracking-space panel and per-eye projection used for ray intersection. All XYW
terms are included; note/page scale is shared, and the existing note-dimming
exception remains. RTT-backed menu producers retain their existing path.
The laser and reticle receive one computed endpoint per presentation.

`tools/title/prepare_context_cursor.py` changes only the exclusive Mouse leaf
in iis, SPMenus and Dialogs, preserving its exact payload length. The native
visual becomes a small centred white core; the mod supplies the animated ring.
A small native pointer remains available with VR disabled. Sprite placement,
ActionScript, hit tests, package/export offsets, fonts, titles and all other
logical bytes remain identical. Legal MoveTo-only SHAPE records fill the fixed
payload; an independent shape reader verifies a closed centred core. No game
assets are committed.

Only these exact installed packages are supported:

- UI_SinglePlayer_SF.xxx: `51CDAB5C...1A1A` -> `AD2029B8...C8CAC`.
- RvGame.xxx: `4153C488...9E9EF` -> `64A9BDA3...DC272`.

Both candidates passed independent native LZO decode roundtrips and whole-file
logical comparisons outside the three declared shape payloads. The full input,
output and movie hashes are in the generated reports under
`build/pointer-followup/{ui-candidate,dialog-candidate}/report.json`.

## Verification and next physical test

- `tools/tests/test-menu-polish.ps1`: projective mapping versus independently
  projected panel points (160 points, both eyes, asymmetric eye offsets/rotation,
  84% dialog and 40% note scales), invalid-projection refusal, existing policies.
- `tools/tests/test-frontend-menu-comfort.ps1`: six related CPU regressions PASS.
- `python -m unittest discover -s tools/title/tests -v`: 12 PASS, including fixed
  shape length, independent edge decoding and action/reference preservation.
- Optimized x86 PE32 `/O2` renderer build PASS. The final generated build
  manifest accompanies the candidate DLL; deployment plan pins its exact hash.
- The two-file asset lease is exercised against a private fake installation:
  WhatIf, install/restore, corrupt-backup refusal, conflict preservation and
  hidden no-launch timeout restore. The DLL/INI guard is exercised separately
  against a fake Steam root. Neither test changes the real installation.

On a fresh user `start`, MAIN installs the DLL/INI using the accepted-profile
guard and installs the two cursor packages using the exact v8 asset manifest.
Arm disjoint `watch-menu-polish-restore.ps1` and
`watch-prepared-assets-restore.ps1` leases before Steam launch. Both restore
their exact baselines after normal exit or a no-launch timeout. Verify hashes.

One uninterrupted headset route: New Game -> open confirmation without accepting
an overwrite -> back -> existing gameplay -> pickup message and a note -> pause
and back -> close normally. User observes arrow replacement and laser/ring/button
agreement at centre and edges. No headset-to-chat switch during the route.

Falsifier: correct candidate movie fingerprints and `context-pointer` projection
logs, but a remaining arrow or position error. That requires identifying the
remaining producer or native stage/dispatch mapping; do not add a guessed offset.
Credits' separate Mouse export is intentionally outside this tested change.
