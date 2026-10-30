# Inner-wrist inspection gate / unresolved native VFX offset

Physical feedback for installed 8EEC46A2: HUD visible, but should activate only
when the inner wrist faces the viewer. Weapon VFX offset FAIL except Volk PASS.
Preserve accepted weapon geometry, aiming, grips, reloads and tracking.

## Visibility change

The old gate opened at dot >= .34 (70 degrees) and stayed open at .16 (81
degrees), admitting an almost edge-on display. The new intentional inspection
gate opens at .80 (36.87 degrees), closes below .65 (49.46 degrees), retaining
distance/gameplay/support gates. Rotation sweep tests cover 0..180 degrees in
both directions. No pose, panel placement, depth or hand offsets changed.
Physical comfort/activation acceptance remains PENDING.

## VFX evidence and next falsifiable step

Latest session archived privately at
`build/wrist-main/session-20261030-vfx-offset/view_matrix.log`, SHA256
E8B5FF0BBFFFE05FAC94F4D977412E9CBE8AEF68D79F8673A17FE464F8B664B4.
Kasimov and Centurion receipts prove fresh measured positions are passed to
native PlayMuzzleFX, yet the user sees offset effects. Thus origin forwarding
alone is insufficient. Exact particle routes report no local-matrix mismatch.
Neither fact proves the visible emitter's rotation or final particle origin.
Volk is the unchanged working control. No new VFX transform fix is asserted.

The prior unpacked native .text archive is absent. Installed/backup EXE .text
is packed and cannot substitute for valid loaded instructions. Do not treat its
garbage disassembly as native evidence.

Default-OFF `[Diagnostic] NativeMuzzleCodeCapture=1` saves exactly 1067 bytes
once, before the existing MinHook installation: metadata plus complete
fingerprinted 197-byte wrapper and 846-byte method. Both range and whole-body
hash checks precede the write. No new hook, native invocation, GPU query or
per-frame tracing. Exclusive creation prevents overwriting older evidence.
Output: `%LOCALAPPDATA%/SingularityVR/native-muzzle-code-<PID>.bin`; it contains
private owned-game code and must not be committed or distributed.

Offline decoder:
`python tools/fx/disassemble_native_muzzle_capture.py <capture> <private-output>`.
It rejects wrong lengths, headers, fingerprints and incomplete decoding.
Expected runtime receipt: `native-fx code capture: saved=1 ... [pre-detour ...]`.
Inspect actual emitter construction/rotation and branch ownership next. If the
supplied position is retained unchanged, investigate final particle transform
instead of modifying that argument again. Do not guess a common weapon offset.

## Verification and boundaries

- Native muzzle suite incl actual binary writer/exclusive-create test: PASS.
- HUD deterministic rotation, placement, projection, source and depth tests: PASS.
- D3D9 and D3D9Ex actual depth/wall/glove/magazine pixel/reset tests: PASS
  outside the restricted GPU sandbox (initial sandbox device creation failed).
- x86 PE32 /O2 build: PASS; final clean provenance in candidate receipt.
- Steam installation unchanged; no game launch; no physical PASS claim.
- Next grouped run: inward/outward wrist rotation, plus native code receipt at
  hook initialization. VFX remains BROKEN pending analysis; no repeated test of
  unchanged VFX is requested as if it were repaired.
