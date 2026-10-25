# MAIN: preserve pose production when omitting native depth

Branch: `fix/main-seeker-graphics-rootcause`, follow-up to `2cc4c63` / tested
DLL `12FEB4999162746ACC85D216D9E644EF3FD6E273F6ED4852139EE94FF49A0F35`.
This does not promote a new accepted baseline or change the installed game.

## Physical result and first proven violation

User: black silhouette removed, but an invisible silhouette now cuts holes in
the gun/glove; Seeker support and shooting work; ordinary support does not
acquire; wheel was not checked. Source evidence:
`build/main-seeker-evidence/session-20261025-161251/view_matrix.log`, SHA256
`21766CA09713162B70CBA6F4FBA8B7864CFAB72B156B8BBCD5E22ACBE10CC3B3`.

- Lines 17920-17924: exact gun and TMD material-0 depth primes are omitted;
  the subsequent Volk draw consumes frame 14451's witness in frame 14452,
  so `ApplyDualActionC` returns false.
- Lines 20718-20721: Valkyrie changes from `applied=0` to `applied=1` WITHIN
  frame 17815. The same-frame controller correction changes after the first
  gun draw because the original wrist witness arrives with the later glove.
- The omit return is before `PrepareIndependentLeft -> CaptureDualActionPose`.
  Omitting rasterization also omitted an unrelated required pose producer.
  `BuildGunC` consumes this frame-keyed witness on individual draw calls.

**Proven:** missing original-wrist production and intra-frame correction
discontinuity. **Inferred:** depth/color pose disagreement causes the physical
hole. The old log does not contain early material-0 CTAB layout or matching
depth/color pixel captures, so physical resolution remains PENDING.

A separate same-frame normal-depth suppression experiment was rejected from
the candidate: its synthetic pixels demonstrated a possible second-prime
mechanism, but the archived log did not prove absence of a depth clear before
that draw. Do not broadly suppress viewport 0/1 foreground depth or alter
ZFUNC/EQUAL, which would damage legitimate foreground rendering.

## Minimal correction

`[Render] TrackedDepthSourcePose=1` (default OFF) retains the ORIGINAL material-0
wrist capture before the existing exact near-prime omission returns.

- `tracked_depth_source.h` requires an already omitted retained VB/IB/range
  sibling, current independent dual-hand context, exact 7444/2626 material-0
  range (first 0, 4402 triangles), non-shadow CTAB bones 5 / 225 regs / local 230.
- It reads only LocalToWorld and original wrist slots 1 and 19 (bones 9/51),
  using the same shader layout and original bind data as the late producer.
- Early and late publication use the existing `dualaction::Frame::Capture`.
  No new aim math, offsets, pose freezing, GPU writes, index changes, extra
  geometry, native calls or changes to depth suppression are introduced.
- Missing/invalid layout or context refuses capture. Unsupported Seeker retains
  its accepted independent pose machinery; no ordinary reference is invented.
- Bounded `tracked-depth-source: CAPTURE/REFUSED` reports the actual early
  layout. `compare ... equal=1` compares original local and wrist uploads in
  the same frame. `equal=0` falsifies the equal-source assumption and records
  maximum absolute differences; an unequal early palette is NOT frozen.

Existing TrackedDepthPrimeFix and DualHandActionPose gates must also be ON.
SeekerBackSlot stays OFF, preserving the requested native heavy lifecycle.

## Ordinary support: diagnostic, not a claimed fix

Imported isolated commits `e00b5c05` / `80e5099c` as `3f921589` / `a960e4a4`.
See `ordinary-support-acquisition-20261025.md`. The actual session contains
48 effective Grip presses and no ordinary latch, then a successful Seeker
latch. Original acquisition code and distances are unchanged. Neither stuck
reload ownership nor a generally reversed barrel is supported by the evidence.

`[Diagnostic] SupportGripProbe=1` (default OFF) records raw/effective input,
context, reservation, exact controller distance and cone alignment, and the
existing latch result. At most 24 squeezes, press/acquire/last-held-release
only; no per-frame logging or input changes. It reports the first failed
condition instead of broadening a reach radius blindly.

## Offline validation and candidate provenance

The candidate manifest records the exact source SHA, clean x86 PE32 `/O2`
DLL SHA, INI delta, accepted-profile hash and test logs. The D3D9/D3D9Ex
fixture exercises the production depth cache, original constant reader and
frame correction in early-prime -> first gun depth -> glove color -> gun
color order, including negative controls and both-eye pixels: PASS on D3D9
and D3D9Ex for both LEQUAL and EQUAL. The overlap changes from background
`204080` (hole) to gun `FFFFFF`; unrelated background remains `204080`.
CPU regressions
cover original animation/scale, stale identity, support gates, reload, measured
aim and WeaponLocker. None establishes headset acceptance.

## Next controlled PSVR2 session

Do not require chat input while wearing the headset. Use the same save:

1. With Valkyrie, move the gun across the area where the invisible silhouette
   previously cut it; rotate the left TMD and use Square once. Check that gun,
   glove and background stay complete.
2. Hold left Grip at the visible Valkyrie fore-end for about one second, move
   the hand slightly along it while holding, then release. Repeat with Volk.
   This exercises normal support and captures its exact refusal if it fails.
3. Open R3 once, check readable icons/background, close the game normally.

Seeker support/shooting already passed the previous physical test; do not ask
the user to repeat unchanged behavior solely for this measurement. After exit,
verify restore hashes and inspect CAPTURE/compare plus support-acquire gates.
