# Seeker final backlog: measured status and acceptance gate

Branch: `side/final-seeker-20261026`, based on `71728b5d7e2712d1640fa18fa58e48bd6dc708ee`.
No runtime behavior, weapon pose, zone, inventory, ammunition, input, or projectile math changed here. No Steam access or game launch.

## Decision

The current source already includes the proven cooked-Y correction and post-native projectile witness. Existing evidence does **not** justify another pose/accuracy patch. Seeker is **NOT physically accepted**: current AB348E06 acceptance did not include Seeker, and the last Seeker session never reaches an accepted back-zone Grip. Native retention and selection cannot be labeled working from `ready` alone.

The historical user selected the native heavy-weapon lifecycle (`SeekerBackSlot=0`). The new completion directive asks to verify the optional back slot again. MAIN must explicitly record which behavior is being accepted; this branch does not silently turn it on or promise persistence for OFF mode.

## Actual archived evidence

Read-only input: `C:/Users/Trilex/vr-mods/SingularityVR-main-seeker-graphics/build/main-seeker-evidence/session-20261025-213034-AC3b/view_matrix.log`.
SHA256: `2951F59695DE71C4651E5AF7746453C1EDF8D3223C96654528A3419B3FEC4859`.
This is the latest archived Seeker-containing session found; newer AC55 retries contain no Seeker. Its deployed source/DLL identities are not inferred from its directory name.

- Lines 11876–11877: exact native heavy hook armed, reciprocal local pawn ready, three recognized owned weapons, owned Seeker available. This rules out a blanket inability to resolve inventory/hook in that session.
- Earlier six `unavailable` Grip refusals precede the recorded legitimate Seeker acquisition.
- Line 15426: after acquisition, first refusal is `outside`, 0.588 m from the zone center against radius 0.160 m. Line 16142: `busy`, support=1, distance 0.576 m. No accepted-zone edge, retention-hook call, selection-requested, or selection-equipped receipt appears. Moving the zone or patching the selector without another observation is unsupported.
- Lines 13782–16310: 12 matching launch/result pairs. All returned native Projectiles have readable initialized Location/Velocity/Rotation. Spawn-location error against supplied measured muzzle: 0 UU at log precision; velocity/input direction difference <=0.000365 degrees; initialized speed 4999.9997–5000.0005 UU/s. Outgoing ray/barrel difference 0.0127–0.1969 degrees. These prove initialization follows the supplied ray, **not** eventual hit accuracy or guided flight behavior.
- `Seeker glove completed` records eyes=2 and successful draw/IB/palette restoration. This is submission evidence, **not** proof of visible glove placement.

## Existing source independently verified

`audit_seeker_geometry.py` compared all 12444 original PSK/GPU positions against the private cooked package: maximum vertex error 0 UU. Compiled geometry constants match the measured original skeleton to 0.000003625 UU. Main body is 14990 faces; attachment is six faces with a distinct one-bone palette. Root/muzzle are native cooked coordinates; reflecting Y again is the previously fixed ~219/225 UU support/muzzle displacement.

Original projectile archetypes retain native `mIsDrunk` main fire and `mSteerable` alternate fire. No spread/recoil/steering was removed. The existing ownership scanner excludes unrelated foreign inventory rather than rejecting the entire valid chain. Selection uses native owned-weapon dispatch, and only observed `Pawn.Weapon` confirms equip.

## Delivered changes

- `tools/reload/analyze_seeker_session.py`: reproducible bounded-log analysis, exact input SHA and optional externally verified deployment identity. Separates ready, Grip acceptance, retention, selection request/equip, initialized launch, successful two-eye draw, and missing save/load/physical evidence. Reports numbers without inventing an accuracy threshold.
- `tools/reload/test_analyze_seeker_session.py`: ten regressions against false acceptance, null/mismatched/orphan spawns, timeout, spread interpretation and restoration failures.
- `tests/test-seeker-native-adapter.cpp`: real compiled x86 `/O2` `Heavy` callback exercise. Own-process executable fixture generates CALL return sites at the audited `SwitchWeapon+D2/+E9`, proves both return retention=0, and proves unrelated caller/ordinary weapon/OFF/pause/foreign owner/wrong thread forward the original heavy result. Existing 100 native selector calls and byte-exact weapon/ammo conservation remain covered. No game process is involved.
- Private generated reports/build output are ignored under `build/seeker-final/`.

Run analyzer (no process access):

```powershell
python tools/reload/analyze_seeker_session.py <archived-view_matrix.log> --output build/seeker-final/session.json
python -m unittest discover -s tools/reload -p test_analyze_seeker_session.py -v
powershell -ExecutionPolicy Bypass -File tools/reload/test-seeker.ps1 -IndexAsset <private-original-seeker.ib>
```

## Verification

Offline PASS: ten analyzer regressions; four x86 `/O2` Seeker programs including actual emitted callback/native selector ABI; D3D9/D3D9Ex identity/corrupt-index/binding/cache/reset/recreation tests; original PSK/PSA/cooked geometry oracle; PE32 `/O2` production build; actual compiled WeaponLocker `[eax+20Ch]`/FName stack regression; compiled Impulse ABI regression. Existing codecvt deprecation warnings remain.

No new physical test, launch, installed DLL, config or save change. HMD acceptance remains PENDING. MAIN's final combined source/DLL must be recorded independently from this standalone build.

## One grouped MAIN acceptance session

Use a legitimate Seeker pickup/save; no spawning, ownership edits, ammo injection or unlocked-save fabrication. Keep current ordinary weapon regression checks in the same grouped session. If Seeker is not available in the user's save, mark the Seeker stage BLOCKED BY GAME PROGRESSION and finish independent backlog work.

1. If testing the optional back slot, MAIN enables only its existing `SeekerBackSlot=1` gate and records that change. After pickup, switch to an ordinary weapon and verify the native owned Seeker remains. Release left support and other busy actions before the right-Grip rising edge at **lower-right back**, not the shoulder: existing center is 18 cm right, 55 cm below head, 32 cm behind body yaw; radius 16 cm. Require first gate `accept`, retention receipt where native switch executes, native selection-requested **and** selection-equipped; repeat the gesture to return to previous ordinary weapon. A request alone fails acceptance. Do not move zone to hide an unavailable/busy refusal.
2. With Seeker equipped, hold/release left Grip near the visible fore-end, rotate wrists/head, and check complete left glove in both eyes, stable support and independent tracking after release. Fire at unobstructed close/medium fixed targets. Compare initial barrel/muzzle alignment separately from native drunk/guided flight. R2/L2 and ordinary weapon aim/reload must remain accepted.
3. Save/load normally; confirm native ownership/Seeker ammunition and ordinary inventory remain unchanged except legitimate use. Recheck retrieval only if enabled. Pair video/HMD feedback with the existing bounded receipts and analyze the exact archived log. Missing save/load or Seeker access stays NOT TESTED.

GO: integrate these offline tools/tests without changing runtime behavior. NO-GO: claim Seeker holster, visible support, final impact, or save/load physically PASS before this stage succeeds. If it fails, the first recorded refusal/result and exact deployed artifact distinguish zone/busy, retention, selector, pose and launch causes; do not restart a broad speculative patch cycle.
