# Failed optical isolation session / recovery

2026-10-20 MAIN, branch fix/rifle-vfx-sniper-grip-scope.

User physically confirms smoke at the rifle muzzle. Preserve native emitter
mode3. Circle fails / R1 reloads both rifle and sniper. Projectiles do not match
the visible barrel; no optical image and approximately40 FPS near the scope.
Private evidence: build/combined-followups/optical-isolation-94c56d1/physical-feedback.log.
Failed DLL9F6707D3B7A3D874B836210D15E3844BB93003EFC051347E2720532FA27505E5.
Accepted DLL7A1DE5854D1EC8635EE77200E3A39F0D4256C3A73ED6A69E99D3C26417B69495
and INI60224CA08A8E9B01E299AD4B14A069A1E854F2B5E46ADFBB0B01A86973A74ADC
independently verified restored after game exit. Do not promote this candidate.

## Evidence, hypotheses, falsifiers

* Reload: exact weapon identity/native method/mesh witness/tracking pass;
  repeated HUD counts -1/-2/-1 invalidate FSM and native blocking. Next-weapon
  path omitted the existing Centurion invalid-HUD recovery. Apply the same
  bounded50-sample /5s cooldown /6-reseat policy, with reseat candidates required
  to have populated reflected counts. No inventory writes or fake defaults.
  Falsifier: no alternative populated HUD exists, or valid counts resume but
  Circle still fails. Then identify native authoritative read producer; do not
  disable validation or globally suppress native reload with unknown ammo.

* Tracer: existing bounded global FX census fills eight slots before actual
  local muzzle callback object85FEBD60 is observed. No tracer armed/refusal line,
  despite fresh native muzzle receipts. Bind using that exact local receipt
  after class/owner/pawn/weapon/liveness checks; retain all wrapper/method/getter
  fingerprints and exact tracer return-site filter. Falsifier: binding still
  waits/refuses, or source hook executes and visual offset persists. A source
  correction is NOT an impact-direction correction: actual SingleLineCheck
  already reports fresh muzzle origins, while spread/controller rotation remain
  native. Do not claim the aiming complaint solved by a tracer-only hook.

* Scope:618 enclosed native draws and2184 constant uploads prove an expensive
  extra view. Three completed RGB samples do not prove sampleable texture or
  aperture. No projection/aperture receipts means an earlier silent exit. Add
  once-per-stage HRESULT witnesses covering resource, GetContainer, exact COM
  identity, backbuffer, raw setters, state block/guard, target/viewport and basis.
  Never bypass a failed identity check. After30 eligible completed frames with
  no successful optical draw, latch a fault and detach on the engine thread.
  This bounds invisible rendering; it is NOT an optical fix. Falsifier: aperture
  draw succeeds but lens stays blank, or slowdown persists after confirmed detach.

No shader constants, accepted particle matches, packages, voices, assets or
installed configuration changed. No new Steam session requested yet.

## Separate render/resolve texture follow-through

Archived FTextureRenderTarget2DResource.InitDynamicRHI proves separate handles:
creation stores TextureRHI+64h, passes that handle to targetable-surface creation,
then stores SurfaceRHI+48h. The owned capture's full737-byte render body finishes
the renderer and invokes the RHI resolve operation before returning. Thus a
GetContainer relationship is not the contract: a targetable surface may be
separate from the sampled texture. Existing ExactTexture validates BOTH native
wrapper identities and COM pointers without requiring that relationship.

Hypothesis: GetContainer silently rejects the separate render surface. Consume
the exact resource's TextureRHI COM pointer only after ExactTexture succeeds;
AddRef for this draw, validate its512-square level0, and sample RGB from that
actual texture level, not merely from the drawing surface. Require the exact
owned native render callback to finish after drawing before presenting it.
No extra resolve/copy, borrowed texture or pointer inferred from dimensions.
Falsifier: this contract refuses, resolved texture contains no RGB, or aperture
draw succeeds but remains invisible. Keep stage receipts and30-frame cutoff.
This remains an unverified producer/consumer repair, not a physical scope pass.
Full resource initializer645/65B4A8A44C0157E5 is independently audited offline
and fingerprinted before runtime texture access; storage instructions at+BBh
and+159h prove the two distinct fields. No hardcoded callable address added.

## Offline delivery, not physical acceptance

Clean code6ca2a4a518ffa79a70e57327850a4946a24769ee, x86 PE32 /O2:
DLL E5557636EBF03DA2DED909EA5EC0CC060A85A059EC881BE35342809545AA0FD0.
Private candidate build/combined-followups/resolved-texture-6ca2a4a:
INI BF2273049DF69B9714A26B54F7C38A94BA28495F881E50B4EAC80212F5AC4006,
plan FC4A1FFB4C6074AE7079A384E1ED878510B9CA5EEF8FDCA89683C5FBD9668DB9.
The earlier feedback-recovery-7bb9df7 directory is superseded; do not deploy it.

PASS: tools/fx/test-native-muzzle.ps1 (nine native /O2 fixtures plus Python),
tools/reload/test-centurion.ps1 -PreparedRoot build/next-weapon-reload/prepared,
tools/tests/test-combined-followups-prep.ps1 (39 fake checks),
tools/tests/test-followup-lease.ps1 (six fake scenarios),
tools/fx/audit_rifle_scope_backend.py against owned PE/text/Engine.u/Valkyrie.u,
production build and accepted145-pin deployment Install -WhatIf (zero writes).
Logs use build/combined-followups/feedback-recovery-* and resolved-texture-*.
No physical acceptance, no merge, installed accepted hashes unchanged.

Next MAIN gate requires fresh user start, current process/accepted-profile
checks, fresh timestamped backup and reviewed guard READY. No old backup/guard
reuse. Proposed short route, with NO headset-to-chat switching:

1. Load the save, allow ten seconds for gameplay; fire Valkyrie into open space
   and near a distinct distant target, then Circle, shoulder ammo, insert.
   Check native R1 does not substitute for manual reload.
2. Switch to Kasimov, repeat Circle/shoulder/insert, raise/lower scope briefly.
   Stop early on discomfort; no need to hold an invisible low-FPS scope.
3. Close normally. MAIN reads HUD recovery/native block/source-hook receipts,
   bounded actual collision-ray origin/direction vs accepted aim, exact sampled
   texture RGB/aperture/refusal and any capture-disable receipt, then verifies
   guard restore hashes. User reports muzzle/aim, reload and scope/FPS once.
