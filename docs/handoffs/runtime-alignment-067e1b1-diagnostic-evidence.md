# Diagnostic evidence: L2 Impulse hides right weapon, R2 restores (`067e1b1`)

**Role:** MAIN runtime evidence for SOL (no rendering changes).  
**Deployed probe commit:** `067e1b1b7301987501354e9dab63030e26eb3d6a`  
**Installed diagnostic DLL SHA256:** `A0A3365A34C0F957263B286CC982A3E5422AA370995183CC0F421252A0763589`  
**Steam restored after session:** `cac4ce3` DLL `BEA5A5623E2995C5DEE85AC30D1736F0B3E3CEAF68AF20A84B86B7DA3E83D581`  
**Log archive:** `deployment/crash-evidence/20261023-067e1b1-l2-weapon-diagnostic/`  
**Full log SHA256:** `16D867C45E5B0E809438EF23FBD92DCCF23406B013115B0A6BBFDA69B0155D2E` (~2.21 MB)

## Confirmed PSVR2 sequence (user)

| Step | Action | Weapon visible |
|------|--------|----------------|
| 1 | Volk S4 / shotgun, TMD idle | Yes |
| 2 | Raise left TMD | Yes |
| 3 | **L2 Impulse** | **No** |
| 4 | **R2 fire** | **Yes** |
| 5 | Release TMD | Yes |
| 6 | Left Grip (two-hand) | TMD hardware hidden; glove/wrist truncated |
| 7 | Release Grip | Yes |
| 8 | Idle | Yes |

**Takeaway:** Regression is tied to **Impulse (L2) while dual-TMD context is active**, not merely entering TMD presentation. **R2 (fire)** coincides with restoration in both UX and logs.

## Probe design limit (must not over-read)

`067e1b1` arms `dual-gun transition/matrix/submitted` only when:

```text
state = g_handsStatus * 2 + g_twoHandActive   (max 24 captures)
```

**L2 Impulse and sustained fire do not change `hands` or `support` by themselves.** Therefore the **L2-held invisible interval has no dedicated matrix capture** unless a transition happens in the same window.

Impulse **is** visible in periodic input correlation:

- Log lines ~6503, ~7301, ~7389, ~7553: `buttons held (frames of 120): impulse …` (and `ageyoung` on ~7389).

## Transition timeline (log line numbers)

| Line | Event |
|------|--------|
| 647 | `mPlayerHandsStatus = 2` (weapon in hands) at gameplay start |
| 3868–3900 | **Last baseline visible contract (hands=2):** valid bone reg **c5**, stereo submit OK |
| 6373–6406 | **TMD raised → hands=1:** transition probe; **incoming/submitting reg c5 degenerate** |
| 6503 | **Impulse held 20/120 frames** (no new dual-gun line — hands/support unchanged) |
| 6625–6703 | **R2 fire:** `StartFire` / `BeginFire` / `FireAmmunition`; **hands=2** @6634; transition @6672 with **valid reg c5** restored |
| 6968–7084 | Two-hand grip ACQUIRED / RELEASED (`support=1` only on transition @6970) |
| 7234–7267 | **hands=1 again** after further fire; reg c5 **degenerate** (same pattern as 6375) |
| 7301 | Impulse 16/120; LEFT trigger raw 1.00 |
| 7415–7453 | **R2 fire → hands=2** @7421; valid reg c5 @7422–7453 |
| 7472–7505 | hands=1 transition again (degenerate c5) |

## Stable identity (all transitions)

- `g_weaponObj` / census: **`69963600`**
- `g_gunVerts`: **`8297`** (Volk shotgun)
- `nativeHands=0` on all nine `dual-gun transition` lines
- `DualWeaponTmdContext=1`, `support=0` except grip window (`support=1` @6970)

## Matrix comparison: visible (hands=2) vs hidden contract (hands=1)

### Visible — hands=2 @ line 3868 (frame 661)

- **reg 230 (local):** proper rotation + translation  
- **reg 5 (bones):** full 3×3 + translation, e.g. row0 `(0.999303, -0.00243539, 0.0372667, -12.0761)`  
- **Submit:** incoming == submitting for both groups; `ride=1` on submit path  
- **Receipt:** `dual-gun submitted: hr=00000000 eyes=2 segments=1`

### Hidden / impaired — hands=1 @ lines 6375–6406 (frame 4322) and 7236–7267 (frame 5265)

- **reg 230:** still a valid rotation (gun-local block looks sane)  
- **reg 5 incoming:** **zero 3×3 rotation**; only column-4-like scalars in rows 0–2, row3 repeats col0:

