# First shots after refill: presentation hold, not ammo failure

2026-10-17 SIDE. User physically confirms refill without old Reload animation,
but reports the first two shots show the projectile without the expected gun
firing animation; subsequent shots are normal. Whether flash/smoke are also
missing is not yet specified. Do not claim all FX accepted or change FX routing.

Frozen14:30 session `build/centurion-reload/feedback-20261017/session-1430.log`,
SHA256 `9688CE9BF78098B5D67E5DF9DD43C576254E6E3592927F86294099A7E950FD69`:
two ready1 result2 native transfers, 0/16/6->6/10/6 then0/10/6->6/4/6,
with HUD ACK94ms/110ms later. No full native Reload invoked on these permits.
Read-only SIDE inspection only; no installed or accepted hashes inferred.

Source mechanism: `centurionvisual::Time` keeps the presentation active2600ms
after close. During that interval PrepareCenturionCylinder replaces the LIVE
gun root with a transaction-held one and the hand path replaces right-hand
bone poses with the held controller-relative pose. This was necessary to mask
the old native Reload sweep. The successful refill-only operation bypasses
that sweep, but the renderer still suppresses subsequent native root motion.
Existing rightHeld counters continue increasing after manual close/HUD ACK.
There is no per-shot trace proving which exact two shots were masked.

Hypothesis / smallest experiment: ONLY when CenturionRefillOnly is enabled,
release native gun/root and right-hand holds immediately at the manual close.
Keep the280ms cylinder-only closing interpolation on the current LIVE native
root, then stop overriding the gun palette. Leave the legacy2600ms path exactly
unchanged when that flag is OFF; keep all open/carry/loaded behavior unchanged.
No changes to engine ammo/permissions/state/timers, native FX, inputs or ADS.

Falsifier: first shots still lack recoil with live gun/hand poses, or flash/smoke
remain missing despite recoil returning. Then inspect that actual fire/FX path,
not force StartFire/Reload or replay a synthetic animation. MAIN physical gate:
fire immediately after manual close and again after3s; compare body recoil and
flash/smoke separately, check cylinder closure and6-shot ammo conservation.
No headset/chat switching. L3/L4 of this presentation change not yet obtained.

## Implementation and offline verification

Source `4962b7aa21e9c2273689eefd94d64883e25e92e8`, PR48 branch. Time uses the
existing refill-only flag, no new INI setting. Gun overrides end280ms after close
in WaitingNative/Cooldown/Idle. During that interval LiveRoot validates and uses
the current native root; neither legacy held-root cache is consumed. RightHold
Apply is bypassed as soon as closed is nonzero. All three Time call sites
(gun/hand/diagnostic) use the same mode. Flag OFF behavior and pre-close poses
are unchanged. No changes to the accepted refill resolver/transfer or native FX.

L1_PASS25 own-process CPU/ABI/resource/GPU programs +62 Python checks. New CPU
regressions compare old2600ms vs new280ms duration through16000 samples, verify
open/carry/loaded unchanged, preserve first-shot root translation/rotation and
body/hammer/trigger matrices during cylinder-only closure, reject NaN without
publishing output. Two new structural guards check actual production hand
release and live-root selection. Synthetic tests do NOT prove game recoil/FX.
Log: `build/centurion-reload/refill-only/recoil-tests.log`.

L2_PASS clean source4962b7a x86 PE32 014C/010B, `/O2`, performance
instrumentation OFF; existing unrelated warnings remain. DLL3206144 bytes:
`8EC19ABE1C39D1ABB93B59FEBEFBC59E4D87F1F153D52C067E7F5FF6910A47B0`.
Build receipt SHA256
`635062E4DC0FF3BDD74C7337F638ACC439B5FB96292048CBF0211186ED6B8647`.
Build log: `build/centurion-reload/refill-only/recoil-build.log`.
Private `build/centurion-reload/recoil-candidate-4962b7a/` has DLL/receipt/manifest
only, NO INI/assets. MAIN integrates this SOURCE delta onto current accepted
integration; do NOT replace current MAIN with an old full SIDE DLL. L3/L4 UNKNOWN
for the recoil correction; clarification about flash/smoke remains unanswered.
SIDE has not launched/deployed/restored Steam or changed any installed file.
