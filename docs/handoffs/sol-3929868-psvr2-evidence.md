# PSVR2 evidence — SOL `3929868` (`56BBC470…`)

**Archive:** `deployment/crash-evidence/20261023-sol-3929868-psvr2/`  
**Log SHA256:** `55BCAA84C6CC4282E1D51469E9C0BC86D2FDF7056A99AC40296B8E716166C318` (3 270 467 bytes)  
**Steam restored:** `514bb03` DLL `19DFDFB7…`, INI `CBA10397…` (verified post-session)

## Physical grades (user + log correlation)

| Area | Grade | User | Runtime receipts |
|------|-------|------|------------------|
| P0 gun visibility + gun-scale | **PARTIAL** | Weapon visible with TMD raised; displaced vs right hand; R2 snaps glove toward weapon pose | `ready reflected` ×4; `scale=0 -> 1` ×8 |
| P1 right glove / wrist | **PARTIAL** | Glove tracks but shifts left when TMD raised | `tmd-right-wrist final` ×72 (36× `hands=2`, 36× `hands=1`); both eyes, `route=SVNC`, `drawSegments=1` |
| P2 knife + TMD glove | **FAIL** | Pickup OK; glove corrupt / fragments with TMD active | **No** `DrawKnifeWithNativeGlove`; `knife-hand: held standalone route` + `native joint` only |
| P3 Volk magazine | **PASS** | Mag + manual reload OK | `stage=12`, 8× `volk-carry-pose`, `native-ammo-ack`, `nativeMagazineDraws=2038` |
| Two-hand TMD support | **PASS** | Short sleeve + support assembly intact | `support=1 eyes=2 tris=7110` + `support=0` release receipts |

---

## P0 — gun-scale override (confirmed) vs alignment (failed)

### Mandatory receipts — **present**

```
3452:dual-tmd gun-scale: ready reflected mGunScaleCtrl=1024 BoneScale=7C ...
4334:dual-tmd gun-scale: local gun_scale scale=0 -> 1 bone=52 ... hands=2 L2=0
4357:dual-tmd gun-scale: local gun_scale scale=0 -> 1 bone=52 ... hands=1 L2=0
```

Registry/refusal fix from `d92a4e5` is **proven at runtime**. This is not bc3ab34’s “refusal before hook” failure mode.

### Witness caveat (for SOL tooling)

All gun-scale and `tmd-right-wrist final` lines log **`L2=0`** even while TMD is raised. TMD activation is logged separately:

```
4335:hands: mPlayerHandsStatus = 1 (TMD RAISED)
4647:hands: mPlayerHandsStatus = 1 (TMD RAISED)
```

Use **`mPlayerHandsStatus` / logged `hands=` (1 = TMD raised, 2 = weapon in hands)** for frame alignment, not the `L2=` field in these witnesses (likely wrong input axis or stale L2 sample).

### First TMD-raised transition (frame 1618) — visibility + displacement mechanism

```
4334-4338: scale=0->1 (hands=2) → mPlayerHandsStatus=1 → dual-tmd right wrist: bone 51 reconstructed → tmd-right-wrist final hands=1 both eyes
```

**Confirmed:** gun-scale override and right-wrist bone-51 reconstruction occur in the **same window** as `mPlayerHandsStatus` flips to raised.

**Confirmed misalignment signal:** final wrist **skin** palette (engine/SVNC) vs **local** controller target diverge in raised state:

| State | Frame | `hands` | `targetRaw` (wrist) | Skin row0 trans (approx) | Local row3 trans |
|-------|-------|---------|---------------------|--------------------------|------------------|
| Weapon in hands | 1096 | 2 | -7569, **-15667**, -23924 | 45.58, … | 0.73, 0.53, **-149.31** |
| TMD raised | 1618 | 1 | -7249, **+18095**, -32239 | 41.20, … | -2.41, -4.45, **-148.52** |

Large flip in skin-space Y (`targetRaw`) between `hands=2` and `hands=1` matches user “shift left / wrong weapon grip” while visibility is restored by scale.

### R2 firing transition (user: glove returns to weapon-holding pose)

```
4609:hands: mPlayerHandsStatus = 2 (weapon in hands)
4610-4616:tmd-right-wrist final hands=2 R2=132 both eyes
4645-4649: scale=0->1 hands=2 then hands=1; mPlayerHandsStatus=1 (TMD RAISED)
4651-4657:tmd-right-wrist final hands=1 R2=255 both eyes
4640-4641:handsymmetry: aim dev yaw -12.1° pitch +35.6° | off dev yaw -14.2° pitch +40.5°
```

