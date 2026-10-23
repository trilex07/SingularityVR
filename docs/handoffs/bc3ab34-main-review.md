# MAIN review: `fix/dual-tmd-l2-knife-volk` @ `bc3ab34`

**Status:** Review complete — **not deployed to Steam by MAIN.**  
**Live Steam (unchanged):** `514bb03` candidate DLL `19DFDFB7…` + `DualWeaponTmdGunScale=1`, `TmdSupportGauntlet=1`.

## Physical model (user clarification — accepted)

- **L2:** right-hand **weapon** disappears; **right glove mesh** can remain near the **body** (not one shared failure).
- Treat as **two hypotheses:** (1) `gun_scale` / owner gate → weapon subtree; (2) TMD material-0 **bone 51** wrist not riding the right controller.

## Branch contents (`bc3ab34` vs `514bb03`)

| Layer | Change | Default |
|-------|--------|---------|
| **Gun scale owner** | `native_dual_tmd_gun_scale.h`: accept audited **reciprocal local pawn** when inventory camera owner missing; refuse if both exist and disagree; bounded `owner-source` / context receipts | `[Input] DualWeaponTmdGunScale=0` |
| **Right TMD wrist** | Reconstruct original **bone 51** (palette slot 19) via existing `TargetWrist` math; `dual-tmd right wrist:` receipt | `[Input] DualWeaponTmdRightWrist=0` |
| **Volk carry magazine** | TMD material-0 wrist witness for **Volk Carrying** only; `Volk carry wrist:` + reload draw gate stages 10–12 | existing reload flags (no new INI) |
| **Knife route** | Read-only `knife-hand: held standalone route …` vs native joint; **no pose/mesh change** | N/A |

Commits: `28d8db6` owner → `9ed4f96` Volk wrist → `4105421` knife trace → `bc3ab34` right wrist anchor.

## Evidence on prior `514bb03` session (SOL)

Log SHA256 `9A70040E720A00A98B5D5E3F420E542E5FEA21D8D3BD01DBD4D45D2FAF2D93A9`:

- `DualWeaponTmdGunScale=1` loaded.
- **Refusal** `local camera-owned pawn unavailable` (~1051) — **no** `ready reflected` / **no** `scale=0 -> 1`.
- Same log shows reciprocal local pawn elsewhere (~618, ~3308) → **owner-path mismatch** is plausible; **not proven** that scale alone explains invisibility.

**MAIN agrees:** prior candidate did **not** prove override execution; wrist observation is **orthogonal**.

## MAIN build verification (`bc3ab34`)

| Check | Result |
|-------|--------|
| Ancestry `cac4ce3` | OK |
| x86 PE32 `/O2` build | OK |
| MAIN DLL SHA256 | `77B88C77615A0170B8AC6E42BBD285DD417C8A76B3004DB5D84B391501FCFB1E` |
| SOL-reported SHA256 | `421ABBC1…777519` (same commit; environment delta expected) |
| `test-dual-tmd-gun-scale.ps1` + compiled ABI | PASS |

Full dual-TMD / gauntlet / OLED suite should be re-run at deploy time (same as `514bb03` gate).

## Knife / INI trap (must fix before knife test)

Installed Steam INI: **`HipKnifeIgnoreTMD=1`** → pre-TMD two-hand rule; **post-TMD right-hand-only policy was not exercised.**  

For the controlled knife segment only: set **`HipKnifeIgnoreTMD=0`** and rely on new **`knife-hand:`** receipts. Do **not** infer mesh offset without route confirmation.

## Recommended single PSVR2 session (MAIN deploy)

**One backup**, one log, three **segments** (stop game between segments if needed to keep log readable).

### Deploy

1. Rollback pointer: `deployment/deploy-backups/20261023-pre-gun-scale-514bb03/` (or fresh backup of current `19DFDFB7`).
2. Install **`bc3ab34`** MAIN build DLL.
3. Preserve all gameplay INI; **only** adjust flags below.

### Segment 1 — L2 weapon (gun scale only)

```ini
DualWeaponTmdReady=1
DualWeaponTmdGunScale=1
DualWeaponTmdRightWrist=0
TmdSupportGauntlet=1   ; keep from accepted path
```

Volk S4, TMD raised, L2 hold/release + L2+R2 as prior script.

**Log must show one of:**

- `dual-tmd gun-scale: local gun_scale scale=0 -> 1 …` (**success path**), or
- `dual-tmd gun-scale: owner-source …` / named **refusal** (not menu idle).

**Pass A:** weapon visible both eyes during L2. **If scale executes and weapon still gone → five-bone / FG route (SOL), not another scale guess.**

### Segment 2 — L2 right glove (wrist only)

Same scene; set **`DualWeaponTmdRightWrist=1`**, leave GunScale as in Segment 1.

**Log:** `dual-tmd right wrist: original bone 51 reconstructed…`

**Pass B′:** glove/wrist follows right controller during L2; weapon criterion unchanged from Segment 1.

### Segment 3 — Knife + Volk magazine smoke

`HipKnifeIgnoreTMD=0`. Pickup knife after TMD; note **`knife-hand:`** vs joint route.

Optional reload: watch **`Volk carry wrist:`** and **`next reload draw gate: stage=… kind=2`**.

**Pass C′:** route documented; magazine **not** claimed fixed without stage 12 + replay draws + visible palm.

### Diagnostics

Turn **`DualWeaponTmdProbe=0`** / **`VolkImpulseSkinProbe=0`** if still enabled — reduce log noise unless SOL requests skin capture.

## MAIN verdict pre-PSVR2

| Item | Verdict |
|------|---------|
| Code review | **Accept for controlled Steam trial** — bounded, default-OFF, falsifiers documented |
| Weapon L2 fix claim | **UNPROVEN** |
| Wrist fix claim | **UNPROVEN** |
| Volk magazine | **UNPROVEN** (producer + receipts only) |
| Knife | **Diagnostic only**; prior log **invalid** for post-TMD policy |

## Next action

On user **“deploy bc3ab34”**, MAIN runs full regression gate, deploys, archives log, grades segments, pushes `handoff/bc3ab34-psvr2-controlled-evidence`, restores checkpoint unless user retains build.

**No merge to `main`.**
