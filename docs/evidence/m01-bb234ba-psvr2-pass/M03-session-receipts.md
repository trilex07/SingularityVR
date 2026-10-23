# M03 live HUD — runtime receipts found inside the archived M01 PASS session log

Log: `view_matrix.log` (same session, SHA256
`89D5479D9848CE109199151CB0CEEA8B71664554B4383E6F5A47455115701CBE`), build
`bb234ba` (contains `591d220` M03 corrections). These are bounded receipts
from the same game session, not a dedicated HUD test.

## Health decoding (the old integer-bit defect)

- `health plate: shown (face-raise) hp=150.000000/150.000000` then
  `health plate: shown (face-raise) hp=101.000000/150.000000` — the player took
  damage during the session and the live float decoding updated the plate from
  full to 101/150 (≈67 %). Under the old bit-pattern decoder such values render
  near-full; the falsifier for `DecodeLiveHealth` did not fire.
- No second health producer line appeared; the duplicate route is blocked in
  the log: `health plate: gate blocked (flag=1 vr=1 dup=1 paused=0 hand=0 eyes=1)`
  — the Katorga plate precedence engaged and the old strip did not also draw.

## Ammo counter follows real state

- `ammo plate: art ready (4 plates + digits)`,
  `ammo plate: HUD cache clip+1404 bin+1400 size+1408 on RvHUD`,
  `ammo plate: ammo unknown (clip=-1 bin=-1 ...) - hidden until readable` then
  `ammo plate: drew plate=2 clip=6 bin=63 250x83 at 1444,2133 (backbuffer 5640x2876)`.
  The drew receipt matches the real Volk state after the reload chain in the
  same session (clip 5→6/2→6, reserve 63→62/63→59 receipts in
  `next-reload-transcript.txt`). The plate hidden until values readable — no
  stale "unknown" numerals.
- Gate receipts show the weapon-wheel/backbuffer gates active
  (`ammo plate: gate blocked (flag=1 vr=1 dup=1 paused=0 wheel=0 armed=0 hand=0 eyes=1)`).

## Not covered by receipts — physical check required

- Per-eye ammo-plate sizing (the per-eye fresh-fallback fix has CPU tests
  `test-ammo-layout`, but the log has no per-eye projection receipts; the one
  `drew` line's 250x83 px at 1444,2133 on a 5640x2876 buffer is one sample).
- Anything clipped/missing/disappearing in the user's own view.
- Health-increase (heal) was not observed in this session.

## Settings unchanged

HUD/Sideplate settings were not modified by the M01 candidate (session INI
diff vs baseline = only the seven M01 shoulder keys). `controller features:
wrist health=1` line present at controller setup.

Grade: **L3 runtime evidence PASS for health-change and duplicate-suppression;
USTERIAL grounding PARTIAL for ammo (real-state keying) — per-eye sizing and
overall visual acceptance still require the brief PSVR2 check requested by
MAIN.** M01 remains PHYSICALLY_VERIFIED and is not influenced by this grading.
