# Combined physical acceptance and follow-ups — 2026-10-19

## Persistent accepted baseline

The user explicitly requested retaining the tested combined candidate as the new
baseline. MAIN verified the closed game and absence of a live restore guard,
119 campaign delta files and their backups, nine reload files, and the protected
UI/font/original RvGame packages. No Steam file was overwritten during promotion.

Installed DLL: `7DDAA0AA0B667EE2476A3D286D6C7CE5CDE187099AE1FB4C1230DADDD3780FCF`.
Installed INI: `B94A4AA761B0EE477B49AB7BB4CA4D1E8A8E120F30F4F5F8A2225B87D4F78931`.
Only AutoResX/Y differed from the tested INI: 5904/3012 -> 6488/3308.
The completed temporary guard refused this cache drift and exited; it did not
restore the old baseline. Do not restart that guard or an earlier runner.

The new local canonical profile is `deployment/accepted-profile.json`, SHA256
`445B31EE72CB06F001A4290EE54330840C3D51B4E54790BA7896970D51591506`,
with 145 explicit file pins. Promotion receipt, accepted DLL/INI copies and the
previous profile are in `deployment/accepted-combined-20261019/`. Prior core,
campaign and reload backups remain at the paths in that receipt. The old
startup-readiness worktree profile was NOT modified; it is historical now.
Future deployments must explicitly use this checkout's NEW profile.

Acceptance is the user's successful combined route, not every dialogue, weapon
or proof of 8K GPU upload. Open issues: two-weapon limit, initial wheel icons,
heal animation, original hero hands in cinematics, NPC-name size.

## Native animation follow-up: offline implementation, NOT installed

The log contains a health-window activation. `PrepareIndependentLeft` already
returns zero during native actions, but `FgIndependentHandsCandidate` interpreted
that zero as failed tracking and removed the draw. Earlier sleeve/replacement
rules also ran before the old animation exemption.

The patch exempts native actions before these destructive rules and excludes
TMD triangle/bone surgery during those actions. A separate default-OFF
`[Render] CutsceneNativeHands=1` experiment gives native-camera cinematic frames
ownership of the ORIGINAL animated foreground: no controller palette or gun/FX
ride, replacement, or tracked-foreground suppression. Bink, menu and gameplay
states do not enable that cinematic exemption. This does not create animations
which the engine never submits.

Three static wiring tests pass; x86 PE32 `/O2` build passes (existing analyzer
warnings remain). Build SHA256:
`8842537F48AD5DA073DD57086E421D2A9B6DB1D0224A1C2C1BF82E0EED5E0FEF`.
It was built with an uncommitted source diff, is not a sealed deployment and is
NOT the installed/accepted DLL. No visual or animation-completion PASS yet.

## Inventory evidence — implementation still open

Read-only inspection of the independently decoded original RvGame package shows
`RvGameSharedPlayerController.mWeaponSlots` is a four-element NameProperty.
Its authored `EWeaponSlot` enum is:

- 0 `EWS_Custom1`
- 1 `EWS_Custom2`
- 2 `EWS_HeavyWeapon`
- 3 `EWS_GrenadeSlot`
- 4 `EWS_MAX` sentinel

Native wrappers exist for `CycleWeapon`, `GetCycleWeapon`, `GetWeaponSlotName`,
`SetWeaponSlotName`, and `ClearAllWeaponSlotNames`. This is package structural
evidence, not proof of native method semantics or authorization to overwrite
slot storage. Increasing an array dimension or treating slots2/3 as ordinary
guns is NOT a safe fix. Prove pickup/displacement and the acquired-versus-carried
inventory mechanism before implementing expanded retention. Do not grant guns
merely because they appear in the catalog.

Wheel labels/icons currently use the session-local observed mesh set (`g_wwSeen`),
not a save-loaded ownership snapshot. Fix ownership initialization together with
the inventory producer, not by exposing every unacquired catalog item.

### Reproducible offline native evidence

`tools/reload/inspect_inventory_registration.py` resolves BOTH registration forms:
preinitialized PE lookup pointers and zero entries populated by a text initializer.
Five deterministic tests cover both forms, missing/ambiguous rows and out-of-text
targets. This matters: treating the zero HasWeapon entry as a function would
disassemble unrelated bytes, not the method. Outputs contain identities/hashes,
not proprietary function bodies. No process is opened.

Measured against owned EXE SHA BD459ACD…0F51 and archived text SHA858F2FC3…C78A
(text base0x402000; audit addresses, never production hardcoded calls):

- Shared controller CycleWeapon wrapper dispatches vtable+0x690; GetCycleWeapon
  +0x694; GetWeaponSlotName +0x740; SetWeaponSlotName +0x744.
- RvPlayerController constructor callback identifies vtable0x19D4C30. Slot getter
  reads exactly four FNames at controller+0x6C4, eight-byte stride; setter also
  notifies the native state/HUD. Direct field writes are not equivalent.
- Native GetCycleWeapon at0x1046FE0 searches authored slots, excluding grenade
  slot3 in its fallback. CycleWeapon ultimately calls pawn vtable+0xB14 with the
  selected FName; this alone does NOT establish inventory retention after pickup.
- RvSharedPawn HasWeapon initializer resolves wrapper0xF635F0, which dispatches
  pawn vtable+0xB18 with an FName argument, returning bool. RvPlayerPawnSP's
  constructor identifies vtable0x19D0788; its HasWeapon target0x102F700 traverses
  pawn+0x3D0 -> manager+0x1E0 -> item+0x1E0, casts each item and compares the
  weapon's FName at+0x2E8/+0x2EC. Package property16108 independently names
  `RvWeaponShared.mWeaponName`. This is actual possession, not catalog visibility.
- Inventory ChangeToWeaponByName wrapper0xF68430 dispatches manager vtable+0x428.
  RvInventoryManager vtable0x19CB480 routes this through reflected script dispatch
  then virtual+0x42C; it is NOT a proven direct safe selector yet.

Next: prove the pickup/removal producer and safe selection ABI; resolve live
fields through reflection and validate target fingerprints before any call.
Do not expand the four-slot array, grant catalog weapons or mistake locker
availability (`mWeaponsAvailableToLocker`) for currently owned inventory.

## NPC names — still open

The 60% package patch is NOT deployed. It caused a pre-device EXE+0xACA50 crash;
the otherwise-identical run with original RvGame booted and passed the physical
route. Preserve original RvGame SHA4153C488…59E9EF. Offline font-field/roundtrip
checks did not prove engine package acceptance. Do not retry the same repack as
an NPC-size fix without new evidence identifying the failure mechanism.

Next physical trial needs a freshly sealed candidate against the NEW accepted
profile and a new user start. No launch was performed for these follow-ups.
