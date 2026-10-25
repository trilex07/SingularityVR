# Ordinary support acquisition after Seeker integration

Scope: ordinary-weapon two-hand acquisition only. Base `2cc4c63cbee49ff3f729e30e25bd3f99cae60705`; branch `fix/ordinary-support-after-seeker`.

## Proven from the latest physical session

Evidence is MAIN's archived `build/main-seeker-evidence/session-20261025-161251/view_matrix.log`, SHA256 `21766CA09713162B70CBA6F4FBA8B7864CFAB72B156B8BBCD5E22ACBE10CC3B3`. The staged candidate was DLL `12FEB4999162746ACC85D216D9E644EF3FD6E273F6ED4852139EE94FF49A0F35`.

- Lines 23989 and 24098: `grip press/release/latch/detach/error=48/48/0/0/0`. These edges count **effective**, ownership-filtered support Grip, not raw controller activity. Ordinary acquisition never latched in this interval.
- Lines 25254-25255: `ACQUIRED gunVerts=12444`; 26172-26173: `RELEASED gunVerts=12444`. Seeker reaches the same input latch successfully. User confirms its supported presentation works.
- Ordinary source/draw census reports `HANDS_WEAPON=2`, weapons 8297 and 11387, support=0. The first failure is before support rendering; changing glove visibility cannot make this latch acquire.
- Volk reload completed `magazine-ejected -> grab-fresh -> inserted/request -> native-ammo-ack` (19451, 19564, 19666, 19719), later phase 0 at 20569. Valkyrie likewise completed at 21013, 21076, 21162, 21168, later phase 0 at 22858. There is no evidence of a permanently stuck carried-magazine owner.

## Source comparison and remaining boundary

The complete support acquisition block is unchanged from pre-Seeker integration `1d5ff9db`. It checks raw OpenXR controller separation and primary -Z direction, ownership, context and the existing `GripLatch`. Its thresholds have not changed since the older pistol-specific correction. Euler-derived -Z agrees with the existing OpenXR YXZ convention.

Seeker-specific additions do not write `g_nextWrist`, controller positions, `g_twoHandActive`, `g_weaponObj` or the ordinary latch. `DualHandActionPose` changes render matrices only and stops applying when support is active. It does not write `HANDS_TMD`; ordinary HANDS_WEAPON makes the native presentation predicate true independently of the dual-context branch.

The new render normalization applies to the four ordinary weapon profiles, not Seeker. A mismatch between the visible fore-end and the old raw-controller acquisition cone is consequently **a hypothesis**, not a proven cause. The archived log has neither attempted-Grip timestamps nor corresponding raw position + orientation samples. Sparse `hands raw XR` lines 22907/22999 have no Grip state or same-sample primary quaternion. Final wrist matrices alone cannot reconstruct the input transaction. No guessed offset/cone relaxation has been committed.

## Smallest missing measurement

`[Diagnostic] SupportGripProbe=1` (default OFF) adds a read-only observation immediately after the existing production latch update in `SyncXRInput`.

For at most 24 raw squeezes, it emits a press, first later acquisition if any, and last-held sample when released. There is no per-frame logging, GPU readback, native call, pose write, or latch mutation. `support-acquire:` records:

- frame, pawn/weapon identity, weapon vertex profile, native hands state and physical controller indices;
- raw/effective Grip, exact production `supportWeaponValid`, tracked, acquire and active results;
- individual context and knife/reload/cylinder ownership gates;
- actual raw primary/support positions, primary forward, distance and alignment.

`gate=` distinguishes reservation, context, tracking, too-close, too-far, forward-cone and eligible-unlatched. The last label exposes the existing one-sample weapon-identity reset without claiming it is a failure. Actual acquire/active remain authoritative; diagnostic labels cannot influence them.

This is a **diagnostic change, not a claimed support fix**. It belongs alongside MAIN's independently proven graphics correction only if MAIN wants the missing support evidence in that same session. MAIN can enable the flag and ask for one left-Grip attempt held for about one second near the visible fore-end of Valkyrie and Volk, then release. No need to repeat already accepted Seeker behavior solely for this probe.

## Verification

- `tools/tests/test-dual-weapon-tmd.ps1 -SkipOled`: PASS, including the new x86 `/O2` probe test, existing hand/TMD/reload/aim/WeaponLocker tests and Python wiring checks.
- New probe test compares diagnostic labels with **98,415 calls to the production `TwoHandGripAllowed` function** across Centurion, Volk, Valkyrie, Kasimov and Seeker. Tests real latch order, identity reset, distance-independent hold, release and disabled/bounded observer behavior. Observer/control latch results match.
- `tools/reload/test-seeker.ps1`: PASS (native property walk, geometry/slot policy, native ownership/selector adapter and projectile ABI regressions).
- x86 PE32 `/O2` build: PASS, `/analyze` format-string guard PASS. Existing narrowing/codecvt warnings remain unchanged. Build used an explicit non-game `-GameBinaries` path, so even install-path discovery was avoided.
- Diagnostic DLL SHA256: `0B5583BD406B1C09B05A513960019D559B543E289E07077065FD9884BAC83230`.

No Steam access, deployment, gameplay launch, input change, support offset change, rendering change, or physical PASS claim.