**Confirmed:** R2 edges correlate with **`hands=2` weapon-in-hands** wrist finals and large **handsymmetry** deviation — consistent with animation-driven weapon pose overriding tracked glove while TMD state toggles.

### P0 edge refusals (bounded, not primary)

```
2161-2163: owner missing / refused reciprocal local pawn (transient menu/load)
```

Not observed during main Volk+TMD gameplay block (`pawn=1BD76400` stable).

---

## P1 — right glove (partial)

- **`dual-tmd right wrist: original bone 51 reconstructed to live controller`** at line 4338 (once in excerpt window; tied to TMD raised).
- **`tmd-right-wrist final`:** stereo matched (`eye=0`/`eye=1` identical skin/local blocks), `valid=1`, `route=SVNC`, `drawSegments=1`.
- **Problem is not missing final-eye samples** — it is **wrong skin vs controller basis when `hands=1`**, while `local=` tracks controller (`localHr=00000000`).

**Confirmed root cause (log-level):** TMD-raised **`mPlayerHandsStatus=1`** uses a different engine wrist/skin path than weapon-holding **`hands=2`**, while VR gun follow still runs; bone-51 reconstruction does not eliminate skin/local divergence.

**Hypothesis (needs SOL code path):** weapon mesh transform still follows engine `gun_scale`/ADS animation palette not re-based to OpenXR right controller during `hands=1`.

---

## P2 — knife + TMD (fail)

```
5388-5390: hip knife PICKED hasTmd=1 ignoreTmd=0
5390: knife-hand: held standalone route holder=1 tmd=1 ignoreTmd=0 jointPose=0 glovePose=0 hands=2
5391-5394: knife-hand: native joint eye=0/1 (stereo), sharedDepth, NOT TMD cuff path
```

**Confirmed:** commit `20f293b` TMD cuff / `DrawKnifeWithNativeGlove` path **did not run** (`jointPose=0`, no DrawKnife log).

During swing with TMD raised:

```
5639-5643: knife SWING → mPlayerHandsStatus = 1 (TMD RAISED)
5645-5646: fg pass 8–9 meshes including 4818 (TMD) + weapon counts
```

**Confirmed regression mechanism (evidence-level):** knife still uses **legacy native joint** draw while TMD **SVNC/SVTG** glove assembly is active — overlapping foreground passes (`8297` gun, `4818` TMD, attachments). Matches user-visible fragmentation.

**Not proven from log alone:** specific IB/palette restore bug; no `indexRestore=FAIL` on knife frames.

---

## P3 — Volk magazine (pass) — preserve

```
5089: grab-fresh phase=2 nativeMagazineDraws=792
5091: Volk carry wrist TMD material-0, segments=1
5092: next reload draw gate stage=12 kind=2 verts=8297
5093-5207: volk-carry-pose samples 1–5 source=2 (TMD wrist), stable deltaMagHand under rotation
5210-5213: inserted/request permit=1 → native-ammo-ack clip 6 reserve 59 consumed=1
```

**Do not change** reload FSM, mount, or `dddb35f` probe wiring.

---

## Two-handed TMD support (pass) — preserve

```
6190:tmd-support-gauntlet: frame=3744 hands=2 support=1 eyes=2 tris=7110 indexRestore=OK paletteRestore=OK
6272: ... support=0 ...
24126/24208: repeat support=1 → support=0
3882-3883: short gauntlet stereo 7110 tris, restore OK
```

---

## Instructions to SOL (next targeted correction only)

1. **P0/P1 — do not touch registry/refusal** (proven fixed). Fix **weapon + right SVNC wrist alignment when `mPlayerHandsStatus=1`**: reconcile engine skin wrist / gun anchor with OpenXR right controller without forcing `nativeHands=1` or guessed INI offsets. Use logged **`hands=1` vs `hands=2`** wrist finals and **`handsymmetry`** at R2 edges.
2. **Instrument/fix L2 witness** if PSVR2 Impulse must gate logic — current `L2=0` in scale/wrist lines is misleading for MAIN acceptance tests.
3. **P2 — disable or fix TMD knife path:** either make `DrawKnifeWithNativeGlove` actually arm (`jointPose`/cuff bind receipts) or **fail closed** to pre-`20f293b` standalone knife when TMD material-0 draw is active; never stack native joint knife over active TMD SVNC glove.
4. **P3 + two-hand support:** no functional changes; regression-only guards.

**No merge to `main`.** Steam is back on **`514bb03`**. No further PSVR2 until SOL ships a narrow candidate.

See also: `deployment/crash-evidence/20261023-sol-3929868-psvr2/log-excerpts.txt`