```text
row0: 0 0 0 -3.25532
row1: 0 0 0 -110.389
row2: 0 0 0 4.86554
row3: 0 0 0 -3.25532
```

- **Submit:** mod **copies the same degenerate c5** (no palette repair on this path)  
- **Receipt:** still `hr=00000000 eyes=2 segments=1` — **HRESULT success is not visibility**

### Restored — hands=2 after R2 @ lines 6672–6703 (frame 4656) and 7422–7453 (frame 5429)

- **reg 5** returns to a normal skinning matrix (non-zero 3×3, e.g. row0 `(0.999574, -0.00154937, 0.0291557, -11.411)` @6678)  
- Same submit pattern: both eyes, `segments=1`, `S_OK`

## Draw admission vs suppression (census)

Per-window `dual-tmd census` buckets by **`g_handsStatus` at draw time** (signature counts, not pixels):

| Frame token | hands=1 `part=gun raw` | hands=2 `part=gun raw` | Notes |
|-------------|------------------------|-------------------------|--------|
| 3600 | 0 | 1350 | TMD raised earlier in session, no gun bucket |
| **4050** | **1002** | 348 | Overlaps **impulse ~6503** window — gun draws **still reach** hook under hands=1 |
| 4500 | 0 | 1350 | During two-hand `support=1` |
| 5400 | 660 | (see extract) | Later impulse / fire cycle |

**Not Case D (global):** gun mesh **does** hit the instrumented draw path during hands=1 / impulse windows (`raw>0`, `reachedStereo>0`, `gunRide>0` for hands=1 @4050).

**Case D (local instrumentation):** no per-frame matrix log for the **steady L2 interval** because probe is transition-triggered.

## Input correlation

- **Impulse:** `buttons held … impulse N` (L2 binding), independent of XInput LT/RT forwarded as 0 in sampled windows (`triggers: … -> game receives LT 0 RT 0`).  
- **R2 fire:** correlates with `aim mode 9: StartFire/BeginFire/FireAmmunition` and **`hands: mPlayerHandsStatus = 2`** within a few lines.  
- **Two-hand grip:** `two-hand grip: ACQUIRED/RELEASED` @6968–7084; `support=1` only on transition capture @6970; user reports wrist truncation (separate from L2 bug).

## TMD assembly (same session, hands=1)

`tmd-assembly bound/source` @6415–6424 shows SVTG/SVNC routes still binding with plausible skin rows for TMD hardware — **TMD path can be healthy while gun bone constants are not**.

## Classification (evidence policy)

| Case | Verdict |
|------|---------|
| **A** — incoming matrices invalid before mod | **Supported** on every **hands=1** gun transition: **reg c5 degenerate** while reg 230 OK. Likely native UE3 presentation / impulse animation state when `mPlayerHandsStatus=1`. |
| **B** — incoming OK, submitting diverges | **Not supported** on captured transitions: submitting **matches** incoming for c5; no mod-side matrix repair attempted. |
| **C** — plausible submit, still invisible | **Plausible** given user report + degenerate skinning + `S_OK` + `nSeg=1`. Clipping/occlusion not ruled in/out without pixel proof. |
| **D** — no gun draw | **Rejected** for mechanism (census + transitions show gun draws). **Accepted** for **L2-only steady state** — probe did not sample it. |

## First violated contract (for SOL)

When **`mPlayerHandsStatus=1` (TMD RAISED)** and the engine still issues **8297-vert gun draws**, **vertex shader bone constants at reg 5 arrive non-physical (zero 3×3)**. The mod **rides camera/local (reg 230)** but **passes bone constants through unchanged**. Physical invisibility on L2 aligns with that state; **R2 fire** flips **`hands=2`** and restores **valid reg 5** without changing weapon object or vert count.

## Required next instrumentation (before another headset test)

Minimal **input-edge-scoped** probe (SOL):

1. Log on **`impulse` press/release edge** and **`StartFire/BeginFire` edge**: `hands`, `support`, `weapon`, `gunVerts`, reg **230 + 5** rows (incoming + submitting), `nSeg`, `hr`.  
2. Optional: log when gun draw sees **`reg5` zero-3×3** regardless of transition (bounded counter).  
3. Do **not** re-run PSVR2 until (1) or (2) exists — MAIN will not repeat unchanged headset experiment.

## Rollback

Steam `Binaries` restored to **`cac4ce3`** backup folder `deployment/deploy-backups/20261023-pre-runtime-alignment-067e1b1/`.
