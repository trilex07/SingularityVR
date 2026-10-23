# MAIN physical acceptance — M01 shoulder pickup (`bb234ba`, PSVR2 PASS, 2026-10-24)

**Verdict: PHYSICAL PASS.** User physically confirmed on PSVR2 (session ended
2026-10-24 00:06): Valkyrie magazine pickup/reload **PASS**, Volk S4
pickup/reload **PASS** (with the user's requested farther-left region), TMD and
two-hand support **PASS**. The game was closed normally; nothing was rolled
back. The accepted baseline rollback copies remain in place.

## Candidate identity

| Item | Value |
| --- | --- |
| Integration branch | `integration/sol-side-master-package-20261023` @ `bb234ba` |
| Source | cherry-pick `-x` of SOL `5105359` onto `591d220` (M03 HUD intact) |
| Installed DLL SHA256 | `78404247B78586FB42416F0A331CD11B13B4DBC0A16708B913A21F04EA374541` (x86 PE32 `/O2`, MAIN build) |
| Installed INI SHA256 (session) | `D2E91B0B2EC0CE2D9DC1C9933D1076AF315937E24F00E5FA88A3956C75A403D8` |
| Install | `tools/deploy-singularityvr.ps1 -DllOnly -IncludeIni` @ `2026-10-23 23:52:43`, accepted-profile + change-plan guarded (M01 guard PASS, post-check PASS) |
| Rollback point | `deployment\install-backups\20261023-235243` — baseline DLL `A61C3B534F5C4C40DC878EC68CC52A77E8599DB57C6272D85254E58B795F6E1C`, INI `4EA3857A0E78033C162280AB1A8FCB8A5D6CA09B72547CE75C8D55688BC904EA` (verified in backup) |
| M09 haptics | NOT in this build (held as separate later module by decision) |

## Effective scoped settings (installed, session state)

```ini
[Input]
ValkyrieShoulderExtendLeftCm=18
ValkyrieShoulderRadiusCm=18
ValkyrieGripReachMs=1800
VolkShoulderExtendLeftCm=18
VolkShoulderRadiusCm=18
VolkGripReachMs=1800
ValkyrieManualReload=1
ReloadPalmGrip=1
ReloadNativeMagazine=1
HipKnifeWithTMD=0

[Diagnostic]
ShoulderPickupProbe=1
```

Full session INI: `SingularityVR.ini.deployed-session` (354 keys; only the seven
keys above differ from the accepted baseline). Build manifest: `d3d9.build.json`.

## Runtime evidence (`view_matrix.log`, SHA256
`89D5479D9848CE109199151CB0CEEA8B71664554B4383E6F5A47455115701CBE`, 5 118 013 bytes)

Receipts (full extracts in `shoulder-gates.txt`, `next-reload-transcript.txt`):

- **13/13 `shoulder pickup gate` receipts were accepted (`grab-fresh`)** —
  Valkyrie ×3 (pointDist 0.094–0.103 m) and Volk ×10 (0.040–0.172 m),
  `radius=0.180 extendLeft=0.180`, `context=1 tracked=1 ammoFresh=1 identity=1`,
  TMD owned, support/knife not blocking. No `event=none` rejects this session.
  One Volk grip (0.040 m) was accepted through the capsule beyond the old
  0.040 m point-distance sampling boundary (`regionDist<pointDist
  0.038<0.040`), proving the capsule path executes.
- **`next carry wrist: kind=1`** — "TMD material-0 published after stereo draw,
  wrist bone=9, segments=1" — the previously missing Valkyrie producer from
  SOL's fix fired. Same receipt for kind=2.
- **`next reload draw gate: stage=12 kind=1` and `stage=12 kind=2`** — placed
  carry draw receipts for both weapons (witness after wrist publish; a later
  `stage=11 kind=2` shows the receipt ladder only, harmless).
- **3× `inserted/request` all with `permit=1` and `renderWitness=1`** (Volk ×2,
  Valkyrie ×1) followed by **3× `native-ammo-ack` `consumed=1`**
  (`nativePermits` 1→2→3):
  - Valkyrie: clip 0 → 12, reserve 12 → 0 (full magazine from eject-on-empty).
  - Volk: clip 5 → 6 (reserve 63→62) and clip 2 → 6 (reserve 63→59; secondary
    insertion untouched behaviour).
- **Firing after reload** evidenced by Volk clip/witness sequence moving
  6→5/3→6/4→5/→2 between reload cycles and Valkyrie returning to `none` phase
  after the ack; a `magazine-ejected` sequence then reloaded again.
- `discarded` 10× are phase=1 reach-window aborts/releases (user dropped the
  grip before the socket); `grab-fresh` → Carrying transitions all logged.
  Unlike the `1c358e2` FAIL, carries no longer end in unusable picks and
  inserts/native permits occur. `grab-fresh` alone was NOT the gate.

Graded per weapon: **Valkyrie PASS**, **Volk PASS** (both pickup methods in the
combined session; receipt transcript covers both kinds).

## Physical scope recorded

PHYSICALLY_VERIFIED (user-accepted, PSVR2 session 2026-10-23 23:53–00:06) for:
TMD-owned Valkyrie and Volk shoulder-magazine pickup, visible carry on the left
glove, insertion, native ammo acknowledgement, firing, two-hand support and TMD
during the session. **Not** asserted: Kaśimov route (unchanged; not retested),
Centurion INI controls (unchanged), insertion-socket tolerance (user noted it
is still slightly strict; delegated to SOL), M03 HUD (graded separately from
this same session, below), and any claim about menus beyond normal play.

## Follow-ups taken from this session

1. SOL: socket/insertion tolerance slightly strict (user usability note).
2. M03 live HUD: session contains the merged HUD code; separate visual grade
   pending (`docs/integration/M03_HUD_LIVE_DATA_AND_MOUNT_GATE.md`).
