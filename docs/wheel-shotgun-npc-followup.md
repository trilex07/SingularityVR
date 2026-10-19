# Wheel, Volk hand pose and NPC name follow-up — 2026-10-19

Status: implemented; offline validation below, **no Steam/headset acceptance**.
This is additive to `weapon-carry-ray-followup.md`, not a promotion or rollback.

## Observations and hypotheses

* User: wheel position differs from equipped gun. Source catalog explicitly
  calls its cycle order proposed/unverified. Runtime queued a fixed number of
  pulses through all catalog weapons even when the inventory has fewer. Also
  raw OpenXR stick +Y-up entered a screen-Y-down classifier unchanged.
  `WeaponWheelObservedSelection=1` (default OFF) translates XR Y once and
  replaces the guessed cycle distance with one native pulse per observed
  weapon change. Stop at the actual target, a repeated weapon, invalid gameplay,
  eight identities, or five seconds. Hand-projected Y is already screen-down
  and must not be flipped. Desktop synthetic input keeps its existing contract.
  Render and commit still use the same catalog gun-position order.
  Falsifier: correct sector logged, but native arrival STOPs or a different gun
  appears. No direct inventory writes or new native method hooks.
* User: Volk right hand clips inside gun; support hand sits beside it, as
  Valkyrie did. Native shipped shotgun includes the two hand palettes; current
  source instead targets a speculative fore-end offset (24,-5,-2 UU), and lets
  its single-hand right glove follow the open controller rather than BuildGunC.
  `VolkAuthoredGrip=1` (default OFF), exact mesh8297 only, preserves the native
  paired pose using the same correction already physically accepted for Valkyrie.
  Single-handed right glove follows BuildGunC. Pistol/rifle/sniper gates remain
  unchanged; switching/holster/live-weapon gates are tested.
  Falsifier: a correctly classified8297 still has displaced hands in PSVR2.
* User: Devlin name too large. Offline original RvGame HUD proves DefineEditText184
  is bound to `_root.name_NPC`, placed as `nameTxt` by sprite185. WFontButton102,
  original height340twips (17px). Offline patch reduces **only this field's
  font height** to204twips (60%). Does not shrink subtitles, notes, prompts or
  weapon text. No GFx ABI guess, renderer-font heuristic or runtime memory write.
  Falsifier: name remains large because later native/AVM1 code overrides font
  size; then inspect that writer, do not globally shrink the font atlas.

## Private asset candidate

`tools/title/prepare_npc_name.py` reads the installed package, requires its
explicit input SHA, refuses unknown field/placement/layout, and writes outside
Steam. The **whole logical package differs at only the two font-height bytes**.
It recompresses only that one affected block and retains every other original
compressed block. Independent project native decoder must reproduce the exact
patched logical bytes before a candidate is reported. No game-derived bytes
are checked into Git.

Prepared from installed RvGame.xxx SHA
`4153C4889C90635069ED226D918CDB76BE618F46B1BAEA152AC10CFFDF59E9EF`:
`build/next-weapon-reload/npc-name-60pct-20261019/RvGame.xxx`, SHA
`FF38C0AA32A54E1D3689EB5676DF4E614E162C180850B478CC14FDE997D2FBE6`.
Native decoder roundtrip PASS. Logical font offset6076755; every other logical
byte, including all existing localization/material/cursor/voice references,
is preserved. This is **not yet installed**. The test lease must back up and
restore this package in addition to DLL, INI and nine reload assets. Do not use
an older DLL/INI-only watcher for it.

`tools/title/deploy_npc_name_test.ps1` is the MAIN-only narrow adapter for this
exact reviewed original/candidate SHA pair. Its ValidateCandidate mode passed
without installing. Deploy/restore require full process visibility and no game,
create a verified original backup, stage and verify the replacement, reject drift,
and never extend or rewrite the accepted profile. The runtime watcher now accepts
paired `-NpcCandidateDir`/`-NpcBackupDir`, verifies the live pair before arming and
after exit, and restores the package with the DLL/INI and reload assets. No live
deployment/rollback of this new adapter has been exercised yet.

## Next physical route (one headset session, no chat switching)

After fresh `start`, MAIN verifies full-process absence and baseline hashes,
prepares a lease including RvGame, deploys with verified hashes and starts Steam.
1. Rifle: idle5s, three short firing bursts spaced3s; check muzzle origin/VFX.
   Existing native muzzle probe records idle+three shots, not a proven rifle FX fix.
2. Rifle Circle reload, take a magazine behind left shoulder; inspect palm
   placement and insert. This tests the preceding carry/ray candidate too.
3. Open the wheel; select each available displayed weapon, including a return
   to Centurion. Expected highlight direction and equipped identity agree.
4. Volk: inspect one-hand and two-hand grip while rotating; no offset tuning
   in the headset and no claim that this session enables every reload FSM.
5. Look at Devlin/friendly NPC; name should be smaller. Check subtitle size
   unchanged, a Polish note, accepted VFX/screenshots/menu, then normal exit.

Do not type in PSVR2. MAIN reads logs automatically, verifies exact restoration
after exit, then asks for one combined result. Candidate remains unaccepted
until this perceptual route passes; no merge without authorization.
