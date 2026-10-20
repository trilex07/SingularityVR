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
