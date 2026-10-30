# Combined inner-wrist HUD and native muzzle direction candidate

## Evidence and correction

Prior physical feedback: HUD visible but admitted edge-on/outward poses; native
VFX offset FAIL except Volk PASS. Origin-only correction executed but was
insufficient. The existing inward inspection gate (.80 enter/.65 leave) remains.
No panel, controller, grip, weapon or reload offsets change in this follow-up.

Authorized Steam diagnostic session PID 23044 captured the complete original
PlayMuzzleFX before detouring. Private receipt SHA256:
D63427414872BB589EB3827515CAEA5EC7F20920B6C5D898B88DC4EC2455E72F.
Read-only loaded-text archive SHA256:
2781F6339FCEC2EAED4F9141E22694A5CB2559B5211CE06CE82283A0C5DC8846.
The latter contains existing detours; the former proves the original parent.
Neither proprietary binary is distributed.

Proven native chain: 0xE7E250 PlayMuzzleFX calls the actual FX vtable +130
helper 0xE7E5A0, returning to parent +1E3. It passes location then a native aim
FVector. The helper copies location and converts the second vector through
0x493480 (atan2-derived rotation). Existing measured-origin forwarding did not
replace this direction with the final rendered VR barrel witness. This is a
proven missing direction propagation path, not yet physical proof of the cure.

The new default-OFF Render/NativeWeaponMuzzleDirection gate forwards the fresh
measured direction only inside the exact local first-person muzzle call. TLS
scope, exact return address, pawn/weapon identity and freshness must all match.
All nine remaining DWORD arguments and the existing origin remain unchanged.
The complete 431-byte helper fingerprint is required before hooking.
Valkyrie, Kasimov and Centurion use their existing validated origin eligibility;
Volk, AI, third-person, impact and shell calls are unchanged. Existing loop
maintenance already uses GunAimDir; no duplicate particle transform is added.
No collision ray, damage, recoil, spread, shader, hand or scope changes.

## ABI and tests

At helper entry: [ESP]=return, +4=location pointer, +8=direction pointer,
+0C..+2C=nine original DWORD arguments; ECX=self. Native ret2C consumes all
44 argument bytes. The fastcall bridge ignores EDX and preserves this stack
contract. Optimized forwarding pushes 11 DWORDs, loads ECX=self, calls the
trampoline, restores EDI/ESI/EBP/EBX and its own 24-byte locals, then ret2C.

- Native muzzle suite: PASS, including 1000 actual x86 forwarding calls checking
  ESP, ECX, both pointers and all nine scalars; stale/wrong-owner/caller/OFF gates.
- Private full-body native contract verification: PASS (parent +1E3,
  position/direction consumption, vector conversion and ret8/ret2C/ret4).
- Actual optimized DLL disassembly regression: PASS.
- WeaponLocker literal [eax+20Ch] compiled ABI regression: PASS.
- Existing wrist rotation/placement and D3D9/D3D9Ex pixel/reset checks: PASS
  earlier in this same scoped candidate; unchanged by direction-only patch.
- x86 PE32 /O2 build: PASS. No FPS or physical visual PASS inferred.

## Candidate and physical gate

Enable Render/NativeWeaponMuzzleDirection=1; keep NativeWeaponMuzzleOrigin=1
and NativeRifleMuzzleOrigin=3. Disable Diagnostic/NativeMuzzleCodeCapture after
the successful capture. Preserve the installed profile and all protected assets.
Hook receipt: `native-fx direction: audited 431-byte spawn ret2C`.
At most twelve correction receipts identify direction/mesh; they are not pixels.

One headset session: rotate inner wrist toward/away from face; fire Valkyrie,
Kasimov and Centurion at two wrist orientations, watching flash/smoke origin and
direction. Volk is the unchanged control. Check grip/reload briefly only for
regressions. Physical HUD activation and corrected VFX are PENDING. If VFX still
offset, correlate the exact visible particle route rather than add guessed
offsets. Preserve backup and verify restored hashes on a failed experiment.
