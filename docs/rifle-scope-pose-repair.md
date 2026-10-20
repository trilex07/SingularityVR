# Native rifle pose / scope binding candidate

2026-10-20, MAIN, `fix/rifle-vfx-sniper-grip-scope`. Offline preparation only;
Steam remains the accepted build. This supersedes the untested diagnostic-only
bb5c1ab candidate, not the accepted profile. Not physically verified.

## Evidence and falsifiers

The tested 4c8a0a7 log proves local first-person Valkyrie PlayMuzzleFX executes,
receives a fresh measured muzzle position and returns a live RvEmitter, while
the user sees no rifle muzzle effect. Origin-only sufficiency is disproven.
Disassembly of that exact native method proves an existing matching looping
emitter skips creation/positioning; newly created emitters take rotation from
the native weapon, not the VR-transformed mesh. The original rifle package
uses a looping, local-space GPU-cloud effect. Pistol draw-pair guesses do not
identify this producer and are not broadened here.

Hypothesis: original looping particles need continuing native emitter pose
updates at the measured VR muzzle, with the accepted firing direction. Mode 3
adds Actor.SetLocation/SetRotation after the original firing call and from the
existing local engine PlayerTick. Exact exec registrations, entire wrappers,
actual E8 targets and complete helpers are checked. Audited contracts:

- SetLocation wrapper 121 bytes / D75050CD7801E36B; +67h helper 29 bytes /
  8A32507F9F7A934A, FVector by value, ret 0Ch.
- SetRotation wrapper 109 bytes / CB407E96A5DEE5B3; +5Bh helper 139 bytes /
  AA63F1F63DEFF3D8, FRotator by value, ret 0Ch.

No executable callable VA is committed. Native movement propagates actor and
component transforms; no raw actor/component transform or activation writes.
Only the exact current local Valkyrie loop is leased, with live pawn/weapon,
FX owner, returned emitter, component and original first-person template checks.
Lease expires 150 ms after PlayMuzzleFX; ticks never renew it. No second play,
spawn, template substitution or infinite particle keepalive. Direction is the
accepted bullet/laser direction; roll zero is explicit, not a claimed authored
bone quaternion. Twelve native movement receipts are not pixel validation.
Falsifier: successful native pose receipts but no correctly placed effect means
transform maintenance is insufficient; inspect the actual GPU-cloud producer,
do not widen unrelated shader matches. Native failures stop the lease.

The tested scope reached the measured lens gate and engine callback but refused
ABI/layout before constructing any object. Its exact live refusal stage is
still unknown; no claim that a particular property or fingerprint caused it.
This candidate binds the exact engine-class exec via the independently audited
FNativeFunctionLookup registry instead of relying on a possibly unbound or
overridden UFunction::Func. The registry requires a unique complete UTF16 symbol
and aligned row in readable non-code sections of the current PE32 image. Every
existing whole-code, helper, property and error-device guard remains required.
Registry/reflection addresses and precise refusal stages are logged. This is
an alternative binding repair, NOT proof of the old refusal's root cause.
Falsifier: the same field/error refusal persists, or registry and reflection
agree and code still refuses. Do not remove that guard; use its actual result.

Own 512-square SceneCapture2D, measured physical lens aperture, exact resource
identity/completion lease, bounded RGB readback, COM/state restoration and
engine-thread ownership remain as documented in optical-scope-native-rifle-
candidate.md. A successful bind is not proof of useful magnified pixels.

## Delivery and test boundary

NativeRifleMuzzleOrigin defaults OFF; mode 2 retains the origin-only control;
mode 3 is the new controlled maintenance experiment. SniperPictureInPicture
also defaults OFF. Offline preparation's optical switch selects mode 3 and
disables large four-frame pass dumps. No assets, saves, original game packages,
accepted pistol/shadow/reload/localization settings are changed.

L1 must cover unique registry lookup/refusals, pose/lease math and regressions.
L2 requires clean x86 PE32 /O2 manifest, SHA and full accepted-profile candidate
preflight. Neither is L3/L4. Wait for fresh user `start` before deployment.
Single no-chat headset route: Valkyrie three spaced bursts into open space,
turn the gun; Kasimov lens to each eye then lower; pistol/reload/pause regression;
normal exit then feedback. Read native transform, binding, RTT/RGB/aperture
receipts and verify restored hashes. Never ask for repeated invisible behavior
without using the new producer evidence.

## Offline delivery

Code `acc2f2bc542a127e1f6622326dab276fd255e17b`, L1/L2 PASS. Clean x86
PE32 `/O2`, no PERFORMANCE_BUILD instrumentation. DLL SHA256:
`51BF3212431E582529E5249A12179E887B698580385C671B34E7406BADC278DC`.
Private artifact `build/combined-followups/pose-repair-acc2f2b` contains DLL,
clean build manifest, scoped INI and exact change plan. INI SHA256:
`28DA83E2759216F15A8806D688591E5A38033286896DD3A8CB1192B4FB325642`;
plan SHA256 `21C77D62793B3252DBD37524E486C07882B3EBDB476BCCCB010B9B48540CD740`.
Nine native FX/scope/registry executables, parser/transform/backend checks,
341 fake handler/opcode checks, 35 candidate-preparation and six restore-lease
fixtures PASS. Full reload/hand/knife/wheel, native coordinate replay, fake
native ABI and own-process GPU replay regression suite also PASS. Owned-code
audit validates all nine wrappers and nine actual helper bodies, not only a
relative-call opcode. Symbols are in .rdata, registry rows in .data, matching
the runtime scanner's non-executable readable section policy.

Full 145-pin accepted-profile candidate preflight via deployer Install -WhatIf
PASS; no files created/written in Steam. Actual accepted DLL7A1DE585...9495,
INI60224CA0...4ADC and profile5A60E4D1...556E rehashed unchanged. No L3/L4
claim. No new Steam launch/guard/deployment in this turn. Later documentation
commits do not change the clean code provenance of this artifact.
