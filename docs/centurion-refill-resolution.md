# Reopened: refill rejected BEFORE invocation

2026-10-17 SIDE. User: no native animation, but clip never refills and cannot
shoot. This REJECTS physical acceptance of source6d7b201's refill-only backend.
Do not promote the corresponding PR45 generic resolver on its L1/L2 alone.

## Observed evidence / hypothesis before changing transfer behavior

Read-only frozen13:52 session:
`build/centurion-reload/feedback-20261017/session-1352.log`, SHA256
`A4A7F28302548697018C554C2B55BAB73D6D7AA021F7BBAEC9EAD6FCA83A4959`.
Three intended manual closes reached the selected branch: snapshot3/16/6,
then0/16/6 twice. ALL log result0 ready0 with live=-1/-1/-1, fault0, followed
by native-timeout after4000ms. Therefore there was NO getter/refill invocation;
this is NOT evidence that original RefillClip cannot transfer ammo. The hook
installed at the audited original method, Func vote172, actual RvWeaponShared.
The current log conflates context refusal with every resolver validation error.

Hypothesis to distinguish, not a speculative fix: a specific reflected binding,
wrapper/body/cast check or context check rejects. Emit EXACT stage/mask/count/
slot/fingerprint at that rejection, rather than lowering a check or changing
gestures, inventory/state/timers, native animation or camera. Offline original
wrapper disassembly agrees with the existing decoders; now test all FOUR actual
archived wrappers with the C++ decoders too, not only the native method bodies.
No evidence yet identifies which runtime stage failed. Class-tail/budget failure
is a possibility, NOT a measured diagnosis or license to increase/skip bounds.

Falsifier: ready1 and live getters/refill run but no conserved transfer; then
investigate the native transfer path instead. If stage=context, investigate that
gate, not reflection. If exact slots/body validation fail, preserve unknown-version
refusal and trace the actual implementation. Do not execute an unvalidated target.

## Implemented read-only preflight and failure diagnostics

Same default-OFF CenturionRefillOnly flag. No permission/transfer policy change.
On current player's already-BLOCKED natural AttemptReload, at most3 read-only
resolver attempts at least1s apart; success stops these. No permit required or
consumed, no getter/refill/original reload invoked. A short empty-trigger press
in an authorized MAIN session can supply the missing evidence BEFORE a full
headset reload route. A normal manual permit also reports exact validation stage.

`centurion refill resolver:` records source, ready, stage, matched bitmask,
visited count, binding name, class/field/wrapper/slot/method and normalized body
fingerprint. Per-permit output remains bounded32 records; preflight independent3.
No raw game bodies, per-draw I/O, live debugger, UObject calls or direct ammo
writes. Existing ABI/body/cast/context guards and fail-closed permit are retained.
Failed resolver never publishes a callable result. No blind fallback to full
Reload after an uncertain transfer. Models, gestures, Polish, rendering unchanged.

## Next MAIN gate (not a request to repeat the entire physical test)

Integrate this diagnostic source delta onto current MAIN, retaining the complete
dependency profile and backup/hash/test lease. Do not deploy this older SIDE DLL
over newer MAIN. Preserve flags; do not combine the PR45 experiment into this run.
Load the Centurion save, fire to empty if needed, press/release trigger briefly,
then normal exit. No repeated manual shoulder/insert/flick needed for preflight.
MAIN reads resolver stage before asking the user for a new full L4 reload trial.
If it reports ready without a transfer, ONE manual close then normal exit is
needed to test the remaining path; no chat switching during the route.

After the stage is recorded, fix that actual producer and rerun gates. Merely
shipping this diagnostic is not fixing the refill. SIDE has not launched,
deployed, restored or modified Steam; actual runtime stage remains UNKNOWN.

## Offline result

L1_PASS25 own-process CPU/ABI/resource/GPU programs,58 Python checks. The SAME
C++ decoders now read ALL FOUR actual archived original wrappers and derive
44C/454/450/4C8 successfully, as well as the original native body fingerprints
and every-byte mutation checks. This does not validate the runtime-selected
wrappers, whose exact identities are the missing evidence. New wiring tests
check the preflight cannot Arm/Commit/Refill/forward and that all strict failure
guards remain. Prior gesture/palm/native-space/shell/state/Reset tests pass.
Log: `build/centurion-reload/refill-only/resolver-tests.log`.
L2_PASS clean source `9c386676a472829b015604271f2c180742d8ae74`, x86 PE32
014C/010B, `/O2`, performance instrumentation OFF (bounded diagnostic still ON
under the existing experimental switch). Existing unrelated warnings remain.
DLL3205632 bytes, SHA256
`F5242ABCF5F3CE1150350BEEC30AF458DE5CBD55F022B1B7D8BE0CE79EAEDE94`.
Build receipt SHA256
`4CC80F1075F65EF170855BE66DE0311B4BDE3C6E2DA079E83AACEE12CDA3B0CE`.
Private `build/centurion-reload/resolver-candidate-9c38667/`: DLL/build receipt/
explicit DIAGNOSTIC manifest only, NO INI/assets. Build log
`build/centurion-reload/refill-only/resolver-build.log`. Not an installed,
accepted or restore baseline; source integration by MAIN required. Source9c38667
is on the existing PR48 branch, no merge. PR45 rollout warning also updated.
No repaired native transfer or new L3/L4 acceptance claimed. Further functional
changes require the runtime rejection stage; SIDE cannot obtain live evidence
by launching/attaching to Steam. The prepared preflight is that next safe gate.
