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
