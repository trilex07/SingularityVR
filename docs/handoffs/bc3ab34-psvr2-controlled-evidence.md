# bc3ab34 controlled PSVR2 evidence — FINAL

**Commit tested:** `bc3ab3455d980ae4440d554da908b0e9fc2ba080`  
**Installed DLL:** `081B278E8B5ECB267EB42F1AF0C5C92227270AED77C08CB0B37519B72C125DF4`  
**Rollback after session:** `19DFDFB7…` (`514bb03`) restored from `deployment/deploy-backups/20261023-pre-bc3ab34-segment-test/`

## Physical verdict (user)

| Segment | Grade | User observation |
|---------|-------|------------------|
| **A** Gun scale | **FAIL** | Weapon still disappears on L2 |
| **B** Right wrist | **FAIL** | Glove/wrist still wrong vs controller |
| **C1** Knife | **FAIL / PARTIAL** | Knife floats; not glued to glove |
| **C2** Volk carry reload | **PARTIAL** | Magazine visible; slightly off palm |
| **Regression** Two-hand TMD | **Not fully exercised** in user report |

## Archived logs

| Segment | SHA256 | Bytes |
|---------|--------|-------|
| A | `CAD65BC4859FBDB78A9D7EF79C6938AD45BE6E1D673D64F24D60A8CB3BF5B97C` | 1 516 175 |
| B | `CE97D74D23C4E7E166893EF812D31730D197ED21362292651AFC7065BFAB678E` | 1 691 278 |
| C | `CD0FF8D56AF95947C48448953CCAE8E26D9A2A6DF0C47245EA57F64246C883EE` | 1 392 994 |

Paths: `deployment/crash-evidence/20261023-bc3ab34-segment-{a,b,c}/view_matrix.log`

---

## A — Gun visibility / override execution

**Override executed:** **NO**

- No `dual-tmd gun-scale: ready reflected` and no `local gun_scale scale=0 -> 1` in Segment A or B logs.
- Segment A (`dual=1` gameplay):
  - ~1083: `refused reflected mGunScaleCtrl/mInviewMesh/local SP class; native result retained` (hook fingerprint never armed).
  - ~2228–2230: `context dual=1 … hands=2 L2=0` then `owner sources camera=00000000 native=00000000` → `refused reciprocal local pawn unavailable or owners disagree`.
- All gun-scale context lines show **`L2=0`** at receipt time — probe sampled dual context transitions, not Impulse-held frames; **does not disprove** L2 UX, but proves **no successful override** on any captured dual window.

**Conclusion for SOL:** User FAIL is consistent with **native gun_scale path never overridden**. Do **not** add another scale guess; fix **mGunScaleCtrl reflection / local pawn resolution at runtime** (or prove getter invocation during L2 with input-edge probe).

---

## B — Right glove wrist

**Receipt:** one line ~5733 in Segment B:  
`dual-tmd right wrist: original bone 51 reconstructed to live controller; support unchanged`

**Physical:** user **FAIL** — wrist still near body during L2.

**Conclusion:** bounded wrist hook **can fire** but did **not** produce acceptable tracking (single receipt vs sustained L2/TMD presentation, or wrong draw path). Next: correlate receipt frame with `hands=1`, material-0 SVNC draws, and whether reconstruction applies every frame during Impulse.

Gun-scale refusals in B same as A — override still not active.

---

## C1 — Knife route

`HipKnifeIgnoreTMD=0` confirmed (~131).

Pickup ~2834–2835:

```text
hip knife: PICKED held=1 … hasTmd=1 ignoreTmd=0
knife-hand: held standalone route holder=1 tmd=1 ignoreTmd=0 jointPose=0 glovePose=0 hands=2
```

**Mechanism:** standalone OpenXR knife renderer — **not** native glove joint (`jointPose=0 glovePose=0`). Matches user “pływa”.

Melee/release receipts present; **not** a placement fix — diagnostic confirmed route.

---

## C2 — Volk carried magazine

Reload FSM: `magazine-ejected` → `grab-fresh` → ~3834 `Volk carry wrist: TMD material-0 published after stereo draw, wrist bone=9, segments=1` → ~3835 `next reload draw gate: stage=12 kind=2` → `inserted/request` → `native-ammo-ack` (6/62); `nativeMagazineDraws` 852→2226.

**Physical PARTIAL:** magazine visible but offset from palm — pipeline reaches **stage 12** and replay draws; placement/tuning remains.

Earlier ~2407: `stage=10` (no wrist for weapon draw) before TMD wrist path engaged.

---

## Regression — tmd-support-gauntlet

Segment C: `tmd-support-gauntlet: enabled=1`; one stereo receipt `support=0 eyes=2 tris=7110 indexRestore=OK paletteRestore=OK` (~2428). **No `support=1` receipt** in archived C log — two-hand grip not confirmed in this capture.

---

## Overall recommendation

**Do not merge `bc3ab34` to main.** Restore **`514bb03`** checkpoint (done).

SOL priorities:

1. **Gun scale:** runtime arm `mGunScaleCtrl` + `EngineNativeLocalPlayerPawn()` during dual gameplay/L2; until `scale=0 -> 1` or explicit refusal with live pawn IDs, no PSVR2 re-test of scale logic.
2. **Right wrist:** why single reconstruction insufficient; frame coverage during `hands=1` + material-0.
3. **Knife:** joint route vs standalone when TMD glove active — mount/wrist bind, not offset guess.
4. **Volk carry:** stage-12 success + palm offset — placement after wrist witness validated.

No MAIN runtime patches.
