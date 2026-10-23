# Volk disappears during Impulse: five-bone evidence and bounded probe

Base: `067e1b1b7301987501354e9dab63030e26eb3d6a` (inherits `cac4ce3`).
Branch: `fix/sol-volk-impulse-skinning`. SIDE; no Steam access/deployment/launch.

## Finding that changes the diagnosis

MAIN's `runtime-alignment-067e1b1-diagnostic-evidence.md` in the MAIN checkout
was read. Its c5 rows are real evidence of one degenerate skin matrix, NOT
evidence that the entire weapon palette collapses. No exact L2 edge was captured.

Re-read the original compressed `RvGameMPLoader_SF.xxx` and independently
exported `ch_inview_shotgun_mesh.psk`, using existing package/PSK parsers.
The named SkeletalMesh's cooked chunk map is **[55,56,53,54,57]**. PSK face
indices (native winding) reproduce the cooked INDEX16 FNV **754008012**,
32505 indices. All 8297 vertices are referenced, all have one influencing bone.
No mixed-weight triangle crosses the five rigid groups:

- Slot 0, c5–c7: bone **55 clip**, 280 GPU vertices, **198 triangles**.
- Slot 1, c8–c10: bone **56 Slide**, 1756 vertices, **2718 triangles**.
- Slot 2, c11–c13: bone **53 cog_shotgun**, 6174 vertices, **7859 triangles**.
- Slot 3, c14–c16: bone **54 release**, 32 vertices, **20 triangles**.
- Slot 4, c17–c19: bone **57 Trigger**, 55 vertices, **40 triangles**.

Register numbers assume the runtime CTAB BoneMatrices base is c5. The probe
uses the actual bound layout base; it does not change shader constants. The
three-row GPU skin matrices are already evaluated transforms, not hierarchical
local bone transforms: zero slot0 does not propagate to the other four slots.

Package SHA256: `871C047B7F24F0F2BD2934E8FF78D16615D2E77FFD652B3ED66EA595B26C2F37`.
PSK SHA256: `941C871B695480E608AC8EB0CA25B99731344ECA57383C9C5927CF71E7BAF1AB`.
No proprietary source/geometry committed.

## Evidence status / hypotheses

PROVEN offline: palette map, all influences, cooked/PSK index identity, c5 is
the magazine rather than the body. MAIN captured valid c230 and actual stereo
submission, but only four skin rows (slot0 plus first row of slot1).

UNPROVEN: why native clip matrix becomes degenerate; whether the body and Slide
also collapse; intentional hands-state suppression versus animation failure;
valid live animation source for independent right-weapon presentation.
Therefore **no geometry/animation correction is justified yet**. No cached pose,
identity matrix, hands-state write, power change or visibility exception added.

Distinguishing outcomes in the next capture:

1. Body/Slide degenerate already in matched hook-input upload: investigate the
   logged native setter caller and its pose producer. Do not infer intent merely
   from hands=1, nor freeze that pose.
2. Hook input valid but DIP incoming degenerate: locate an intervening constant
   write. Hook-input match requires same shader/frame and a full 15-row upload.
3. Incoming valid but submitting degenerate: mod draw-time transformation owns
   the first demonstrated violation.
4. All five bones valid through both eyes but invisible: investigate actual
   VB/IB/material/render state/content; this disproves palette collapse alone.

## Implemented diagnostic (default OFF)

`[Diagnostic] VolkImpulseSkinProbe=1`. Independent of `DualWeaponTmdProbe`.

- Input edge receipts at the existing injected Impulse button's press/release.
- Actual local-weapon StartFire/BeginFire/FireAmmunition entry/exit receipts
  through existing ProcessEvent and resolved function names, independent of
  AimMode. No new hook. Exit scope unwinds even on an early return.
- Render hands/support transitions plus event mailbox. Short pulses between
  draws are retained as event bits; immediate receipts retain ordering/time.
- At most 64 event receipts and 64 draw samples per process. Sampling offsets
  0,1,4,24 frames following an event/state transition; one matched draw per frame.
  Repeated events reset the short window; source updates stop at sample cap.
- Exact Volk signature (8297/0/10835, min/base=0, triangle list), scene,
  foreground and non-shadow gates. Captures pointer identities VB/IB/VS and
  stream stride/offset; this signature alone is NOT a runtime IB checksum proof.
- Fifteen consumed skin rows, determinant/finiteness for all five bones;
  four LocalToWorld rows. Incoming and each actual stereo callback are distinct.
- Prior incoming draw snapshot includes frame, weapon and state: diagnostic
  comparison ONLY. Never used to render; reject it as a same-weapon predecessor
  if frame/weapon identities differ or it is invalid.
- Original SetVSConstF input recorded before mod processing, only for complete
  c5..c19 writes. Partial overlapping writes invalidate this extra witness.
  Missing witness is explicitly logged, not fabricated from stale registers.
  Caller VA and executable module base support follow-up native disassembly.
- Actual submitted segment range/HRESULT/bound IB; total eyes/segments;
  explicit receipt if an early gate prevents reaching stereo submission.

Only existing CPU-side constant getters, metadata COM getters and fixed-size
storage are used. No buffer locks, GPU readback/synchronization or render-loop
heap allocation is introduced. Bounded logs can perturb timing; not a performance
benchmark. No UObject animation-node offsets guessed. Actual animation-node
ownership is still unknown; logs expose hands/native-animation context, not an
invented authoritative pose-owner pointer.

## Offline validation

- `tools/reload/test_volk_skin_private.py`: PASS on the above private inputs;
  all vertices/faces, five slot IDs, index identity, 10637 non-clip faces.
- `tools/tests/test-dual-weapon-tmd.ps1`: PASS including new compiled x86 probe
  window/mailbox tests, input/native aim, reloads, hands, WeaponLocker, wiring
  and OLED CPU/D3D11 GPU comparisons.
- `tools/gloves/test-tmd-short-gauntlet.ps1`: PASS dispatch, 7110 stereo draws,
  49-run fallback, sleeve rotations and no double transform.
- `tools/gloves/test-tmd-runtime-regressions.ps1`: CPU fixtures PASS; sandbox
  could not create D3D9 HAL. The same compiled D3D9 fixture run outside sandbox
  PASS on D3D9 and D3D9Ex: 2x7110 triangles, IB restored, 3304 pixels per eye.
- x86 PE32 `/O2` build: PASS. DLL SHA256:
  `2DC5EF570CB0D8E46FD6F0D65D9EF8B019D1CAB566E5DA4352776F7A0105A754`.
- Steam / physical PSVR2: NOT RUN by SIDE. **Not a gameplay fix / not L4 PASS.**

## Single MAIN validation sequence (about 20 seconds)

Use the same Volk save, with existing accepted gameplay flags unchanged; enable
only the new diagnostic. Keep the weapon visible, no Grip, idle 2 seconds.
Press L2, hold one second, release, wait one second. Press R2 once, wait one
second. Repeat once; then hold L2 and fire R2 once, release both. Stop normally.
No headset-to-chat switching, weapon switch, reload or unrelated UI testing.
MAIN checks the `volk-skin event`, `hook-input`, `incoming`, `submitting` and
`segment` records for all five bones and both eyes. New process if cap exhausted.

GO for reviewed bounded diagnostic only. NO-GO for claiming disappearance fixed.
The unrelated truncated support glove, knife, Pad12 and magazine branch remain
unchanged; this task deliberately does not conflate them with weapon skinning.
